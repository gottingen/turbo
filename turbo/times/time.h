// Copyright (C) 2024 EA group inc.
// Author: Jeff.li lijippy@163.com
// All rights reserved.
// This program is free software: you can redistribute it and/or modify
// it under the terms of the GNU Affero General Public License as published
// by the Free Software Foundation, either version 3 of the License, or
// (at your option) any later version.
//
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU Affero General Public License for more details.
//
// You should have received a copy of the GNU Affero General Public License
// along with this program.  If not, see <https://www.gnu.org/licenses/>.
//
//
// -----------------------------------------------------------------------------
// File: time.h
// -----------------------------------------------------------------------------
//
// This header file defines abstractions for computing with absolute points
// in time, durations of time, and formatting and parsing time within a given
// time zone. The following abstractions are defined:
//
//  * `turbo::Time` defines an absolute, specific instance in time
//  * `turbo::Duration` defines a signed, fixed-length span of time
//  * `turbo::TimeZone` defines geopolitical time zone regions (as collected
//     within the IANA Time Zone database (https://www.iana.org/time-zones)).
//
// Note: Absolute times are distinct from civil times, which refer to the
// human-scale time commonly represented by `YYYY-MM-DD hh:mm:ss`. The mapping
// between absolute and civil times can be specified by use of time zones
// (`turbo::TimeZone` within this API). That is:
//
//   Civil Time = F(Absolute Time, Time Zone)
//   Absolute Time = G(Civil Time, Time Zone)
//
// See civil_time.h for abstractions related to constructing and manipulating
// civil time.
//
// Example:
//
//   turbo::TimeZone nyc;
//   // TimeZone::load() may fail so it's always better to check for success.
//   if (!turbo::TimeZone::load("America/New_York", &nyc)) {
//      // handle error case
//   }
//
//   // My flight leaves NYC on Jan 2, 2017 at 03:04:05
//   turbo::CivilSecond cs(2017, 1, 2, 3, 4, 5);
//   turbo::Time takeoff = turbo::Time::from_civil(cs, nyc);
//
//   turbo::Duration flight_duration = turbo::Duration::hours(21) + turbo::Duration::minutes(35);
//   turbo::Time landing = takeoff + flight_duration;
//
//   turbo::TimeZone syd;
//   if (!turbo::TimeZone::load("Australia/Sydney", &syd)) {
//      // handle error case
//   }
//   std::string s = turbo::Time::format(
//       "My flight will land in Sydney on %Y-%m-%d at %H:%M:%S",
//       landing, syd);

#ifndef TURBO_TIME_TIME_H_
#define TURBO_TIME_TIME_H_

#if !defined(_MSC_VER)

#include <sys/time.h>

#else
// We don't include `winsock2.h` because it drags in `windows.h` and friends,
// and they define conflicting macros like OPAQUE, ERROR, and more. This has the
// potential to break Turbo users.
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
#include <limits>
#include <ostream>
#include <string>
#include <type_traits>
#include <utility>

#include <turbo/base/config.h>
#include <turbo/base/macros.h>
#include <turbo/strings/string_view.h>
#include <turbo/times/civil_time.h>
#include <turbo/times/cctz/time_zone.h>

namespace turbo {
    TURBO_NAMESPACE_BEGIN

    class Duration;  // Defined below
    class Time;      // Defined below
    class TimeZone;  // Defined below

    namespace time_internal {
        TURBO_ATTRIBUTE_CONST_FUNCTION constexpr Time FromUnixDuration(Duration d);

        TURBO_ATTRIBUTE_CONST_FUNCTION constexpr Duration ToUnixDuration(Time t);

        TURBO_ATTRIBUTE_CONST_FUNCTION constexpr int64_t GetRepHi(Duration d);

        TURBO_ATTRIBUTE_CONST_FUNCTION constexpr uint32_t GetRepLo(Duration d);

        TURBO_ATTRIBUTE_CONST_FUNCTION constexpr Duration MakeDuration(int64_t hi,
                                                                       uint32_t lo);

        TURBO_ATTRIBUTE_CONST_FUNCTION constexpr Duration MakeDuration(int64_t hi,
                                                                       int64_t lo);

        TURBO_ATTRIBUTE_CONST_FUNCTION inline Duration MakePosDoubleDuration(double n);

        constexpr int64_t kTicksPerNanosecond = 4;
        constexpr int64_t kTicksPerSecond = 1000 * 1000 * 1000 * kTicksPerNanosecond;

        template<std::intmax_t N>
        TURBO_ATTRIBUTE_CONST_FUNCTION constexpr Duration FromInt64(int64_t v,
                                                                    std::ratio<1, N>);

        TURBO_ATTRIBUTE_CONST_FUNCTION constexpr Duration FromInt64(int64_t v,
                                                                    std::ratio<60>);

        TURBO_ATTRIBUTE_CONST_FUNCTION constexpr Duration FromInt64(int64_t v,
                                                                    std::ratio<3600>);

        template<typename T>
        using EnableIfIntegral = typename std::enable_if<
                std::is_integral<T>::value || std::is_enum<T>::value, int>::type;
        template<typename T>
        using EnableIfFloat =
                typename std::enable_if<std::is_floating_point<T>::value, int>::type;
    }  // namespace time_internal

    // Duration
    //
    // The `turbo::Duration` class represents a signed, fixed-length amount of time.
    // A `Duration` is generated using a unit-specific factory function, or is
    // the result of subtracting one `turbo::Time` from another. Durations behave
    // like unit-safe integers and they support all the natural integer-like
    // arithmetic operations. Arithmetic overflows and saturates at +/- infinity.
    // `Duration` should be passed by value rather than const reference.
    //
    // Factory functions `Duration::nanoseconds()`, `Duration::microseconds()`, `Duration::milliseconds()`,
    // `Duration::seconds()`, `Duration::minutes()`, `Duration::hours()` and `Duration::max_infinite()` allow for
    // creation of constexpr `Duration` values
    //
    // Examples:
    //
    //   constexpr turbo::Duration ten_ns = turbo::Duration::nanoseconds(10);
    //   constexpr turbo::Duration min = turbo::Duration::minutes(1);
    //   constexpr turbo::Duration hour = turbo::Duration::hours(1);
    //   turbo::Duration dur = 60 * min;  // dur == hour
    //   turbo::Duration half_sec = turbo::Duration::milliseconds(500);
    //   turbo::Duration quarter_sec = 0.25 * turbo::Duration::seconds(1);
    //
    // `Duration` values can be easily converted to an integral number of units
    // using the division operator.
    //
    // Example:
    //
    //   constexpr turbo::Duration dur = turbo::Duration::milliseconds(1500);
    //   int64_t ns = dur / turbo::Duration::nanoseconds(1);   // ns == 1500000000
    //   int64_t ms = dur / turbo::Duration::milliseconds(1);  // ms == 1500
    //   int64_t sec = dur / turbo::Duration::seconds(1);    // sec == 1 (subseconds truncated)
    //   int64_t min = dur / turbo::Duration::minutes(1);    // min == 0
    //
    // See the `Duration::idiv()` and `Duration::fdiv()` functions below for details on
    // how to access the fractional parts of the quotient.
    //
    // Alternatively, conversions can be performed using helpers such as
    // `Duration::to_microseconds()` and `Duration::to_double_seconds()`.
    class Duration {
    public:
        // Value semantics.
        constexpr Duration() : rep_hi_(0), rep_lo_(0) {}  // zero-length duration

        // Copyable.
#if !defined(__clang__) && defined(_MSC_VER) && _MSC_VER < 1930
        // Explicitly defining the constexpr copy constructor avoids an MSVC bug.
        constexpr Duration(const Duration& d)
            : rep_hi_(d.rep_hi_), rep_lo_(d.rep_lo_) {}
#else

        constexpr Duration(const Duration &d) = default;

#endif

        Duration &operator=(const Duration &d) = default;

        // Compound assignment operators.
        Duration &operator+=(Duration d);

        Duration &operator-=(Duration d);

        Duration &operator*=(int64_t r);

        Duration &operator*=(double r);

        Duration &operator/=(int64_t r);

        Duration &operator/=(double r);

        Duration &operator%=(Duration rhs);

        // Overloads that forward to either the int64_t or double overloads above.
        // Integer operands must be representable as int64_t. Integer division is
        // truncating, so values less than the resolution will be returned as zero.
        // Floating-point multiplication and division is rounding (halfway cases
        // rounding away from zero), so values less than the resolution may be
        // returned as either the resolution or zero.  In particular, `d / 2.0`
        // can produce `d` when it is the resolution and "even".
        template<typename T, time_internal::EnableIfIntegral<T> = 0>
        Duration &operator*=(T r) {
            int64_t x = r;
            return *this *= x;
        }

        template<typename T, time_internal::EnableIfIntegral<T> = 0>
        Duration &operator/=(T r) {
            int64_t x = r;
            return *this /= x;
        }

        template<typename T, time_internal::EnableIfFloat<T> = 0>
        Duration &operator*=(T r) {
            double x = r;
            return *this *= x;
        }

        template<typename T, time_internal::EnableIfFloat<T> = 0>
        Duration &operator/=(T r) {
            double x = r;
            return *this /= x;
        }

        template<typename H>
        friend H turbo_hash_value(H h, Duration d) {
            return H::combine(std::move(h), d.rep_hi_.Get(), d.rep_lo_);
        }

    public:
        // Duration::idiv()
        //
        // Divides a numerator `Duration` by a denominator `Duration`, returning the
        // quotient and remainder. The remainder always has the same sign as the
        // numerator. The returned quotient and remainder respect the identity:
        //
        //   numerator = denominator * quotient + remainder
        //
        // Returned quotients are capped to the range of `int64_t`, with the difference
        // spilling into the remainder to uphold the above identity. This means that the
        // remainder returned could differ from the remainder returned by
        // `Duration::operator%` for huge quotients.
        //
        // See also the notes on `Duration::max_infinite()` below regarding the behavior of
        // division involving zero and infinite durations.
        //
        // Example:
        //
        //   constexpr turbo::Duration a =
        //       turbo::Duration::seconds(std::numeric_limits<int64_t>::max());  // big
        //   constexpr turbo::Duration b = turbo::Duration::nanoseconds(1);       // small
        //
        //   turbo::Duration rem = a % b;
        //   // rem == turbo::Duration::zero()
        //
        //   // Here, q would overflow int64_t, so rem accounts for the difference.
        //   int64_t q = turbo::Duration::idiv(a, b, &rem);
        //   // q == std::numeric_limits<int64_t>::max(), rem == a - b * q
        static int64_t idiv(Duration num, Duration den, Duration *rem);

