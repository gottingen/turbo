
/****************************************************************
 * Copyright (c) 2022, liyinbin
 * All rights reserved.
 * Author by liyinbin (jeff.li) lijippy@163.com
 *****************************************************************/
//

// The implementation of the flare::time_point class, which is declared in
// //flare/times/time.h.
//
// The representation for an flare::time_point is an flare::duration offset from the
// epoch.  We use the traditional Unix epoch (1970-01-01 00:00:00 +0000)
// for convenience, but this is not exposed in the API and could be changed.
//
// NOTE: To keep type verbosity to a minimum, the following variable naming
// conventions are used throughout this file.
//
// tz: An flare::time_zone
// ci: An flare::time_zone::chrono_info
// ti: An flare::time_zone::time_info
// cd: An flare::chrono_day or a flare::times_internal::civil_day
// cs: An flare::chrono_second or a flare::times_internal::civil_second
// bd: An flare::time_point::breakdown
// cl: A flare::times_internal::time_zone::civil_lookup
// al: A flare::times_internal::time_zone::absolute_lookup

#include "flare/times/time.h"

#if defined(_MSC_VER)
#include <winsock2.h>  // for timeval
#endif

#include <ctime>
#include <limits>

#ifdef _WIN32
#include <windows.h>
#endif

#include <algorithm>
#include <atomic>
#include <cerrno>
#include <cstdint>
#include "flare/base/profile.h"
#include "flare/thread/spinlock.h"
#include "flare/times/internal/unscaled_cycle_clock.h"
#include "flare/thread/thread_annotations.h"

namespace flare {

    namespace {

        FLARE_FORCE_INLINE flare::times_internal::time_point<flare::times_internal::seconds> internal_unix_epoch() {
            return std::chrono::time_point_cast<flare::times_internal::seconds>(
                    std::chrono::system_clock::from_time_t(0));
        }

        // Floors d to the next unit boundary closer to negative infinity.
        FLARE_FORCE_INLINE int64_t FloorToUnit(flare::duration d, flare::duration unit) {
            flare::duration rem;
            int64_t q = duration::integer_div_duration(d, unit, &rem);
            return (q > 0 ||
                    rem >= zero_duration() ||
                    q == std::numeric_limits<int64_t>::min()) ? q : q - 1;
        }

        FLARE_FORCE_INLINE flare::time_point::breakdown InfiniteFutureBreakdown() {
            flare::time_point::breakdown bd;
            bd.year = std::numeric_limits<int64_t>::max();
            bd.month = 12;
            bd.day = 31;
            bd.hour = 23;
            bd.minute = 59;
            bd.second = 59;
            bd.subsecond = flare::infinite_duration();
            bd.weekday = 4;
            bd.yearday = 365;
            bd.offset = 0;
            bd.is_dst = false;
            bd.zone_abbr = "-00";
            return bd;
        }

        FLARE_FORCE_INLINE flare::time_point::breakdown InfinitePastBreakdown() {
            time_point::breakdown bd;
            bd.year = std::numeric_limits<int64_t>::min();
            bd.month = 1;
            bd.day = 1;
            bd.hour = 0;
            bd.minute = 0;
            bd.second = 0;
            bd.subsecond = -flare::infinite_duration();
            bd.weekday = 7;
            bd.yearday = 1;
            bd.offset = 0;
            bd.is_dst = false;
            bd.zone_abbr = "-00";
            return bd;
        }

        FLARE_FORCE_INLINE flare::time_zone::chrono_info InfiniteFutureCivilInfo() {
            time_zone::chrono_info ci;
            ci.cs = chrono_second::max();
            ci.subsecond = infinite_duration();
            ci.offset = 0;
            ci.is_dst = false;
            ci.zone_abbr = "-00";
            return ci;
        }

        FLARE_FORCE_INLINE flare::time_zone::chrono_info InfinitePastCivilInfo() {
            time_zone::chrono_info ci;
            ci.cs = chrono_second::min();
            ci.subsecond = -infinite_duration();
            ci.offset = 0;
            ci.is_dst = false;
            ci.zone_abbr = "-00";
            return ci;
        }

        FLARE_FORCE_INLINE flare::time_conversion InfiniteFutureTimeConversion() {
            flare::time_conversion tc;
            tc.pre = tc.trans = tc.post = flare::time_point::infinite_future();
            tc.kind = flare::time_conversion::UNIQUE;
            tc.normalized = true;
            return tc;
        }

        FLARE_FORCE_INLINE time_conversion InfinitePastTimeConversion() {
            flare::time_conversion tc;
            tc.pre = tc.trans = tc.post = flare::time_point::infinite_past();
            tc.kind = flare::time_conversion::UNIQUE;
            tc.normalized = true;
            return tc;
        }

// Makes a time_point from sec, overflowing to infinite_future/infinite_past as
// necessary. If sec is min/max, then consult cs+tz to check for overlow.
        time_point MakeTimeWithOverflow(const flare::times_internal::time_point<flare::times_internal::seconds> &sec,
                                        const flare::times_internal::civil_second &cs,
                                        const flare::times_internal::time_zone &tz,
                                        bool *normalized = nullptr) {
            const auto max = flare::times_internal::time_point<flare::times_internal::seconds>::max();
            const auto min = flare::times_internal::time_point<flare::times_internal::seconds>::min();
            if (sec == max) {
                const auto al = tz.lookup(max);
                if (cs > al.cs) {
                    if (normalized)
                        *normalized = true;
                    return flare::time_point::infinite_future();
                }
            }
            if (sec == min) {
                const auto al = tz.lookup(min);
                if (cs < al.cs) {
                    if (normalized)
                        *normalized = true;
                    return flare::time_point::infinite_past();
                }
            }
            const auto hi = (sec - internal_unix_epoch()).count();
            return time_point::from_unix_duration(duration::make_duration(hi));
        }

// Returns Mon=1..Sun=7.
        FLARE_FORCE_INLINE int MapWeekday(const flare::times_internal::weekday &wd) {
            switch (wd) {
                case flare::times_internal::weekday::monday:
                    return 1;
                case flare::times_internal::weekday::tuesday:
                    return 2;
                case flare::times_internal::weekday::wednesday:
                    return 3;
                case flare::times_internal::weekday::thursday:
                    return 4;
                case flare::times_internal::weekday::friday:
                    return 5;
                case flare::times_internal::weekday::saturday:
                    return 6;
                case flare::times_internal::weekday::sunday:
                    return 7;
            }
            return 1;
        }

