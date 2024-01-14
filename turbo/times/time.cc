// Copyright 2020 The Turbo Authors.
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//      https://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.

// The implementation of the turbo::Time class, which is declared in
// //turbo/time.h.
//
// The representation for an turbo::Time is an turbo::Duration offset from the
// epoch.  We use the traditional Unix epoch (1970-01-01 00:00:00 +0000)
// for convenience, but this is not exposed in the API and could be changed.
//
// NOTE: To keep type verbosity to a minimum, the following variable naming
// conventions are used throughout this file.
//
// tz: An turbo::TimeZone
// ci: An turbo::TimeZone::CivilInfo
// ti: An turbo::TimeZone::TimeInfo
// cd: An turbo::CivilDay or a cctz::civil_day
// cs: An turbo::CivilSecond or a cctz::civil_second
// bd: An turbo::Time::Breakdown
// cl: A cctz::time_zone::civil_lookup
// al: A cctz::time_zone::absolute_lookup

#include "turbo/times/time.h"

#if defined(_MSC_VER)
#include <winsock2.h>  // for timeval
#endif

#include <cstring>
#include <ctime>
#include <limits>

#include "turbo/times/cctz/civil_time.h"
#include "turbo/times/cctz/time_zone.h"

namespace cctz = turbo::time_internal::cctz;

namespace turbo {

    namespace {

        inline cctz::time_point <cctz::seconds> cctz_unix_epoch() {
            return std::chrono::time_point_cast<cctz::seconds>(
                    std::chrono::system_clock::from_time_t(0));
        }

        // Floors d to the next unit boundary closer to negative infinity.
        inline int64_t FloorToUnit(turbo::Duration d, turbo::Duration unit) {
            turbo::Duration rem;
            int64_t q = turbo::safe_int_mod(d, unit, &rem);
            return (q > 0 || rem >= Duration::zero() ||
                    q == std::numeric_limits<int64_t>::min())
                   ? q
                   : q - 1;
        }

        inline turbo::CivilInfo InfiniteFutureCivilInfo() {
            CivilInfo ci;
            ci.cs = CivilSecond::max();
            ci.subsecond = Duration::infinite();
            ci.offset = 0;
            ci.is_dst = false;
            ci.zone_abbr = "-00";
            return ci;
        }

        inline turbo::CivilInfo InfinitePastCivilInfo() {
            CivilInfo ci;
            ci.cs = CivilSecond::min();
            ci.subsecond = -Duration::infinite();
            ci.offset = 0;
            ci.is_dst = false;
            ci.zone_abbr = "-00";
            return ci;
        }

        inline turbo::TimeConversion InfiniteFutureTimeConversion() {
            turbo::TimeConversion tc;
            tc.pre = tc.trans = tc.post = turbo::infinite_future();
            tc.kind = turbo::TimeConversion::UNIQUE;
            tc.normalized = true;
            return tc;
        }

        inline TimeConversion InfinitePastTimeConversion() {
            turbo::TimeConversion tc;
            tc.pre = tc.trans = tc.post = turbo::infinite_past();
            tc.kind = turbo::TimeConversion::UNIQUE;
            tc.normalized = true;
            return tc;
        }

        // Makes a Time from sec, overflowing to infinite_future/infinite_past as
        // necessary. If sec is min/max, then consult cs+tz to check for overlow.
        Time MakeTimeWithOverflow(const cctz::time_point <cctz::seconds> &sec,
                                  const cctz::civil_second &cs,
                                  const cctz::time_zone &tz,
                                  bool *normalized = nullptr) {
            const auto max = cctz::time_point<cctz::seconds>::max();
            const auto min = cctz::time_point<cctz::seconds>::min();
            if (sec == max) {
                const auto al = tz.lookup(max);
                if (cs > al.cs) {
                    if (normalized) *normalized = true;
                    return turbo::infinite_future();
                }
            }
            if (sec == min) {
                const auto al = tz.lookup(min);
                if (cs < al.cs) {
                    if (normalized) *normalized = true;
                    return turbo::infinite_past();
                }
            }
            const auto hi = (sec - cctz_unix_epoch()).count();
            return time_internal::FromUnixDuration(time_internal::MakeDuration(hi));
        }

        // Returns Mon=1..Sun=7.
        inline int MapWeekday(const cctz::weekday &wd) {
            switch (wd) {
                case cctz::weekday::monday:
                    return 1;
                case cctz::weekday::tuesday:
                    return 2;
                case cctz::weekday::wednesday:
                    return 3;
                case cctz::weekday::thursday:
                    return 4;
                case cctz::weekday::friday:
                    return 5;
                case cctz::weekday::saturday:
                    return 6;
                case cctz::weekday::sunday:
                    return 7;
            }
            return 1;
        }