        // Duration::fdiv()
        //
        // Divides a `Duration` numerator into a fractional number of units of a
        // `Duration` denominator.
        //
        // See also the notes on `Duration::max_infinite()` below regarding the behavior of
        // division involving zero and infinite durations.
        //
        // Example:
        //
        //   double d = turbo::Duration::fdiv(turbo::Duration::milliseconds(1500), turbo::Duration::seconds(1));
        //   // d == 1.5
        TURBO_ATTRIBUTE_CONST_FUNCTION static double fdiv(Duration num, Duration den);

    public:
        // Duration::zero()
        //
        // Returns a zero-length duration. This function behaves just like the default
        // constructor, but the name helps make the semantics clear at call sites.
        TURBO_ATTRIBUTE_CONST_FUNCTION static constexpr Duration zero();
        // Duration::max_infinite()/Duration::min_infinite()
        //
        // Returns an infinite `Duration`.  To get a `Duration` representing negative
        // infinity, use `Duration::min_infinite()`.
        //
        // Duration arithmetic overflows to +/- infinity and saturates. In general,
        // arithmetic with `Duration` infinities is similar to IEEE 754 infinities
        // except where IEEE 754 NaN would be involved, in which case +/-
        // `Duration::max_infinite()` is used in place of a "nan" Duration.
        //
        // Examples:
        //
        //   constexpr turbo::Duration inf = turbo::Duration::max_infinite();
        //   const turbo::Duration d = ... any finite duration ...
        //
        //   inf == inf + inf
        //   inf == inf + d
        //   inf == inf - inf
        //   -inf == d - inf
        //
        //   inf == d * 1e100
        //   inf == inf / 2
        //   0 == d / inf
        //   INT64_MAX == inf / d
        //
        //   d < inf
        //   -inf < d
        //
        //   // Division by zero returns infinity, or INT64_MIN/MAX where appropriate.
        //   inf == d / 0
        //   INT64_MAX == d / turbo::Duration::zero()
        //
        // The examples involving the `/` operator above also apply to `Duration::idiv()`
        // and `Duration::fdiv()`.
        TURBO_ATTRIBUTE_CONST_FUNCTION static constexpr Duration max_infinite();

        TURBO_ATTRIBUTE_CONST_FUNCTION static constexpr Duration min_infinite();
        // Duration::abs()
        //
        // Returns the absolute value of a duration.
        TURBO_ATTRIBUTE_CONST_FUNCTION static Duration abs(Duration d);

        // Duration::trunc()
        //
        // Truncates a duration (toward zero) to a multiple of a non-zero unit.
        //
        // Example:
        //
        //   turbo::Duration d = turbo::Duration::nanoseconds(123456789);
        //   turbo::Duration a = turbo::Duration::trunc(d, turbo::Duration::microseconds(1));  // 123456us
        TURBO_ATTRIBUTE_CONST_FUNCTION static Duration trunc(Duration d, Duration unit);

        // Duration::floor()
        //
        // Floors a duration using the passed duration unit to its largest value not
        // greater than the duration.
        //
        // Example:
        //
        //   turbo::Duration d = turbo::Duration::nanoseconds(123456789);
        //   turbo::Duration b = turbo::Duration::floor(d, turbo::Duration::microseconds(1));  // 123456us
        TURBO_ATTRIBUTE_CONST_FUNCTION static Duration floor(Duration d, Duration unit);

        // Duration::ceil()
        //
        // Returns the ceiling of a duration using the passed duration unit to its
        // smallest value not less than the duration.
        //
        // Example:
        //
        //   turbo::Duration d = turbo::Duration::nanoseconds(123456789);
        //   turbo::Duration c = turbo::Duration::ceil(d, turbo::Duration::microseconds(1));   // 123457us
        TURBO_ATTRIBUTE_CONST_FUNCTION static Duration ceil(Duration d, Duration unit);
    public:
        // Factory functions for constructing `Duration` values from an integral number
        // of the unit indicated by the factory function's name. The number must be
        // representable as int64_t.
        //
        // NOTE: no "Days()" factory function exists because "a day" is ambiguous.
        // Civil days are not always 24 hours long, and a 24-hour duration often does
        // not correspond with a civil day. If a 24-hour duration is needed, use
        // `turbo::Duration::hours(24)`. If you actually want a civil day, use turbo::CivilDay
        // from civil_time.h.
        //
        // Example:
        //
        //   turbo::Duration a = turbo::Duration::seconds(60);
        //   turbo::Duration b = turbo::Duration::minutes(1);  // b == a
        template<typename T, time_internal::EnableIfIntegral<T> = 0>
        TURBO_ATTRIBUTE_CONST_FUNCTION static constexpr Duration nanoseconds(T n) {
            return time_internal::FromInt64(n, std::nano{});
        }

        template<typename T, time_internal::EnableIfIntegral<T> = 0>
        TURBO_ATTRIBUTE_CONST_FUNCTION static constexpr Duration microseconds(T n) {
            return time_internal::FromInt64(n, std::micro{});
        }

        template<typename T, time_internal::EnableIfIntegral<T> = 0>
        TURBO_ATTRIBUTE_CONST_FUNCTION static constexpr Duration milliseconds(T n) {
            return time_internal::FromInt64(n, std::milli{});
        }

        template<typename T, time_internal::EnableIfIntegral<T> = 0>
        TURBO_ATTRIBUTE_CONST_FUNCTION static constexpr Duration seconds(T n) {
            return time_internal::FromInt64(n, std::ratio<1>{});
        }

        template<typename T, time_internal::EnableIfIntegral<T> = 0>
        TURBO_ATTRIBUTE_CONST_FUNCTION static constexpr Duration minutes(T n) {
            return time_internal::FromInt64(n, std::ratio<60>{});
        }

        template<typename T, time_internal::EnableIfIntegral<T> = 0>
        TURBO_ATTRIBUTE_CONST_FUNCTION constexpr static Duration hours(T n) {
            return time_internal::FromInt64(n, std::ratio<3600>{});
        }
        // Factory overloads for constructing `Duration` values from a floating-point
        // number of the unit indicated by the factory function's name. These functions
        // exist for convenience, but they are not as efficient as the integral
        // factories, which should be preferred.
        //
        // Example:
        //
        //   auto a = turbo::Duration::seconds(1.5);        // OK
        //   auto b = turbo::Duration::milliseconds(1500);  // BETTER
        template<typename T, time_internal::EnableIfFloat<T> = 0>
        TURBO_ATTRIBUTE_CONST_FUNCTION static Duration nanoseconds(T n) {
            return n * nanoseconds(1);
        }
        template<typename T, time_internal::EnableIfFloat<T> = 0>
        TURBO_ATTRIBUTE_CONST_FUNCTION static Duration microseconds(T n) {
            return n * microseconds(1);
        }

        template<typename T, time_internal::EnableIfFloat<T> = 0>
        TURBO_ATTRIBUTE_CONST_FUNCTION static Duration milliseconds(T n) {
            return n * milliseconds(1);
        }

        template<typename T, time_internal::EnableIfFloat<T> = 0>
        TURBO_ATTRIBUTE_CONST_FUNCTION static Duration seconds(T n) {
            if (n >= 0) {  // Note: `NaN >= 0` is false.
                if (n >= static_cast<T>((std::numeric_limits<int64_t>::max)())) {
                    return Duration::max_infinite();
                }
                return time_internal::MakePosDoubleDuration(n);
            } else {
                if (std::isnan(n))
                    return std::signbit(n) ? Duration::min_infinite() : Duration::max_infinite();
                if (n <= (std::numeric_limits<int64_t>::min)()) return Duration::min_infinite();
                return -time_internal::MakePosDoubleDuration(-n);
            }
        }

        template<typename T, time_internal::EnableIfFloat<T> = 0>
        TURBO_ATTRIBUTE_CONST_FUNCTION static Duration minutes(T n) {
            return n * minutes(1);
        }

        template<typename T, time_internal::EnableIfFloat<T> = 0>
        TURBO_ATTRIBUTE_CONST_FUNCTION static Duration hours(T n) {
            return n * hours(1);
        }


    public:
        // Helper functions that convert a Duration to an integral count of the
        // indicated unit. These return the same results as the `Duration::idiv()`
        // function, though they usually do so more efficiently; see the
        // documentation of `Duration::idiv()` for details about overflow, etc.
        //
        // Example:
        //
        //   turbo::Duration d = turbo::Duration::milliseconds(1500);
        //   int64_t isec = turbo::Duration::to_seconds(d);  // isec == 1
        TURBO_ATTRIBUTE_CONST_FUNCTION static int64_t to_nanoseconds(Duration d);

        TURBO_ATTRIBUTE_CONST_FUNCTION static int64_t to_microseconds(Duration d);

        TURBO_ATTRIBUTE_CONST_FUNCTION static int64_t to_milliseconds(Duration d);

        TURBO_ATTRIBUTE_CONST_FUNCTION static int64_t to_seconds(Duration d);

        TURBO_ATTRIBUTE_CONST_FUNCTION static int64_t to_minutes(Duration d);

        TURBO_ATTRIBUTE_CONST_FUNCTION static int64_t to_hours(Duration d);
        // Helper functions that convert a Duration to a floating point count of the
        // indicated unit. These functions are shorthand for the `Duration::fdiv()`
        // function above; see its documentation for details about overflow, etc.
        //
        // Example:
        //
        //   turbo::Duration d = turbo::Duration::milliseconds(1500);
        //   double dsec = turbo::Duration::to_double_seconds(d);  // dsec == 1.5
        TURBO_ATTRIBUTE_CONST_FUNCTION static double to_double_nanoseconds(Duration d);

        TURBO_ATTRIBUTE_CONST_FUNCTION static double to_double_microseconds(Duration d);

        TURBO_ATTRIBUTE_CONST_FUNCTION static double to_double_milliseconds(Duration d);

        TURBO_ATTRIBUTE_CONST_FUNCTION static double to_double_seconds(Duration d);

        TURBO_ATTRIBUTE_CONST_FUNCTION static double to_double_minutes(Duration d);

        TURBO_ATTRIBUTE_CONST_FUNCTION static double to_double_hours(Duration d);
        //
        // Converts an turbo::Duration to any of the pre-defined std::chrono durations.
        // If overflow would occur, the returned value will saturate at the min/max
        // chrono duration value instead.
        //
        // Example:
        //
        //   turbo::Duration d = turbo::Duration::microseconds(123);
        //   auto x = turbo::Duration::to_chrono_microseconds(d);
        //   auto y = turbo::Duration::to_chrono_nanoseconds(d);  // x == y
        //   auto z = turbo::Duration::to_chrono_seconds(turbo::Duration::max_infinite());
        //   // z == std::chrono::seconds::max()
        TURBO_ATTRIBUTE_CONST_FUNCTION static std::chrono::nanoseconds to_chrono_nanoseconds(Duration d);

