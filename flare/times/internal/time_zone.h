
/****************************************************************
 * Copyright (c) 2022, liyinbin
 * All rights reserved.
 * Author by liyinbin (jeff.li) lijippy@163.com
 *****************************************************************/
#ifndef FLARE_TIMES_INTERNAL_TIME_ZONE_H_
#define FLARE_TIMES_INTERNAL_TIME_ZONE_H_

#include <chrono>
#include <cstdint>
#include <string>
#include <utility>
#include "flare/base/profile.h"
#include "flare/times/internal/chrono_time_internal.h"

namespace flare::times_internal {


    // Convenience aliases. Not intended as public API points.
    template<typename D>
    using time_point = std::chrono::time_point<std::chrono::system_clock, D>;
    using seconds = std::chrono::duration<std::int_fast64_t>;
    using sys_seconds = seconds;  // Deprecated.  Use flare::times_internal::seconds instead.

    namespace times_detail {
        template<typename D>
        FLARE_FORCE_INLINE std::pair<time_point<seconds>, D>

        split_seconds(const time_point<D> &tp) {
            auto sec = std::chrono::time_point_cast<seconds>(tp);
            auto sub = tp - sec;
            if (sub.count() < 0) {
                sec -= seconds(1);
                sub += seconds(1);
            }
            return {sec, std::chrono::duration_cast<D>(sub)};
        }

        FLARE_FORCE_INLINE std::pair<time_point<seconds>, seconds>

        split_seconds(const time_point<seconds> &tp) {
            return {tp, seconds::zero()};
        }
    }  // namespace times_internal

// flare::times_internal::time_zone is an opaque, small, value-type class representing a
// geo-political region within which particular rules are used for mapping
// between absolute and civil times. time_point zones are named using the TZ
// identifiers from the IANA time_point Zone Database, such as "America/Los_Angeles"
// or "Australia/Sydney". time_point zones are created from factory functions such
// as load_time_zone(). Note: strings like "PST" and "EDT" are not valid TZ
// identifiers.
//
// Example:
//   flare::times_internal::time_zone utc = flare::times_internal::utc_time_zone();
//   flare::times_internal::time_zone pst = flare::times_internal::fixed_time_zone(std::chrono::hours(-8));
//   flare::times_internal::time_zone loc = flare::times_internal::local_time_zone();
//   flare::times_internal::time_zone lax;
//   if (!flare::times_internal::load_time_zone("America/Los_Angeles", &lax)) { ... }
//
// See also:
// - http://www.iana.org/time-zones
// - https://en.wikipedia.org/wiki/Zoneinfo
    class time_zone {
    public:
        time_zone() : time_zone(nullptr) {}  // Equivalent to UTC
        time_zone(const time_zone &) = default;

        time_zone &operator=(const time_zone &) = default;

        std::string name() const;

        // An absolute_lookup represents the civil time (flare::times_internal::civil_second) within
        // this time_zone at the given absolute time (time_point). There are
        // additionally a few other fields that may be useful when working with
        // older APIs, such as std::tm.
        //
        // Example:
        //   const flare::times_internal::time_zone tz = ...
        //   const auto tp = std::chrono::system_clock::now();
        //   const flare::times_internal::time_zone::absolute_lookup al = tz.lookup(tp);
        struct absolute_lookup {
            civil_second cs;
            // Note: The following fields exist for backward compatibility with older
            // APIs. Accessing these fields directly is a sign of imprudent logic in
            // the calling code. Modern time-related code should only access this data
            // indirectly by way of flare::times_internal::format().
            int offset;        // civil seconds east of UTC
            bool is_dst;       // is offset non-standard?
            const char *abbr;  // time-zone abbreviation (e.g., "PST")
        };

        absolute_lookup lookup(const time_point<seconds> &tp) const;

        template<typename D>
        absolute_lookup lookup(const time_point<D> &tp) const {
            return lookup(times_detail::split_seconds(tp).first);
        }

