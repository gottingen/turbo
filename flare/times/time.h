
/****************************************************************
 * Copyright (c) 2022, liyinbin
 * All rights reserved.
 * Author by liyinbin (jeff.li) lijippy@163.com
 *****************************************************************/
#ifndef FLARE_TIMES_TIME_H_
#define FLARE_TIMES_TIME_H_

#if !defined(_MSC_VER)

#include <sys/time.h>

#else
// We don't include `winsock2.h` because it drags in `windows.h` and friends,
// and they define conflicting macros like OPAQUE, ERROR, and more. This has the
// potential to break flare users.
//
// Instead we only forward declare `timeval` and require Windows users include
// `winsock2.h` themselves. This is both inconsistent and troublesome, but so is
// including 'windows.h' so we are picking the lesser of two evils here.
struct timeval;
#endif

#include <chrono>  // NOLINT(build/c++11)
#include <cmath>
#include <cstdint>
#include <ctime>
#include <ostream>
#include <string>
#include <type_traits>
#include <utility>
#include <string_view>
#include "flare/base/profile.h"
#include "flare/times/duration.h"
#include "flare/times/internal/chrono_time_internal.h"
#include "flare/times/civil_time.h"
#include "flare/times/internal/time_zone.h"

namespace flare {

    // fwd declare
    class time_point;

    // time_now()
    //
    // Returns the current time, expressed as an `flare::time_point` absolute time value.
    time_point time_now();

    // get_current_time_nanos()
    //
    // Returns the current time, expressed as a count of nanoseconds since the Unix
    // Epoch (https://en.wikipedia.org/wiki/Unix_time). Prefer `flare::time_now()` instead
    // for all but the most performance-sensitive cases (i.e. when you are calling
    // this function hundreds of thousands of times per second).
    int64_t get_current_time_nanos();

    inline int64_t get_current_time_micros() {
        return get_current_time_nanos() / 1000;
    }

    // sleep_for()
    //
    // Sleeps for the specified duration, expressed as an `flare::duration`.
    //
    // Notes:
    // * signal interruptions will not reduce the sleep duration.
    // * Returns immediately when passed a nonpositive duration.
    void sleep_for(flare::duration duration);

    class time_zone;  // Defined below

    // time_point
    //
    // An `flare::time_point` represents a specific instant in time. Arithmetic operators
    // are provided for naturally expressing time calculations. Instances are
    // created using `flare::time_now()` and the `flare::From*()` factory functions that
    // accept the gamut of other time representations. Formatting and parsing
    // functions are provided for conversion to and from strings.  `flare::time_point`
    // should be passed by value rather than const reference.
    //
    // `flare::time_point` assumes there are 60 seconds in a minute, which means the
    // underlying time scales must be "smeared" to eliminate leap seconds.
    // See https://developers.google.com/time/smear.
    //
    // Even though `flare::time_point` supports a wide range of timestamps, exercise
    // caution when using values in the distant past. `flare::time_point` uses the
    // Proleptic Gregorian calendar, which extends the Gregorian calendar backward
    // to dates before its introduction in 1582.
    // See https://en.wikipedia.org/wiki/Proleptic_Gregorian_calendar
    // for more information. Use the ICU calendar classes to convert a date in
    // some other calendar (http://userguide.icu-project.org/datetime/calendar).
    //
    // Similarly, standardized time zones are a reasonably recent innovation, with
    // the Greenwich prime meridian being established in 1884. The TZ database
    // itself does not profess accurate offsets for timestamps prior to 1970. The
    // breakdown of future timestamps is subject to the whim of regional
    // governments.
    //
    // The `flare::time_point` class represents an instant in time as a count of clock
    // ticks of some granularity (resolution) from some starting point (epoch).
    //
    // `flare::time_point` uses a resolution that is high enough to avoid loss in
    // precision, and a range that is wide enough to avoid overflow, when
    // converting between tick counts in most Google time scales (i.e., resolution
    // of at least one nanosecond, and range +/-100 billion years).  Conversions
    // between the time scales are performed by truncating (towards negative
    // infinity) to the nearest representable point.
    //
    // Examples:
    //
    //   flare::time_point t1 = ...;
    //   flare::time_point t2 = t1 + flare::minutes(2);
    //   flare::duration d = t2 - t1;  // == flare::minutes(2)
    //
    class time_point {
    public:
        // Value semantics.

        // Returns the Unix epoch.  However, those reading your code may not know
        // or expect the Unix epoch as the default value, so make your code more
        // readable by explicitly initializing all instances before use.
        //
        // Example:
        //   flare::time_point t = flare::unix_epoch();
        //   flare::time_point t = flare::time_now();
        //   flare::time_point t = flare::time_from_timeval(tv);
        //   flare::time_point t = flare::infinite_past();
        constexpr time_point() = default;

        // Copyable.
        constexpr time_point(const time_point &t) = default;

        time_point &operator=(const time_point &t) = default;

        // Assignment operators.
        time_point &operator+=(duration d) {
            rep_ += d;
            return *this;
        }

        time_point &operator-=(duration d) {
            rep_ -= d;
            return *this;
        }