        TURBO_ATTRIBUTE_CONST_FUNCTION static std::chrono::microseconds to_chrono_microseconds(Duration d);

        TURBO_ATTRIBUTE_CONST_FUNCTION static std::chrono::milliseconds to_chrono_milliseconds(Duration d);

        TURBO_ATTRIBUTE_CONST_FUNCTION static std::chrono::seconds to_chrono_seconds(Duration d);

        TURBO_ATTRIBUTE_CONST_FUNCTION static std::chrono::minutes to_chrono_minutes(Duration d);

        TURBO_ATTRIBUTE_CONST_FUNCTION static std::chrono::hours to_chrono_hours(Duration d);

        TURBO_ATTRIBUTE_CONST_FUNCTION static timespec to_timespec(Duration d);

        TURBO_ATTRIBUTE_CONST_FUNCTION static timeval to_timeval(Duration d);
    public:
        // Duration::from_chrono()
        //
        // Converts any of the pre-defined std::chrono durations to an turbo::Duration.
        //
        // Example:
        //
        //   std::chrono::milliseconds ms(123);
        //   turbo::Duration d = turbo::Duration::from_chrono(ms);
        TURBO_ATTRIBUTE_PURE_FUNCTION static constexpr Duration from_chrono(
                const std::chrono::nanoseconds &d);

        TURBO_ATTRIBUTE_PURE_FUNCTION static constexpr Duration from_chrono(
                const std::chrono::microseconds &d);

        TURBO_ATTRIBUTE_PURE_FUNCTION static constexpr Duration from_chrono(
                const std::chrono::milliseconds &d);

        TURBO_ATTRIBUTE_PURE_FUNCTION static constexpr Duration from_chrono(
                const std::chrono::seconds &d);

        TURBO_ATTRIBUTE_PURE_FUNCTION static constexpr Duration from_chrono(
                const std::chrono::minutes &d);

        TURBO_ATTRIBUTE_PURE_FUNCTION static constexpr Duration from_chrono(
                const std::chrono::hours &d);

        TURBO_ATTRIBUTE_CONST_FUNCTION static Duration from_timespec(timespec ts);

        TURBO_ATTRIBUTE_CONST_FUNCTION static Duration from_timeval(timeval tv);

    public:
        // Duration::format()
        //
        // Returns a string representing the duration in the form "72h3m0.5s".
        // Returns "inf" or "-inf" for +/- `Duration::max_infinite()`.
        TURBO_ATTRIBUTE_CONST_FUNCTION static std::string format(Duration d);
        // Duration::parse()
        //
        // Parses a duration string consisting of a possibly signed sequence of
        // decimal numbers, each with an optional fractional part and a unit
        // suffix.  The valid suffixes are "ns", "us" "ms", "s", "m", and "h".
        // Simple examples include "300ms", "-1.5h", and "2h45m".  Parses "0" as
        // `Duration::zero()`. Parses "inf" and "-inf" as +/- `Duration::max_infinite()`.
        static bool parse(std::string_view dur_string, Duration *d);

    private:
        friend constexpr int64_t time_internal::GetRepHi(Duration d);

        friend constexpr uint32_t time_internal::GetRepLo(Duration d);

        friend constexpr Duration time_internal::MakeDuration(int64_t hi,
                                                              uint32_t lo);

        constexpr Duration(int64_t hi, uint32_t lo) : rep_hi_(hi), rep_lo_(lo) {}

        // We store `rep_hi_` 4-byte rather than 8-byte aligned to avoid 4 bytes of
        // tail padding.
        class HiRep {
        public:
            // Default constructor default-initializes `hi_`, which has the same
            // semantics as default-initializing an `int64_t` (undetermined value).
            HiRep() = default;

            HiRep(const HiRep &) = default;

            HiRep &operator=(const HiRep &) = default;

            explicit constexpr HiRep(const int64_t value)
                    :  // C++17 forbids default-initialization in constexpr contexts. We can
            // remove this in C++20.
#if defined(TURBO_IS_BIG_ENDIAN) && TURBO_IS_BIG_ENDIAN
                    hi_(0),
                    lo_(0)
#else
                    lo_(0),
                    hi_(0)
#endif
            {
                *this = value;
            }

            constexpr int64_t Get() const {
                const uint64_t unsigned_value =
                        (static_cast<uint64_t>(hi_) << 32) | static_cast<uint64_t>(lo_);
                // `static_cast<int64_t>(unsigned_value)` is implementation-defined
                // before c++20. On all supported platforms the behaviour is that mandated
                // by c++20, i.e. "If the destination type is signed, [...] the result is
                // the unique value of the destination type equal to the source value
                // modulo 2^n, where n is the number of bits used to represent the
                // destination type."
                static_assert(
                        (static_cast<int64_t>((std::numeric_limits<uint64_t>::max)()) ==
                         int64_t{-1}) &&
                        (static_cast<int64_t>(static_cast<uint64_t>(
                                                      (std::numeric_limits<int64_t>::max)()) +
                                              1) ==
                         (std::numeric_limits<int64_t>::min)()),
                        "static_cast<int64_t>(uint64_t) does not have c++20 semantics");
                return static_cast<int64_t>(unsigned_value);
            }

            constexpr HiRep &operator=(const int64_t value) {
                // "If the destination type is unsigned, the resulting value is the
                // smallest unsigned value equal to the source value modulo 2^n
                // where `n` is the number of bits used to represent the destination
                // type".
                const auto unsigned_value = static_cast<uint64_t>(value);
                hi_ = static_cast<uint32_t>(unsigned_value >> 32);
                lo_ = static_cast<uint32_t>(unsigned_value);
                return *this;
            }

        private:
            // Notes:
            //  - Ideally we would use a `char[]` and `std::bitcast`, but the latter
            //    does not exist (and is not constexpr in `turbo`) before c++20.
            //  - Order is optimized depending on endianness so that the compiler can
            //    turn `Get()` (resp. `operator=()`) into a single 8-byte load (resp.
            //    store).
#if defined(TURBO_IS_BIG_ENDIAN) && TURBO_IS_BIG_ENDIAN
            uint32_t hi_;
            uint32_t lo_;
#else
            uint32_t lo_;
            uint32_t hi_;
#endif
        };

        HiRep rep_hi_;
        uint32_t rep_lo_;
    };

    // Relational Operators
    TURBO_ATTRIBUTE_CONST_FUNCTION constexpr bool operator<(Duration lhs,
                                                            Duration rhs);

    TURBO_ATTRIBUTE_CONST_FUNCTION constexpr bool operator>(Duration lhs,
                                                            Duration rhs) {
        return rhs < lhs;
    }

    TURBO_ATTRIBUTE_CONST_FUNCTION constexpr bool operator>=(Duration lhs,
                                                             Duration rhs) {
        return !(lhs < rhs);
    }

    TURBO_ATTRIBUTE_CONST_FUNCTION constexpr bool operator<=(Duration lhs,
                                                             Duration rhs) {
        return !(rhs < lhs);
    }

    TURBO_ATTRIBUTE_CONST_FUNCTION constexpr bool operator==(Duration lhs,
                                                             Duration rhs);

    TURBO_ATTRIBUTE_CONST_FUNCTION constexpr bool operator!=(Duration lhs,
                                                             Duration rhs) {
        return !(lhs == rhs);
    }

    // Additive Operators
    TURBO_ATTRIBUTE_CONST_FUNCTION constexpr Duration operator-(Duration d);

    TURBO_ATTRIBUTE_CONST_FUNCTION inline Duration operator+(Duration lhs,
                                                             Duration rhs) {
        return lhs += rhs;
    }

    TURBO_ATTRIBUTE_CONST_FUNCTION inline Duration operator-(Duration lhs,
                                                             Duration rhs) {
        return lhs -= rhs;
    }

    // Multiplicative Operators
    // Integer operands must be representable as int64_t.
    template<typename T>
    TURBO_ATTRIBUTE_CONST_FUNCTION Duration operator*(Duration lhs, T rhs) {
        return lhs *= rhs;
    }

    template<typename T>
    TURBO_ATTRIBUTE_CONST_FUNCTION Duration operator*(T lhs, Duration rhs) {
        return rhs *= lhs;
    }

    template<typename T>
    TURBO_ATTRIBUTE_CONST_FUNCTION Duration operator/(Duration lhs, T rhs) {
        return lhs /= rhs;
    }

    TURBO_ATTRIBUTE_CONST_FUNCTION inline int64_t operator/(Duration lhs,
                                                            Duration rhs) {
        return Duration::idiv(lhs, rhs,
                              &lhs);  // trunc towards zero
    }

    TURBO_ATTRIBUTE_CONST_FUNCTION inline Duration operator%(Duration lhs,
                                                             Duration rhs) {
        return lhs %= rhs;
    }

    // Output stream operator.
    inline std::ostream &operator<<(std::ostream &os, Duration d) {
        return os << Duration::format(d);
    }

    // Support for str_format(), str_cat() etc.
    template<typename Sink>
    void turbo_stringify(Sink &sink, Duration d) {
        sink.Append(Duration::format(d));
    }

    // turbo_parse_flag()
    //
    // Parses a command-line flag string representation `text` into a Duration
    // value. Duration flags must be specified in a format that is valid input for
    // `turbo::Duration::parse()`.
    bool turbo_parse_flag(std::string_view text, Duration *dst, std::string *error);


    // turbo_unparse_flag()
    //
    // Unparses a Duration value into a command-line string representation using
    // the format specified by `turbo::Duration::parse()`.
    std::string turbo_unparse_flag(Duration d);

    // Time
    //
    // An `turbo::Time` represents a specific instant in time. Arithmetic operators
    // are provided for naturally expressing time calculations. Instances are
    // created using `turbo::Time::current_time()` and the `turbo::From*()` factory functions that
    // accept the gamut of other time representations. Formatting and parsing
    // functions are provided for conversion to and from strings.  `turbo::Time`
    // should be passed by value rather than const reference.
    //
    // `turbo::Time` assumes there are 60 seconds in a minute, which means the
    // underlying time scales must be "smeared" to eliminate leap seconds.
    // See https://developers.google.com/time/smear.
    //
    // Even though `turbo::Time` supports a wide range of timestamps, exercise
    // caution when using values in the distant past. `turbo::Time` uses the
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
    // The `turbo::Time` class represents an instant in time as a count of clock
    // ticks of some granularity (resolution) from some starting point (epoch).
    //
    // `turbo::Time` uses a resolution that is high enough to avoid loss in
    // precision, and a range that is wide enough to avoid overflow, when
    // converting between tick counts in most Google time scales (i.e., resolution
    // of at least one nanosecond, and range +/-100 billion years).  Conversions
    // between the time scales are performed by truncating (towards negative
    // infinity) to the nearest representable point.
    //
    // Examples:
    //
    //   turbo::Time t1 = ...;
    //   turbo::Time t2 = t1 + turbo::Duration::minutes(2);
    //   turbo::Duration d = t2 - t1;  // == turbo::Duration::minutes(2)
    //
    class Time {
    public:
        // Value semantics.