        bool FindTransition(const flare::times_internal::time_zone &tz,
                            bool (flare::times_internal::time_zone::*find_transition)(
                                    const flare::times_internal::time_point<flare::times_internal::seconds> &tp,
                                    flare::times_internal::time_zone::civil_transition *trans) const,
                            time_point t, time_zone::chrono_transition *trans) {
            // Transitions are second-aligned, so we can discard any fractional part.
            const auto tp = internal_unix_epoch() + flare::times_internal::seconds(t.to_unix_seconds());
            flare::times_internal::time_zone::civil_transition tr;
            if (!(tz.*find_transition)(tp, &tr))
                return false;
            trans->from = chrono_second(tr.from);
            trans->to = chrono_second(tr.to);
            return true;
        }

    }  // namespace

    //
    // time_point
    //

    flare::time_point::breakdown time_point::in(flare::time_zone tz) const {
        if (*this == flare::time_point::infinite_future())
            return InfiniteFutureBreakdown();
        if (*this == flare::time_point::infinite_past())
            return InfinitePastBreakdown();

        const auto tp = internal_unix_epoch() + flare::times_internal::seconds(duration::get_rep_hi(rep_));
        const auto al = flare::times_internal::time_zone(tz).lookup(tp);
        const auto cs = al.cs;
        const auto cd = flare::times_internal::civil_day(cs);

        flare::time_point::breakdown bd;
        bd.year = cs.year();
        bd.month = cs.month();
        bd.day = cs.day();
        bd.hour = cs.hour();
        bd.minute = cs.minute();
        bd.second = cs.second();
        bd.subsecond = duration::make_duration(0, duration::get_rep_lo(rep_));
        bd.weekday = MapWeekday(flare::times_internal::get_weekday(cd));
        bd.yearday = flare::times_internal::get_yearday(cd);
        bd.offset = al.offset;
        bd.is_dst = al.is_dst;
        bd.zone_abbr = al.abbr;
        return bd;
    }

    //
    // Conversions from/to other time types.
    //

    flare::time_point time_point::from_date(double udate) {
        return time_point::from_unix_duration(duration::milliseconds(udate));
    }

    flare::time_point time_point::from_universal(int64_t universal) {
        return flare::time_point::universal_epoch() + 100 * duration::nanoseconds(universal);
    }

    int64_t time_point::to_unix_nanos() const {
        if (duration::get_rep_hi(time_point::to_unix_duration(*this)) >= 0 &&
            duration::get_rep_hi(time_point::to_unix_duration(*this)) >> 33 == 0) {
            return (duration::get_rep_hi(time_point::to_unix_duration(*this)) *
                    1000 * 1000 * 1000) +
                   (duration::get_rep_lo(time_point::to_unix_duration(*this)) / 4);
        }
        return FloorToUnit(time_point::to_unix_duration(*this), duration::nanoseconds(1));
    }

    int64_t time_point::to_unix_micros() const {
        if (duration::get_rep_hi(time_point::to_unix_duration(*this)) >= 0 &&
            duration::get_rep_hi(time_point::to_unix_duration(*this)) >> 43 == 0) {
            return (duration::get_rep_hi(time_point::to_unix_duration(*this)) *
                    1000 * 1000) +
                   (duration::get_rep_lo(time_point::to_unix_duration(*this)) / 4000);
        }
        return FloorToUnit(time_point::to_unix_duration(*this), duration::microseconds(1));
    }

    int64_t time_point::to_unix_millis() const {
        if (duration::get_rep_hi(time_point::to_unix_duration(*this)) >= 0 &&
            duration::get_rep_hi(time_point::to_unix_duration(*this)) >> 53 == 0) {
            return (duration::get_rep_hi(time_point::to_unix_duration(*this)) * 1000) +
                   (duration::get_rep_lo(time_point::to_unix_duration(*this)) /
                    (4000 * 1000));
        }
        return FloorToUnit(time_point::to_unix_duration(*this), duration::milliseconds(1));
    }

    int64_t time_point::to_unix_seconds() const {
        return duration::get_rep_hi(time_point::to_unix_duration(*this));
    }

    time_t time_point::to_time_t() const { return to_timespec().tv_sec; }

    double time_point::to_date() const {
        return rep_.float_div_duration(duration::milliseconds(1));
    }

    int64_t time_point::to_universal() const {
        return flare::FloorToUnit(*this - flare::time_point::universal_epoch(), duration::nanoseconds(100));
    }