        // time_point::breakdown
        //
        // The calendar and wall-clock (aka "civil time") components of an
        // `flare::time_point` in a certain `flare::time_zone`. This struct is not
        // intended to represent an instant in time. So, rather than passing
        // a `time_point::breakdown` to a function, pass an `flare::time_point` and an
        // `flare::time_zone`.
        //
        // Deprecated. Use `flare::time_zone::chrono_info`.
        struct breakdown {
            int64_t year;          // year (e.g., 2013)
            int month;           // month of year [1:12]
            int day;             // day of month [1:31]
            int hour;            // hour of day [0:23]
            int minute;          // minute of hour [0:59]
            int second;          // second of minute [0:59]
            duration subsecond;  // [seconds(0):seconds(1)) if finite
            int weekday;         // 1==Mon, ..., 7=Sun
            int yearday;         // day of year [1:366]

            // Note: The following fields exist for backward compatibility
            // with older APIs.  Accessing these fields directly is a sign of
            // imprudent logic in the calling code.  Modern time-related code
            // should only access this data indirectly by way of format_time().
            // These fields are undefined for infinite_future() and infinite_past().
            int offset;             // seconds east of UTC
            bool is_dst;            // is offset non-standard?
            const char *zone_abbr;  // time-zone abbreviation (e.g., "PST")
        };

        // time_point::in()
        //
        // Returns the breakdown of this instant in the given time_zone.
        //
        // Deprecated. Use `flare::time_zone::At(time_point)`.
        [[nodiscard]] breakdown in(time_zone tz) const;

    public:
        // to_unix_nanos()
        // to_unix_micros()
        // to_unix_millis()
        // to_unix_seconds()
        // to_time_t()
        // to_date()
        // to_universal()
        //
        // Converts an `flare::time_point` to a variety of other representations.  Note that
        // these operations round down toward negative infinity where necessary to
        // adjust to the resolution of the result type.  Beware of possible time_t
        // over/underflow in ToTime{T,val,spec}() on 32-bit platforms.
        [[nodiscard]] int64_t to_unix_nanos() const;

        [[nodiscard]] int64_t to_unix_micros() const;

        [[nodiscard]] int64_t to_unix_millis() const;

        [[nodiscard]] int64_t to_unix_seconds() const;

        [[nodiscard]] time_t to_time_t() const;

        [[nodiscard]] double to_date() const;

        [[nodiscard]] int64_t to_universal() const;

        [[nodiscard]] timespec to_timespec() const;

        [[nodiscard]] timeval to_timeval() const;

        [[nodiscard]] duration to_duration() const;

        // to_chrono_time()
        //
        // Converts an flare::time_point to a std::chrono::system_clock::time_point. If
        // overflow would occur, the returned value will saturate at the min/max time
        // point value instead.
        //
        // Example:
        //
        //   flare::time_point t = flare::from_time_t(123);
        //   auto tp = flare::to_chrono_time(t);
        //   // tp == std::chrono::system_clock::from_time_t(123);
        [[nodiscard]] std::chrono::system_clock::time_point to_chrono_time() const;


    public:

        // unix_epoch()
        //
        // Returns the `flare::time_point` representing "1970-01-01 00:00:00.0 +0000".
        static constexpr time_point unix_epoch();

        // universal_epoch()
        //
        // Returns the `flare::time_point` representing "0001-01-01 00:00:00.0 +0000", the
        // epoch of the ICU Universal time_point Scale.
        static constexpr time_point universal_epoch();

        // infinite_future()
        //
        // Returns an `flare::time_point` that is infinitely far in the future.
        static constexpr time_point infinite_future();

        // infinite_past()
        //
        // Returns an `flare::time_point` that is infinitely far in the past.
        static constexpr time_point infinite_past();

        // from_unix_nanos()
        // from_unix_micros()
        // from_unix_millis()
        // from_unix_seconds()
        // from_time_t()
        // from_date()
        // from_universal()
        //
        // Creates an `flare::time_point` from a variety of other representations.
        static constexpr time_point from_unix_nanos(int64_t ns);

        static constexpr time_point from_unix_micros(int64_t us);

        static constexpr time_point from_unix_millis(int64_t ms);

        static constexpr time_point from_unix_seconds(int64_t s);

        static constexpr time_point from_time_t(time_t t);

        static time_point from_date(double udate);

        static time_point from_universal(int64_t universal);

        static time_point from_timespec(timespec ts);

        static time_point from_timeval(timeval tv);

        // future_xx
        // make time_point for future from now
        static inline time_point future_timeval(timeval tv);

        static inline time_point future_timespec(timespec ts);

        static inline time_point future_unix_duration(duration d);

        static inline time_point future_unix_nanos(int64_t ns);

        static inline time_point future_unix_micros(int64_t us);

        static inline time_point future_unix_millis(int64_t ms);

        static inline time_point future_unix_seconds(int64_t s);

        static inline time_point future_time_t(time_t t);

        // from_chrono()
        //
        // Converts a std::chrono::system_clock::time_point to an flare::time_point.
        //
        // Example:
        //
        //   auto tp = std::chrono::system_clock::from_time_t(123);
        //   flare::time_point t = flare::from_chrono(tp);
        //   // t == flare::from_time_t(123)
        static time_point from_chrono(const std::chrono::system_clock::time_point &tp);