        // Returns the Unix epoch.  However, those reading your code may not know
        // or expect the Unix epoch as the default value, so make your code more
        // readable by explicitly initializing all instances before use.
        //
        // Example:
        //   turbo::Time t = turbo::Time::from_unix_epoch();
        //   turbo::Time t = turbo::Time::current_time();
        //   turbo::Time t = turbo::Time::from_timeval(tv);
        //   turbo::Time t = turbo::Time::past_infinite();
        constexpr Time() = default;

        // Copyable.
        constexpr Time(const Time &t) = default;

        Time &operator=(const Time &t) = default;

        // Assignment operators.
        Time &operator+=(Duration d) {
            rep_ += d;
            return *this;
        }

        Time &operator-=(Duration d) {
            rep_ -= d;
            return *this;
        }

    public:
        static Time current_time();

        static int64_t current_nanoseconds();

        template<typename T = int64_t>
        static T current_microseconds();

        template<typename T = int64_t>
        static T current_milliseconds();

        template<typename T = int64_t>
        static T current_seconds();

    public:
        TURBO_ATTRIBUTE_CONST_FUNCTION static constexpr Time from_nanoseconds(int64_t ns);

        TURBO_ATTRIBUTE_CONST_FUNCTION static constexpr Time from_microseconds(int64_t us);

        TURBO_ATTRIBUTE_CONST_FUNCTION static constexpr Time from_milliseconds(int64_t ms);

        TURBO_ATTRIBUTE_CONST_FUNCTION static constexpr Time from_seconds(int64_t s);

        TURBO_ATTRIBUTE_CONST_FUNCTION static constexpr Time from_time_t(time_t t);

        // Time::from_udate()
        // Time::from_universal()
        //
        // Creates an `turbo::Time` from a variety of other representations.  See
        // https://unicode-org.github.io/icu/userguide/datetime/universaltimescale.html


        TURBO_ATTRIBUTE_CONST_FUNCTION static Time from_udate(double udate);

        TURBO_ATTRIBUTE_CONST_FUNCTION static Time from_universal(int64_t universal);

        TURBO_ATTRIBUTE_CONST_FUNCTION static Time from_timespec(timespec ts);

        TURBO_ATTRIBUTE_CONST_FUNCTION static Time from_timeval(timeval tv);
        // Time::from_chrono()
        //
        // Converts a std::chrono::system_clock::time_point to an turbo::Time.
        //
        // Example:
        //
        //   auto tp = std::chrono::system_clock::from_time_t(123);
        //   turbo::Time t = turbo::Time::from_chrono(tp);
        //   // t == turbo::Time::from_time_t(123)
        TURBO_ATTRIBUTE_PURE_FUNCTION static Time from_chrono(const std::chrono::system_clock::time_point &tp);

        // Time::from_tm()
        //
        // Converts the `tm_year`, `tm_mon`, `tm_mday`, `tm_hour`, `tm_min`, and
        // `tm_sec` fields to an `turbo::Time` using the given time zone. See ctime(3)
        // for a description of the expected values of the tm fields. If the civil time
        // is unique (see `turbo::TimeZone::at(turbo::CivilSecond)` above), the matching
        // time instant is returned.  Otherwise, the `tm_isdst` field is consulted to
        // choose between the possible results.  For a repeated civil time, `tm_isdst !=
        // 0` returns the matching DST instant, while `tm_isdst == 0` returns the
        // matching non-DST instant.  For a skipped civil time there is no matching
        // instant, so `tm_isdst != 0` returns the DST instant, and `tm_isdst == 0`
        // returns the non-DST instant, that would have matched if the transition never
        // happened.
        TURBO_ATTRIBUTE_PURE_FUNCTION static Time from_tm(const struct tm &tm, TimeZone tz);

        // Time::from_civil()
        //
        // Helper for TimeZone::at(CivilSecond) that provides "order-preserving
        // semantics." If the civil time maps to a unique time, that time is
        // returned. If the civil time is repeated in the given time zone, the
        // time using the pre-transition offset is returned. Otherwise, the
        // civil time is skipped in the given time zone, and the transition time
        // is returned. This means that for any two civil times, ct1 and ct2,
        // (ct1 < ct2) => (Time::from_civil(ct1) <= Time::from_civil(ct2)), the equal case
        // being when two non-existent civil times map to the same transition time.
        //
        // Note: Accepts civil times of any alignment.
        TURBO_ATTRIBUTE_PURE_FUNCTION static Time from_civil(CivilSecond ct,
                                                             TimeZone tz);

        // Time::from_unix_epoch()
        //
        // Returns the `turbo::Time` representing "1970-01-01 00:00:00.0 +0000".
        TURBO_ATTRIBUTE_CONST_FUNCTION static constexpr Time from_unix_epoch();

        // Time::from_universal_epoch()
        //
        // Returns the `turbo::Time` representing "0001-01-01 00:00:00.0 +0000", the
        // epoch of the ICU Universal Time Scale.
        TURBO_ATTRIBUTE_CONST_FUNCTION static constexpr Time from_universal_epoch();

    public:
        // future_infinite()
        //
        // Returns an `turbo::Time` that is infinitely far in the future.
        TURBO_ATTRIBUTE_CONST_FUNCTION static constexpr Time future_infinite();

        TURBO_ATTRIBUTE_CONST_FUNCTION static Time future_time(Duration d);

        TURBO_ATTRIBUTE_CONST_FUNCTION static int64_t future_nanoseconds(int64_t ns);

        TURBO_ATTRIBUTE_CONST_FUNCTION static int64_t future_microseconds(int64_t us);

        TURBO_ATTRIBUTE_CONST_FUNCTION static int64_t future_milliseconds(int64_t ms);

        TURBO_ATTRIBUTE_CONST_FUNCTION static int64_t future_seconds(int64_t s);

    public:
        // past_infinite()
        //
        // Returns an `turbo::Time` that is infinitely far in the past.
        TURBO_ATTRIBUTE_CONST_FUNCTION static constexpr Time past_infinite();

        TURBO_ATTRIBUTE_CONST_FUNCTION static Time past_time(Duration d);

        TURBO_ATTRIBUTE_CONST_FUNCTION static int64_t past_nanoseconds(int64_t ns);

        TURBO_ATTRIBUTE_CONST_FUNCTION static int64_t past_microseconds(int64_t us);

        TURBO_ATTRIBUTE_CONST_FUNCTION static int64_t past_milliseconds(int64_t ms);

        TURBO_ATTRIBUTE_CONST_FUNCTION static int64_t past_seconds(int64_t s);

    public:
        // Converts an `turbo::Time` to a variety of other representations.  See
        // https://unicode-org.github.io/icu/userguide/datetime/universaltimescale.html
        //
        // Note that these operations round down toward negative infinity where
        // necessary to adjust to the resolution of the result type.  Beware of
        // possible time_t over/underflow in ToTime{T,val,spec}() on 32-bit platforms.
        TURBO_ATTRIBUTE_CONST_FUNCTION static int64_t to_nanoseconds(Time t);

        TURBO_ATTRIBUTE_CONST_FUNCTION static int64_t to_microseconds(Time t);

        TURBO_ATTRIBUTE_CONST_FUNCTION static int64_t to_milliseconds(Time t);

        TURBO_ATTRIBUTE_CONST_FUNCTION static int64_t to_seconds(Time t);

        TURBO_ATTRIBUTE_CONST_FUNCTION static time_t to_time_t(Time t);

        TURBO_ATTRIBUTE_CONST_FUNCTION static double to_udate(Time t);

        TURBO_ATTRIBUTE_CONST_FUNCTION static int64_t to_universal(Time t);
        // Time::to_chrono()
        //
        // Converts an turbo::Time to a std::chrono::system_clock::time_point. If
        // overflow would occur, the returned value will saturate at the min/max time
        // point value instead.
        //
        // Example:
        //
        //   turbo::Time t = turbo::Time::from_time_t(123);
        //   auto tp = turbo::Time::to_chrono(t);
        //   // tp == std::chrono::system_clock::from_time_t(123);
        TURBO_ATTRIBUTE_CONST_FUNCTION static std::chrono::system_clock::time_point to_chrono(Time);

        TURBO_ATTRIBUTE_CONST_FUNCTION static timespec to_timespec(Time t);

        TURBO_ATTRIBUTE_CONST_FUNCTION static timeval to_timeval(Time t);

        // Helpers for TimeZone::at(Time) to return particularly aligned civil times.
        //
        // Example:
        //
        //   turbo::Time t = ...;
        //   turbo::TimeZone tz = ...;
        //   const auto cd = turbo::Time::to_civil_day(t, tz);
        TURBO_ATTRIBUTE_PURE_FUNCTION static CivilSecond to_civil_second(Time t, TimeZone tz);

        TURBO_ATTRIBUTE_PURE_FUNCTION static CivilMinute to_civil_minute(Time t, TimeZone tz);

        TURBO_ATTRIBUTE_PURE_FUNCTION static CivilHour to_civil_hour(Time t, TimeZone tz);

        TURBO_ATTRIBUTE_PURE_FUNCTION static CivilDay to_civil_day(Time t, TimeZone tz);

        TURBO_ATTRIBUTE_PURE_FUNCTION static CivilMonth to_civil_month(Time t, TimeZone tz);

        TURBO_ATTRIBUTE_PURE_FUNCTION static CivilYear to_civil_year(Time t, TimeZone tz);

        // Time::to_tm()
        //
        // Converts the given `turbo::Time` to a struct tm using the given time zone.
        // See ctime(3) for a description of the values of the tm fields.
        TURBO_ATTRIBUTE_PURE_FUNCTION static struct tm to_tm(Time t, TimeZone tz);
        // Time::to_utc_*
        TURBO_ATTRIBUTE_PURE_FUNCTION static CivilSecond to_utc_civil_second(Time t);

        TURBO_ATTRIBUTE_PURE_FUNCTION static CivilMinute to_utc_civil_minute(Time t);

        TURBO_ATTRIBUTE_PURE_FUNCTION static CivilHour to_utc_civil_hour(Time t);

        TURBO_ATTRIBUTE_PURE_FUNCTION static CivilDay to_utc_civil_day(Time t);

        TURBO_ATTRIBUTE_PURE_FUNCTION static CivilMonth to_utc_civil_month(Time t);

        TURBO_ATTRIBUTE_PURE_FUNCTION static CivilYear to_utc_civil_year(Time t);

        TURBO_ATTRIBUTE_PURE_FUNCTION static struct tm to_utc_tm(Time t);
        // Time::to_local_*
        TURBO_ATTRIBUTE_PURE_FUNCTION static CivilSecond to_local_civil_second(Time t);