    flare::time_point time_point::from_timespec(timespec ts) {
        return from_unix_duration(duration::from_timespec(ts));
    }

    flare::time_point time_point::from_timeval(timeval tv) {
        return from_unix_duration(duration::from_timeval(tv));
    }

    timespec time_point::to_timespec() const {
        timespec ts{0, 0};
        flare::duration d = to_unix_duration(*this);
        if (!d.is_infinite_duration()) {
            ts.tv_sec = duration::get_rep_hi(d);
            if (ts.tv_sec == duration::get_rep_hi(d)) {  // no time_t narrowing
                ts.tv_nsec = duration::get_rep_lo(d) / 4;  // floor
                return ts;
            }
        }
        if (d >= flare::zero_duration()) {
            ts.tv_sec = std::numeric_limits<time_t>::max();
            ts.tv_nsec = 1000 * 1000 * 1000 - 1;
        } else {
            ts.tv_sec = std::numeric_limits<time_t>::min();
            ts.tv_nsec = 0;
        }
        return ts;
    }

    timeval time_point::to_timeval() const {
        timeval tv{0, 0};
        timespec ts = to_timespec();
        tv.tv_sec = ts.tv_sec;
        if (tv.tv_sec != ts.tv_sec) {  // narrowing
            if (ts.tv_sec < 0) {
                tv.tv_sec = std::numeric_limits<decltype(tv.tv_sec)>::min();
                tv.tv_usec = 0;
            } else {
                tv.tv_sec = std::numeric_limits<decltype(tv.tv_sec)>::max();
                tv.tv_usec = 1000 * 1000 - 1;
            }
            return tv;
        }
        tv.tv_usec = static_cast<int>(ts.tv_nsec / 1000);  // suseconds_t
        return tv;
    }

    time_point time_point::from_chrono(const std::chrono::system_clock::time_point &tp) {
        return time_point::from_unix_duration(duration::from_chrono(tp - std::chrono::system_clock::from_time_t(0)));
    }

    std::chrono::system_clock::time_point time_point::to_chrono_time() const {
        using D = std::chrono::system_clock::duration;
        auto d = time_point::to_unix_duration(*this);
        if (d < zero_duration())
            d = d.floor(duration::from_chrono(D{1}));
        return std::chrono::system_clock::from_time_t(0) +
               d.to_chrono_duration<D>();
    }

//
// time_zone
//

    flare::time_zone::chrono_info time_zone::at(time_point t) const {
        if (t == flare::time_point::infinite_future())
            return InfiniteFutureCivilInfo();
        if (t == flare::time_point::infinite_past())
            return InfinitePastCivilInfo();

        const auto ud = time_point::to_unix_duration(t);
        const auto tp = internal_unix_epoch() + flare::times_internal::seconds(duration::get_rep_hi(ud));
        const auto al = cz_.lookup(tp);

        time_zone::chrono_info ci;
        ci.cs = chrono_second(al.cs);
        ci.subsecond = duration::make_duration(0, duration::get_rep_lo(ud));
        ci.offset = al.offset;
        ci.is_dst = al.is_dst;
        ci.zone_abbr = al.abbr;
        return ci;
    }

    flare::time_zone::time_info time_zone::at(chrono_second ct) const {
        const flare::times_internal::civil_second cs(ct);
        const auto cl = cz_.lookup(cs);

        time_zone::time_info ti;
        switch (cl.kind) {
            case flare::times_internal::time_zone::civil_lookup::UNIQUE:
                ti.kind = time_zone::time_info::UNIQUE;
                break;
            case flare::times_internal::time_zone::civil_lookup::SKIPPED:
                ti.kind = time_zone::time_info::SKIPPED;
                break;
            case flare::times_internal::time_zone::civil_lookup::REPEATED:
                ti.kind = time_zone::time_info::REPEATED;
                break;
        }
        ti.pre = MakeTimeWithOverflow(cl.pre, cs, cz_);
        ti.trans = MakeTimeWithOverflow(cl.trans, cs, cz_);
        ti.post = MakeTimeWithOverflow(cl.post, cs, cz_);
        return ti;
    }

    bool time_zone::next_transition(time_point t, chrono_transition *trans) const {
        return FindTransition(cz_, &flare::times_internal::time_zone::next_transition, t, trans);
    }

    bool time_zone::prev_transition(time_point t, chrono_transition *trans) const {
        return FindTransition(cz_, &flare::times_internal::time_zone::prev_transition, t, trans);
    }

//
// Conversions involving time zones.
//

    flare::time_conversion convert_date_time(int64_t year, int mon, int day, int hour,
                                             int min, int sec, time_zone tz) {
        // Avoids years that are too extreme for chrono_second to normalize.
        if (year > 300000000000)
            return InfiniteFutureTimeConversion();
        if (year < -300000000000)
            return InfinitePastTimeConversion();

        const chrono_second cs(year, mon, day, hour, min, sec);
        const auto ti = tz.at(cs);

        time_conversion tc;
        tc.pre = ti.pre;
        tc.trans = ti.trans;
        tc.post = ti.post;
        switch (ti.kind) {
            case time_zone::time_info::UNIQUE:
                tc.kind = time_conversion::UNIQUE;
                break;
            case time_zone::time_info::SKIPPED:
                tc.kind = time_conversion::SKIPPED;
                break;
            case time_zone::time_info::REPEATED:
                tc.kind = time_conversion::REPEATED;
                break;
        }
        tc.normalized = false;
        if (year != cs.year() || mon != cs.month() || day != cs.day() ||
            hour != cs.hour() || min != cs.minute() || sec != cs.second()) {
            tc.normalized = true;
        }
        return tc;
    }