        // A civil_lookup represents the absolute time(s) (time_point) that
        // correspond to the given civil time (flare::times_internal::civil_second) within this
        // time_zone. Usually the given civil time represents a unique instant
        // in time, in which case the conversion is unambiguous. However,
        // within this time zone, the given civil time may be skipped (e.g.,
        // during a positive UTC offset shift), or repeated (e.g., during a
        // negative UTC offset shift). To account for these possibilities,
        // civil_lookup is richer than just a single time_point.
        //
        // In all cases the civil_lookup::kind enum will indicate the nature
        // of the given civil-time argument, and the pre, trans, and post
        // members will give the absolute time answers using the pre-transition
        // offset, the transition point itself, and the post-transition offset,
        // respectively (all three times are equal if kind == UNIQUE). If any
        // of these three absolute times is outside the representable range of a
        // time_point<seconds> the field is set to its maximum/minimum value.
        //
        // Example:
        //   flare::times_internal::time_zone lax;
        //   if (!flare::times_internal::load_time_zone("America/Los_Angeles", &lax)) { ... }
        //
        //   // A unique civil time.
        //   auto jan01 = lax.lookup(flare::times_internal::civil_second(2011, 1, 1, 0, 0, 0));
        //   // jan01.kind == flare::times_internal::time_zone::civil_lookup::UNIQUE
        //   // jan01.pre    is 2011/01/01 00:00:00 -0800
        //   // jan01.trans  is 2011/01/01 00:00:00 -0800
        //   // jan01.post   is 2011/01/01 00:00:00 -0800
        //
        //   // A Spring DST transition, when there is a gap in civil time.
        //   auto mar13 = lax.lookup(flare::times_internal::civil_second(2011, 3, 13, 2, 15, 0));
        //   // mar13.kind == flare::times_internal::time_zone::civil_lookup::SKIPPED
        //   // mar13.pre   is 2011/03/13 03:15:00 -0700
        //   // mar13.trans is 2011/03/13 03:00:00 -0700
        //   // mar13.post  is 2011/03/13 01:15:00 -0800
        //
        //   // A Fall DST transition, when civil times are repeated.
        //   auto nov06 = lax.lookup(flare::times_internal::civil_second(2011, 11, 6, 1, 15, 0));
        //   // nov06.kind == flare::times_internal::time_zone::civil_lookup::REPEATED
        //   // nov06.pre   is 2011/11/06 01:15:00 -0700
        //   // nov06.trans is 2011/11/06 01:00:00 -0800
        //   // nov06.post  is 2011/11/06 01:15:00 -0800
        struct civil_lookup {
            enum civil_kind {
                UNIQUE,    // the civil time was singular (pre == trans == post)
                SKIPPED,   // the civil time did not exist (pre >= trans > post)
                REPEATED,  // the civil time was ambiguous (pre < trans <= post)
            } kind;
            time_point<seconds> pre;    // uses the pre-transition offset
            time_point<seconds> trans;  // instant of civil-offset change
            time_point<seconds> post;   // uses the post-transition offset
        };

        civil_lookup lookup(const civil_second &cs) const;

        // Finds the time of the next/previous offset change in this time zone.
        //
        // By definition, next_transition(tp, &trans) returns false when tp has
        // its maximum value, and prev_transition(tp, &trans) returns false
        // when tp has its minimum value. If the zone has no transitions, the
        // result will also be false no matter what the argument.
        //
        // Otherwise, when tp has its minimum value, next_transition(tp, &trans)
        // returns true and sets trans to the first recorded transition. Chains
        // of calls to next_transition()/prev_transition() will eventually return
        // false, but it is unspecified exactly when next_transition(tp, &trans)
        // jumps to false, or what time is set by prev_transition(tp, &trans) for
        // a very distant tp.
        //
        // Note: Enumeration of time-zone transitions is for informational purposes
        // only. Modern time-related code should not care about when offset changes
        // occur.
        //
        // Example:
        //   flare::times_internal::time_zone nyc;
        //   if (!flare::times_internal::load_time_zone("America/New_York", &nyc)) { ... }
        //   const auto now = std::chrono::system_clock::now();
        //   auto tp = flare::times_internal::time_point<flare::times_internal::seconds>::min();
        //   flare::times_internal::time_zone::civil_transition trans;
        //   while (tp <= now && nyc.next_transition(tp, &trans)) {
        //     // transition: trans.from -> trans.to
        //     tp = nyc.lookup(trans.to).trans;
        //   }
        struct civil_transition {
            civil_second from;  // the civil time we jump from
            civil_second to;    // the civil time we jump to
        };

