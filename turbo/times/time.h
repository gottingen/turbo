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
//

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

#include "turbo/platform/port.h"
#include "turbo/strings/string_view.h"
#include "turbo/times/civil_time.h"
#include "turbo/times/cctz/time_zone.h"

namespace turbo {

    class Duration;  // Defined below
    class Time;      // Defined below
    class TimeZone;  // Defined below
}  // namespace turbo
namespace turbo::time_internal {
    int64_t safe_int_mod(bool satq, Duration num, Duration den, Duration *rem);

    constexpr Time FromUnixDuration(Duration d);

    constexpr Duration ToUnixDuration(Time t);

    constexpr int64_t GetRepHi(Duration d);

    constexpr uint32_t GetRepLo(Duration d);

    constexpr Duration MakeDuration(int64_t hi,
                                    uint32_t lo);

    constexpr Duration MakeDuration(int64_t hi,
                                    int64_t lo);

    inline Duration make_pos_double_duration(double n);

    constexpr int64_t kTicksPerNanosecond = 4;
    constexpr int64_t kTicksPerSecond = 1000 * 1000 * 1000 * kTicksPerNanosecond;

    template<std::intmax_t N>
    constexpr Duration FromInt64(int64_t v,
                                 std::ratio<1, N>);

    constexpr Duration FromInt64(int64_t v,
                                 std::ratio<60>);

    constexpr Duration FromInt64(int64_t v,
                                 std::ratio<3600>);

    template<typename T>
    using EnableIfIntegral = typename std::enable_if<
            std::is_integral<T>::value || std::is_enum<T>::value, int>::type;
    template<typename T>
    using EnableIfFloat =
            typename std::enable_if<std::is_floating_point<T>::value, int>::type;
}  // namespace turbo::time_internal

namespace turbo {

    /**
     * @ingroup turbo_times_duration
     * @brief The `turbo::Duration` class represents a signed, fixed-length amount of time.
     *        A `Duration` is generated using a unit-specific factory function, or is
     *        the result of subtracting one `turbo::Time` from another. Durations behave
     *        like unit-safe integers and they support all the natural integer-like
     *        arithmetic operations. Arithmetic overflows and saturates at +/- infinity.
     *        `Duration` should be passed by value rather than const reference.
     *
     *        Factory functions `nanoseconds()`, `microseconds()`, `milliseconds()`,
     *        `seconds()`, `minutes()`, `hours()` and `infinite_duration()` allow for
     *        creation of constexpr `Duration` values
     *
     *        Examples:
     *        @code
     *        constexpr turbo::Duration ten_ns = turbo::nanoseconds(10);
     *        constexpr turbo::Duration min = turbo::minutes(1);
     *        constexpr turbo::Duration hour = turbo::hours(1);
     *        turbo::Duration dur = 60 * min;  // dur == hour
     *        turbo::Duration half_sec = turbo::milliseconds(500);
     *        turbo::Duration quarter_sec = 0.25 * turbo::seconds(1);
     *        @endcode
     *
     *        `Duration` values can be easily converted to an integral number of units
     *        using the division operator.
     *
     *        Example:
     *        @code
     *        constexpr turbo::Duration dur = turbo::milliseconds(1500);
     *        int64_t ns = dur / turbo::nanoseconds(1);   // ns == 1500000000
     *        int64_t ms = dur / turbo::milliseconds(1);  // ms == 1500
     *        int64_t sec = dur / turbo::seconds(1);    // sec == 1 (subseconds truncated)
     *        int64_t min = dur / turbo::minutes(1);    // min == 0
     *        @endcode
     *
     *        See the `safe_int_mod()` and `safe_float_mod()` functions below for details on
     *        how to access the fractional parts of the quotient.
     *
     *        Alternatively, conversions can be performed using helpers such as
     *        `to_int64_microseconds()` and `to_double_seconds()`.
     */
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
        // Integer operands must be representable as int64_t.
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
        friend H hash_value(H h, Duration d) {
            return H::combine(std::move(h), d.rep_hi_, d.rep_lo_);
        }

    private:
        friend constexpr int64_t time_internal::GetRepHi(Duration d);

        friend constexpr uint32_t time_internal::GetRepLo(Duration d);

        friend constexpr Duration time_internal::MakeDuration(int64_t hi,
                                                              uint32_t lo);

        constexpr Duration(int64_t hi, uint32_t lo) : rep_hi_(hi), rep_lo_(lo) {}