        bool FindTransition(const cctz::time_zone &tz,
                            bool (cctz::time_zone::*find_transition)(
                                    const cctz::time_point <cctz::seconds> &tp,
                                    cctz::time_zone::civil_transition *trans) const,
                            Time t, TimeZone::CivilTransition *trans) {
            // Transitions are second-aligned, so we can discard any fractional part.
            const auto tp = cctz_unix_epoch() + cctz::seconds(to_unix_seconds(t));
            cctz::time_zone::civil_transition tr;
            if (!(tz.*find_transition)(tp, &tr)) return false;
            trans->from = CivilSecond(tr.from);
            trans->to = CivilSecond(tr.to);
            return true;
        }

    }  // namespace

    //
    // Conversions from/to other time types.
    //

    turbo::Time from_udate(double udate) {
        return time_internal::FromUnixDuration(turbo::milliseconds(udate));
    }

    turbo::Time from_universal(int64_t universal) {
        return turbo::universal_epoch() + 100 * turbo::nanoseconds(universal);
    }

    int64_t to_unix_nanos(Time t) {
        if (time_internal::GetRepHi(time_internal::ToUnixDuration(t)) >= 0 &&
            time_internal::GetRepHi(time_internal::ToUnixDuration(t)) >> 33 == 0) {
            return (time_internal::GetRepHi(time_internal::ToUnixDuration(t)) *
                    1000 * 1000 * 1000) +
                   (time_internal::GetRepLo(time_internal::ToUnixDuration(t)) / 4);
        }
        return FloorToUnit(time_internal::ToUnixDuration(t), turbo::nanoseconds(1));
    }

    int64_t to_unix_micros(Time t) {
        if (time_internal::GetRepHi(time_internal::ToUnixDuration(t)) >= 0 &&
            time_internal::GetRepHi(time_internal::ToUnixDuration(t)) >> 43 == 0) {
            return (time_internal::GetRepHi(time_internal::ToUnixDuration(t)) *
                    1000 * 1000) +
                   (time_internal::GetRepLo(time_internal::ToUnixDuration(t)) / 4000);
        }
        return FloorToUnit(time_internal::ToUnixDuration(t), turbo::microseconds(1));
    }

    int64_t to_unix_millis(Time t) {
        if (time_internal::GetRepHi(time_internal::ToUnixDuration(t)) >= 0 &&
            time_internal::GetRepHi(time_internal::ToUnixDuration(t)) >> 53 == 0) {
            return (time_internal::GetRepHi(time_internal::ToUnixDuration(t)) * 1000) +
                   (time_internal::GetRepLo(time_internal::ToUnixDuration(t)) /
                    (4000 * 1000));
        }
        return FloorToUnit(time_internal::ToUnixDuration(t), turbo::milliseconds(1));
    }

    int64_t to_unix_seconds(Time t) {
        return time_internal::GetRepHi(time_internal::ToUnixDuration(t));
    }

    time_t to_time_t(Time t) { return turbo::to_timespec(t).tv_sec; }

    double to_udate(Time t) {
        return time_internal::ToUnixDuration(t).safe_float_mod(turbo::milliseconds(1));
    }

    int64_t to_universal(turbo::Time t) {
        return turbo::FloorToUnit(t - turbo::universal_epoch(), turbo::nanoseconds(100));
    }

    turbo::Time time_from_timespec(timespec ts) {
        return time_internal::FromUnixDuration(turbo::duration_from_timespec(ts));
    }

    turbo::Time time_from_timeval(timeval tv) {
        return time_internal::FromUnixDuration(turbo::duration_from_timeval(tv));
    }

    timespec to_timespec(Time t) {
        timespec ts;
        turbo::Duration d = time_internal::ToUnixDuration(t);
        if (!d.is_infinite()) {
            ts.tv_sec = static_cast<decltype(ts.tv_sec)>(time_internal::GetRepHi(d));
            if (ts.tv_sec == time_internal::GetRepHi(d)) {  // no time_t narrowing
                ts.tv_nsec = time_internal::GetRepLo(d) / 4;  // floor
                return ts;
            }
        }
        if (d >= turbo::Duration::zero()) {
            ts.tv_sec = std::numeric_limits<time_t>::max();
            ts.tv_nsec = 1000 * 1000 * 1000 - 1;
        } else {
            ts.tv_sec = std::numeric_limits<time_t>::min();
            ts.tv_nsec = 0;
        }
        return ts;
    }