        // Map between a time_point and a duration since the Unix epoch.  Note that these
        // functions depend on the above mentioned choice of the Unix epoch for the
        // time_point representation (and both need to be time_point friends).  Without this
        // knowledge, we would need to add-in/subtract-out unix_epoch() respectively.
        static constexpr time_point from_unix_duration(duration d) { return time_point(d); }

        static constexpr duration to_unix_duration(time_point t) { return t.rep_; }


    private:

        friend constexpr bool operator<(time_point lhs, time_point rhs);

        friend constexpr bool operator==(time_point lhs, time_point rhs);

        friend duration operator-(time_point lhs, time_point rhs);

        constexpr explicit time_point(duration rep) : rep_(rep) {}

        duration rep_;
    };

    // Relational Operators
    constexpr bool operator<(time_point lhs, time_point rhs) { return lhs.rep_ < rhs.rep_; }

    constexpr bool operator>(time_point lhs, time_point rhs) { return rhs < lhs; }

    constexpr bool operator>=(time_point lhs, time_point rhs) { return !(lhs < rhs); }

    constexpr bool operator<=(time_point lhs, time_point rhs) { return !(rhs < lhs); }

    constexpr bool operator==(time_point lhs, time_point rhs) { return lhs.rep_ == rhs.rep_; }

    constexpr bool operator!=(time_point lhs, time_point rhs) { return !(lhs == rhs); }

    // Additive Operators
    FLARE_FORCE_INLINE time_point operator+(time_point lhs, duration rhs) { return lhs += rhs; }

    FLARE_FORCE_INLINE time_point operator+(duration lhs, time_point rhs) { return rhs += lhs; }

    FLARE_FORCE_INLINE time_point operator-(time_point lhs, duration rhs) { return lhs -= rhs; }

    FLARE_FORCE_INLINE duration operator-(time_point lhs, time_point rhs) { return lhs.rep_ - rhs.rep_; }


    constexpr time_point time_point::unix_epoch() { return time_point(); }


    constexpr time_point time_point::universal_epoch() {
        // 719162 is the number of days from 0001-01-01 to 1970-01-01,
        // assuming the Gregorian calendar.
        return time_point(duration::universal_duration());
    }

    constexpr time_point time_point::infinite_future() {
        return time_point(duration::infinite_future());
    }

    constexpr time_point time_point::infinite_past() {
        return time_point(duration::infinite_pass());
    }


    FLARE_FORCE_INLINE duration time_point::to_duration() const {
        return duration::nanoseconds(to_unix_nanos());
    }




    // Support for flag values of type time_point. time_point flags must be specified in a
    // format that matches flare::RFC3339_full. For example:
    //

    // time_zone
    //
    // The `flare::time_zone` is an opaque, small, value-type class representing a
    // geo-political region within which particular rules are used for converting
    // between absolute and civil times (see https://git.io/v59Ly). `flare::time_zone`
    // values are named using the TZ identifiers from the IANA time_point Zone Database,
    // such as "America/Los_Angeles" or "Australia/Sydney". `flare::time_zone` values
    // are created from factory functions such as `flare::load_time_zone()`. Note:
    // strings like "PST" and "EDT" are not valid TZ identifiers. Prefer to pass by
    // value rather than const reference.
    //
    //
    // Examples:
    //
    //   flare::time_zone utc = flare::utc_time_zone();
    //   flare::time_zone pst = flare::fixed_time_zone(-8 * 60 * 60);
    //   flare::time_zone loc = flare::local_time_zone();
    //   flare::time_zone lax;
    //   if (!flare::load_time_zone("America/Los_Angeles", &lax)) {
    //     // handle error case
    //   }
    //
    class time_zone {
    public:
        explicit time_zone(flare::times_internal::time_zone tz) : cz_(tz) {}

        time_zone() = default;  // UTC, but prefer utc_time_zone() to be explicit.

        // Copyable.
        time_zone(const time_zone &) = default;

        time_zone &operator=(const time_zone &) = default;

        explicit operator flare::times_internal::time_zone() const { return cz_; }

        std::string name() const { return cz_.name(); }

        // time_zone::chrono_info
        //
        // Information about the civil time corresponding to an absolute time.
        // This struct is not intended to represent an instant in time. So, rather
        // than passing a `time_zone::chrono_info` to a function, pass an `flare::time_point`
        // and an `flare::time_zone`.
        struct chrono_info {
            chrono_second cs;
            duration subsecond;

            // Note: The following fields exist for backward compatibility
            // with older APIs.  Accessing these fields directly is a sign of
            // imprudent logic in the calling code.  Modern time-related code
            // should only access this data indirectly by way of format_time().
            // These fields are undefined for infinite_future() and infinite_past().
            int offset;             // seconds east of UTC
            bool is_dst;            // is offset non-standard?
            const char *zone_abbr;  // time-zone abbreviation (e.g., "PST")
        };

        // time_zone::At(time_point)
        //
        // Returns the civil time for this time_zone at a certain `flare::time_point`.
        // If the input time is infinite, the output civil second will be set to
        // chrono_second::max() or min(), and the subsecond will be infinite.
        //
        // Example:
        //
        //   const auto epoch = lax.At(flare::unix_epoch());
        //   // epoch.cs == 1969-12-31 16:00:00
        //   // epoch.subsecond == flare::zero_duration()
        //   // epoch.offset == -28800
        //   // epoch.is_dst == false
        //   // epoch.abbr == "PST"
        chrono_info at(time_point t) const;