        TURBO_ATTRIBUTE_PURE_FUNCTION static CivilMinute to_local_civil_minute(Time t);

        TURBO_ATTRIBUTE_PURE_FUNCTION static CivilHour to_local_civil_hour(Time t);

        TURBO_ATTRIBUTE_PURE_FUNCTION static CivilDay to_local_civil_day(Time t);

        TURBO_ATTRIBUTE_PURE_FUNCTION static CivilMonth to_local_civil_month(Time t);

        TURBO_ATTRIBUTE_PURE_FUNCTION static CivilYear to_local_civil_year(Time t);

        TURBO_ATTRIBUTE_PURE_FUNCTION static struct tm to_local_tm(Time t);

    public:
        // Time::parse()
        //
        // Parses an input string according to the provided format string and
        // returns the corresponding `turbo::Time`. Uses strftime()-like formatting
        // options, with the same extensions as Time::format(), but with the
        // exceptions that %E#S is interpreted as %E*S, and %E#f as %E*f.  %Ez
        // and %E*z also accept the same inputs, which (along with %z) includes
        // 'z' and 'Z' as synonyms for +00:00.  %ET accepts either 'T' or 't'.
        //
        // %Y consumes as many numeric characters as it can, so the matching data
        // should always be terminated with a non-numeric.  %E4Y always consumes
        // exactly four characters, including any sign.
        //
        // Unspecified fields are taken from the default date and time of ...
        //
        //   "1970-01-01 00:00:00.0 +0000"
        //
        // For example, parsing a string of "15:45" (%H:%M) will return an turbo::Time
        // that represents "1970-01-01 15:45:00.0 +0000".
        //
        // Note that since Time::parse() returns time instants, it makes the most sense
        // to parse fully-specified date/time strings that include a UTC offset (%z,
        // %Ez, or %E*z).
        //
        // Note also that `turbo::Time::parse()` only heeds the fields year, month, day,
        // hour, minute, (fractional) second, and UTC offset.  Other fields, like
        // weekday (%a or %A), while parsed for syntactic validity, are ignored
        // in the conversion.
        //
        // Date and time fields that are out-of-range will be treated as errors
        // rather than normalizing them like `turbo::CivilSecond` does.  For example,
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
        // `turbo::Time` will be `turbo::Time::future_infinite()` and `true` will be returned.
        // If the input string is "infinite-past", the returned `turbo::Time` will be
        // `turbo::Time::past_infinite()` and `true` will be returned.
        //
        static bool parse(std::string_view format, std::string_view input, Time *time,
                          std::string *err);

        // Like Time::parse() above, but if the format string does not contain a UTC
        // offset specification (%z/%Ez/%E*z) then the input is interpreted in the
        // given TimeZone.  This means that the input, by itself, does not identify a
        // unique instant.  Being time-zone dependent, it also admits the possibility
        // of ambiguity or non-existence, in which case the "pre" time (as defined
        // by TimeZone::TimeInfo) is returned.  For these reasons we recommend that
        // all date/time strings include a UTC offset so they're context independent.
        static bool parse(std::string_view format, std::string_view input, TimeZone tz,
                          Time *time, std::string *err);

        // Time::format()
        //
        // Formats the given `turbo::Time` in the `turbo::TimeZone` according to the
        // provided format string. Uses strftime()-like formatting options, with
        // the following extensions:
        //
        //   - %Ez  - RFC3339-compatible numeric UTC offset (+hh:mm or -hh:mm)
        //   - %E*z - Full-resolution numeric UTC offset (+hh:mm:ss or -hh:mm:ss)
        //   - %E#S - Seconds with # digits of fractional precision
        //   - %E*S - Seconds with full fractional precision (a literal '*')
        //   - %E#f - Fractional seconds with # digits of precision
        //   - %E*f - Fractional seconds with full precision (a literal '*')
        //   - %E4Y - Four-character years (-999 ... -001, 0000, 0001 ... 9999)
        //   - %ET  - The RFC3339 "date-time" separator "T"
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
        //   turbo::CivilSecond cs(2013, 1, 2, 3, 4, 5);
        //   turbo::Time t = turbo::Time::from_civil(cs, lax);
        //   std::string f = turbo::Time::format("%H:%M:%S", t, lax);  // "03:04:05"
        //   f = turbo::Time::format("%H:%M:%E3S", t, lax);  // "03:04:05.000"
        //
        // Note: If the given `turbo::Time` is `turbo::Time::future_infinite()`, the returned
        // string will be exactly "infinite-future". If the given `turbo::Time` is
        // `turbo::Time::past_infinite()`, the returned string will be exactly "infinite-past".
        // In both cases the given format string and `turbo::TimeZone` are ignored.
        //
        TURBO_ATTRIBUTE_PURE_FUNCTION static std::string format(std::string_view format,
                                                                Time t, TimeZone tz);

        // Convenience functions that format the given time using the RFC3339_full
        // format.  The first overload uses the provided TimeZone, while the second
        // uses TimeZone::local().
        TURBO_ATTRIBUTE_PURE_FUNCTION static std::string format(Time t, TimeZone tz);

        TURBO_ATTRIBUTE_PURE_FUNCTION static std::string format(Time t);

    public:
        template<typename H>
        friend H turbo_hash_value(H h, Time t) {
            return H::combine(std::move(h), t.rep_);
        }

    private:
        friend constexpr Time time_internal::FromUnixDuration(Duration d);

        friend constexpr Duration time_internal::ToUnixDuration(Time t);

        friend constexpr bool operator<(Time lhs, Time rhs);

        friend constexpr bool operator==(Time lhs, Time rhs);

        friend Duration operator-(Time lhs, Time rhs);

        constexpr explicit Time(Duration rep) : rep_(rep) {}

        Duration rep_;
    };

    // Relational Operators
    TURBO_ATTRIBUTE_CONST_FUNCTION constexpr bool operator<(Time lhs, Time rhs) {
        return lhs.rep_ < rhs.rep_;
    }

    TURBO_ATTRIBUTE_CONST_FUNCTION constexpr bool operator>(Time lhs, Time rhs) {
        return rhs < lhs;
    }

    TURBO_ATTRIBUTE_CONST_FUNCTION constexpr bool operator>=(Time lhs, Time rhs) {
        return !(lhs < rhs);
    }

    TURBO_ATTRIBUTE_CONST_FUNCTION constexpr bool operator<=(Time lhs, Time rhs) {
        return !(rhs < lhs);
    }

    TURBO_ATTRIBUTE_CONST_FUNCTION constexpr bool operator==(Time lhs, Time rhs) {
        return lhs.rep_ == rhs.rep_;
    }

    TURBO_ATTRIBUTE_CONST_FUNCTION constexpr bool operator!=(Time lhs, Time rhs) {
        return !(lhs == rhs);
    }

    // Additive Operators
    TURBO_ATTRIBUTE_CONST_FUNCTION inline Time operator+(Time lhs, Duration rhs) {
        return lhs += rhs;
    }

    TURBO_ATTRIBUTE_CONST_FUNCTION inline Time operator+(Duration lhs, Time rhs) {
        return rhs += lhs;
    }

    TURBO_ATTRIBUTE_CONST_FUNCTION inline Time operator-(Time lhs, Duration rhs) {
        return lhs -= rhs;
    }

    TURBO_ATTRIBUTE_CONST_FUNCTION inline Duration operator-(Time lhs, Time rhs) {
        return lhs.rep_ - rhs.rep_;
    }

    // turbo_parse_flag()
    //
    // Parses the command-line flag string representation `text` into a Time value.
    // Time flags must be specified in a format that matches turbo::RFC3339_full.
    //
    // For example:
    //
    //   --start_time=2016-01-02T03:04:05.678+08:00
    //
    // Note: A UTC offset (or 'Z' indicating a zero-offset from UTC) is required.
    //
    // Additionally, if you'd like to specify a time as a count of
    // seconds/milliseconds/etc from the Unix epoch, use an turbo::Duration flag
    // and add that duration to turbo::Time::from_unix_epoch() to get an turbo::Time.
    bool turbo_parse_flag(std::string_view text, Time *t, std::string *error);

    // turbo_unparse_flag()
    //
    // Unparses a Time value into a command-line string representation using
    // the format specified by `turbo::Time::parse()`.
    std::string turbo_unparse_flag(Time t);

    // sleep_for()
    //
    // Sleeps for the specified duration, expressed as an `turbo::Duration`.
    //
    // Notes:
    // * Signal interruptions will not reduce the sleep duration.
    // * Returns immediately when passed a nonpositive duration.
    void sleep_for(turbo::Duration duration);

    inline void sleep_until(turbo::Time time) {
        sleep_for(time - turbo::Time::current_time());
    }

    // TimeZone
    //
    // The `turbo::TimeZone` is an opaque, small, value-type class representing a
    // geo-political region within which particular rules are used for converting
    // between absolute and civil times (see https://git.io/v59Ly). `turbo::TimeZone`
    // values are named using the TZ identifiers from the IANA Time Zone Database,
    // such as "America/Los_Angeles" or "Australia/Sydney". `turbo::TimeZone` values
    // are created from factory functions such as `turbo::TimeZone::load()`. Note:
    // strings like "PST" and "EDT" are not valid TZ identifiers. Prefer to pass by
    // value rather than const reference.
    //
    // For more on the fundamental concepts of time zones, absolute times, and civil
    // times, see https://github.com/google/cctz#fundamental-concepts
    //
    // Examples:
    //
    //   turbo::TimeZone utc = turbo::TimeZone::utc();
    //   turbo::TimeZone pst = turbo::TimeZone::fixed(-8 * 60 * 60);
    //   turbo::TimeZone loc = turbo::TimeZone::local();
    //   turbo::TimeZone lax;
    //   if (!turbo::TimeZone::load("America/Los_Angeles", &lax)) {
    //     // handle error case
    //   }
    //
    // See also:
    // - https://github.com/google/cctz
    // - https://www.iana.org/time-zones
    // - https://en.wikipedia.org/wiki/Zoneinfo
    class TimeZone {
    public:
        explicit TimeZone(time_internal::cctz::time_zone tz) : cz_(tz) {}

        TimeZone() = default;  // UTC, but prefer TimeZone::utc() to be explicit.

        // Copyable.
        TimeZone(const TimeZone &) = default;

        TimeZone &operator=(const TimeZone &) = default;

        explicit operator time_internal::cctz::time_zone() const { return cz_; }

        std::string name() const { return cz_.name(); }

    public:
        // TimeZone::load()`
        //
        // Loads the named zone. May perform I/O on the initial load of the named
        // zone. If the name is invalid, or some other kind of error occurs, returns
        // `false` and `*tz` is set to the UTC time zone.
        static bool load(std::string_view name, TimeZone *tz);