        bool next_transition(const time_point<seconds> &tp,
                             civil_transition *trans) const;

        template<typename D>
        bool next_transition(const time_point<D> &tp, civil_transition *trans) const {
            return next_transition(times_detail::split_seconds(tp).first, trans);
        }

        bool prev_transition(const time_point<seconds> &tp,
                             civil_transition *trans) const;

        template<typename D>
        bool prev_transition(const time_point<D> &tp, civil_transition *trans) const {
            return prev_transition(times_detail::split_seconds(tp).first, trans);
        }

        // version() and description() provide additional information about the
        // time zone. The content of each of the returned strings is unspecified,
        // however, when the IANA time_point Zone Database is the underlying data source
        // the version() std::string will be in the familar form (e.g, "2018e") or
        // empty when unavailable.
        //
        // Note: These functions are for informational or testing purposes only.
        std::string version() const;  // empty when unknown
        std::string description() const;

        // Relational operators.
        friend bool operator==(time_zone lhs, time_zone rhs) {
            return &lhs.effective_impl() == &rhs.effective_impl();
        }

        friend bool operator!=(time_zone lhs, time_zone rhs) { return !(lhs == rhs); }

        class Impl;

    private:
        explicit time_zone(const Impl *impl) : impl_(impl) {}

        const Impl &effective_impl() const;  // handles implicit UTC
        const Impl *impl_;
    };

    // Loads the named time zone. May perform I/O on the initial load.
    // If the name is invalid, or some other kind of error occurs, returns
    // false and "*tz" is set to the UTC time zone.
    bool load_time_zone(const std::string &name, time_zone *tz);

    // Returns a time_zone representing UTC. Cannot fail.
    time_zone utc_time_zone();

    // Returns a time zone that is a fixed offset (seconds east) from UTC.
    // Note: If the absolute value of the offset is greater than 24 hours
    // you'll get UTC (i.e., zero offset) instead.
    time_zone fixed_time_zone(const seconds &offset);

    // Returns a time zone representing the local time zone. Falls back to UTC.
    // Note: local_time_zone.name() may only be something like "localtime".
    time_zone local_time_zone();

    // Returns the civil time (flare::times_internal::civil_second) within the given time zone at
    // the given absolute time (time_point). Since the additional fields provided
    // by the time_zone::absolute_lookup struct should rarely be needed in modern
    // code, this convert() function is simpler and should be preferred.
    template<typename D>
    FLARE_FORCE_INLINE civil_second

    convert(const time_point<D> &tp, const time_zone &tz) {
        return tz.lookup(tp).cs;
    }

    // Returns the absolute time (time_point) that corresponds to the given civil
    // time within the given time zone. If the civil time is not unique (i.e., if
    // it was either repeated or non-existent), then the returned time_point is
    // the best estimate that preserves relative order. That is, this function
    // guarantees that if cs1 < cs2, then convert(cs1, tz) <= convert(cs2, tz).
    FLARE_FORCE_INLINE time_point<seconds>

    convert(const civil_second &cs,
            const time_zone &tz) {
        const time_zone::civil_lookup cl = tz.lookup(cs);
        if (cl.kind == time_zone::civil_lookup::SKIPPED)
            return cl.trans;
        return cl.pre;
    }

    namespace times_detail {
        using femtoseconds = std::chrono::duration<std::int_fast64_t, std::femto>;

        std::string format(const std::string &, const time_point<seconds> &,
                           const femtoseconds &, const time_zone &);

        bool parse(const std::string &, const std::string &, const time_zone &,
                   time_point<seconds> *, femtoseconds *, std::string *err = nullptr);
    }  // namespace times_detail