        // time_zone::time_info
        //
        // Information about the absolute times corresponding to a civil time.
        // (Subseconds must be handled separately.)
        //
        // It is possible for a caller to pass a civil-time value that does
        // not represent an actual or unique instant in time (due to a shift
        // in UTC offset in the time_zone, which results in a discontinuity in
        // the civil-time components). For example, a daylight-saving-time
        // transition skips or repeats civil times---in the United States,
        // March 13, 2011 02:15 never occurred, while November 6, 2011 01:15
        // occurred twice---so requests for such times are not well-defined.
        // To account for these possibilities, `flare::time_zone::time_info` is
        // richer than just a single `flare::time_point`.
        struct time_info {
            enum chrono_kind {
                UNIQUE,    // the civil time was singular (pre == trans == post)
                SKIPPED,   // the civil time did not exist (pre >= trans > post)
                REPEATED,  // the civil time was ambiguous (pre < trans <= post)
            } kind;
            time_point pre;    // time calculated using the pre-transition offset
            time_point trans;  // when the civil-time discontinuity occurred
            time_point post;   // time calculated using the post-transition offset
        };

        // time_zone::At(chrono_second)
        //
        // Returns an `flare::time_info` containing the absolute time(s) for this
        // time_zone at an `flare::chrono_second`. When the civil time is skipped or
        // repeated, returns times calculated using the pre-transition and post-
        // transition UTC offsets, plus the transition time itself.
        //
        // Examples:
        //
        //   // A unique civil time
        //   const auto jan01 = lax.At(flare::chrono_second(2011, 1, 1, 0, 0, 0));
        //   // jan01.kind == time_zone::time_info::UNIQUE
        //   // jan01.pre    is 2011-01-01 00:00:00 -0800
        //   // jan01.trans  is 2011-01-01 00:00:00 -0800
        //   // jan01.post   is 2011-01-01 00:00:00 -0800
        //
        //   // A Spring DST transition, when there is a gap in civil time
        //   const auto mar13 = lax.At(flare::chrono_second(2011, 3, 13, 2, 15, 0));
        //   // mar13.kind == time_zone::time_info::SKIPPED
        //   // mar13.pre   is 2011-03-13 03:15:00 -0700
        //   // mar13.trans is 2011-03-13 03:00:00 -0700
        //   // mar13.post  is 2011-03-13 01:15:00 -0800
        //
        //   // A Fall DST transition, when civil times are repeated
        //   const auto nov06 = lax.At(flare::chrono_second(2011, 11, 6, 1, 15, 0));
        //   // nov06.kind == time_zone::time_info::REPEATED
        //   // nov06.pre   is 2011-11-06 01:15:00 -0700
        //   // nov06.trans is 2011-11-06 01:00:00 -0800
        //   // nov06.post  is 2011-11-06 01:15:00 -0800
        time_info at(chrono_second ct) const;

        // time_zone::next_transition()
        // time_zone::prev_transition()
        //
        // Finds the time of the next/previous offset change in this time zone.
        //
        // By definition, `next_transition(t, &trans)` returns false when `t` is
        // `infinite_future()`, and `prev_transition(t, &trans)` returns false
        // when `t` is `infinite_past()`. If the zone has no transitions, the
        // result will also be false no matter what the argument.
        //
        // Otherwise, when `t` is `infinite_past()`, `next_transition(t, &trans)`
        // returns true and sets `trans` to the first recorded transition. Chains
        // of calls to `next_transition()/prev_transition()` will eventually return
        // false, but it is unspecified exactly when `next_transition(t, &trans)`
        // jumps to false, or what time is set by `prev_transition(t, &trans)` for
        // a very distant `t`.
        //
        // Note: Enumeration of time-zone transitions is for informational purposes
        // only. Modern time-related code should not care about when offset changes
        // occur.
        //
        // Example:
        //   flare::time_zone nyc;
        //   if (!flare::load_time_zone("America/New_York", &nyc)) { ... }
        //   const auto now = flare::time_now();
        //   auto t = flare::infinite_past();
        //   flare::time_zone::chrono_transition trans;
        //   while (t <= now && nyc.next_transition(t, &trans)) {
        //     // transition: trans.from -> trans.to
        //     t = nyc.At(trans.to).trans;
        //   }
        struct chrono_transition {
            chrono_second from;  // the civil time we jump from
            chrono_second to;    // the civil time we jump to
        };

        bool next_transition(time_point t, chrono_transition *trans) const;

        bool prev_transition(time_point t, chrono_transition *trans) const;


    private:
        friend bool operator==(time_zone a, time_zone b) { return a.cz_ == b.cz_; }

        friend bool operator!=(time_zone a, time_zone b) { return a.cz_ != b.cz_; }

        friend std::ostream &operator<<(std::ostream &os, time_zone tz) {
            return os << tz.name();
        }

        flare::times_internal::time_zone cz_;
    };