        // TimeZone::fixed()
        //
        // Returns a TimeZone that is a fixed offset (seconds east) from UTC.
        // Note: If the absolute value of the offset is greater than 24 hours
        // you'll get UTC (i.e., no offset) instead.
        static TimeZone fixed(int seconds);

        // TimeZone::utc()
        //
        // Convenience method returning the UTC time zone.
        static TimeZone utc();

        // TimeZone::local()
        //
        // Convenience method returning the local time zone, or UTC if there is
        // no configured local zone.  Warning: Be wary of using TimeZone::local(),
        // and particularly so in a server process, as the zone configured for the
        // local machine should be irrelevant.  Prefer an explicit zone name.
        static TimeZone local();

        // TimeZone::CivilInfo
        //
        // Information about the civil time corresponding to an absolute time.
        // This struct is not intended to represent an instant in time. So, rather
        // than passing a `TimeZone::CivilInfo` to a function, pass an `turbo::Time`
        // and an `turbo::TimeZone`.
        struct CivilInfo {
            CivilSecond cs;
            Duration subsecond;

            // Note: The following fields exist for backward compatibility
            // with older APIs.  Accessing these fields directly is a sign of
            // imprudent logic in the calling code.  Modern time-related code
            // should only access this data indirectly by way of Time::format().
            // These fields are undefined for InfiniteFuture() and Time::past_infinite().
            int offset;             // seconds east of UTC
            bool is_dst;            // is offset non-standard?
            const char *zone_abbr;  // time-zone abbreviation (e.g., "PST")
        };

        // TimeZone::at(Time)
        //
        // Returns the civil time for this TimeZone at a certain `turbo::Time`.
        // If the input time is infinite, the output civil second will be set to
        // CivilSecond::max() or min(), and the subsecond will be infinite.
        //
        // Example:
        //
        //   const auto epoch = lax.at(turbo::Time::from_unix_epoch());
        //   // epoch.cs == 1969-12-31 16:00:00
        //   // epoch.subsecond == turbo::Duration::zero()
        //   // epoch.offset == -28800
        //   // epoch.is_dst == false
        //   // epoch.abbr == "PST"
        CivilInfo at(Time t) const;

        // TimeZone::TimeInfo
        //
        // Information about the absolute times corresponding to a civil time.
        // (Subseconds must be handled separately.)
        //
        // It is possible for a caller to pass a civil-time value that does
        // not represent an actual or unique instant in time (due to a shift
        // in UTC offset in the TimeZone, which results in a discontinuity in
        // the civil-time components). For example, a daylight-saving-time
        // transition skips or repeats civil times---in the United States,
        // March 13, 2011 02:15 never occurred, while November 6, 2011 01:15
        // occurred twice---so requests for such times are not well-defined.
        // To account for these possibilities, `turbo::TimeZone::TimeInfo` is
        // richer than just a single `turbo::Time`.
        struct TimeInfo {
            enum CivilKind {
                UNIQUE,    // the civil time was singular (pre == trans == post)
                SKIPPED,   // the civil time did not exist (pre >= trans > post)
                REPEATED,  // the civil time was ambiguous (pre < trans <= post)
            } kind;
            Time pre;    // time calculated using the pre-transition offset
            Time trans;  // when the civil-time discontinuity occurred
            Time post;   // time calculated using the post-transition offset
        };

        // TimeZone::at(CivilSecond)
        //
        // Returns an `turbo::TimeInfo` containing the absolute time(s) for this
        // TimeZone at an `turbo::CivilSecond`. When the civil time is skipped or
        // repeated, returns times calculated using the pre-transition and post-
        // transition UTC offsets, plus the transition time itself.
        //
        // Examples:
        //
        //   // A unique civil time
        //   const auto jan01 = lax.at(turbo::CivilSecond(2011, 1, 1, 0, 0, 0));
        //   // jan01.kind == TimeZone::TimeInfo::UNIQUE
        //   // jan01.pre    is 2011-01-01 00:00:00 -0800
        //   // jan01.trans  is 2011-01-01 00:00:00 -0800
        //   // jan01.post   is 2011-01-01 00:00:00 -0800
        //
        //   // A Spring DST transition, when there is a gap in civil time
        //   const auto mar13 = lax.at(turbo::CivilSecond(2011, 3, 13, 2, 15, 0));
        //   // mar13.kind == TimeZone::TimeInfo::SKIPPED
        //   // mar13.pre   is 2011-03-13 03:15:00 -0700
        //   // mar13.trans is 2011-03-13 03:00:00 -0700
        //   // mar13.post  is 2011-03-13 01:15:00 -0800
        //
        //   // A Fall DST transition, when civil times are repeated
        //   const auto nov06 = lax.at(turbo::CivilSecond(2011, 11, 6, 1, 15, 0));
        //   // nov06.kind == TimeZone::TimeInfo::REPEATED
        //   // nov06.pre   is 2011-11-06 01:15:00 -0700
        //   // nov06.trans is 2011-11-06 01:00:00 -0800
        //   // nov06.post  is 2011-11-06 01:15:00 -0800
        TimeInfo at(CivilSecond ct) const;

        // TimeZone::NextTransition()
        // TimeZone::PrevTransition()
        //
        // Finds the time of the next/previous offset change in this time zone.
        //
        // By definition, `NextTransition(t, &trans)` returns false when `t` is
        // `InfiniteFuture()`, and `PrevTransition(t, &trans)` returns false
        // when `t` is `Time::past_infinite()`. If the zone has no transitions, the
        // result will also be false no matter what the argument.
        //
        // Otherwise, when `t` is `Time::past_infinite()`, `NextTransition(t, &trans)`
        // returns true and sets `trans` to the first recorded transition. Chains
        // of calls to `NextTransition()/PrevTransition()` will eventually return
        // false, but it is unspecified exactly when `NextTransition(t, &trans)`
        // jumps to false, or what time is set by `PrevTransition(t, &trans)` for
        // a very distant `t`.
        //
        // Note: Enumeration of time-zone transitions is for informational purposes
        // only. Modern time-related code should not care about when offset changes
        // occur.
        //
        // Example:
        //   turbo::TimeZone nyc;
        //   if (!turbo::TimeZone::load("America/New_York", &nyc)) { ... }
        //   const auto now = turbo::Time::current_time();
        //   auto t = turbo::Time::past_infinite();
        //   turbo::TimeZone::CivilTransition trans;
        //   while (t <= now && nyc.NextTransition(t, &trans)) {
        //     // transition: trans.from -> trans.to
        //     t = nyc.at(trans.to).trans;
        //   }
        struct CivilTransition {
            CivilSecond from;  // the civil time we jump from
            CivilSecond to;    // the civil time we jump to
        };

        bool NextTransition(Time t, CivilTransition *trans) const;

        bool PrevTransition(Time t, CivilTransition *trans) const;

        template<typename H>
        friend H turbo_hash_value(H h, TimeZone tz) {
            return H::combine(std::move(h), tz.cz_);
        }

    private:
        friend bool operator==(TimeZone a, TimeZone b) { return a.cz_ == b.cz_; }

        friend bool operator!=(TimeZone a, TimeZone b) { return a.cz_ != b.cz_; }

        friend std::ostream &operator<<(std::ostream &os, TimeZone tz) {
            return os << tz.name();
        }

        time_internal::cctz::time_zone cz_;
    };

    inline bool TimeZone::load(std::string_view name, TimeZone *tz) {
        if (name == "localtime") {
            *tz = TimeZone(time_internal::cctz::local_time_zone());
            return true;
        }
        time_internal::cctz::time_zone cz;
        const bool b = time_internal::cctz::load_time_zone(std::string(name), &cz);
        *tz = TimeZone(cz);
        return b;
    }

    inline TimeZone TimeZone::fixed(int seconds) {
        return TimeZone(
                time_internal::cctz::fixed_time_zone(std::chrono::seconds(seconds)));
    }

    inline TimeZone TimeZone::utc() {
        return TimeZone(time_internal::cctz::utc_time_zone());
    }

    // TimeZone::local()
    //
    // Convenience method returning the local time zone, or UTC if there is
    // no configured local zone.  Warning: Be wary of using TimeZone::local(),
    // and particularly so in a server process, as the zone configured for the
    // local machine should be irrelevant.  Prefer an explicit zone name.
    inline TimeZone TimeZone::local() {
        return TimeZone(time_internal::cctz::local_time_zone());
    }

    // RFC3339_full
    // RFC3339_sec
    //
    // Time::format()/Time::parse() format specifiers for RFC3339 date/time strings,
    // with trailing zeros trimmed or with fractional seconds omitted altogether.
    //
    // Note that RFC3339_sec[] matches an ISO 8601 extended format for date and
    // time with UTC offset.  Also note the use of "%Y": RFC3339 mandates that
    // years have exactly four digits, but we allow them to take their natural
    // width.
    TURBO_DLL extern const char RFC3339_full[];  // %Y-%m-%d%ET%H:%M:%E*S%Ez
    TURBO_DLL extern const char RFC3339_sec[];   // %Y-%m-%d%ET%H:%M:%S%Ez

    // RFC1123_full
    // RFC1123_no_wday
    //
    // Time::format()/Time::parse() format specifiers for RFC1123 date/time strings.
    TURBO_DLL extern const char RFC1123_full[];     // %a, %d %b %E4Y %H:%M:%S %z
    TURBO_DLL extern const char RFC1123_no_wday[];  // %d %b %E4Y %H:%M:%S %z

    // Output stream operator.
    inline std::ostream &operator<<(std::ostream &os, Time t) {
        return os << Time::format(t);
    }

    // Support for str_format(), str_cat() etc.
    template<typename Sink>
    void turbo_stringify(Sink &sink, Time t) {
        sink.Append(Time::format(t));
    }

    // ============================================================================
    // Implementation Details Follow
    // ============================================================================

    namespace time_internal {

        // Creates a Duration with a given representation.
        // REQUIRES: hi,lo is a valid representation of a Duration as specified
        // in time/duration.cc.
        TURBO_ATTRIBUTE_CONST_FUNCTION constexpr Duration MakeDuration(int64_t hi,
                                                                       uint32_t lo = 0) {
            return Duration(hi, lo);
        }

        TURBO_ATTRIBUTE_CONST_FUNCTION constexpr Duration MakeDuration(int64_t hi,
                                                                       int64_t lo) {
            return MakeDuration(hi, static_cast<uint32_t>(lo));
        }