    flare::time_point from_tm(const struct tm &tm, flare::time_zone tz) {
        chrono_year_t tm_year = tm.tm_year;
        // Avoids years that are too extreme for chrono_second to normalize.
        if (tm_year > 300000000000ll)
            return time_point::infinite_future();
        if (tm_year < -300000000000ll)
            return time_point::infinite_past();
        int tm_mon = tm.tm_mon;
        if (tm_mon == std::numeric_limits<int>::max()) {
            tm_mon -= 12;
            tm_year += 1;
        }
        const auto ti = tz.at(chrono_second(tm_year + 1900, tm_mon + 1, tm.tm_mday,
                                            tm.tm_hour, tm.tm_min, tm.tm_sec));
        return tm.tm_isdst == 0 ? ti.post : ti.pre;
    }

    struct tm to_tm(flare::time_point t, flare::time_zone tz) {
        struct tm tm = {};

        const auto ci = tz.at(t);
        const auto &cs = ci.cs;
        tm.tm_sec = cs.second();
        tm.tm_min = cs.minute();
        tm.tm_hour = cs.hour();
        tm.tm_mday = cs.day();
        tm.tm_mon = cs.month() - 1;

        // Saturates tm.tm_year in cases of over/underflow, accounting for the fact
        // that tm.tm_year is years since 1900.
        if (cs.year() < std::numeric_limits<int>::min() + 1900) {
            tm.tm_year = std::numeric_limits<int>::min();
        } else if (cs.year() > std::numeric_limits<int>::max()) {
            tm.tm_year = std::numeric_limits<int>::max() - 1900;
        } else {
            tm.tm_year = static_cast<int>(cs.year() - 1900);
        }

        switch (get_weekday(cs)) {
            case chrono_weekday::sunday:
                tm.tm_wday = 0;
                break;
            case chrono_weekday::monday:
                tm.tm_wday = 1;
                break;
            case chrono_weekday::tuesday:
                tm.tm_wday = 2;
                break;
            case chrono_weekday::wednesday:
                tm.tm_wday = 3;
                break;
            case chrono_weekday::thursday:
                tm.tm_wday = 4;
                break;
            case chrono_weekday::friday:
                tm.tm_wday = 5;
                break;
            case chrono_weekday::saturday:
                tm.tm_wday = 6;
                break;
        }
        tm.tm_yday = get_yearday(cs) - 1;
        tm.tm_isdst = ci.is_dst ? 1 : 0;

        return tm;
    }

}  // namespace flare


namespace flare {

    time_point time_now() {
        // TODO(bww): Get a timespec instead so we don't have to divide.
        int64_t n = flare::get_current_time_nanos();
        if (n >= 0) {
            return time_point::from_unix_duration(
                    duration::make_duration(n / 1000000000, n % 1000000000 * 4));
        }
        return time_point::from_unix_duration(flare::duration::nanoseconds(n));
    }

}  // namespace flare

// Decide if we should use the fast get_current_time_nanos() algorithm
// based on the cyclecounter, otherwise just get the time directly
// from the OS on every call. This can be chosen at compile-time via
// -DFLARE_USE_CYCLECLOCK_FOR_GET_CURRENT_TIME_NANOS=[0|1]
#ifndef FLARE_USE_CYCLECLOCK_FOR_GET_CURRENT_TIME_NANOS
#if FLARE_USE_UNSCALED_CYCLECLOCK
#define FLARE_USE_CYCLECLOCK_FOR_GET_CURRENT_TIME_NANOS 1
#else
#define FLARE_USE_CYCLECLOCK_FOR_GET_CURRENT_TIME_NANOS 0
#endif
#endif

#if defined(__APPLE__) || defined(_WIN32)

#include "flare/times/internal/chrono_time.h"

#else
#include "flare/times/internal/chrono_posix_time.h"
#endif

// Allows override by test.
#ifndef GET_CURRENT_TIME_NANOS_FROM_SYSTEM
#define GET_CURRENT_TIME_NANOS_FROM_SYSTEM() \
  ::flare::times_internal::get_current_time_nanos_from_system()
#endif

#if !FLARE_USE_CYCLECLOCK_FOR_GET_CURRENT_TIME_NANOS
namespace flare {

    int64_t get_current_time_nanos() {
        return GET_CURRENT_TIME_NANOS_FROM_SYSTEM();
    }

}  // namespace flare
#else  // Use the cyclecounter-based implementation below.

// Allows override by test.
#ifndef GET_CURRENT_TIME_NANOS_CYCLECLOCK_NOW
#define GET_CURRENT_TIME_NANOS_CYCLECLOCK_NOW() \
  ::flare::unscaled_cycle_clock_wrapper_for_get_current_time::now()
#endif

// The following counters are used only by the test code.
static int64_t stats_initializations;
static int64_t stats_reinitializations;
static int64_t stats_calibrations;
static int64_t stats_slow_paths;
static int64_t stats_fast_slow_paths;

namespace flare {

    // This is a friend wrapper around unscaled_cycle_clock::now()
    // (needed to access unscaled_cycle_clock).
    class unscaled_cycle_clock_wrapper_for_get_current_time {
    public:
        static int64_t now() { return times_internal::unscaled_cycle_clock::now(); }
    };

    // uint64_t is used in this module to provide an extra bit in multiplications