    timeval to_timeval(Time t) {
        timeval tv;
        timespec ts = turbo::to_timespec(t);
        tv.tv_sec = static_cast<decltype(tv.tv_sec)>(ts.tv_sec);
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

    Time from_chrono(const std::chrono::system_clock::time_point &tp) {
        return time_internal::FromUnixDuration(time_internal::from_chrono(
                tp - std::chrono::system_clock::from_time_t(0)));
    }

    std::chrono::system_clock::time_point to_chrono_time(turbo::Time t) {
        using D = std::chrono::system_clock::duration;
        auto d = time_internal::ToUnixDuration(t);
        if (d < Duration::zero()) d = floor(d, from_chrono(D{1}));
        return std::chrono::system_clock::from_time_t(0) + d.ToChronoDuration<D>();
    }

    //
    // TimeZone
    //

    turbo::CivilInfo TimeZone::at(Time t) const {
        if (t == turbo::infinite_future()) return InfiniteFutureCivilInfo();
        if (t == turbo::infinite_past()) return InfinitePastCivilInfo();

        const auto ud = time_internal::ToUnixDuration(t);
        const auto tp = cctz_unix_epoch() + cctz::seconds(time_internal::GetRepHi(ud));
        const auto al = cz_.lookup(tp);

        CivilInfo ci;
        ci.cs = CivilSecond(al.cs);
        ci.subsecond = time_internal::MakeDuration(0, time_internal::GetRepLo(ud));
        ci.offset = al.offset;
        ci.is_dst = al.is_dst;
        ci.zone_abbr = al.abbr;
        return ci;
    }

    turbo::TimeZone::TimeInfo TimeZone::At(CivilSecond ct) const {
        const cctz::civil_second cs(ct);
        const auto cl = cz_.lookup(cs);

        TimeZone::TimeInfo ti;
        switch (cl.kind) {
            case cctz::time_zone::civil_lookup::UNIQUE:
                ti.kind = TimeZone::TimeInfo::UNIQUE;
                break;
            case cctz::time_zone::civil_lookup::SKIPPED:
                ti.kind = TimeZone::TimeInfo::SKIPPED;
                break;
            case cctz::time_zone::civil_lookup::REPEATED:
                ti.kind = TimeZone::TimeInfo::REPEATED;
                break;
        }
        ti.pre = MakeTimeWithOverflow(cl.pre, cs, cz_);
        ti.trans = MakeTimeWithOverflow(cl.trans, cs, cz_);
        ti.post = MakeTimeWithOverflow(cl.post, cs, cz_);
        return ti;
    }

    bool TimeZone::NextTransition(Time t, CivilTransition *trans) const {
        return FindTransition(cz_, &cctz::time_zone::next_transition, t, trans);
    }

    bool TimeZone::PrevTransition(Time t, CivilTransition *trans) const {
        return FindTransition(cz_, &cctz::time_zone::prev_transition, t, trans);
    }

//
// Conversions involving time zones.
//

    turbo::TimeConversion convert_date_time(int64_t year, int mon, int day, int hour,
                                          int min, int sec, TimeZone tz) {
        // Avoids years that are too extreme for CivilSecond to normalize.
        if (year > 300000000000) return InfiniteFutureTimeConversion();
        if (year < -300000000000) return InfinitePastTimeConversion();

        const CivilSecond cs(year, mon, day, hour, min, sec);
        const auto ti = tz.At(cs);

        TimeConversion tc;
        tc.pre = ti.pre;
        tc.trans = ti.trans;
        tc.post = ti.post;
        switch (ti.kind) {
            case TimeZone::TimeInfo::UNIQUE:
                tc.kind = TimeConversion::UNIQUE;
                break;
            case TimeZone::TimeInfo::SKIPPED:
                tc.kind = TimeConversion::SKIPPED;
                break;
            case TimeZone::TimeInfo::REPEATED:
                tc.kind = TimeConversion::REPEATED;
                break;
        }
        tc.normalized = false;
        if (year != cs.year() || mon != cs.month() || day != cs.day() ||
            hour != cs.hour() || min != cs.minute() || sec != cs.second()) {
            tc.normalized = true;
        }
        return tc;
    }

    turbo::Time from_tm(const struct tm &tm, turbo::TimeZone tz) {
        civil_year_t tm_year = tm.tm_year;
        // Avoids years that are too extreme for CivilSecond to normalize.
        if (tm_year > 300000000000ll) return infinite_future();
        if (tm_year < -300000000000ll) return infinite_past();
        int tm_mon = tm.tm_mon;
        if (tm_mon == std::numeric_limits<int>::max()) {
            tm_mon -= 12;
            tm_year += 1;
        }
        const auto ti = tz.At(CivilSecond(tm_year + 1900, tm_mon + 1, tm.tm_mday,
                                          tm.tm_hour, tm.tm_min, tm.tm_sec));
        return tm.tm_isdst == 0 ? ti.post : ti.pre;
    }

    struct tm to_tm(turbo::Time t, turbo::TimeZone tz) {
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
            case Weekday::sunday:
                tm.tm_wday = 0;
                break;
            case Weekday::monday:
                tm.tm_wday = 1;
                break;
            case Weekday::tuesday:
                tm.tm_wday = 2;
                break;
            case Weekday::wednesday:
                tm.tm_wday = 3;
                break;
            case Weekday::thursday:
                tm.tm_wday = 4;
                break;
            case Weekday::friday:
                tm.tm_wday = 5;
                break;
            case Weekday::saturday:
                tm.tm_wday = 6;
                break;
        }
        tm.tm_yday = get_year_day(cs) - 1;
        tm.tm_isdst = ci.is_dst ? 1 : 0;

        return tm;
    }

}  // namespace turbo