        // Make a Duration value from a floating-point number, as long as that number
        // is in the range [ 0 .. numeric_limits<int64_t>::max ), that is, as long as
        // it's positive and can be converted to int64_t without risk of UB.
        TURBO_ATTRIBUTE_CONST_FUNCTION inline Duration MakePosDoubleDuration(double n) {
            const int64_t int_secs = static_cast<int64_t>(n);
            const uint32_t ticks = static_cast<uint32_t>(
                    std::round((n - static_cast<double>(int_secs)) * kTicksPerSecond));
            return ticks < kTicksPerSecond
                   ? MakeDuration(int_secs, ticks)
                   : MakeDuration(int_secs + 1, ticks - kTicksPerSecond);
        }

        // Creates a normalized Duration from an almost-normalized (sec,ticks)
        // pair. sec may be positive or negative.  ticks must be in the range
        // -kTicksPerSecond < *ticks < kTicksPerSecond.  If ticks is negative it
        // will be normalized to a positive value in the resulting Duration.
        TURBO_ATTRIBUTE_CONST_FUNCTION constexpr Duration MakeNormalizedDuration(
                int64_t sec, int64_t ticks) {
            return (ticks < 0) ? MakeDuration(sec - 1, ticks + kTicksPerSecond)
                               : MakeDuration(sec, ticks);
        }

        // Provide access to the Duration representation.
        TURBO_ATTRIBUTE_CONST_FUNCTION constexpr int64_t GetRepHi(Duration d) {
            return d.rep_hi_.Get();
        }

        TURBO_ATTRIBUTE_CONST_FUNCTION constexpr uint32_t GetRepLo(Duration d) {
            return d.rep_lo_;
        }

        // Returns true iff d is positive or negative infinity.
        TURBO_ATTRIBUTE_CONST_FUNCTION constexpr bool IsInfiniteDuration(Duration d) {
            return GetRepLo(d) == ~uint32_t{0};
        }

        // Returns an infinite Duration with the opposite sign.
        // REQUIRES: IsInfiniteDuration(d)
        TURBO_ATTRIBUTE_CONST_FUNCTION constexpr Duration OppositeInfinity(Duration d) {
            return GetRepHi(d) < 0
                   ? MakeDuration((std::numeric_limits<int64_t>::max)(), ~uint32_t{0})
                   : MakeDuration((std::numeric_limits<int64_t>::min)(),
                                  ~uint32_t{0});
        }

        // Returns (-n)-1 (equivalently -(n+1)) without avoidable overflow.
        TURBO_ATTRIBUTE_CONST_FUNCTION constexpr int64_t NegateAndSubtractOne(
                int64_t n) {
            // Note: Good compilers will optimize this expression to ~n when using
            // a two's-complement representation (which is required for int64_t).
            return (n < 0) ? -(n + 1) : (-n) - 1;
        }

        // Map between a Time and a Duration since the Unix epoch.  Note that these
        // functions depend on the above mentioned choice of the Unix epoch for the
        // Time representation (and both need to be Time friends).  Without this
        // knowledge, we would need to add-in/subtract-out Time::from_unix_epoch() respectively.
        TURBO_ATTRIBUTE_CONST_FUNCTION constexpr Time FromUnixDuration(Duration d) {
            return Time(d);
        }

        TURBO_ATTRIBUTE_CONST_FUNCTION constexpr Duration ToUnixDuration(Time t) {
            return t.rep_;
        }

        template<std::intmax_t N>
        TURBO_ATTRIBUTE_CONST_FUNCTION constexpr Duration FromInt64(int64_t v,
                                                                    std::ratio<1, N>) {
            static_assert(0 < N && N <= 1000 * 1000 * 1000, "Unsupported ratio");
            // Subsecond ratios cannot overflow.
            return MakeNormalizedDuration(
                    v / N, v % N * kTicksPerNanosecond * 1000 * 1000 * 1000 / N);
        }

        TURBO_ATTRIBUTE_CONST_FUNCTION constexpr Duration FromInt64(int64_t v,
                                                                    std::ratio<60>) {
            return (v <= (std::numeric_limits<int64_t>::max)() / 60 &&
                    v >= (std::numeric_limits<int64_t>::min)() / 60)
                   ? MakeDuration(v * 60)
                   : v > 0 ? Duration::max_infinite() : Duration::min_infinite();
        }

        TURBO_ATTRIBUTE_CONST_FUNCTION constexpr Duration FromInt64(int64_t v,
                                                                    std::ratio<3600>) {
            return (v <= (std::numeric_limits<int64_t>::max)() / 3600 &&
                    v >= (std::numeric_limits<int64_t>::min)() / 3600)
                   ? MakeDuration(v * 3600)
                   : v > 0 ? Duration::max_infinite() : Duration::min_infinite();
        }

        // IsValidRep64<T>(0) is true if the expression `int64_t{std::declval<T>()}` is
        // valid. That is, if a T can be assigned to an int64_t without narrowing.
        template<typename T>
        constexpr auto IsValidRep64(int) -> decltype(int64_t{std::declval<T>()} == 0) {
            return true;
        }

        template<typename T>
        constexpr auto IsValidRep64(char) -> bool {
            return false;
        }

        // Converts a std::chrono::duration to an turbo::Duration.
        template<typename Rep, typename Period>
        TURBO_ATTRIBUTE_PURE_FUNCTION constexpr Duration FromChrono(
                const std::chrono::duration<Rep, Period> &d) {
            static_assert(IsValidRep64<Rep>(0), "duration::rep is invalid");
            return FromInt64(int64_t{d.count()}, Period{});
        }

        template<typename Ratio>
        TURBO_ATTRIBUTE_CONST_FUNCTION int64_t ToInt64(Duration d, Ratio) {
            // Note: This may be used on MSVC, which may have a system_clock period of
            // std::ratio<1, 10 * 1000 * 1000>
            return Duration::to_seconds(d * Ratio::den / Ratio::num);
        }
        // Fastpath implementations for the 6 common duration units.
        TURBO_ATTRIBUTE_CONST_FUNCTION inline int64_t ToInt64(Duration d, std::nano) {
            return Duration::to_nanoseconds(d);
        }

        TURBO_ATTRIBUTE_CONST_FUNCTION inline int64_t ToInt64(Duration d, std::micro) {
            return Duration::to_microseconds(d);
        }

        TURBO_ATTRIBUTE_CONST_FUNCTION inline int64_t ToInt64(Duration d, std::milli) {
            return Duration::to_milliseconds(d);
        }

        TURBO_ATTRIBUTE_CONST_FUNCTION inline int64_t ToInt64(Duration d,
                                                              std::ratio<1>) {
            return Duration::to_seconds(d);
        }

        TURBO_ATTRIBUTE_CONST_FUNCTION inline int64_t ToInt64(Duration d,
                                                              std::ratio<60>) {
            return Duration::to_minutes(d);
        }

        TURBO_ATTRIBUTE_CONST_FUNCTION inline int64_t ToInt64(Duration d,
                                                              std::ratio<3600>) {
            return Duration::to_hours(d);
        }

        // Converts an turbo::Duration to a chrono duration of type T.
        template<typename T>
        TURBO_ATTRIBUTE_CONST_FUNCTION T ToChronoDuration(Duration d) {
            using Rep = typename T::rep;
            using Period = typename T::period;
            static_assert(IsValidRep64<Rep>(0), "duration::rep is invalid");
            if (time_internal::IsInfiniteDuration(d))
                return d < Duration::zero() ? (T::min)() : (T::max)();
            const auto v = ToInt64(d, Period{});
            if (v > (std::numeric_limits<Rep>::max)()) return (T::max)();
            if (v < (std::numeric_limits<Rep>::min)()) return (T::min)();
            return T{v};
        }

    }  // namespace time_internal

    TURBO_ATTRIBUTE_CONST_FUNCTION constexpr bool operator<(Duration lhs,
                                                            Duration rhs) {
        return time_internal::GetRepHi(lhs) != time_internal::GetRepHi(rhs)
               ? time_internal::GetRepHi(lhs) < time_internal::GetRepHi(rhs)
               : time_internal::GetRepHi(lhs) == (std::numeric_limits<int64_t>::min)()
                 ? time_internal::GetRepLo(lhs) + 1 <
                   time_internal::GetRepLo(rhs) + 1
                 : time_internal::GetRepLo(lhs) < time_internal::GetRepLo(rhs);
    }

    TURBO_ATTRIBUTE_CONST_FUNCTION constexpr bool operator==(Duration lhs,
                                                             Duration rhs) {
        return time_internal::GetRepHi(lhs) == time_internal::GetRepHi(rhs) &&
               time_internal::GetRepLo(lhs) == time_internal::GetRepLo(rhs);
    }

    TURBO_ATTRIBUTE_CONST_FUNCTION constexpr Duration operator-(Duration d) {
        // This is a little interesting because of the special cases.
        //
        // If rep_lo_ is zero, we have it easy; it's safe to negate rep_hi_, we're
        // dealing with an integral number of seconds, and the only special case is
        // the maximum negative finite duration, which can't be negated.
        //
        // Infinities stay infinite, and just change direction.
        //
        // Finally we're in the case where rep_lo_ is non-zero, and we can borrow
        // a second's worth of ticks and avoid overflow (as negating int64_t-min + 1
        // is safe).
        return time_internal::GetRepLo(d) == 0
               ? time_internal::GetRepHi(d) ==
                 (std::numeric_limits<int64_t>::min)()
                 ? Duration::max_infinite()
                 : time_internal::MakeDuration(-time_internal::GetRepHi(d))
               : time_internal::IsInfiniteDuration(d)
                 ? time_internal::OppositeInfinity(d)
                 : time_internal::MakeDuration(
                                time_internal::NegateAndSubtractOne(
                                        time_internal::GetRepHi(d)),
                                time_internal::kTicksPerSecond -
                                time_internal::GetRepLo(d));
    }
    TURBO_NAMESPACE_END
}  // namespace turbo

namespace turbo {
    /// Duration inlines and implementation details
    // Duration::zero()
    //
    // Returns a zero-length duration. This function behaves just like the default
    // constructor, but the name helps make the semantics clear at call sites.
    TURBO_ATTRIBUTE_CONST_FUNCTION constexpr Duration Duration::zero() {
        return Duration();
    }

    // Duration::abs()
    //
    // Returns the absolute value of a duration.
    TURBO_ATTRIBUTE_CONST_FUNCTION inline Duration Duration::abs(Duration d) {
        return (d < Duration::zero()) ? -d : d;
    }

    TURBO_ATTRIBUTE_CONST_FUNCTION constexpr Duration Duration::max_infinite() {
        return time_internal::MakeDuration((std::numeric_limits<int64_t>::max)(),
                                           ~uint32_t{0});
    }

    TURBO_ATTRIBUTE_CONST_FUNCTION constexpr Duration Duration::min_infinite() {
        return -max_infinite();
    }

    TURBO_ATTRIBUTE_PURE_FUNCTION constexpr Duration Duration::from_chrono(
            const std::chrono::nanoseconds &d) {
        return time_internal::FromChrono(d);
    }