    // Return the time in ns as told by the kernel interface.  Place in *cycleclock
    // the value of the cycleclock at about the time of the syscall.
    // This call represents the time base that this module synchronizes to.
    // Ensures that *cycleclock does not step back by up to (1 << 16) from
    // last_cycleclock, to discard small backward counter steps.  (Larger steps are
    // assumed to be complete resyncs, which shouldn't happen.  If they do, a full
    // reinitialization of the outer algorithm should occur.)
    static int64_t get_current_time_nanos_from_kernel(uint64_t last_cycleclock,
                                                      uint64_t *cycleclock) {
        // We try to read clock values at about the same time as the kernel clock.
        // This value gets adjusted up or down as estimate of how long that should
        // take, so we can reject attempts that take unusually long.
        static std::atomic<uint64_t> approx_syscall_time_in_cycles{10 * 1000};

        uint64_t local_approx_syscall_time_in_cycles =  // local copy
                approx_syscall_time_in_cycles.load(std::memory_order_relaxed);

        int64_t current_time_nanos_from_system;
        uint64_t before_cycles;
        uint64_t after_cycles;
        uint64_t elapsed_cycles;
        int loops = 0;
        do {
            before_cycles = GET_CURRENT_TIME_NANOS_CYCLECLOCK_NOW();
            current_time_nanos_from_system = GET_CURRENT_TIME_NANOS_FROM_SYSTEM();
            after_cycles = GET_CURRENT_TIME_NANOS_CYCLECLOCK_NOW();
            // elapsed_cycles is unsigned, so is large on overflow
            elapsed_cycles = after_cycles - before_cycles;
            if (elapsed_cycles >= local_approx_syscall_time_in_cycles &&
                ++loops == 20) {  // clock changed frequencies?  Back off.
                loops = 0;
                if (local_approx_syscall_time_in_cycles < 1000 * 1000) {
                    local_approx_syscall_time_in_cycles =
                            (local_approx_syscall_time_in_cycles + 1) << 1;
                }
                approx_syscall_time_in_cycles.store(
                        local_approx_syscall_time_in_cycles,
                        std::memory_order_relaxed);
            }
        } while (elapsed_cycles >= local_approx_syscall_time_in_cycles ||
                 last_cycleclock - after_cycles < (static_cast<uint64_t>(1) << 16));

        // Number of times in a row we've seen a kernel time call take substantially
        // less than approx_syscall_time_in_cycles.
        static std::atomic<uint32_t> seen_smaller{0};

        // Adjust approx_syscall_time_in_cycles to be within a factor of 2
        // of the typical time to execute one iteration of the loop above.
        if ((local_approx_syscall_time_in_cycles >> 1) < elapsed_cycles) {
            // measured time is no smaller than half current approximation
            seen_smaller.store(0, std::memory_order_relaxed);
        } else if (seen_smaller.fetch_add(1, std::memory_order_relaxed) >= 3) {
            // smaller delays several times in a row; reduce approximation by 12.5%
            const uint64_t new_approximation =
                    local_approx_syscall_time_in_cycles -
                    (local_approx_syscall_time_in_cycles >> 3);
            approx_syscall_time_in_cycles.store(new_approximation,
                                                std::memory_order_relaxed);
            seen_smaller.store(0, std::memory_order_relaxed);
        }

        *cycleclock = after_cycles;
        return current_time_nanos_from_system;
    }


// ---------------------------------------------------------------------
// An implementation of reader-write locks that use no atomic ops in the read
// case.  This is a generalization of Lamport's method for reading a multiword
// clock.  Increment a word on each write acquisition, using the low-order bit
// as a spinlock; the word is the high word of the "clock".  Readers read the
// high word, then all other data, then the high word again, and repeat the
// read if the reads of the high words yields different answers, or an odd
// value (either case suggests possible interference from a writer).
// Here we use a spinlock to ensure only one writer at a time, rather than
// spinning on the bottom bit of the word to benefit from spin_lock
// spin-delay tuning.

// Acquire seqlock (*seq) and return the value to be written to unlock.
    static FLARE_FORCE_INLINE uint64_t SeqAcquire(std::atomic<uint64_t> *seq) {
        uint64_t x = seq->fetch_add(1, std::memory_order_relaxed);

        // We put a release fence between update to *seq and writes to shared data.
        // Thus all stores to shared data are effectively release operations and
        // update to *seq above cannot be re-ordered past any of them.  Note that
        // this barrier is not for the fetch_add above.  A release barrier for the
        // fetch_add would be before it, not after.
        std::atomic_thread_fence(std::memory_order_release);

        return x + 2;   // original word plus 2
    }

// Release seqlock (*seq) by writing x to it---a value previously returned by
// SeqAcquire.
    static FLARE_FORCE_INLINE void SeqRelease(std::atomic<uint64_t> *seq, uint64_t x) {
        // The unlock store to *seq must have release ordering so that all
        // updates to shared data must finish before this store.
        seq->store(x, std::memory_order_release);  // release lock for readers
    }

// ---------------------------------------------------------------------

// "nsscaled" is unit of time equal to a (2**kScale)th of a nanosecond.
    enum {
        kScale = 30
    };

// The minimum interval between samples of the time base.
// We pick enough time to amortize the cost of the sample,
// to get a reasonably accurate cycle counter rate reading,
// and not so much that calculations will overflow 64-bits.
    static const uint64_t kMinNSBetweenSamples = 2000 << 20;

// We require that kMinNSBetweenSamples shifted by kScale
// have at least a bit left over for 64-bit calculations.
    static_assert(((kMinNSBetweenSamples << (kScale + 1)) >> (kScale + 1)) ==
                  kMinNSBetweenSamples,
                  "cannot represent kMaxBetweenSamplesNSScaled");