    // load_time_zone()
    //
    // Loads the named zone. May perform I/O on the initial load of the named
    // zone. If the name is invalid, or some other kind of error occurs, returns
    // `false` and `*tz` is set to the UTC time zone.
    FLARE_FORCE_INLINE bool load_time_zone(const std::string &name, time_zone *tz) {
        if (name == "localtime") {
            *tz = time_zone(flare::times_internal::local_time_zone());
            return true;
        }
        flare::times_internal::time_zone cz;
        const bool b = times_internal::load_time_zone(name, &cz);
        *tz = time_zone(cz);
        return b;
    }

    // fixed_time_zone()
    //
    // Returns a time_zone that is a fixed offset (seconds east) from UTC.
    // Note: If the absolute value of the offset is greater than 24 hours
    // you'll get UTC (i.e., no offset) instead.
    FLARE_FORCE_INLINE time_zone fixed_time_zone(int seconds) {
        return time_zone(
                flare::times_internal::fixed_time_zone(std::chrono::seconds(seconds)));
    }

    // utc_time_zone()
    //
    // Convenience method returning the UTC time zone.
    FLARE_FORCE_INLINE time_zone utc_time_zone() {
        return time_zone(flare::times_internal::utc_time_zone());
    }

    // local_time_zone()
    //
    // Convenience method returning the local time zone, or UTC if there is
    // no configured local zone.  Warning: Be wary of using local_time_zone(),
    // and particularly so in a server process, as the zone configured for the
    // local machine should be irrelevant.  Prefer an explicit zone name.
    FLARE_FORCE_INLINE time_zone local_time_zone() {
        return time_zone(flare::times_internal::local_time_zone());
    }

    // to_chrono_second()
    // to_chrono_minute()
    // to_chrono_hour()
    // to_chrono_day()
    // to_chrono_month()
    // to_chrono_year()
    //
    // Helpers for time_zone::At(time_point) to return particularly aligned civil times.
    //
    // Example:
    //
    //   flare::time_point t = ...;
    //   flare::time_zone tz = ...;
    //   const auto cd = flare::to_chrono_day(t, tz);
    FLARE_FORCE_INLINE chrono_second to_chrono_second(time_point t, time_zone tz) {
        return tz.at(t).cs;  // already a chrono_second
    }

    FLARE_FORCE_INLINE chrono_minute to_chrono_minute(time_point t, time_zone tz) {
        return chrono_minute(tz.at(t).cs);
    }

    FLARE_FORCE_INLINE chrono_hour to_chrono_hour(time_point t, time_zone tz) {
        return chrono_hour(tz.at(t).cs);
    }

    FLARE_FORCE_INLINE chrono_day to_chrono_day(time_point t, time_zone tz) {
        return chrono_day(tz.at(t).cs);
    }

    FLARE_FORCE_INLINE chrono_month to_chrono_month(time_point t, time_zone tz) {
        return chrono_month(tz.at(t).cs);
    }

    FLARE_FORCE_INLINE chrono_year to_chrono_year(time_point t, time_zone tz) {
        return chrono_year(tz.at(t).cs);
    }

    // from_chrono()
    //
    // Helper for time_zone::At(chrono_second) that provides "order-preserving
    // semantics." If the civil time maps to a unique time, that time is
    // returned. If the civil time is repeated in the given time zone, the
    // time using the pre-transition offset is returned. Otherwise, the
    // civil time is skipped in the given time zone, and the transition time
    // is returned. This means that for any two civil times, ct1 and ct2,
    // (ct1 < ct2) => (from_chrono(ct1) <= from_chrono(ct2)), the equal case
    // being when two non-existent civil times map to the same transition time.
    //
    // Note: Accepts civil times of any alignment.
    FLARE_FORCE_INLINE time_point from_chrono(chrono_second ct, time_zone tz) {
        const auto ti = tz.at(ct);
        if (ti.kind == time_zone::time_info::SKIPPED)
            return ti.trans;
        return ti.pre;
    }

    // time_conversion
    //
    // An `flare::time_conversion` represents the conversion of year, month, day,
    // hour, minute, and second values (i.e., a civil time), in a particular
    // `flare::time_zone`, to a time instant (an absolute time), as returned by
    // `flare::convert_date_time()`. Legacy version of `flare::time_zone::time_info`.
    //
    // Deprecated. Use `flare::time_zone::time_info`.
    struct
    time_conversion {
        time_point pre;    // time calculated using the pre-transition offset
        time_point trans;  // when the civil-time discontinuity occurred
        time_point post;   // time calculated using the post-transition offset

        enum Kind {
            UNIQUE,    // the civil time was singular (pre == trans == post)
            SKIPPED,   // the civil time did not exist
            REPEATED,  // the civil time was ambiguous
        };
        Kind kind;

        bool normalized;  // input values were outside their valid ranges
    };