    // Formats the given time_point in the given flare::times_internal::time_zone according to
    // the provided format string. Uses strftime()-like formatting options,
    // with the following extensions:
    //
    //   - %Ez  - RFC3339-compatible numeric UTC offset (+hh:mm or -hh:mm)
    //   - %E*z - Full-resolution numeric UTC offset (+hh:mm:ss or -hh:mm:ss)
    //   - %E#S - seconds with # digits of fractional precision
    //   - %E*S - seconds with full fractional precision (a literal '*')
    //   - %E#f - Fractional seconds with # digits of precision
    //   - %E*f - Fractional seconds with full precision (a literal '*')
    //   - %E4Y - Four-character years (-999 ... -001, 0000, 0001 ... 9999)
    //
    // Note that %E0S behaves like %S, and %E0f produces no characters. In
    // contrast %E*f always produces at least one digit, which may be '0'.
    //
    // Note that %Y produces as many characters as it takes to fully render the
    // year. A year outside of [-999:9999] when formatted with %E4Y will produce
    // more than four characters, just like %Y.
    //
    // Tip: Format strings should include the UTC offset (e.g., %z, %Ez, or %E*z)
    // so that the resulting string uniquely identifies an absolute time.
    //
    // Example:
    //   flare::times_internal::time_zone lax;
    //   if (!flare::times_internal::load_time_zone("America/Los_Angeles", &lax)) { ... }
    //   auto tp = flare::times_internal::convert(flare::times_internal::civil_second(2013, 1, 2, 3, 4, 5), lax);
    //   std::string f = flare::times_internal::format("%H:%M:%S", tp, lax);  // "03:04:05"
    //   f = flare::times_internal::format("%H:%M:%E3S", tp, lax);            // "03:04:05.000"
    template<typename D>
    FLARE_FORCE_INLINE std::string format(const std::string &fmt, const time_point<D> &tp,
           const time_zone &tz) {
        const auto p = times_detail::split_seconds(tp);
        const auto n = std::chrono::duration_cast<times_detail::femtoseconds>(p.second);
        return times_detail::format(fmt, p.first, n, tz);
    }

    // Parses an input string according to the provided format string and
    // returns the corresponding time_point. Uses strftime()-like formatting
    // options, with the same extensions as flare::times_internal::format(), but with the
    // exceptions that %E#S is interpreted as %E*S, and %E#f as %E*f. %Ez
    // and %E*z also accept the same inputs.
    //
    // %Y consumes as many numeric characters as it can, so the matching data
    // should always be terminated with a non-numeric. %E4Y always consumes
    // exactly four characters, including any sign.
    //
    // Unspecified fields are taken from the default date and time of ...
    //
    //   "1970-01-01 00:00:00.0 +0000"
    //
    // For example, parsing a string of "15:45" (%H:%M) will return a time_point
    // that represents "1970-01-01 15:45:00.0 +0000".
    //
    // Note that parse() returns time instants, so it makes most sense to parse
    // fully-specified date/time strings that include a UTC offset (%z, %Ez, or
    // %E*z).
    //
    // Note also that parse() only heeds the fields year, month, day, hour,
    // minute, (fractional) second, and UTC offset. Other fields, like weekday (%a
    // or %A), while parsed for syntactic validity, are ignored in the conversion.
    //
    // Date and time fields that are out-of-range will be treated as errors rather
    // than normalizing them like flare::times_internal::civil_second() would do. For example, it
    // is an error to parse the date "Oct 32, 2013" because 32 is out of range.
    //
    // A second of ":60" is normalized to ":00" of the following minute with
    // fractional seconds discarded. The following table shows how the given
    // seconds and subseconds will be parsed:
    //
    //   "59.x" -> 59.x  // exact
    //   "60.x" -> 00.0  // normalized
    //   "00.x" -> 00.x  // exact
    //
    // Errors are indicated by returning false.
    //
    // Example:
    //   const flare::times_internal::time_zone tz = ...
    //   std::chrono::system_clock::time_point tp;
    //   if (flare::times_internal::parse("%Y-%m-%d", "2015-10-09", tz, &tp)) {
    //     ...
    //   }
    template<typename D>
    FLARE_FORCE_INLINE bool parse(const std::string &fmt, const std::string &input,
                                  const time_zone &tz, time_point<D> *tpp) {
        time_point<seconds> sec;
        times_detail::femtoseconds fs;
        const bool b = times_detail::parse(fmt, input, tz, &sec, &fs);
        if (b) {
            // TODO: Return false if unrepresentable as a time_point<D>.
            *tpp = std::chrono::time_point_cast<D>(sec);
            *tpp += std::chrono::duration_cast<D>(fs);
        }
        return b;
    }

}  // namespace flare::times_internal
#endif  // FLARE_TIMES_INTERNAL_TIME_ZONE_H_