    // A reader-writer lock protecting the static locations below.
    // See SeqAcquire() and SeqRelease() above.
    static flare::spinlock lock;
    static std::atomic<uint64_t> seq(0);

    // data from a sample of the kernel's time value
    struct time_sample_atomic {
        std::atomic<uint64_t> raw_ns;              // raw kernel time
        std::atomic<uint64_t> base_ns;             // our estimate of time
        std::atomic<uint64_t> base_cycles;         // cycle counter reading
        std::atomic<uint64_t> nsscaled_per_cycle;  // cycle period
        // cycles before we'll sample again (a scaled reciprocal of the period,
        // to avoid a division on the fast path).
        std::atomic<uint64_t> min_cycles_per_sample;
    };
    // Same again, but with non-atomic types
    struct time_sample {
        uint64_t raw_ns{0};                 // raw kernel time
        uint64_t base_ns{0};                // our estimate of time
        uint64_t base_cycles{0};            // cycle counter reading
        uint64_t nsscaled_per_cycle{0};     // cycle period
        uint64_t min_cycles_per_sample{0};  // approx cycles before next sample
    };

    static struct time_sample_atomic last_sample;   // the last sample; under seq

    static int64_t get_current_time_nanos_slow_path() FLARE_COLD;

    // Read the contents of *atomic into *sample.
    // Each field is read atomically, but to maintain atomicity between fields,
    // the access must be done under a lock.
    static void read_time_sample_atomic(const struct time_sample_atomic *atomic,
                                        struct time_sample *sample) {
        sample->base_ns = atomic->base_ns.load(std::memory_order_relaxed);
        sample->base_cycles = atomic->base_cycles.load(std::memory_order_relaxed);
        sample->nsscaled_per_cycle =
                atomic->nsscaled_per_cycle.load(std::memory_order_relaxed);
        sample->min_cycles_per_sample =
                atomic->min_cycles_per_sample.load(std::memory_order_relaxed);
        sample->raw_ns = atomic->raw_ns.load(std::memory_order_relaxed);
    }

    // Public routine.
    // Algorithm:  We wish to compute real time from a cycle counter.  In normal
    // operation, we construct a piecewise linear approximation to the kernel time
    // source, using the cycle counter value.  The start of each line segment is at
    // the same point as the end of the last, but may have a different slope (that
    // is, a different idea of the cycle counter frequency).  Every couple of
    // seconds, the kernel time source is sampled and compared with the current
    // approximation.  A new slope is chosen that, if followed for another couple
    // of seconds, will correct the error at the current position.  The information
    // for a sample is in the "last_sample" struct.  The linear approximation is
    //   estimated_time = last_sample.base_ns +
    //     last_sample.ns_per_cycle * (counter_reading - last_sample.base_cycles)
    // (ns_per_cycle is actually stored in different units and scaled, to avoid
    // overflow).  The base_ns of the next linear approximation is the
    // estimated_time using the last approximation; the base_cycles is the cycle
    // counter value at that time; the ns_per_cycle is the number of ns per cycle
    // measured since the last sample, but adjusted so that most of the difference
    // between the estimated_time and the kernel time will be corrected by the
    // estimated time to the next sample.  In normal operation, this algorithm
    // relies on:
    // - the cycle counter and kernel time rates not changing a lot in a few
    //   seconds.
    // - the client calling into the code often compared to a couple of seconds, so
    //   the time to the next correction can be estimated.
    // Any time ns_per_cycle is not known, a major error is detected, or the
    // assumption about frequent calls is violated, the implementation returns the
    // kernel time.  It records sufficient data that a linear approximation can
    // resume a little later.