    // convert_date_time()
    //
    // Legacy version of `flare::time_zone::At(flare::chrono_second)` that takes
    // the civil time as six, separate values (YMDHMS).
    //
    // The input month, day, hour, minute, and second values can be outside
    // of their valid ranges, in which case they will be "normalized" during
    // the conversion.
    //
    // Example:
    //
    //   // "October 32" normalizes to "November 1".
    //   flare::time_conversion tc =
    //       flare::convert_date_time(2013, 10, 32, 8, 30, 0, lax);
    //   // tc.kind == time_conversion::UNIQUE && tc.normalized == true
    //   // flare::to_chrono_day(tc.pre, tz).month() == 11
    //   // flare::to_chrono_day(tc.pre, tz).day() == 1
    //
    // Deprecated. Use `flare::time_zone::At(chrono_second)`.
    time_conversion convert_date_time(int64_t year, int mon, int day, int hour,
                                      int min, int sec, time_zone tz);

    // format_date_time()
    //
    // A convenience wrapper for `flare::convert_date_time()` that simply returns
    // the "pre" `flare::time_point`.  That is, the unique result, or the instant that
    // is correct using the pre-transition offset (as if the transition never
    // happened).
    //
    // Example:
    //
    //   flare::time_point t = flare::format_date_time(2017, 9, 26, 9, 30, 0, lax);
    //   // t = 2017-09-26 09:30:00 -0700
    //
    // Deprecated. Use `flare::from_chrono(chrono_second, time_zone)`. Note that the
    // behavior of `from_chrono()` differs from `format_date_time()` for skipped civil
    // times. If you care about that see `flare::time_zone::At(flare::chrono_second)`.
    FLARE_FORCE_INLINE time_point format_date_time(int64_t year, int mon, int day, int hour,
                                                   int min, int sec, time_zone tz) {
        return convert_date_time(year, mon, day, hour, min, sec, tz).pre;
    }

    // from_tm()
    //
    // Converts the `tm_year`, `tm_mon`, `tm_mday`, `tm_hour`, `tm_min`, and
    // `tm_sec` fields to an `flare::time_point` using the given time zone. See ctime(3)
    // for a description of the expected values of the tm fields. If the indicated
    // time instant is not unique (see `flare::time_zone::at(flare::chrono_second)`
    // above), the `tm_isdst` field is consulted to select the desired instant
    // (`tm_isdst` > 0 means DST, `tm_isdst` == 0 means no DST, `tm_isdst` < 0
    // means use the post-transition offset).
    time_point from_tm(const struct tm &tm, time_zone tz);

    // to_tm()
    //
    // Converts the given `flare::time_point` to a struct tm using the given time zone.
    // See ctime(3) for a description of the values of the tm fields.
    struct tm to_tm(time_point t, time_zone tz);

    inline struct tm local_tm(time_point t) {
        return to_tm(t, flare::local_time_zone());
    }

    inline struct tm utc_tm(time_point t) {
        return to_tm(t, flare::utc_time_zone());
    }

    inline bool operator==(const std::tm &tm1, const std::tm &tm2) {
        return (tm1.tm_sec == tm2.tm_sec && tm1.tm_min == tm2.tm_min && tm1.tm_hour == tm2.tm_hour &&
                tm1.tm_mday == tm2.tm_mday &&
                tm1.tm_mon == tm2.tm_mon && tm1.tm_year == tm2.tm_year && tm1.tm_isdst == tm2.tm_isdst);
    }

    inline bool operator!=(const std::tm &tm1, const std::tm &tm2) {
        return !(tm1 == tm2);
    }

    // RFC3339_full
    // RFC3339_sec
    //
    // format_time()/parse_time() format specifiers for RFC3339 date/time strings,
    // with trailing zeros trimmed or with fractional seconds omitted altogether.
    //
    // Note that RFC3339_sec[] matches an ISO 8601 extended format for date and
    // time with UTC offset.  Also note the use of "%Y": RFC3339 mandates that
    // years have exactly four digits, but we allow them to take their natural
    // width.
    extern const char RFC3339_full[];  // %Y-%m-%dT%H:%M:%E*S%Ez
    extern const char RFC3339_sec[];   // %Y-%m-%dT%H:%M:%S%Ez

    // RFC1123_full
    // RFC1123_no_wday
    //
    // format_time()/parse_time() format specifiers for RFC1123 date/time strings.
    extern const char RFC1123_full[];     // %a, %d %b %E4Y %H:%M:%S %z
    extern const char RFC1123_no_wday[];  // %d %b %E4Y %H:%M:%S %z

    // format_time()
    //
    // Formats the given `flare::time_point` in the `flare::time_zone` according to the
    // provided format string. Uses strftime()-like formatting options, with
    // the following extensions:
    //
    //   - %Ez  - RFC3339-compatible numeric UTC offset (+hh:mm or -hh:mm)
    //   - %E*z - Full-resolution numeric UTC offset (+hh:mm:ss or -hh:mm:ss)
    //   - %E#S - seconds with # digits of fractional precision
    //   - %E*S - seconds with full fractional precision (a literal '*')
    //   - %E#f - Fractional seconds with # digits of precision
    //   - %E*f - Fractional seconds with full precision (a literal '*')
    //   - %E4Y - Four-character years (-999 ... -001, 0000, 0001 ... 9999)
    //
    // Note that %E0S behaves like %S, and %E0f produces no characters.  In
    // contrast %E*f always produces at least one digit, which may be '0'.
    //
    // Note that %Y produces as many characters as it takes to fully render the
    // year.  A year outside of [-999:9999] when formatted with %E4Y will produce
    // more than four characters, just like %Y.
    //
    // We recommend that format strings include the UTC offset (%z, %Ez, or %E*z)
    // so that the result uniquely identifies a time instant.
    //
    // Example:
    //
    //   flare::chrono_second cs(2013, 1, 2, 3, 4, 5);
    //   flare::time_point t = flare::from_chrono(cs, lax);
    //   std::string f = flare::format_time("%H:%M:%S", t, lax);  // "03:04:05"
    //   f = flare::format_time("%H:%M:%E3S", t, lax);  // "03:04:05.000"
    //
    // Note: If the given `flare::time_point` is `flare::infinite_future()`, the returned
    // string will be exactly "infinite-future". If the given `flare::time_point` is
    // `flare::infinite_past()`, the returned string will be exactly "infinite-past".
    // In both cases the given format string and `flare::time_zone` are ignored.
    //
    std::string format_time(const std::string &format, time_point t, time_zone tz);