        int64_t rep_hi_;
        uint32_t rep_lo_;
    };

    // Relational Operators
    constexpr bool operator<(Duration lhs,
                             Duration rhs);

    constexpr bool operator>(Duration lhs,
                             Duration rhs) {
        return rhs < lhs;
    }

    constexpr bool operator>=(Duration lhs,
                              Duration rhs) {
        return !(lhs < rhs);
    }

    constexpr bool operator<=(Duration lhs,
                              Duration rhs) {
        return !(rhs < lhs);
    }

    constexpr bool operator==(Duration lhs,
                              Duration rhs);

    constexpr bool operator!=(Duration lhs,
                              Duration rhs) {
        return !(lhs == rhs);
    }

    // Additive Operators
    constexpr Duration operator-(Duration d);

    inline Duration operator+(Duration lhs,
                              Duration rhs) {
        return lhs += rhs;
    }

    inline Duration operator-(Duration lhs,
                              Duration rhs) {
        return lhs -= rhs;
    }

    // Multiplicative Operators
    // Integer operands must be representable as int64_t.
    template<typename T>
    Duration operator*(Duration lhs, T rhs) {
        return lhs *= rhs;
    }

    template<typename T>
    Duration operator*(T lhs, Duration rhs) {
        return rhs *= lhs;
    }

    template<typename T>
    Duration operator/(Duration lhs, T rhs) {
        return lhs /= rhs;
    }

    inline int64_t operator/(Duration lhs,
                             Duration rhs) {
        return time_internal::safe_int_mod(true, lhs, rhs,
                                           &lhs);  // trunc towards zero
    }

    inline Duration operator%(Duration lhs,
                              Duration rhs) {
        return lhs %= rhs;
    }


    /**
     * @ingroup turbo_times_duration
     * @brief Divides a numerator `Duration` by a denominator `Duration`, returning the
     *        quotient and remainder. The remainder always has the same sign as the
     *        numerator. The returned quotient and remainder respect the identity:
     *        numerator = denominator * quotient + remainder
     *        Returned quotients are capped to the range of `int64_t`, with the difference
     *        spilling into the remainder to uphold the above identity. This means that the
     *        remainder returned could differ from the remainder returned by
     *        `Duration::operator%` for huge quotients.
     *        See also the notes on `infinite_duration()` below regarding the behavior of
     *        division involving zero and infinite durations.
     *        Example:
     *        @code
     *        constexpr turbo::Duration a = turbo::seconds(std::numeric_limits<int64_t>::max());  // big
     *        constexpr turbo::Duration b = turbo::nanoseconds(1);       // small
     *        turbo::Duration rem = a % b;
     *        // rem == turbo::zero_duration()
     *        // Here, q would overflow int64_t, so rem accounts for the difference.
     *        int64_t q = turbo::safe_int_mod(a, b, &rem);
     *        // q == std::numeric_limits<int64_t>::max(), rem == a - b * q
     *        @endcode
     * @param num the numerator
     * @param den the denominator
     * @param rem the remainder
     * @return the quotient
     */
    inline int64_t safe_int_mod(Duration num, Duration den, Duration *rem) {
        return time_internal::safe_int_mod(true, num, den,
                                           rem);  // trunc towards zero
    }

    /**
     * @ingroup turbo_times_duration
     * @brief Divides a `Duration` numerator into a fractional number of units of a
     *        `Duration` denominator.
     *        See also the notes on `infinite_duration()` below regarding the behavior of
     *        division involving zero and infinite durations.
     *        @see `turbo::safe_int_mod()` for a version that returns the quotient and remainder.
     *        Example:
     *        @code
     *        double d = turbo::safe_float_mod(turbo::milliseconds(1500), turbo::seconds(1));
     *        // d == 1.5
     *        @endcode
     * @param num the numerator
     * @param den the denominator
     * @return the quotient
     */
    double safe_float_mod(Duration num, Duration den);

    /**
     * @ingroup turbo_times_duration
     * @brief Returns a zero-length duration. This function behaves just like the default
     *        constructor, but the name helps make the semantics clear at call sites.
     * @return
     */
    constexpr Duration zero_duration() {
        return Duration{};
    }

    /**
     * @ingroup turbo_times_duration
     * @brief Returns the absolute value of a duration.
     * @param d
     * @return
     */
    inline Duration abs_duration(Duration d) {
        return (d < zero_duration()) ? -d : d;
    }

    /**
     * @ingroup turbo_times_duration
     * @brief Truncates a duration (toward zero) to a multiple of a non-zero unit.
     *        Example:
     *        @code
     *        turbo::Duration d = turbo::nanoseconds(123456789);
     *        turbo::Duration a = turbo::trunc(d, turbo::microseconds(1));  // 123456us
     *        @endcode
     * @param d
     * @param unit
     * @return
     */
    Duration trunc(Duration d, Duration unit);

    /**
     * @ingroup turbo_times_duration
     * @brief Floors a duration using the passed duration unit to its largest value not
     *        greater than the duration.
     *        Example:
     *        @code
     *        turbo::Duration d = turbo::nanoseconds(123456789);
     *        turbo::Duration b = turbo::floor(d, turbo::microseconds(1));  // 123456us
     *        @endcode
     * @param d
     * @param unit
     * @return
     */
    Duration floor(Duration d, Duration unit);

    /**
     * @ingroup turbo_times_duration
     * @brief Returns the ceiling of a duration using the passed duration unit to its
     *        smallest value not less than the duration.
     *        Example:
     *        @code
     *        turbo::Duration d = turbo::nanoseconds(123456789);
     *        turbo::Duration c = turbo::ceil(d, turbo::microseconds(1));   // 123457us
     *        @endcode
     * @param d
     * @param unit
     * @return
     */
    Duration ceil(Duration d, Duration unit);

    /**
     * @ingroup turbo_times_duration
     * @brief Returns an infinite `Duration`.  To get a `Duration` representing negative
     *        infinity, use `-infinite_duration()`.
     *
     *        Duration arithmetic overflows to +/- infinity and saturates. In general,
     *        arithmetic with `Duration` infinities is similar to IEEE 754 infinities
     *        except where IEEE 754 NaN would be involved, in which case +/-
     *        `infinite_duration()` is used in place of a "nan" Duration.
     *        Examples:
     *        @code
     *        constexpr turbo::Duration inf = turbo::infinite_duration();
     *        const turbo::Duration d = ... any finite duration ...
     *
     *        inf == inf + inf
     *        inf == inf + d
     *        inf == inf - inf
     *        -inf == d - inf
     *
     *        inf == d * 1e100
     *        inf == inf / 2
     *        0 == d / inf
     *        INT64_MAX == inf / d
     *
     *        d < inf
     *        -inf < d
     *
     *        // Division by zero returns infinity, or INT64_MIN/MAX where appropriate.
     *        inf == d / 0
     *        INT64_MAX == d / turbo::zero_duration()
     *        @endcode
     * @return
     */
    constexpr Duration infinite_duration();

    /**
     * @ingroup turbo_times_duration
     * @function nanoseconds()
     * @function microseconds()
     * @function milliseconds()
     * @function seconds()
     * @function minutes()
     * @function hours()
     * @brief Factory overloads for constructing `Duration` values from an integral
     *        number of the unit indicated by the factory function's name. These
     *        functions exist for convenience, but they are not as efficient as the
     *        integral factories, which should be preferred.
     *        Example:
     *        @code
     *        turbo::Duration a = turbo::seconds(60);
     *        turbo::Duration b = turbo::minutes(1);  // b == a
     *        auto a = turbo::seconds(1.5);        // OK
     *        auto b = turbo::milliseconds(1500);  // BETTER
     *        @endcode
     * @note no "Days()" factory function exists because "a day" is ambiguous.
     *          Civil days are not always 24 hours long, and a 24-hour duration often does
     *          not correspond with a civil day. If a 24-hour duration is needed, use
     *          `turbo::hours(24)`. If you actually want a civil day, use turbo::CivilDay
     *          from civil_time.h.
     * @param n
     * @return
     */
    template<typename T, time_internal::EnableIfIntegral<T> = 0>
    constexpr Duration nanoseconds(T n) {
        return time_internal::FromInt64(n, std::nano{});
    }

    template<typename T, time_internal::EnableIfIntegral<T> = 0>
    constexpr Duration microseconds(T n) {
        return time_internal::FromInt64(n, std::micro{});
    }

    template<typename T, time_internal::EnableIfIntegral<T> = 0>
    constexpr Duration milliseconds(T n) {
        return time_internal::FromInt64(n, std::milli{});
    }

    template<typename T, time_internal::EnableIfIntegral<T> = 0>
    constexpr Duration seconds(T n) {
        return time_internal::FromInt64(n, std::ratio<1>{});
    }

    template<typename T, time_internal::EnableIfIntegral<T> = 0>
    constexpr Duration minutes(T n) {
        return time_internal::FromInt64(n, std::ratio<60>{});
    }

    template<typename T, time_internal::EnableIfIntegral<T> = 0>
    constexpr Duration hours(T n) {
        return time_internal::FromInt64(n, std::ratio<3600>{});
    }

    template<typename T, time_internal::EnableIfFloat<T> = 0>
    Duration nanoseconds(T n) {
        return n * nanoseconds(1);
    }

    template<typename T, time_internal::EnableIfFloat<T> = 0>
    Duration microseconds(T n) {
        return n * microseconds(1);
    }

    template<typename T, time_internal::EnableIfFloat<T> = 0>
    Duration milliseconds(T n) {
        return n * milliseconds(1);
    }

    template<typename T, time_internal::EnableIfFloat<T> = 0>
    Duration seconds(T n) {
        if (n >= 0) {  // Note: `NaN >= 0` is false.
            if (n >= static_cast<T>((std::numeric_limits<int64_t>::max)())) {
                return infinite_duration();
            }
            return time_internal::make_pos_double_duration(n);
        } else {
            if (std::isnan(n))
                return std::signbit(n) ? -infinite_duration() : infinite_duration();
            if (n <= (std::numeric_limits<int64_t>::min)()) return -infinite_duration();
            return -time_internal::make_pos_double_duration(-n);
        }
    }

    template<typename T, time_internal::EnableIfFloat<T> = 0>
    Duration minutes(T n) {
        return n * minutes(1);
    }

    template<typename T, time_internal::EnableIfFloat<T> = 0>
    Duration hours(T n) {
        return n * hours(1);
    }

    /**
     * @ingroup turbo_times_duration
     * @brief Helper functions that convert a Duration to an integral count of the
     *        indicated unit. These return the same results as the `safe_int_mod()`
     *        function, though they usually do so more efficiently; see the
     *        documentation of `safe_int_mod()` for details about overflow, etc.
     *
     *        Example:
     *        @code
     *        turbo::Duration d = turbo::milliseconds(1500);
     *        int64_t isec = turbo::to_int64_seconds(d);  // isec == 1
     *        @endcode
     * @param d the duration to get from
     * @return the integral count of the indicated unit
     */
    int64_t to_int64_nanoseconds(Duration d);

    /**
     * @ingroup turbo_times_duration
     * @brief  similar to `to_int64_nanoseconds()`
     * @see `to_int64_nanoseconds()`
     * @param d
     * @return
     */
    int64_t to_int64_microseconds(Duration d);

    /**
     * @ingroup turbo_times_duration
     * @brief  similar to `to_int64_nanoseconds()`
     * @see `to_int64_nanoseconds()`
     * @param d
     * @return
     */
    int64_t to_int64_milliseconds(Duration d);

    /**
     * @ingroup turbo_times_duration
     * @brief  similar to `to_int64_nanoseconds()`
     * @see `to_int64_nanoseconds()`
     * @param d
     * @return
     */
    int64_t to_int64_seconds(Duration d);

    /**
     * @ingroup turbo_times_duration
     * @brief  similar to `to_int64_nanoseconds()`
     * @see `to_int64_nanoseconds()`
     * @param d
     * @return
     */
    int64_t to_int64_minutes(Duration d);

    /**
     * @ingroup turbo_times_duration
     * @brief  similar to `to_int64_nanoseconds()`
     * @see `to_int64_nanoseconds()`
     * @param d
     * @return
     */
    int64_t to_int64_hours(Duration d);

    /**
     * @ingroup turbo_times_duration
     * @brief Helper functions that convert a Duration to a floating point count of the
     *       indicated unit. These functions are shorthand for the `safe_float_mod()`
     *       function above; see its documentation for details about overflow, etc.
     *
     *       Example:
     *       @code
     *       turbo::Duration d = turbo::milliseconds(1500);
     *       double dsec = turbo::to_double_seconds(d);  // dsec == 1.5
     *       @endcode
     * @param d
     * @return
     */
    double to_double_nanoseconds(Duration d);

    /**
     * @ingroup turbo_times_duration
     * @brief  similar to `to_double_nanoseconds()`
     * @see `to_double_nanoseconds()`
     * @param d
     * @return
     */
    double to_double_microseconds(Duration d);

    /**
     * @ingroup turbo_times_duration
     * @brief  similar to `to_double_nanoseconds()`
     * @see `to_double_nanoseconds()`
     * @param d
     * @return
     */
    double to_double_milliseconds(Duration d);

    /**
     * @ingroup turbo_times_duration
     * @brief  similar to `to_double_nanoseconds()`
     * @see `to_double_nanoseconds()`
     * @param d
     * @return
     */
    double to_double_seconds(Duration d);

    /**
     * @ingroup turbo_times_duration
     * @brief  similar to `to_double_nanoseconds()`
     * @see `to_double_nanoseconds()`
     * @param d
     * @return
     */
    double to_double_minutes(Duration d);

    /**
     * @ingroup turbo_times_duration
     * @brief  similar to `to_double_nanoseconds()`
     * @see `to_double_nanoseconds()`
     * @param d
     * @return
     */
    double to_double_hours(Duration d);

    /**
     * @ingroup turbo_times_duration
     * @brief Converts any of the pre-defined std::chrono durations to an turbo::Duration.
     *        Example:
     *        @code
     *        std::chrono::milliseconds ms(123);
     *        turbo::Duration d = turbo::from_chrono(ms);
     *        @endcode
     * @param d
     * @return
     */
    constexpr Duration from_chrono(
            const std::chrono::nanoseconds &d);

    /**
     * @ingroup turbo_times_duration
     * @brief  similar to `from_chrono()`
     * @see `from_chrono()`
     * @param d
     * @return
     */
    constexpr Duration from_chrono(
            const std::chrono::microseconds &d);

    /**
     * @ingroup turbo_times_duration
     * @brief  similar to `from_chrono()`
     * @see `from_chrono()`
     * @param d
     * @return
     */
    constexpr Duration from_chrono(
            const std::chrono::milliseconds &d);

    /**
     * @ingroup turbo_times_duration
     * @brief  similar to `from_chrono()`
     * @see `from_chrono()`
     * @param d
     * @return
     */
    constexpr Duration from_chrono(
            const std::chrono::seconds &d);

    /**
     * @ingroup turbo_times_duration
     * @brief  similar to `from_chrono()`
     * @see `from_chrono()`
     * @param d
     * @return
     */
    constexpr Duration from_chrono(
            const std::chrono::minutes &d);

    /**
     * @ingroup turbo_times_duration
     * @brief  similar to `from_chrono()`
     * @see `from_chrono()`
     * @param d
     * @return
     */
    constexpr Duration from_chrono(
            const std::chrono::hours &d);

    /**
     * @ingroup turbo_times_duration
     * @brief Converts an turbo::Duration to any of the pre-defined std::chrono durations.
     *        If overflow would occur, the returned value will saturate at the min/max
     *        chrono duration value instead.
     *        Example:
     *        @code
     *        turbo::Duration d = turbo::microseconds(123);
     *        auto x = turbo::to_chrono_microseconds(d);
     *        auto y = turbo::to_chrono_nanoseconds(d);  // x == y
     *        auto z = turbo::to_chrono_seconds(turbo::infinite_duration());
     *        // z == std::chrono::seconds::max()
     *        @endcode
     * @param d
     * @return
     */
    std::chrono::nanoseconds to_chrono_nanoseconds(
            Duration d);

    /**
     * @ingroup turbo_times_duration
     * @brief  similar to `to_chrono_nanoseconds()`
     * @see `to_chrono_nanoseconds()`
     * @param d
     * @return
     */
    std::chrono::microseconds to_chrono_microseconds(
            Duration d);

    /**
     * @ingroup turbo_times_duration
     * @brief  similar to `to_chrono_nanoseconds()`
     * @see `to_chrono_nanoseconds()`
     * @param d
     * @return
     */
    std::chrono::milliseconds to_chrono_milliseconds(
            Duration d);

    /**
     * @ingroup turbo_times_duration
     * @brief  similar to `to_chrono_nanoseconds()`
     * @see `to_chrono_nanoseconds()`
     * @param d
     * @return
     */
    std::chrono::seconds to_chrono_seconds(Duration d);

    /**
     * @ingroup turbo_times_duration
     * @brief  similar to `to_chrono_seconds()`
     * @see `to_chrono_seconds()`
     * @param d
     * @return
     */
    std::chrono::minutes to_chrono_minutes(Duration d);

    /**
     * @ingroup turbo_times_duration
     * @brief  similar to `to_chrono_nanoseconds()`
     * @see `to_chrono_nanoseconds()`
     * @param d
     * @return
     */
    std::chrono::hours to_chrono_hours(Duration d);

    /**
     * @ingroup turbo_times_duration
     * @brief Returns a string representing the duration in the form "72h3m0.5s".
     *        Returns "inf" or "-inf" for +/- `infinite_duration()`.
     * @param d
     * @return
     */
    std::string format_duration(Duration d);

    // Output stream operator.
    inline std::ostream &operator<<(std::ostream &os, Duration d) {
        return os << format_duration(d);
    }

    /**
     * @ingroup turbo_times_duration
     * @brief Parses a duration string consisting of a possibly signed sequence of
     *        decimal numbers, each with an optional fractional part and a unit
     *        suffix.  The valid suffixes are "ns", "us" "ms", "s", "m", and "h".
     *        Simple examples include "300ms", "-1.5h", and "2h45m".  Parses "0" as
     *        `zero_duration()`. Parses "inf" and "-inf" as +/- `infinite_duration()`.
     * @param dur_string
     * @param d
     * @return
     */
    bool parse_duration(std::string_view dur_string, Duration *d);

    /**
     * @brief The `turbo::Time` class represents a specific instant in time. Arithmetic operators
     *        are provided for naturally expressing time calculations. Instances are
     *        created using `turbo::time_now()` and the `turbo::From*()` factory functions that
     *        accept the gamut of other time representations. Formatting and parsing
     *        functions are provided for conversion to and from strings.  `turbo::Time`
     *        should be passed by value rather than const reference.
     *
     *        `turbo::Time` assumes there are 60 seconds in a minute, which means the
     *        underlying time scales must be "smeared" to eliminate leap seconds.
     *        See https://developers.google.com/time/smear.
     *
     *        Even though `turbo::Time` supports a wide range of timestamps, exercise
     *        caution when using values in the distant past. `turbo::Time` uses the
     *        Proleptic Gregorian calendar, which extends the Gregorian calendar backward
     *        to dates before its introduction in 1582.
     *        See https://en.wikipedia.org/wiki/Proleptic_Gregorian_calendar
     *        for more information. Use the ICU calendar classes to convert a date in
     *        some other calendar (http://userguide.icu-project.org/datetime/calendar).
     *
     *        Similarly, standardized time zones are a reasonably recent innovation, with
     *        the Greenwich prime meridian being established in 1884. The TZ database
     *        itself does not profess accurate offsets for timestamps prior to 1970. The
     *        breakdown of future timestamps is subject to the whim of regional
     *        governments.
     *
     *        The `turbo::Time` class represents an instant in time as a count of clock
     *        ticks of some granularity (resolution) from some starting point (epoch).
     *
     *        `turbo::Time` uses a resolution that is high enough to avoid loss in
     *        precision, and a range that is wide enough to avoid overflow, when
     *        converting between tick counts in most Google time scales (i.e., resolution
     *        of at least one nanosecond, and range +/-100 billion years).  Conversions
     *        between the time scales are performed by truncating (towards negative
     *        infinity) to the nearest representable point.
     *
     *        Examples:
     *        @code
     *        turbo::Time t1 = ...;
     *        turbo::Time t2 = t1 + turbo::minutes(2);
     *        turbo::Duration d = t2 - t1;  // == turbo::minutes(2)
     *        @endcode
     */
    class Time {
    public:
        // Value semantics.

        // Returns the Unix epoch.  However, those reading your code may not know
        // or expect the Unix epoch as the default value, so make your code more
        // readable by explicitly initializing all instances before use.
        //
        // Example:
        //   turbo::Time t = turbo::unix_epoch();
        //   turbo::Time t = turbo::time_now();
        //   turbo::Time t = turbo::time_from_timeval(tv);
        //   turbo::Time t = turbo::infinite_past();
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

        // Time::Breakdown
        //
        // The calendar and wall-clock (aka "civil time") components of an
        // `turbo::Time` in a certain `turbo::TimeZone`. This struct is not
        // intended to represent an instant in time. So, rather than passing
        // a `Time::Breakdown` to a function, pass an `turbo::Time` and an
        // `turbo::TimeZone`.
        //
        // Deprecated. Use `turbo::TimeZone::CivilInfo`.
        struct
        Breakdown {
            int64_t year;        // year (e.g., 2013)
            int month;           // month of year [1:12]
            int day;             // day of month [1:31]
            int hour;            // hour of day [0:23]
            int minute;          // minute of hour [0:59]
            int second;          // second of minute [0:59]
            Duration subsecond;  // [seconds(0):seconds(1)) if finite
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

        // Time::In()
        //
        // Returns the breakdown of this instant in the given TimeZone.
        //
        // Deprecated. Use `turbo::TimeZone::At(Time)`.
        Breakdown In(TimeZone tz) const;

        template<typename H>
        friend H hash_value(H h, Time t) {
            return H::combine(std::move(h), t.rep_);
        }

    private:
        friend constexpr Time time_internal::FromUnixDuration(Duration d);

        friend constexpr Duration time_internal::ToUnixDuration(Time t);

        friend constexpr bool operator<(Time lhs, Time rhs);

        friend constexpr bool operator==(Time lhs, Time rhs);

        friend Duration operator-(Time lhs, Time rhs);

        friend constexpr Time universal_epoch();

        friend constexpr Time infinite_future();

        friend constexpr Time infinite_past();

        constexpr explicit Time(Duration rep) : rep_(rep) {}

        Duration rep_;
    };

    // Relational Operators
    constexpr bool operator<(Time lhs, Time rhs) {
        return lhs.rep_ < rhs.rep_;
    }

    constexpr bool operator>(Time lhs, Time rhs) {
        return rhs < lhs;
    }

    constexpr bool operator>=(Time lhs, Time rhs) {
        return !(lhs < rhs);
    }

    constexpr bool operator<=(Time lhs, Time rhs) {
        return !(rhs < lhs);
    }

    constexpr bool operator==(Time lhs, Time rhs) {
        return lhs.rep_ == rhs.rep_;
    }

    constexpr bool operator!=(Time lhs, Time rhs) {
        return !(lhs == rhs);
    }

    // Additive Operators
    inline Time operator+(Time lhs, Duration rhs) {
        return lhs += rhs;
    }

    inline Time operator+(Duration lhs, Time rhs) {
        return rhs += lhs;
    }

    inline Time operator-(Time lhs, Duration rhs) {
        return lhs -= rhs;
    }

    inline Duration operator-(Time lhs, Time rhs) {
        return lhs.rep_ - rhs.rep_;
    }

    /**
     * @ingroup turbo_times_time_point_create
     * @brief Returns the `turbo::Time` representing "1970-01-01 00:00:00.0 +0000".
     * @return
     */
    constexpr Time unix_epoch() { return Time(); }

    /**
     * @ingroup turbo_times_time_point_create
     * @brief Returns the `turbo::Time` representing "0001-01-01 00:00:00.0 +0000", the
     *        epoch of the ICU Universal Time Scale.
     * @return
     */
    constexpr Time universal_epoch() {
        // 719162 is the number of days from 0001-01-01 to 1970-01-01,
        // assuming the Gregorian calendar.
        return Time(
                time_internal::MakeDuration(-24 * 719162 * int64_t{3600}, uint32_t{0}));
    }

    /**
     * @ingroup turbo_times_time_point_create
     * @brief Returns an `turbo::Time` that is infinitely far in the future.
     * @return
     */
    constexpr Time infinite_future() {
        return Time(time_internal::MakeDuration((std::numeric_limits<int64_t>::max)(),
                                                ~uint32_t{0}));
    }

    /**
     * @ingroup turbo_times_time_point_create
     * @brief Returns an `turbo::Time` that is infinitely far in the past.
     * @return
     */
    constexpr Time infinite_past() {
        return Time(time_internal::MakeDuration((std::numeric_limits<int64_t>::min)(),
                                                ~uint32_t{0}));
    }

    /**
     * @ingroup turbo_times_time_point_create
     * @brief Creates an `turbo::Time` from a variety of other representations.
     * @param ns
     * @return
     */
    constexpr Time from_unix_nanos(int64_t ns);

    /**
     * @ingroup turbo_times_time_point_create
     * @brief  similar to `from_unix_nanos()`
     * @see `from_unix_nanos()`
     * @param us
     * @return
     */
    constexpr Time from_unix_micros(int64_t us);

    /**
     * @ingroup turbo_times_time_point_create
     * @brief  similar to `from_unix_nanos()`
     * @see `from_unix_nanos()`
     * @param ms
     * @return
     */
    constexpr Time from_unix_millis(int64_t ms);

    /**
     * @ingroup turbo_times_time_point_create
     * @brief  similar to `from_unix_nanos()`
     * @see `from_unix_nanos()`
     * @param s
     * @return
     */
    constexpr Time from_unix_seconds(int64_t s);

    /**
     * @ingroup turbo_times_time_point_create
     * @brief  similar to `from_unix_nanos()`
     * @see `from_unix_nanos()`
     * @param t
     * @return
     */
    constexpr Time from_time_t(time_t t);

    /**
     * @ingroup turbo_times_time_point_create
     * @brief  similar to `from_unix_nanos()`
     * @see `from_unix_nanos()`
     * @param udate
     * @return
     */
    Time from_udate(double udate);

    /**
     * @ingroup turbo_times_time_point_create
     * @brief  similar to `from_unix_nanos()`
     * @see `from_unix_nanos()`
     * @param universal
     * @return
     */
    Time from_universal(int64_t universal);

    /**
     * @ingroup turbo_times_time_point
     * @brief Converts an `turbo::Time` to a variety of other representations. Note that
     *       these operations round down toward negative infinity where necessary to
     *       adjust to the resolution of the result type.  Beware of possible time_t
     *       over/underflow in ToTime{T,val,spec}() on 32-bit platforms.
     *       Example:
     *       @code
     *       turbo::Time t = turbo::from_unix_seconds(123);
     *       int64_t ns = turbo::to_unix_nanos(t);  // ns == 123000000000
     *       @endcode
     * @param t the time to convert
     * @return the converted time
     */
    int64_t to_unix_nanos(Time t);

    /**
     * @ingroup turbo_times_time_point
     * @brief  similar to `to_unix_nanos()`
     * @see `to_unix_nanos()`
     * @param t
     * @return
     */
    int64_t to_unix_micros(Time t);

    /**
     * @ingroup turbo_times_time_point
     * @brief  similar to `to_unix_nanos()`
     * @see `to_unix_nanos()`
     * @param t
     * @return
     */
    int64_t to_unix_millis(Time t);

    /**
     * @ingroup turbo_times_time_point
     * @brief  similar to `to_unix_nanos()`
     * @see `to_unix_nanos()`
     * @param t
     * @return
     */
    int64_t to_unix_seconds(Time t);

    /**
     * @ingroup turbo_times_time_point
     * @brief  similar to `to_unix_nanos()`
     * @see `to_unix_nanos()`
     * @param t
     * @return
     */
    time_t to_time_t(Time t);

    /**
     * @ingroup turbo_times_time_point
     * @brief  similar to `to_unix_nanos()`
     * @see `to_unix_nanos()`
     * @param t
     * @return
     */
    double to_udate(Time t);

    /**
     * @ingroup turbo_times_time_point
     * @brief  similar to `to_unix_nanos()`
     * @see `to_unix_nanos()`
     * @param t
     * @return
     */
    int64_t to_universal(Time t);

    /**
     * @ingroup turbo_times_time_point
     * @brief Converts a `timespec` to a `Duration`.
     * @param ts
     * @return
     */
    Duration duration_from_timespec(timespec ts);

    /**
     * @ingroup turbo_times_time_point
     * @brief  similar to `duration_from_timespec()`
     * @see `duration_from_timespec()`
     * @param tv
     * @return
     */
    Duration duration_from_timeval(timeval tv);

    /**
     * @ingroup turbo_times_time_point
     * @brief  similar to `duration_from_timespec()`
     * @see `duration_from_timespec()`
     * @param d
     * @return
     */
    timespec to_timespec(Duration d);

    /**
     * @ingroup turbo_times_time_point
     * @brief  similar to `duration_from_timespec()`
     * @see `duration_from_timespec()`
     * @param d
     * @return
     */
    timeval to_timeval(Duration d);

    /**
     * @ingroup turbo_times_time_point
     * @brief Converts a `std::chrono::system_clock::time_point` to an `turbo::Time`.
     * @param tp
     * @return
     */
    Time time_from_timespec(timespec ts);

    /**
     * @ingroup turbo_times_time_point
     * @brief  similar to `time_from_timespec()`
     * @see `time_from_timespec()`
     * @param tv
     * @return
     */
    Time time_from_timeval(timeval tv);

    /**
     * @ingroup turbo_times_time_point
     * @brief  similar to `time_from_timespec()`
     * @see `time_from_timespec()`
     * @param t
     * @return
     */
    timespec to_timespec(Time t);

    /**
     * @ingroup turbo_times_time_point
     * @brief  similar to `time_from_timespec()`
     * @see `time_from_timespec()`
     * @param t
     * @return
     */
    timeval to_timeval(Time t);

    /**
     * @ingroup turbo_times_time_point
     * @brief  Converts a `std::chrono::system_clock::time_point` to an `turbo::Time`.
     *         Example:
     *         @code {.cpp}
     *         auto tp = std::chrono::system_clock::from_time_t(123);
     *         turbo::Time t = turbo::from_chrono(tp);
     *         // t == turbo::from_time_t(123)
     *         @endcode
     * @param tp
     * @return
     */
    Time
    from_chrono(const std::chrono::system_clock::time_point &tp);

    /**
     * @ingroup turbo_times_time_point_create
     * @brief Converts an `turbo::Time` to a `std::chrono::system_clock::time_point`.
     *        If overflow would occur, the returned value will saturate at the min/max
     *        time point value instead.
     *        Example:
     *        @code
     *        turbo::Time t = turbo::from_time_t(123);
     *        auto tp = turbo::to_chrono_time(t);
     *        // tp == std::chrono::system_clock::from_time_t(123);
     *        @endcode
     * @param t
     * @return
     */
    std::chrono::system_clock::time_point
    to_chrono_time(Time);

    /**
     * @ingroup turbo_times_time_zone
     * @brief The `turbo::TimeZone` is an opaque, small, value-type class representing a
     *        geo-political region within which particular rules are used for converting
     *        between absolute and civil times (see https://git.io/v59Ly). `turbo::TimeZone`
     *        values are named using the TZ identifiers from the IANA Time Zone Database,
     *        such as "America/Los_Angeles" or "Australia/Sydney". `turbo::TimeZone` values
     *        are created from factory functions such as `turbo::load_time_zone()`. Note:
     *        strings like "PST" and "EDT" are not valid TZ identifiers. Prefer to pass by
     *        value rather than const reference.
     *
     *        For more on the fundamental concepts of time zones, absolute times, and civil
     *        times, see https://github.com/google/cctz#fundamental-concepts
     *
     *        Examples:
     *        @code
     *        turbo::TimeZone utc = turbo::utc_time_zone();
     *        turbo::TimeZone pst = turbo::fixed_time_zone(-8 * 60 * 60);
     *        turbo::TimeZone loc = turbo::local_time_zone();
     *        turbo::TimeZone lax;
     *        if (!turbo::load_time_zone("America/Los_Angeles", &lax)) {
     *              // handle error case
     *        }
     *        @endcode
     *        See also:
     *        - https://github.com/google/cctz
     *        - https://www.iana.org/time-zones
     *        - https://en.wikipedia.org/wiki/Zoneinfo
     */
    class TimeZone {
    public:
        explicit TimeZone(time_internal::cctz::time_zone tz) : cz_(tz) {}

        TimeZone() = default;  // UTC, but prefer utc_time_zone() to be explicit.

        // Copyable.
        TimeZone(const TimeZone &) = default;

        TimeZone &operator=(const TimeZone &) = default;

        explicit operator time_internal::cctz::time_zone() const { return cz_; }

        std::string name() const { return cz_.name(); }

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
            // should only access this data indirectly by way of format_time().
            // These fields are undefined for infinite_future() and infinite_past().
            int offset;             // seconds east of UTC
            bool is_dst;            // is offset non-standard?
            const char *zone_abbr;  // time-zone abbreviation (e.g., "PST")
        };

        /**
         * @brief Returns the civil time for this TimeZone at a certain `turbo::Time`.
         *        If the input time is infinite, the output civil second will be set to
         *        CivilSecond::max() or min(), and the subsecond will be infinite.
         *
         *        Example:
         *        @code
         *        const auto epoch = lax.At(turbo::unix_epoch());
         *        // epoch.cs == 1969-12-31 16:00:00
         *        // epoch.subsecond == turbo::zero_duration()
         *        // epoch.offset == -28800
         *        // epoch.is_dst == false
         *        // epoch.abbr == "PST"
         *        @endcode
         * @param t the time to convert
         * @return the converted time
         */
        CivilInfo At(Time t) const;

        /**
         * @brief Information about the absolute times corresponding to a civil time.
         *        (Subseconds must be handled separately.)
         *
         *        It is possible for a caller to pass a civil-time value that does
         *        not represent an actual or unique instant in time (due to a shift
         *        in UTC offset in the TimeZone, which results in a discontinuity in
         *        the civil-time components). For example, a daylight-saving-time
         *        transition skips or repeats civil times---in the United States,
         *        March 13, 2011 02:15 never occurred, while November 6, 2011 01:15
         *        occurred twice---so requests for such times are not well-defined.
         *        To account for these possibilities, `turbo::TimeZone::TimeInfo` is
         *        richer than just a single `turbo::Time`.
         */
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

        /**
         * @brief Returns the absolute time(s) for this TimeZone at a certain `turbo::CivilSecond`.
         *        When the civil time is skipped or repeated, returns times calculated using
         *        the pre-transition and post-transition UTC offsets, plus the transition time
         *        itself.
         *
         *        Examples:
         *        @code
         *        const auto jan01 = lax.At(turbo::CivilSecond(2011, 1, 1, 0, 0, 0));
         *        // jan01.kind == TimeZone::TimeInfo::UNIQUE
         *        // jan01.pre    is 2011-01-01 00:00:00 -0800
         *        // jan01.trans  is 2011-01-01 00:00:00 -0800
         *        // jan01.post   is 2011-01-01 00:00:00 -0800
         *
         *        // A Spring DST transition, when there is a gap in civil time
         *        const auto mar13 = lax.At(turbo::CivilSecond(2011, 3, 13, 2, 15, 0));
         *        // mar13.kind == TimeZone::TimeInfo::SKIPPED
         *        // mar13.pre   is 2011-03-13 03:15:00 -0700
         *        // mar13.trans is 2011-03-13 03:00:00 -0700
         *        // mar13.post  is 2011-03-13 01:15:00 -0800
         *
         *        // A Fall DST transition, when civil times are repeated
         *        const auto nov06 = lax.At(turbo::CivilSecond(2011, 11, 6, 1, 15, 0));
         *        // nov06.kind == TimeZone::TimeInfo::REPEATED
         *        // nov06.pre   is 2011-11-06 01:15:00 -0700
         *        // nov06.trans is 2011-11-06 01:00:00 -0800
         *        // nov06.post  is 2011-11-06 01:15:00 -0800
         *        @endcode
         * @param cs the civil time to convert
         * @return the converted time
         */
        TimeInfo At(CivilSecond ct) const;

        /**
         * @brief Finds the time of the next offset change in this time zone.
         *       By definition, `NextTransition(t, &trans)` returns false when `t` is
         *       `infinite_future()`. If the zone has no transitions, the result will
         *       also be false no matter what the argument.
         *
         *       Otherwise, when `t` is `infinite_past()`, `NextTransition(t, &trans)`
         *       returns true and sets `trans` to the first recorded transition. Chains
         *       of calls to `NextTransition()/PrevTransition()` will eventually return
         *       false, but it is unspecified exactly when `NextTransition(t, &trans)`
         *       jumps to false, or what time is set by `PrevTransition(t, &trans)` for
         *       a very distant `t`.
         *
         * @note Enumeration of time-zone transitions is for informational purposes only.
         *      Modern time-related code should not care about when offset changes occur.
         *      Example:
         *      @code
         *      turbo::TimeZone nyc;
         *      if (!turbo::load_time_zone("America/New_York", &nyc)) { ... }
         *      const auto now = turbo::time_now();
         *      auto t = turbo::infinite_past();
         *      turbo::TimeZone::CivilTransition trans;
         *      while (t <= now && nyc.NextTransition(t, &trans)) {
         *          // transition: trans.from -> trans.to
         *          t = nyc.At(trans.to).trans;
         *      }
         *     @endcode
         */
        struct CivilTransition {
            CivilSecond from;  // the civil time we jump from
            CivilSecond to;    // the civil time we jump to
        };

        bool NextTransition(Time t, CivilTransition *trans) const;

        bool PrevTransition(Time t, CivilTransition *trans) const;

        template<typename H>
        friend H hash_value(H h, TimeZone tz) {
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

    /**
     * @ingroup turbo_times_time_zone
     * @brief Loads the named zone. May perform I/O on the initial load of the named
     *        zone. If the name is invalid, or some other kind of error occurs, returns
     *        `false` and `*tz` is set to the UTC time zone.
     * @param name
     * @param tz
     * @return
     */
    inline bool load_time_zone(std::string_view name, TimeZone *tz) {
        if (name == "localtime") {
            *tz = TimeZone(time_internal::cctz::local_time_zone());
            return true;
        }
        time_internal::cctz::time_zone cz;
        const bool b = time_internal::cctz::load_time_zone(std::string(name), &cz);
        *tz = TimeZone(cz);
        return b;
    }


    /**`
     * @ingroup turbo_times_time_zone
     * @brief Returns a TimeZone that is a fixed offset (seconds east) from UTC.
     *        Note: If the absolute value of the offset is greater than 24 hours
     *        you'll get UTC (i.e., no offset) instead.
     * @param seconds
     * @return
     */
    inline TimeZone fixed_time_zone(int seconds) {
        return TimeZone(
                time_internal::cctz::fixed_time_zone(std::chrono::seconds(seconds)));
    }

    /**
     * @ingroup turbo_times_time_zone
     * @brief Convenience method returning the UTC time zone.
     * @return
     */
    inline TimeZone utc_time_zone() {
        return TimeZone(time_internal::cctz::utc_time_zone());
    }

    /**
     * @ingroup turbo_times_time_zone
     * @brief Convenience method returning the local time zone, or UTC if there is
     *        no configured local zone.  Warning: Be wary of using local_time_zone(),
     *        and particularly so in a server process, as the zone configured for the
     *        local machine should be irrelevant.  Prefer an explicit zone name.
     * @return
     */
    inline TimeZone local_time_zone() {
        return TimeZone(time_internal::cctz::local_time_zone());
    }

    /**
     * @ingroup turbo_times_time_zone
     * @brief Helpers for TimeZone::At(Time) to return particularly aligned civil times.
     *       Example:
     *       @code
     *       turbo::Time t = ...;
     *       turbo::TimeZone tz = ...;
     *       const auto cd = turbo::to_civil_day(t, tz);
     *       @endcode
     * @param t the time to convert
     * @param tz the time zone to use
     * @return the converted time
     */
    inline CivilSecond to_civil_second(Time t,
                                       TimeZone tz) {
        return tz.At(t).cs;  // already a CivilSecond
    }

    /**
     * @ingroup turbo_times_time_zone
     * @brief  similar to `to_civil_second()`
     * @see `to_civil_second()`
     * @param t
     * @param tz
     * @return
     */
    inline CivilMinute to_civil_minute(Time t,
                                       TimeZone tz) {
        return CivilMinute(tz.At(t).cs);
    }

    /**
     * @ingroup turbo_times_time_zone
     * @brief  similar to `to_civil_second()`
     * @see `to_civil_second()`
     * @param t
     * @param tz
     * @return
     */
    inline CivilHour to_civil_hour(Time t, TimeZone tz) {
        return CivilHour(tz.At(t).cs);
    }

    /**
     * @ingroup turbo_times_time_zone
     * @brief  similar to `to_civil_second()`
     * @see `to_civil_second()`
     * @param t
     * @param tz
     * @return
     */
    inline CivilDay to_civil_day(Time t, TimeZone tz) {
        return CivilDay(tz.At(t).cs);
    }

    /**
     * @ingroup turbo_times_time_zone
     * @brief  similar to `to_civil_second()`
     * @see `to_civil_second()`
     * @param t
     * @param tz
     * @return
     */
    inline CivilMonth to_civil_month(Time t,
                                   TimeZone tz) {
        return CivilMonth(tz.At(t).cs);
    }

    /**
     * @ingroup turbo_times_time_zone
     * @brief  similar to `to_civil_second()`
     * @see `to_civil_second()`
     * @param t
     * @param tz
     * @return
     */
    inline CivilYear to_civil_year(Time t, TimeZone tz) {
        return CivilYear(tz.At(t).cs);
    }

    /**
     * @ingroup turbo_times_time_zone
     * @brief Helper for TimeZone::At(CivilSecond) that provides "order-preserving
     *       semantics." If the civil time maps to a unique time, that time is
     *       returned. If the civil time is repeated in the given time zone, the
     *       time using the pre-transition offset is returned. Otherwise, the
     *       civil time is skipped in the given time zone, and the transition time
     *       is returned. This means that for any two civil times, ct1 and ct2,
     *       (ct1 < ct2) => (from_civil(ct1) <= from_civil(ct2)), the equal case
     *       being when two non-existent civil times map to the same transition time.
     *
     *       Note: Accepts civil times of any alignment.
     * @param ct
     * @param tz
     * @return
     */
    inline Time from_civil(CivilSecond ct,
                           TimeZone tz) {
        const auto ti = tz.At(ct);
        if (ti.kind == TimeZone::TimeInfo::SKIPPED) return ti.trans;
        return ti.pre;
    }

    // TimeConversion
    //
    // An `turbo::TimeConversion` represents the conversion of year, month, day,
    // hour, minute, and second values (i.e., a civil time), in a particular
    // `turbo::TimeZone`, to a time instant (an absolute time), as returned by
    // `turbo::convert_date_time()`. Legacy version of `turbo::TimeZone::TimeInfo`.
    //
    // Deprecated. Use `turbo::TimeZone::TimeInfo`.
    struct
    TimeConversion {
        Time pre;    // time calculated using the pre-transition offset
        Time trans;  // when the civil-time discontinuity occurred
        Time post;   // time calculated using the post-transition offset

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
    // Legacy version of `turbo::TimeZone::At(turbo::CivilSecond)` that takes
    // the civil time as six, separate values (YMDHMS).
    //
    // The input month, day, hour, minute, and second values can be outside
    // of their valid ranges, in which case they will be "normalized" during
    // the conversion.
    //
    // Example:
    //
    //   // "October 32" normalizes to "November 1".
    //   turbo::TimeConversion tc =
    //       turbo::convert_date_time(2013, 10, 32, 8, 30, 0, lax);
    //   // tc.kind == TimeConversion::UNIQUE && tc.normalized == true
    //   // turbo::to_civil_day(tc.pre, tz).month() == 11
    //   // turbo::to_civil_day(tc.pre, tz).day() == 1
    //
    // Deprecated. Use `turbo::TimeZone::At(CivilSecond)`.
    TimeConversion convert_date_time(int64_t year, int mon, int day, int hour,
                                   int min, int sec, TimeZone tz);

    // from_date_time()
    //
    // A convenience wrapper for `turbo::convert_date_time()` that simply returns
    // the "pre" `turbo::Time`.  That is, the unique result, or the instant that
    // is correct using the pre-transition offset (as if the transition never
    // happened).
    //
    // Example:
    //
    //   turbo::Time t = turbo::from_date_time(2017, 9, 26, 9, 30, 0, lax);
    //   // t = 2017-09-26 09:30:00 -0700
    //
    // Deprecated. Use `turbo::from_civil(CivilSecond, TimeZone)`. Note that the
    // behavior of `from_civil()` differs from `from_date_time()` for skipped civil
    // times. If you care about that see `turbo::TimeZone::At(turbo::CivilSecond)`.
    /**
     * @ingroup turbo_times_time_zone
     * @brief  similar to `convert_date_time()` but returns the "pre" `turbo::Time`
     *         (the unique result, or the instant that is correct using the pre-transition
     *         offset (as if the transition never happened)).
     *         Deprecated. Use `turbo::from_civil(CivilSecond, TimeZone)`. Note that the
     *         behavior of `from_civil()` differs from `from_date_time()` for skipped civil
     *         times. If you care about that see `turbo::TimeZone::At(turbo::CivilSecond)`.
     *         Example:
     *         @code
     *         turbo::Time t = turbo::from_date_time(2017, 9, 26, 9, 30, 0, lax);
     *         // t = 2017-09-26 09:30:00 -0700
     *         @endcode
     *
     * @see `convert_date_time()`
     * @param year
     * @param mon
     * @param day
     * @param hour
     * @param min
     * @param sec
     * @param tz
     * @return
     */
    inline Time from_date_time(int64_t year, int mon, int day, int hour,
                             int min, int sec, TimeZone tz) {
        return convert_date_time(year, mon, day, hour, min, sec, tz).pre;
    }

    /**
     * @ingroup turbo_times_time_zone
     * @brief Converts the given `tm_year`, `tm_mon`, `tm_mday`, `tm_hour`, `tm_min`, and
     *        `tm_sec` fields to an `turbo::Time` using the given time zone. See ctime(3)
     *        for a description of the expected values of the tm fields. If the civil time
     *        is unique (see `turbo::TimeZone::At(turbo::CivilSecond)` above), the matching
     *        time instant is returned.  Otherwise, the `tm_isdst` field is consulted to
     *        choose between the possible results.  For a repeated civil time, `tm_isdst !=
     *        0` returns the matching DST instant, while `tm_isdst == 0` returns the
     *        matching non-DST instant.  For a skipped civil time there is no matching
     *        instant, so `tm_isdst != 0` returns the DST instant, and `tm_isdst == 0`
     *        returns the non-DST instant, that would have matched if the transition never
     *        happened.
     *        Example:
     *        @code
     *        struct tm tm;
     *        tm.tm_year = 2017 - 1900;
     *        tm.tm_mon = 9 - 1;
     *        tm.tm_mday = 26;
     *        tm.tm_hour = 9;
     *        tm.tm_min = 30;
     *        tm.tm_sec = 0;
     *        tm.tm_isdst = -1;
     *        turbo::Time t = turbo::from_tm(tm, lax);
     *        // t == 2017-09-26 09:30:00 -0700
     *        @endcode
     * @param tm
     * @param tz
     * @return
     */
    Time from_tm(const struct tm &tm, TimeZone tz);

    /**
     * @ingroup turbo_times_time_zone
     * @brief Converts the given `turbo::Time` to a struct tm using the given time zone.
     *        See ctime(3) for a description of the values of the tm fields.
     * @param t
     * @param tz
     * @return
     */
    struct tm to_tm(Time t, TimeZone tz);

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
    TURBO_DLL extern const char RFC3339_full[];  // %Y-%m-%d%ET%H:%M:%E*S%Ez
    TURBO_DLL extern const char RFC3339_sec[];   // %Y-%m-%d%ET%H:%M:%S%Ez

    // RFC1123_full
    // RFC1123_no_wday
    //
    // format_time()/parse_time() format specifiers for RFC1123 date/time strings.
    TURBO_DLL extern const char RFC1123_full[];     // %a, %d %b %E4Y %H:%M:%S %z
    TURBO_DLL extern const char RFC1123_no_wday[];  // %d %b %E4Y %H:%M:%S %z

    /**
     * @ingroup turbo_times_time_zone
     * @brief Formats the given `turbo::Time` in the `turbo::TimeZone` according to the
     *        provided format string. Uses strftime()-like formatting options, with
     *        the following extensions:
     *        - %Ez  - RFC3339-compatible numeric UTC offset (+hh:mm or -hh:mm)
     *        - %E*z - Full-resolution numeric UTC offset (+hh:mm:ss or -hh:mm:ss)
     *        - %E#S - seconds with # digits of fractional precision
     *        - %E*S - seconds with full fractional precision (a literal '*')
     *        - %E#f - Fractional seconds with # digits of precision
     *        - %E*f - Fractional seconds with full precision (a literal '*')
     *        - %E4Y - Four-character years (-999 ... -001, 0000, 0001 ... 9999)
     *        - %ET  - The RFC3339 "date-time" separator "T"
     *        Note that %E0S behaves like %S, and %E0f produces no characters.  In
     *        contrast %E*f always produces at least one digit, which may be '0'.
     *        Note that %Y produces as many characters as it takes to fully render the
     *        year.  A year outside of [-999:9999] when formatted with %E4Y will produce
     *        more than four characters, just like %Y.
     *        We recommend that format strings include the UTC offset (%z, %Ez, or %E*z)
     *        so that the result uniquely identifies a time instant.
     *        Example:
     *        @code
     *        turbo::CivilSecond cs(2013, 1, 2, 3, 4, 5);
     *        turbo::Time t = turbo::from_civil(cs, lax);
     *        std::string f = turbo::format_time("%H:%M:%S", t, lax);  // "03:04:05"
     *        f = turbo::format_time("%H:%M:%E3S", t, lax);  // "03:04:05.000"
     *        @endcode
     *        Note: If the given `turbo::Time` is `turbo::infinite_future()`, the returned
     *        string will be exactly "infinite-future". If the given `turbo::Time` is
     *        `turbo::infinite_past()`, the returned string will be exactly "infinite-past".
     *        In both cases the given format string and `turbo::TimeZone` are ignored.
     *        Example:
     *        @code
     *        turbo::Time t = turbo::infinite_future();
     *        std::string f = turbo::format_time("%H:%M:%S", t, lax);  // "infinite-future"
     *        @endcode
     * @param format
     * @param t
     * @param tz
     * @return
     */
    std::string format_time(std::string_view format,
                            Time t, TimeZone tz);

    // Convenience functions that format the given time using the RFC3339_full
    // format.  The first overload uses the provided TimeZone, while the second
    // uses local_time_zone().
    std::string format_time(Time t, TimeZone tz);

    std::string format_time(Time t);

    // Output stream operator.
    inline std::ostream &operator<<(std::ostream &os, Time t) {
        return os << format_time(t);
    }

    /**
     * @ingroup turbo_times_time_zone
     * @brief Parses an input string according to the provided format string and
     *        returns the corresponding `turbo::Time`. Uses strftime()-like formatting
     *        options, with the same extensions as format_time(), but with the
     *        exceptions that %E#S is interpreted as %E*S, and %E#f as %E*f.  %Ez
     *        and %E*z also accept the same inputs, which (along with %z) includes
     *        'z' and 'Z' as synonyms for +00:00.  %ET accepts either 'T' or 't'.
     *        %Y consumes as many numeric characters as it can, so the matching data
     *        should always be terminated with a non-numeric.  %E4Y always consumes
     *        exactly four characters, including any sign.
     *        Unspecified fields are taken from the default date and time of ...
     *        "1970-01-01 00:00:00.0 +0000"
     *        For example, parsing a string of "15:45" (%H:%M) will return an turbo::Time
     *        that represents "1970-01-01 15:45:00.0 +0000".
     *        Note that since parse_time() returns time instants, it makes the most sense
     *        to parse fully-specified date/time strings that include a UTC offset (%z,
     *        %Ez, or %E*z).
     *        Note also that `turbo::parse_time()` only heeds the fields year, month, day,
     *        hour, minute, (fractional) second, and UTC offset.  Other fields, like
     *        weekday (%a or %A), while parsed for syntactic validity, are ignored
     *        in the conversion.
     *        Date and time fields that are out-of-range will be treated as errors
     *        rather than normalizing them like `turbo::CivilSecond` does.  For example,
     *        it is an error to parse the date "Oct 32, 2013" because 32 is out of range.
     *        A leap second of ":60" is normalized to ":00" of the following minute with
     *        fractional seconds discarded.  The following table shows how the given
     *        seconds and subseconds will be parsed:
     *        "59.x" -> 59.x  // exact
     *        "60.x" -> 00.0  // normalized
     *        "00.x" -> 00.x  // exact
     *        Errors are indicated by returning false and assigning an error message
     *        to the "err" out param if it is non-null.
     *        Note: If the input string is exactly "infinite-future", the returned
     *        `turbo::Time` will be `turbo::infinite_future()` and `true` will be returned.
     *        If the input string is "infinite-past", the returned `turbo::Time` will be
     *        `turbo::infinite_past()` and `true` will be returned.
     * @param format
     * @param input
     * @param time
     * @param err
     * @return
     */
    bool parse_time(std::string_view format, std::string_view input, Time *time,
                    std::string *err);

    /**
     * @ingroup turbo_times_time_zone
     * @brief Like `parse_time()` above, but if the format string does not contain a UTC
     *        offset specification (%z/%Ez/%E*z) then the input is interpreted in the
     *        given TimeZone.  This means that the input, by itself, does not identify a
     *        unique instant.  Being time-zone dependent, it also admits the possibility
     *        of ambiguity or non-existence, in which case the "pre" time (as defined
     *        by TimeZone::TimeInfo) is returned.  For these reasons we recommend that
     *        all date/time strings include a UTC offset so they're context independent.
     *        Example:
     *        @code
     *        turbo::Time t;
     *        std::string err;
     *        bool b = turbo::parse_time("%Y-%m-%d %H:%M:%S", "2013-10-19 12:34:56",
     *                                   lax, &t, &err);
     *        // b == true && err.empty() && t == 2013-10-19 12:34:56 -0700
     *        @endcode
     * @param format
     * @param input
     * @param tz
     * @param time
     * @param err
     * @return
     */
    bool parse_time(std::string_view format, std::string_view input, TimeZone tz,
                    Time *time, std::string *err);

    // ============================================================================
    // Implementation Details Follow
    // ============================================================================

    namespace time_internal {

        // Creates a Duration with a given representation.
        // REQUIRES: hi,lo is a valid representation of a Duration as specified
        // in time/duration.cc.
        constexpr Duration MakeDuration(int64_t hi,
                                        uint32_t lo = 0) {
            return Duration(hi, lo);
        }

        constexpr Duration MakeDuration(int64_t hi,
                                        int64_t lo) {
            return MakeDuration(hi, static_cast<uint32_t>(lo));
        }

        // Make a Duration value from a floating-point number, as long as that number
        // is in the range [ 0 .. numeric_limits<int64_t>::max ), that is, as long as
        // it's positive and can be converted to int64_t without risk of UB.
        inline Duration make_pos_double_duration(double n) {
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
        constexpr Duration MakeNormalizedDuration(
                int64_t sec, int64_t ticks) {
            return (ticks < 0) ? MakeDuration(sec - 1, ticks + kTicksPerSecond)
                               : MakeDuration(sec, ticks);
        }

        // Provide access to the Duration representation.
        constexpr int64_t GetRepHi(Duration d) {
            return d.rep_hi_;
        }

        constexpr uint32_t GetRepLo(Duration d) {
            return d.rep_lo_;
        }

        // Returns true iff d is positive or negative infinity.
        constexpr bool IsInfiniteDuration(Duration d) {
            return GetRepLo(d) == ~uint32_t{0};
        }

        // Returns an infinite Duration with the opposite sign.
        // REQUIRES: IsInfiniteDuration(d)
        constexpr Duration OppositeInfinity(Duration d) {
            return GetRepHi(d) < 0
                   ? MakeDuration((std::numeric_limits<int64_t>::max)(), ~uint32_t{0})
                   : MakeDuration((std::numeric_limits<int64_t>::min)(),
                                  ~uint32_t{0});
        }

        // Returns (-n)-1 (equivalently -(n+1)) without avoidable overflow.
        constexpr int64_t NegateAndSubtractOne(
                int64_t n) {
            // Note: Good compilers will optimize this expression to ~n when using
            // a two's-complement representation (which is required for int64_t).
            return (n < 0) ? -(n + 1) : (-n) - 1;
        }

        // Map between a Time and a Duration since the Unix epoch.  Note that these
        // functions depend on the above mentioned choice of the Unix epoch for the
        // Time representation (and both need to be Time friends).  Without this
        // knowledge, we would need to add-in/subtract-out unix_epoch() respectively.
        constexpr Time FromUnixDuration(Duration d) {
            return Time(d);
        }

        constexpr Duration ToUnixDuration(Time t) {
            return t.rep_;
        }

        template<std::intmax_t N>
        constexpr Duration FromInt64(int64_t v,
                                     std::ratio<1, N>) {
            static_assert(0 < N && N <= 1000000000, "Unsupported ratio");
            // Subsecond ratios cannot overflow.
            return MakeNormalizedDuration(
                    v / N, v % N * kTicksPerNanosecond * 1000 * 1000 * 1000 / N);
        }

        constexpr Duration FromInt64(int64_t v,
                                     std::ratio<60>) {
            return (v <= (std::numeric_limits<int64_t>::max)() / 60 &&
                    v >= (std::numeric_limits<int64_t>::min)() / 60)
                   ? MakeDuration(v * 60)
                   : v > 0 ? infinite_duration() : -infinite_duration();
        }

        constexpr Duration FromInt64(int64_t v,
                                     std::ratio<3600>) {
            return (v <= (std::numeric_limits<int64_t>::max)() / 3600 &&
                    v >= (std::numeric_limits<int64_t>::min)() / 3600)
                   ? MakeDuration(v * 3600)
                   : v > 0 ? infinite_duration() : -infinite_duration();
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
        constexpr Duration from_chrono(
                const std::chrono::duration<Rep, Period> &d) {
            static_assert(IsValidRep64<Rep>(0), "duration::rep is invalid");
            return FromInt64(int64_t{d.count()}, Period{});
        }

        template<typename Ratio>
        int64_t ToInt64(Duration d, Ratio) {
            // Note: This may be used on MSVC, which may have a system_clock period of
            // std::ratio<1, 10 * 1000 * 1000>
            return to_int64_seconds(d * Ratio::den / Ratio::num);
        }

        // Fastpath implementations for the 6 common duration units.
        inline int64_t ToInt64(Duration d, std::nano) {
            return to_int64_nanoseconds(d);
        }

        inline int64_t ToInt64(Duration d, std::micro) {
            return to_int64_microseconds(d);
        }

        inline int64_t ToInt64(Duration d, std::milli) {
            return to_int64_milliseconds(d);
        }

        inline int64_t ToInt64(Duration d,
                               std::ratio<1>) {
            return to_int64_seconds(d);
        }

        inline int64_t ToInt64(Duration d,
                               std::ratio<60>) {
            return to_int64_minutes(d);
        }

        inline int64_t ToInt64(Duration d,
                               std::ratio<3600>) {
            return to_int64_hours(d);
        }

        // Converts an turbo::Duration to a chrono duration of type T.
        template<typename T>
        T ToChronoDuration(Duration d) {
            using Rep = typename T::rep;
            using Period = typename T::period;
            static_assert(IsValidRep64<Rep>(0), "duration::rep is invalid");
            if (time_internal::IsInfiniteDuration(d))
                return d < zero_duration() ? (T::min)() : (T::max)();
            const auto v = ToInt64(d, Period{});
            if (v > (std::numeric_limits<Rep>::max)()) return (T::max)();
            if (v < (std::numeric_limits<Rep>::min)()) return (T::min)();
            return T{v};
        }

    }  // namespace time_internal

    constexpr bool operator<(Duration lhs,
                             Duration rhs) {
        return time_internal::GetRepHi(lhs) != time_internal::GetRepHi(rhs)
               ? time_internal::GetRepHi(lhs) < time_internal::GetRepHi(rhs)
               : time_internal::GetRepHi(lhs) == (std::numeric_limits<int64_t>::min)()
                 ? time_internal::GetRepLo(lhs) + 1 <
                   time_internal::GetRepLo(rhs) + 1
                 : time_internal::GetRepLo(lhs) < time_internal::GetRepLo(rhs);
    }

    constexpr bool operator==(Duration lhs,
                              Duration rhs) {
        return time_internal::GetRepHi(lhs) == time_internal::GetRepHi(rhs) &&
               time_internal::GetRepLo(lhs) == time_internal::GetRepLo(rhs);
    }

    constexpr Duration operator-(Duration d) {
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
                 ? infinite_duration()
                 : time_internal::MakeDuration(-time_internal::GetRepHi(d))
               : time_internal::IsInfiniteDuration(d)
                 ? time_internal::OppositeInfinity(d)
                 : time_internal::MakeDuration(
                                time_internal::NegateAndSubtractOne(
                                        time_internal::GetRepHi(d)),
                                time_internal::kTicksPerSecond -
                                time_internal::GetRepLo(d));
    }

    constexpr Duration infinite_duration() {
        return time_internal::MakeDuration((std::numeric_limits<int64_t>::max)(),
                                           ~uint32_t{0});
    }

    constexpr Duration from_chrono(
            const std::chrono::nanoseconds &d) {
        return time_internal::from_chrono(d);
    }

    constexpr Duration from_chrono(
            const std::chrono::microseconds &d) {
        return time_internal::from_chrono(d);
    }

    constexpr Duration from_chrono(
            const std::chrono::milliseconds &d) {
        return time_internal::from_chrono(d);
    }

    constexpr Duration from_chrono(
            const std::chrono::seconds &d) {
        return time_internal::from_chrono(d);
    }

    constexpr Duration from_chrono(
            const std::chrono::minutes &d) {
        return time_internal::from_chrono(d);
    }

    constexpr Duration from_chrono(
            const std::chrono::hours &d) {
        return time_internal::from_chrono(d);
    }

    constexpr Time from_unix_nanos(int64_t ns) {
        return time_internal::FromUnixDuration(nanoseconds(ns));
    }

    constexpr Time from_unix_micros(int64_t us) {
        return time_internal::FromUnixDuration(microseconds(us));
    }

    constexpr Time from_unix_millis(int64_t ms) {
        return time_internal::FromUnixDuration(milliseconds(ms));
    }

    constexpr Time from_unix_seconds(int64_t s) {
        return time_internal::FromUnixDuration(seconds(s));
    }

    constexpr Time from_time_t(time_t t) {
        return time_internal::FromUnixDuration(seconds(t));
    }

}  // namespace turbo

#endif  // TURBO_TIME_TIME_H_