    TURBO_ATTRIBUTE_PURE_FUNCTION constexpr Duration Duration::from_chrono(
            const std::chrono::microseconds &d) {
        return time_internal::FromChrono(d);
    }

    TURBO_ATTRIBUTE_PURE_FUNCTION constexpr Duration Duration::from_chrono(
            const std::chrono::milliseconds &d) {
        return time_internal::FromChrono(d);
    }

    TURBO_ATTRIBUTE_PURE_FUNCTION constexpr Duration Duration::from_chrono(
            const std::chrono::seconds &d) {
        return time_internal::FromChrono(d);
    }

    TURBO_ATTRIBUTE_PURE_FUNCTION constexpr Duration Duration::from_chrono(
            const std::chrono::minutes &d) {
        return time_internal::FromChrono(d);
    }

    TURBO_ATTRIBUTE_PURE_FUNCTION constexpr Duration Duration::from_chrono(
            const std::chrono::hours &d) {
        return time_internal::FromChrono(d);
    }

}  // namespace turbo

namespace turbo {
    /// Time inlines and implementation details
    template<typename T>
    inline T Time::current_microseconds() {
        return static_cast<T>(current_nanoseconds()) / T(1000);
    }

    template<typename T>
    inline T Time::current_milliseconds() {
        return static_cast<T>(current_nanoseconds()) / T(1000000);
    }

    template<typename T>
    inline T Time::current_seconds() {
        return static_cast<T>(current_nanoseconds()) / T(1000000000);
    }

    TURBO_ATTRIBUTE_CONST_FUNCTION constexpr Time Time::from_nanoseconds(int64_t ns) {
        return time_internal::FromUnixDuration(Duration::nanoseconds(ns));
    }

    TURBO_ATTRIBUTE_CONST_FUNCTION constexpr Time Time::from_microseconds(int64_t us) {
        return time_internal::FromUnixDuration(Duration::microseconds(us));
    }

    TURBO_ATTRIBUTE_CONST_FUNCTION constexpr Time Time::from_milliseconds(int64_t ms) {
        return time_internal::FromUnixDuration(Duration::milliseconds(ms));
    }

    TURBO_ATTRIBUTE_CONST_FUNCTION constexpr Time Time::from_seconds(int64_t s) {
        return time_internal::FromUnixDuration(Duration::seconds(s));
    }

    TURBO_ATTRIBUTE_CONST_FUNCTION constexpr Time Time::from_time_t(time_t t) {
        return time_internal::FromUnixDuration(Duration::seconds(t));
    }

    TURBO_ATTRIBUTE_PURE_FUNCTION inline Time Time::from_civil(CivilSecond ct,
                                                               TimeZone tz) {
        const auto ti = tz.at(ct);
        if (ti.kind == TimeZone::TimeInfo::SKIPPED) return ti.trans;
        return ti.pre;
    }

    // future_infinite()
    //
    // Returns an `turbo::Time` that is infinitely far in the future.
    TURBO_ATTRIBUTE_CONST_FUNCTION constexpr Time Time::future_infinite() {
        return Time(time_internal::MakeDuration((std::numeric_limits<int64_t>::max)(),
                                                ~uint32_t{0}));
    }

    // past_infinite()
    //
    // Returns an `turbo::Time` that is infinitely far in the past.
    TURBO_ATTRIBUTE_CONST_FUNCTION constexpr Time Time::past_infinite() {
        return Time(time_internal::MakeDuration((std::numeric_limits<int64_t>::min)(),
                                                ~uint32_t{0}));
    }

    // Time::from_unix_epoch()
    //
    // Returns the `turbo::Time` representing "1970-01-01 00:00:00.0 +0000".
    TURBO_ATTRIBUTE_CONST_FUNCTION constexpr Time Time::from_unix_epoch() { return Time(); }

    // Time::from_universal_epoch()
    //
    // Returns the `turbo::Time` representing "0001-01-01 00:00:00.0 +0000", the
    // epoch of the ICU Universal Time Scale.
    TURBO_ATTRIBUTE_CONST_FUNCTION constexpr Time Time::from_universal_epoch() {
        // 719162 is the number of days from 0001-01-01 to 1970-01-01,
        // assuming the Gregorian calendar.
        return Time(
                time_internal::MakeDuration(-24 * 719162 * int64_t{3600}, uint32_t{0}));
    }

    TURBO_ATTRIBUTE_PURE_FUNCTION inline CivilSecond Time::to_civil_second(Time t,
                                                                           TimeZone tz) {
        return tz.at(t).cs;  // already a CivilSecond
    }

    TURBO_ATTRIBUTE_PURE_FUNCTION inline CivilMinute Time::to_civil_minute(Time t, TimeZone tz) {
        return CivilMinute(tz.at(t).cs);
    }

    TURBO_ATTRIBUTE_PURE_FUNCTION inline CivilHour Time::to_civil_hour(Time t, TimeZone tz) {
        return CivilHour(tz.at(t).cs);
    }

    TURBO_ATTRIBUTE_PURE_FUNCTION inline CivilDay Time::to_civil_day(Time t, TimeZone tz) {
        return CivilDay(tz.at(t).cs);
    }

    TURBO_ATTRIBUTE_PURE_FUNCTION inline CivilMonth Time::to_civil_month(Time t,
                                                                         TimeZone tz) {
        return CivilMonth(tz.at(t).cs);
    }

    TURBO_ATTRIBUTE_PURE_FUNCTION inline CivilYear Time::to_civil_year(Time t, TimeZone tz) {
        return CivilYear(tz.at(t).cs);
    }

    /// to utc
    TURBO_ATTRIBUTE_PURE_FUNCTION inline CivilSecond Time::to_utc_civil_second(Time t) {
        return CivilSecond(TimeZone::utc().at(t).cs);
    }

    TURBO_ATTRIBUTE_PURE_FUNCTION inline CivilMinute Time::to_utc_civil_minute(Time t) {
        return CivilMinute(TimeZone::utc().at(t).cs);
    }

    TURBO_ATTRIBUTE_PURE_FUNCTION inline CivilHour Time::to_utc_civil_hour(Time t) {
        return CivilHour(TimeZone::utc().at(t).cs);
    }

    TURBO_ATTRIBUTE_PURE_FUNCTION inline CivilDay Time::to_utc_civil_day(Time t) {
        return CivilDay(TimeZone::utc().at(t).cs);
    }

    TURBO_ATTRIBUTE_PURE_FUNCTION inline CivilMonth Time::to_utc_civil_month(Time t) {
        return CivilMonth(TimeZone::utc().at(t).cs);
    }

    TURBO_ATTRIBUTE_PURE_FUNCTION inline CivilYear Time::to_utc_civil_year(Time t) {
        return CivilYear(TimeZone::utc().at(t).cs);
    }

    TURBO_ATTRIBUTE_PURE_FUNCTION inline struct tm Time::to_utc_tm(Time t) {
        return to_tm(t, TimeZone::utc());
    }
    /// to local

    TURBO_ATTRIBUTE_PURE_FUNCTION inline CivilSecond Time::to_local_civil_second(Time t) {
        return CivilSecond(TimeZone::local().at(t).cs);
    }

    TURBO_ATTRIBUTE_PURE_FUNCTION inline CivilMinute Time::to_local_civil_minute(Time t) {
        return CivilMinute(TimeZone::local().at(t).cs);
    }

    TURBO_ATTRIBUTE_PURE_FUNCTION inline CivilHour Time::to_local_civil_hour(Time t) {
        return CivilHour(TimeZone::local().at(t).cs);
    }

    TURBO_ATTRIBUTE_PURE_FUNCTION inline CivilDay Time::to_local_civil_day(Time t) {
        return CivilDay(TimeZone::local().at(t).cs);
    }

    TURBO_ATTRIBUTE_PURE_FUNCTION inline CivilMonth Time::to_local_civil_month(Time t) {
        return CivilMonth(TimeZone::local().at(t).cs);
    }

    TURBO_ATTRIBUTE_PURE_FUNCTION inline CivilYear Time::to_local_civil_year(Time t) {
        return CivilYear(TimeZone::local().at(t).cs);
    }

    TURBO_ATTRIBUTE_PURE_FUNCTION inline struct tm Time::to_local_tm(Time t) {
        return to_tm(t, TimeZone::local());
    }

    /// futures
    TURBO_ATTRIBUTE_CONST_FUNCTION inline Time Time::future_time(Duration d) {
        return Time::current_time() + d;
    }

    TURBO_ATTRIBUTE_CONST_FUNCTION inline int64_t Time::future_nanoseconds(int64_t ns) {
        return Time::current_nanoseconds() + ns;
    }

    TURBO_ATTRIBUTE_CONST_FUNCTION inline int64_t Time::future_microseconds(int64_t us) {
        return Time::current_microseconds() + us;
    }

    TURBO_ATTRIBUTE_CONST_FUNCTION inline int64_t Time::future_milliseconds(int64_t ms) {
        return Time::current_milliseconds() + ms;
    }

    TURBO_ATTRIBUTE_CONST_FUNCTION inline int64_t Time::future_seconds(int64_t s) {
        return Time::current_seconds() + s;
    }

    /// past
    TURBO_ATTRIBUTE_CONST_FUNCTION inline Time Time::past_time(Duration d) {
        return Time::current_time() - d;
    }

    TURBO_ATTRIBUTE_CONST_FUNCTION inline int64_t Time::past_nanoseconds(int64_t ns) {
        return Time::current_nanoseconds() - ns;
    }

    TURBO_ATTRIBUTE_CONST_FUNCTION inline int64_t Time::past_microseconds(int64_t us) {
        return Time::current_microseconds() - us;
    }

    TURBO_ATTRIBUTE_CONST_FUNCTION inline int64_t Time::past_milliseconds(int64_t ms) {
        return Time::current_milliseconds() - ms;

    }

    TURBO_ATTRIBUTE_CONST_FUNCTION inline int64_t Time::past_seconds(int64_t s) {
        return Time::current_seconds() - s;
    }

}  // namespace turbo
// In some build configurations we pass --detect-odr-violations to the
// gold linker.  This causes it to flag weak symbol overrides as ODR
// violations.  Because ODR only applies to C++ and not C,
// --detect-odr-violations ignores symbols not mangled with C++ names.
// By changing our extension points to be extern "C", we dodge this
// check.
extern "C" {
TURBO_DLL void TURBO_INTERNAL_C_SYMBOL(TurboInternalSleepFor)(
        turbo::Duration duration);
}  // extern "C"

inline void turbo::sleep_for(turbo::Duration duration) {
    TURBO_INTERNAL_C_SYMBOL(TurboInternalSleepFor)(duration);
}

#endif  // TURBO_TIME_TIME_H_