    // Convenience functions that format the given time using the RFC3339_full
    // format.  The first overload uses the provided time_zone, while the second
    // uses local_time_zone().
    std::string format_time(time_point t, time_zone tz);

    std::string format_time(time_point t);

    // Output stream operator.
    FLARE_FORCE_INLINE std::ostream &operator<<(std::ostream &os, time_point t) {
        return os << format_time(t);
    }

    // parse_time()
    //
    // Parses an input string according to the provided format string and
    // returns the corresponding `flare::time_point`. Uses strftime()-like formatting
    // options, with the same extensions as format_time(), but with the
    // exceptions that %E#S is interpreted as %E*S, and %E#f as %E*f.  %Ez
    // and %E*z also accept the same inputs.
    //
    // %Y consumes as many numeric characters as it can, so the matching data
    // should always be terminated with a non-numeric.  %E4Y always consumes
    // exactly four characters, including any sign.
    //
    // Unspecified fields are taken from the default date and time of ...
    //
    //   "1970-01-01 00:00:00.0 +0000"
    //
    // For example, parsing a string of "15:45" (%H:%M) will return an flare::time_point
    // that represents "1970-01-01 15:45:00.0 +0000".
    //
    // Note that since parse_time() returns time instants, it makes the most sense
    // to parse fully-specified date/time strings that include a UTC offset (%z,
    // %Ez, or %E*z).
    //
    // Note also that `flare::parse_time()` only heeds the fields year, month, day,
    // hour, minute, (fractional) second, and UTC offset.  Other fields, like
    // weekday (%a or %A), while parsed for syntactic validity, are ignored
    // in the conversion.
    //
    // Date and time fields that are out-of-range will be treated as errors
    // rather than normalizing them like `flare::chrono_second` does.  For example,
    // it is an error to parse the date "Oct 32, 2013" because 32 is out of range.
    //
    // A leap second of ":60" is normalized to ":00" of the following minute
    // with fractional seconds discarded.  The following table shows how the
    // given seconds and subseconds will be parsed:
    //
    //   "59.x" -> 59.x  // exact
    //   "60.x" -> 00.0  // normalized
    //   "00.x" -> 00.x  // exact
    //
    // Errors are indicated by returning false and assigning an error message
    // to the "err" out param if it is non-null.
    //
    // Note: If the input string is exactly "infinite-future", the returned
    // `flare::time_point` will be `flare::infinite_future()` and `true` will be returned.
    // If the input string is "infinite-past", the returned `flare::time_point` will be
    // `flare::infinite_past()` and `true` will be returned.
    //
    bool parse_time(const std::string &format, const std::string &input, time_point *time,
                    std::string *err);

    // Like parse_time() above, but if the format string does not contain a UTC
    // offset specification (%z/%Ez/%E*z) then the input is interpreted in the
    // given time_zone.  This means that the input, by itself, does not identify a
    // unique instant.  Being time-zone dependent, it also admits the possibility
    // of ambiguity or non-existence, in which case the "pre" time (as defined
    // by time_zone::time_info) is returned.  For these reasons we recommend that
    // all date/time strings include a UTC offset so they're context independent.
    bool parse_time(const std::string &format, const std::string &input, time_zone tz,
                    time_point *time, std::string *err);


    constexpr time_point time_point::from_unix_nanos(int64_t ns) {
        return time_point::from_unix_duration(duration::nanoseconds(ns));
    }

    constexpr time_point time_point::from_unix_micros(int64_t us) {
        return time_point::from_unix_duration(duration::microseconds(us));
    }

    constexpr time_point time_point::from_unix_millis(int64_t ms) {
        return time_point::from_unix_duration(duration::milliseconds(ms));
    }

    constexpr time_point time_point::from_unix_seconds(int64_t s) {
        return time_point::from_unix_duration(duration::seconds(s));
    }

    constexpr time_point time_point::from_time_t(time_t t) {
        return time_point::from_unix_duration(duration::seconds(t));
    }

    inline time_point time_point::future_timeval(timeval tv) {
        return time_now() + duration::from_timeval(tv);
    }

    inline time_point time_point::future_timespec(timespec ts) {
        return time_now() + duration::from_timespec(ts);
    }

    inline time_point time_point::future_unix_duration(duration d) {
        return time_now() + d;
    }