    int64_t get_current_time_nanos() {
        // read the data from the "last_sample" struct (but don't need raw_ns yet)
        // The reads of "seq" and test of the values emulate a reader lock.
        uint64_t base_ns;
        uint64_t base_cycles;
        uint64_t nsscaled_per_cycle;
        uint64_t min_cycles_per_sample;
        uint64_t seq_read0;
        uint64_t seq_read1;

        // If we have enough information to interpolate, the value returned will be
        // derived from this cycleclock-derived time estimate.  On some platforms
        // (POWER) the function to retrieve this value has enough complexity to
        // contribute to register pressure - reading it early before initializing
        // the other pieces of the calculation minimizes spill/restore instructions,
        // minimizing icache cost.
        uint64_t now_cycles = GET_CURRENT_TIME_NANOS_CYCLECLOCK_NOW();

        // Acquire pairs with the barrier in SeqRelease - if this load sees that
        // store, the shared-data reads necessarily see that SeqRelease's updates
        // to the same shared data.
        seq_read0 = seq.load(std::memory_order_acquire);

        base_ns = last_sample.base_ns.load(std::memory_order_relaxed);
        base_cycles = last_sample.base_cycles.load(std::memory_order_relaxed);
        nsscaled_per_cycle =
                last_sample.nsscaled_per_cycle.load(std::memory_order_relaxed);
        min_cycles_per_sample =
                last_sample.min_cycles_per_sample.load(std::memory_order_relaxed);

        // This acquire fence pairs with the release fence in SeqAcquire.  Since it
        // is sequenced between reads of shared data and seq_read1, the reads of
        // shared data are effectively acquiring.
        std::atomic_thread_fence(std::memory_order_acquire);

        // The shared-data reads are effectively acquire ordered, and the
        // shared-data writes are effectively release ordered. Therefore if our
        // shared-data reads see any of a particular update's shared-data writes,
        // seq_read1 is guaranteed to see that update's SeqAcquire.
        seq_read1 = seq.load(std::memory_order_relaxed);

        // Fast path.  Return if min_cycles_per_sample has not yet elapsed since the
        // last sample, and we read a consistent sample.  The fast path activates
        // only when min_cycles_per_sample is non-zero, which happens when we get an
        // estimate for the cycle time.  The predicate will fail if now_cycles <
        // base_cycles, or if some other thread is in the slow path.
        //
        // Since we now read now_cycles before base_ns, it is possible for now_cycles
        // to be less than base_cycles (if we were interrupted between those loads and
        // last_sample was updated). This is harmless, because delta_cycles will wrap
        // and report a time much much bigger than min_cycles_per_sample. In that case
        // we will take the slow path.
        uint64_t delta_cycles = now_cycles - base_cycles;
        if (seq_read0 == seq_read1 && (seq_read0 & 1) == 0 &&
            delta_cycles < min_cycles_per_sample) {
            return base_ns + ((delta_cycles * nsscaled_per_cycle) >> kScale);
        }
        return get_current_time_nanos_slow_path();
    }

// Return (a << kScale)/b.
// Zero is returned if b==0.   Scaling is performed internally to
// preserve precision without overflow.
    static uint64_t SafeDivideAndScale(uint64_t a, uint64_t b) {
        // Find maximum safe_shift so that
        //  0 <= safe_shift <= kScale  and  (a << safe_shift) does not overflow.
        int safe_shift = kScale;
        while (((a << safe_shift) >> safe_shift) != a) {
            safe_shift--;
        }
        uint64_t scaled_b = b >> (kScale - safe_shift);
        uint64_t quotient = 0;
        if (scaled_b != 0) {
            quotient = (a << safe_shift) / scaled_b;
        }
        return quotient;
    }

    static uint64_t update_last_sample(
            uint64_t now_cycles, uint64_t now_ns, uint64_t delta_cycles,
            const struct time_sample *sample)

    FLARE_COLD;

    // The slow path of get_current_time_nanos().  This is taken while gathering
    // initial samples, when enough time has elapsed since the last sample, and if
    // any other thread is writing to last_sample.
    //
    // Manually mark this 'noinline' to minimize stack frame size of the fast
    // path.  Without this, sometimes a compiler may inline this big block of code
    // into the fast path.  That causes lots of register spills and reloads that
    // are unnecessary unless the slow path is taken.
    //
    // TODO(flare-team): Remove this attribute when our compiler is smart enough
    // to do the right thing.
    FLARE_NO_INLINE
    static int64_t get_current_time_nanos_slow_path() FLARE_LOCKS_EXCLUDED(lock) {
        // Serialize access to slow-path.  Fast-path readers are not blocked yet, and
        // code below must not modify last_sample until the seqlock is acquired.
        lock.lock();

        // Sample the kernel time base.  This is the definition of
        // "now" if we take the slow path.
        static uint64_t last_now_cycles;  // protected by lock
        uint64_t now_cycles;
        uint64_t now_ns = get_current_time_nanos_from_kernel(last_now_cycles, &now_cycles);
        last_now_cycles = now_cycles;

        uint64_t estimated_base_ns;

        // ----------
        // Read the "last_sample" values again; this time holding the write lock.
        struct time_sample sample;
        read_time_sample_atomic(&last_sample, &sample);

        // ----------
        // Try running the fast path again; another thread may have updated the
        // sample between our run of the fast path and the sample we just read.
        uint64_t delta_cycles = now_cycles - sample.base_cycles;
        if (delta_cycles < sample.min_cycles_per_sample) {
            // Another thread updated the sample.  This path does not take the seqlock
            // so that blocked readers can make progress without blocking new readers.
            estimated_base_ns = sample.base_ns +
                                ((delta_cycles * sample.nsscaled_per_cycle) >> kScale);
            stats_fast_slow_paths++;
        } else {
            estimated_base_ns =
                    update_last_sample(now_cycles, now_ns, delta_cycles, &sample);
        }

        lock.unlock();

        return estimated_base_ns;
    }