    inline time_point time_point::future_unix_nanos(int64_t ns) {
        return time_now() + duration::nanoseconds(ns);
    }

    inline time_point time_point::future_unix_micros(int64_t us) {
        return time_now() + duration::microseconds(us);
    }

    inline time_point time_point::future_unix_millis(int64_t ms) {
        return time_now() + duration::milliseconds(ms);
    }

    inline time_point time_point::future_unix_seconds(int64_t s) {
        return time_now() + duration::seconds(s);
    }

    inline time_point time_point::future_time_t(time_t t) {
        return time_now() + duration::seconds(t);
    }

    inline int utc_minutes_offset(const std::tm &tm) {

#ifdef _WIN32
#if _WIN32_WINNT < _WIN32_WINNT_WS08
        TIME_ZONE_INFORMATION tzinfo;
        auto rv = GetTimeZoneInformation(&tzinfo);
#else
        DYNAMIC_TIME_ZONE_INFORMATION tzinfo;
        auto rv = GetDynamicTimeZoneInformation(&tzinfo);
#endif
        if (rv == TIME_ZONE_ID_INVALID)
            throw flare::log_ex("Failed getting timezone info. ", errno);

        int offset = -tzinfo.Bias;
        if (tm.tm_isdst)
        {
            offset -= tzinfo.DaylightBias;
        }
        else
        {
            offset -= tzinfo.StandardBias;
        }
        return offset;
#else

#if defined(sun) || defined(__sun) || defined(_AIX)
        // 'tm_gmtoff' field is BSD extension and it's missing on SunOS/Solaris
        struct helper {
            static long int calculate_gmt_offset(const std::tm &localtm = details::os::localtime(), const std::tm &gmtm = details::os::gmtime())
            {
                int local_year = localtm.tm_year + (1900 - 1);
                int gmt_year = gmtm.tm_year + (1900 - 1);

                long int days = (
                    // difference in day of year
                    localtm.tm_yday -
                    gmtm.tm_yday

                    // + intervening leap days
                    + ((local_year >> 2) - (gmt_year >> 2)) - (local_year / 100 - gmt_year / 100) +
                    ((local_year / 100 >> 2) - (gmt_year / 100 >> 2))

                    // + difference in years * 365 */
                    + (long int)(local_year - gmt_year) * 365);

                long int hours = (24 * days) + (localtm.tm_hour - gmtm.tm_hour);
                long int mins = (60 * hours) + (localtm.tm_min - gmtm.tm_min);
                long int secs = (60 * mins) + (localtm.tm_sec - gmtm.tm_sec);

                return secs;
            }
        };

        auto offset_seconds = helper::calculate_gmt_offset(tm);
#else
        auto offset_seconds = tm.tm_gmtoff;
#endif

        return static_cast<int>(offset_seconds / 60);
#endif
    }

    // ----------------------------------------
    // Control frequency of operations.
    // ----------------------------------------
    // Example:
    //   EveryManyUS every_1s(1000000L);
    //   while (1) {
    //       ...
    //       if (every_1s) {
    //           // be here at most once per second
    //       }
    //   }
    class every_duration {
    public:
        explicit every_duration(duration d)
                : _last_time(time_now()), _interval(d) {}

        operator bool() {
            const auto now = time_now();
            if (now < _last_time + _interval) {
                return false;
            }
            _last_time = now;
            return true;
        }

    private:
        time_point _last_time;
        const duration _interval;
    };

    // ---------------
    //  Count elapses
    // ---------------
    class stop_watcher {
    public:

        enum TimerType {
            STARTED,
        };

        stop_watcher() : _stop(0), _start(0) {}

        explicit stop_watcher(const TimerType) {
            start();
        }

        // Start this timer
        void start() {
            _start = get_current_time_nanos();
            _stop = _start;
        }

        // Stop this timer
        void stop() {
            _stop = get_current_time_nanos();
        }

        // Get the elapse from start() to stop(), in various units.
        [[nodiscard]] constexpr duration elapsed() const { return duration::nanoseconds(_stop - _start); }

        [[nodiscard]] constexpr int64_t n_elapsed() const { return _stop - _start; }

        [[nodiscard]] constexpr int64_t u_elapsed() const { return n_elapsed() / 1000L; }

        [[nodiscard]] constexpr int64_t m_elapsed() const { return u_elapsed() / 1000L; }

        [[nodiscard]] constexpr int64_t s_elapsed() const { return m_elapsed() / 1000L; }

        [[nodiscard]] constexpr double n_elapsed(double) const { return (double) (_stop - _start); }

        [[nodiscard]] constexpr double u_elapsed(double) const { return (double) n_elapsed() / 1000.0; }

        [[nodiscard]] constexpr double m_elapsed(double) const { return (double) u_elapsed() / 1000.0; }

        [[nodiscard]] constexpr double s_elapsed(double) const { return (double) m_elapsed() / 1000.0; }

    private:
        int64_t _stop;
        int64_t _start;
    };

}  // namespace flare

extern "C" {
void flare_internal_sleep_for(flare::duration duration);
}  // extern "C"

FLARE_FORCE_INLINE void flare::sleep_for(flare::duration duration) {
    flare_internal_sleep_for(duration);
}

#endif  // FLARE_TIMES_TIME_H_