    // Main part of the algorithm.  Locks out readers, updates the approximation
    // using the new sample from the kernel, and stores the result in last_sample
    // for readers.  Returns the new estimated time.
    static uint64_t update_last_sample(uint64_t now_cycles, uint64_t now_ns,
                                       uint64_t delta_cycles,
                                       const struct time_sample *sample) FLARE_EXCLUSIVE_LOCKS_REQUIRED(lock) {
        uint64_t estimated_base_ns = now_ns;
        uint64_t lock_value = SeqAcquire(&seq);  // acquire seqlock to block readers

        // The 5s in the next if-statement limits the time for which we will trust
        // the cycle counter and our last sample to give a reasonable result.
        // Errors in the rate of the source clock can be multiplied by the ratio
        // between this limit and kMinNSBetweenSamples.
        if (sample->raw_ns == 0 ||  // no recent sample, or clock went backwards
            sample->raw_ns + static_cast<uint64_t>(5) * 1000 * 1000 * 1000 < now_ns ||
            now_ns < sample->raw_ns || now_cycles < sample->base_cycles) {
            // record this sample, and forget any previously known slope.
            last_sample.raw_ns.store(now_ns, std::memory_order_relaxed);
            last_sample.base_ns.store(estimated_base_ns, std::memory_order_relaxed);
            last_sample.base_cycles.store(now_cycles, std::memory_order_relaxed);
            last_sample.nsscaled_per_cycle.store(0, std::memory_order_relaxed);
            last_sample.min_cycles_per_sample.store(0, std::memory_order_relaxed);
            stats_initializations++;
        } else if (sample->raw_ns + 500 * 1000 * 1000 < now_ns &&
                   sample->base_cycles + 50 < now_cycles) {
            // Enough time has passed to compute the cycle time.
            if (sample->nsscaled_per_cycle != 0) {  // Have a cycle time estimate.
                // Compute time from counter reading, but avoiding overflow
                // delta_cycles may be larger than on the fast path.
                uint64_t estimated_scaled_ns;
                int s = -1;
                do {
                    s++;
                    estimated_scaled_ns = (delta_cycles >> s) * sample->nsscaled_per_cycle;
                } while (estimated_scaled_ns / sample->nsscaled_per_cycle !=
                         (delta_cycles >> s));
                estimated_base_ns = sample->base_ns +
                                    (estimated_scaled_ns >> (kScale - s));
            }

            // Compute the assumed cycle time kMinNSBetweenSamples ns into the future
            // assuming the cycle counter rate stays the same as the last interval.
            uint64_t ns = now_ns - sample->raw_ns;
            uint64_t measured_nsscaled_per_cycle = SafeDivideAndScale(ns, delta_cycles);

            uint64_t assumed_next_sample_delta_cycles =
                    SafeDivideAndScale(kMinNSBetweenSamples, measured_nsscaled_per_cycle);

            int64_t diff_ns = now_ns - estimated_base_ns;  // estimate low by this much

            // We want to set nsscaled_per_cycle so that our estimate of the ns time
            // at the assumed cycle time is the assumed ns time.
            // That is, we want to set nsscaled_per_cycle so:
            //  kMinNSBetweenSamples + diff_ns  ==
            //  (assumed_next_sample_delta_cycles * nsscaled_per_cycle) >> kScale
            // But we wish to damp oscillations, so instead correct only most
            // of our current error, by solving:
            //  kMinNSBetweenSamples + diff_ns - (diff_ns / 16) ==
            //  (assumed_next_sample_delta_cycles * nsscaled_per_cycle) >> kScale
            ns = kMinNSBetweenSamples + diff_ns - (diff_ns / 16);
            uint64_t new_nsscaled_per_cycle =
                    SafeDivideAndScale(ns, assumed_next_sample_delta_cycles);
            if (new_nsscaled_per_cycle != 0 &&
                diff_ns < 100 * 1000 * 1000 && -diff_ns < 100 * 1000 * 1000) {
                // record the cycle time measurement
                last_sample.nsscaled_per_cycle.store(
                        new_nsscaled_per_cycle, std::memory_order_relaxed);
                uint64_t new_min_cycles_per_sample =
                        SafeDivideAndScale(kMinNSBetweenSamples, new_nsscaled_per_cycle);
                last_sample.min_cycles_per_sample.store(
                        new_min_cycles_per_sample, std::memory_order_relaxed);
                stats_calibrations++;
            } else {  // something went wrong; forget the slope
                last_sample.nsscaled_per_cycle.store(0, std::memory_order_relaxed);
                last_sample.min_cycles_per_sample.store(0, std::memory_order_relaxed);
                estimated_base_ns = now_ns;
                stats_reinitializations++;
            }
            last_sample.raw_ns.store(now_ns, std::memory_order_relaxed);
            last_sample.base_ns.store(estimated_base_ns, std::memory_order_relaxed);
            last_sample.base_cycles.store(now_cycles, std::memory_order_relaxed);
        } else {
            // have a sample, but no slope; waiting for enough time for a calibration
            stats_slow_paths++;
        }

        SeqRelease(&seq, lock_value);  // release the readers

        return estimated_base_ns;
    }

}  // namespace flare
#endif  // FLARE_USE_CYCLECLOCK_FOR_GET_CURRENT_TIME_NANOS

namespace flare {

    namespace {

// Returns the maximum duration that SleepOnce() can sleep for.
        constexpr flare::duration max_sleep() {
#ifdef _WIN32
            // Windows Sleep() takes unsigned long argument in milliseconds.
            return flare::milliseconds(
                std::numeric_limits<unsigned long>::max());  // NOLINT(runtime/int)
#else
            return flare::duration::seconds(std::numeric_limits<time_t>::max());
#endif
        }

// Sleeps for the given duration.
// REQUIRES: to_sleep <= max_sleep().
        void sleep_once(flare::duration to_sleep) {
#ifdef _WIN32
            Sleep(to_sleep / flare::milliseconds(1));
#else
            struct timespec sleep_time = to_sleep.to_timespec();
            while (nanosleep(&sleep_time, &sleep_time) != 0 && errno == EINTR) {
                // Ignore signals and wait for the full interval to elapse.
            }
#endif
        }

    }  // namespace

}  // namespace flare

extern "C" {

FLARE_WEAK void flare_internal_sleep_for(flare::duration duration) {
    while (duration > flare::zero_duration()) {
        flare::duration to_sleep = std::min(duration, flare::max_sleep());
        flare::sleep_once(to_sleep);
        duration -= to_sleep;
    }
}

}  // extern "C"
