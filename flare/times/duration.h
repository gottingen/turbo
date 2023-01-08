
/****************************************************************
 * Copyright (c) 2022, liyinbin
 * All rights reserved.
 * Author by liyinbin (jeff.li) lijippy@163.com
 *****************************************************************/
#ifndef FLARE_TIMES_DURATION_H_
#define FLARE_TIMES_DURATION_H_

#include <string_view>
#include <chrono>
#include <string>
#include "flare/base/profile.h"
#include "flare/base/int128.h"
#include "flare/base/math.h"

namespace flare {

    class duration;

    namespace times_internal {
        constexpr int64_t kint64max = std::numeric_limits<int64_t>::max();
        constexpr int64_t kint64min = std::numeric_limits<int64_t>::min();


        constexpr int64_t kTicksPerNanosecond = 4;
        constexpr int64_t kTicksPerSecond = 1000 * 1000 * 1000 * kTicksPerNanosecond;

        bool is_valid_divisor(double d);

        void normalize_ticks(int64_t *sec, int64_t *ticks);

        uint64_t encode_twos_comp(int64_t v);

        int64_t decode_twos_comp(uint64_t v);

        template<typename T>
        using enable_if_integral = typename std::enable_if<
                std::is_integral<T>::value || std::is_enum<T>::value, int>::type;
        template<typename T>
        using enable_if_float =
        typename std::enable_if<std::is_floating_point<T>::value, int>::type;

        // Returns (-n)-1 (equivalently -(n+1)) without avoidable overflow.
        constexpr int64_t negate_and_subtract_one(int64_t n) {
            // Note: Good compilers will optimize this expression to ~n when using
            // a two's-complement representation (which is required for int64_t).
            return (n < 0) ? -(n + 1) : (-n) - 1;
        }

        // is_valid_rep64<T>(0) is true if the expression `int64_t{std::declval<T>()}` is
        // valid. That is, if a T can be assigned to an int64_t without narrowing.
        template<typename T>
        constexpr auto is_valid_rep64(int) -> decltype(int64_t{std::declval<T>()} == 0) {
            return true;
        }

        template<typename T>
        constexpr auto is_valid_rep64(char) -> bool {
            return false;
        }
    }  // namespace times_internal

// Additive Operators
    constexpr duration operator-(duration d);

    // duration
    //
    // The `flare::duration` class represents a signed, fixed-length span of time.
    // A `duration` is generated using a unit-specific factory function, or is
    // the result of subtracting one `flare::time_point` from another. Durations behave
    // like unit-safe integers and they support all the natural integer-like
    // arithmetic operations. Arithmetic overflows and saturates at +/- infinity.
    // `duration` should be passed by value rather than const reference.
    //
    // Factory functions `nanoseconds()`, `microseconds()`, `milliseconds()`,
    // `seconds()`, `minutes()`, `hours()` and `infinite_duration()` allow for
    // creation of constexpr `duration` values
    //
    // Examples:
    //
    //   constexpr flare::duration ten_ns = flare::nanoseconds(10);
    //   constexpr flare::duration min = flare::minutes(1);
    //   constexpr flare::duration hour = flare::hours(1);
    //   flare::duration dur = 60 * min;  // dur == hour
    //   flare::duration half_sec = flare::milliseconds(500);
    //   flare::duration quarter_sec = 0.25 * flare::seconds(1);
    //
    // `duration` values can be easily converted to an integral number of units
    // using the division operator.
    //
    // Example:
    //
    //   constexpr flare::duration dur = flare::milliseconds(1500);
    //   int64_t ns = dur / flare::nanoseconds(1);   // ns == 1500000000
    //   int64_t ms = dur / flare::milliseconds(1);  // ms == 1500
    //   int64_t sec = dur / flare::seconds(1);    // sec == 1 (subseconds truncated)
    //   int64_t min = dur / flare::minutes(1);    // min == 0
    //
    // See the `integer_div_duration()` and `float_div_duration()` functions below for details on
    // how to access the fractional parts of the quotient.
    //
    // Alternatively, conversions can be performed using helpers such as
    // `to_int64_microseconds()` and `to_double_seconds()`.
    class duration {
    public:
        // Value semantics.
        constexpr duration() : _rep_hi(0), _rep_lo(0) {}  // zero-length duration

        // Copyable.
#if !defined(__clang__) && defined(_MSC_VER) && _MSC_VER < 1910
        // Explicitly defining the constexpr copy constructor avoids an MSVC bug.
        constexpr duration(const duration& d)
            : rep_hi_(d.rep_hi_), rep_lo_(d.rep_lo_) {}
#else

        constexpr duration(const duration &d) = default;

#endif

        duration &operator=(const duration &d) = default;

        // Compound assignment operators.
        duration &operator+=(duration d);

        duration &operator-=(duration d);

        duration &operator*=(int64_t r);

        duration &operator*=(double r);

        duration &operator/=(int64_t r);

        duration &operator/=(double r);

        duration &operator%=(duration rhs);

        // Overloads that forward to either the int64_t or double overloads above.
        // Integer operands must be representable as int64_t.
        template<typename T>
        duration &operator*=(T r) {
            int64_t x = r;
            return *this *= x;
        }

        template<typename T>
        duration &operator/=(T r) {
            int64_t x = r;
            return *this /= x;
        }

        duration &operator*=(float r) { return *this *= static_cast<double>(r); }

        duration &operator/=(float r) { return *this /= static_cast<double>(r); }

    public:
        // Returns true iff d is positive or negative infinity.
        constexpr bool is_infinite_duration() const { return _rep_lo == ~0U; }

        // to_int64_nanoseconds()
        // to_int64_microseconds()
        // to_int64_milliseconds()
        // to_int64_seconds()
        // to_int64_minutes()
        // to_int64_hours()
        //
        // Helper functions that convert a duration to an integral count of the
        // indicated unit. These functions are shorthand for the `integer_div_duration()`
        // function above; see its documentation for details about overflow, etc.
        //
        // Example:
        //
        //   flare::duration d = flare::milliseconds(1500);
        //   int64_t isec = d.to_int64_seconds(d);  // isec == 1

        int64_t to_int64_nanoseconds() const;

        int64_t to_int64_microseconds() const;

        int64_t to_int64_milliseconds() const;

        int64_t to_int64_seconds() const;

        int64_t to_int64_minutes() const;

        int64_t to_int64_hours() const;

        //
        // Helper functions that convert a duration to a floating point count of the
        // indicated unit. These functions are shorthand for the `float_div_duration()`
        // function above; see its documentation for details about overflow, etc.
        //
        // Example:
        //
        //   flare::duration d = flare::milliseconds(1500);
        //   double dsec = flare::ToDoubleSeconds(d);  // dsec == 1.5
        double to_double_nanoseconds() const;

        double to_double_microseconds() const;

        double to_double_milliseconds() const;

        double to_double_seconds() const;

        double to_double_minutes() const;

        double to_double_hours() const;

        // to_chrono_nanoseconds()
        // to_chrono_microseconds()
        // to_chrono_milliseconds()
        // to_chrono_seconds()
        // to_chrono_minutes()
        // to_chrono_hours()
        //
        // Converts an flare::duration to any of the pre-defined std::chrono durations.
        // If overflow would occur, the returned value will saturate at the min/max
        // chrono duration value instead.
        //
        // Example:
        //
        //   flare::duration d = flare::microseconds(123);
        //   auto x = d.to_chrono_microseconds();
        //   auto y = d.to_chrono_nanoseconds();  // x == y
        //   auto z = flare::infinite_duration().to_chrono_seconds();
        //   // z == std::chrono::seconds::max()
        std::chrono::nanoseconds to_chrono_nanoseconds() const;

        std::chrono::microseconds to_chrono_microseconds() const;

        std::chrono::milliseconds to_chrono_milliseconds() const;

        std::chrono::seconds to_chrono_seconds() const;

        std::chrono::minutes to_chrono_minutes() const;

        std::chrono::hours to_chrono_hours() const;

        // format_duration()
        //
        // Returns a string representing the duration in the form "72h3m0.5s".
        // Returns "inf" or "-inf" for +/- `infinite_duration()`.
        std::string format_duration() const;

        // float_div_duration()
        //
        // Divides a `duration` numerator into a fractional number of units of a
        // `duration` denominator.
        //
        // See also the notes on `infinite_duration()` below regarding the behavior of
        // division involving zero and infinite durations.
        //
        // Example:
        //
        //   double d = flare::float_div_duration(flare::milliseconds(1500), flare::seconds(1));
        //   // d == 1.5
        double float_div_duration(duration den) const;

        // trunc()
        //
        // Truncates a duration (toward zero) to a multiple of a non-zero unit.
        //
        // Example:
        //
        //   flare::duration d = flare::nanoseconds(123456789);
        //   flare::duration a = flare::trunc(flare::microseconds(1));  // 123456us
        duration trunc(duration unit) const;

        // floor()
        //
        // Floors a duration using the passed duration unit to its largest value not
        // greater than the duration.
        //
        // Example:
        //
        //   flare::duration d = flare::nanoseconds(123456789);
        //   flare::duration b = flare::floor(flare::microseconds(1));  // 123456us
        duration floor(duration unit) const;

        // ceil()
        //
        // Returns the ceiling of a duration using the passed duration unit to its
        // smallest value not less than the duration.
        //
        // Example:
        //
        //   flare::duration d = flare::nanoseconds(123456789);
        //   flare::duration c = flare::ceil(flare::microseconds(1));   // 123457us
        duration ceil(duration unit) const;

        // to_timespec()
        // to_timeval()
        timespec to_timespec() const;

        timeval to_timeval() const;

    public:

        // nanoseconds()
        // microseconds()
        // milliseconds()
        // seconds()
        // minutes()
        // hours()
        //
        // Factory functions for constructing `duration` values from an integral number
        // of the unit indicated by the factory function's name. The number must be
        // representable as int64_t.
        //
        // NOTE: no "Days()" factory function exists because "a day" is ambiguous.
        // Civil days are not always 24 hours long, and a 24-hour duration often does
        // not correspond with a civil day. If a 24-hour duration is needed, use
        // `flare::duration::hours(24)`. If you actually want a civil day, use flare::chrono_day
        // from civil_time.h.
        //
        // Example:
        //
        //   flare::duration a = flare::duration::seconds(60);
        //   flare::duration b = flare::duration::minutes(1);  // b == a

        static constexpr duration nanoseconds(int64_t n);

        static constexpr duration microseconds(int64_t n);

        static constexpr duration milliseconds(int64_t n);

        static constexpr duration seconds(int64_t n);

        static constexpr duration minutes(int64_t n);

        static constexpr duration hours(int64_t n);

        // Factory overloads for constructing `duration` values from a floating-point
        // number of the unit indicated by the factory function's name. These functions
        // exist for convenience, but they are not as efficient as the integral
        // factories, which should be preferred.
        //
        // Example:
        //
        //   auto a = flare::seconds(1.5);        // OK
        //   auto b = flare::milliseconds(1500);  // BETTER
        template<typename T, times_internal::enable_if_float<T> = 0>
        static duration nanoseconds(T n) {
            return n * nanoseconds(1);
        }

        template<typename T, times_internal::enable_if_float<T> = 0>
        static duration microseconds(T n) {
            return n * microseconds(1);
        }

        template<typename T, times_internal::enable_if_float<T> = 0>
        static duration milliseconds(T n) {
            return n * milliseconds(1);
        }

        template<typename T, times_internal::enable_if_float<T> = 0>
        static duration seconds(T n) {
            if (n >= 0) {  // Note: `NaN >= 0` is false.
                if (n >= static_cast<T>((std::numeric_limits<int64_t>::max)())) {
                    return infinite_future();
                }
                return make_pos_double_duration(n);
            } else {
                if (std::isnan(n))
                    return std::signbit(n) ? -infinite_future() : infinite_future();
                if (n <= (std::numeric_limits<int64_t>::min)())
                    return -infinite_future();
                return -make_pos_double_duration(-n);
            }
        }

        template<typename T, times_internal::enable_if_float<T> = 0>
        static duration minutes(T n) {
            return n * minutes(1);
        }

        template<typename T, times_internal::enable_if_float<T> = 0>
        static duration hours(T n) {
            return n * hours(1);
        }

        // from_chrono()
        //
        // Converts any of the pre-defined std::chrono durations to an flare::duration.
        //
        // Example:
        //
        //   std::chrono::milliseconds ms(123);
        //   flare::duration d = flare::from_chrono(ms);
        static constexpr duration from_chrono(const std::chrono::nanoseconds &d);

        static constexpr duration from_chrono(const std::chrono::microseconds &d);

        static constexpr duration from_chrono(const std::chrono::milliseconds &d);

        static constexpr duration from_chrono(const std::chrono::seconds &d);

        static constexpr duration from_chrono(const std::chrono::minutes &d);

        static constexpr duration from_chrono(const std::chrono::hours &d);

        // from_timespec()
        // from_timeval()
        static duration from_timespec(timespec ts);

        static duration from_timeval(timeval tv);

        static int64_t integer_div_duration(duration num, duration den, duration *rem);


    public:

        // Returns an infinite duration with the opposite sign.
        // REQUIRES: is_infinite_duration(d)
        static constexpr duration opposite_infinity(duration d);

        static constexpr duration universal_duration();

        template<std::intmax_t N>
        static constexpr duration from_int64(int64_t v, std::ratio<1, N>);

        static constexpr duration infinite_future();

        static constexpr duration infinite_pass();

        static constexpr duration from_int64(int64_t v, std::ratio<60>);

        static constexpr duration from_int64(int64_t v, std::ratio<3600>);

        // Converts a std::chrono::duration to an flare::duration.
        template<typename Rep, typename Period>
        static constexpr duration from_chrono_internal(const std::chrono::duration<Rep, Period> &d);


    public:

        // Converts an flare::duration to a chrono duration of type T.
        template<typename T>
        T to_chrono_duration() const;

        // Provide access to the duration representation.
        static constexpr int64_t get_rep_hi(duration d) { return d._rep_hi; }

        static constexpr uint32_t get_rep_lo(duration d) { return d._rep_lo; }

        //note that these interface use by flare internal
        // Creates a duration with a given representation.
        // REQUIRES: hi,lo is a valid representation of a duration as specified
        // in time/duration.cc.
        static constexpr duration make_duration(int64_t hi, uint32_t lo = 0);

        static constexpr duration make_duration(int64_t hi, int64_t lo);

    private:

        // Make a duration value from a floating-point number, as long as that number
        // is in the range [ 0 .. numeric_limits<int64_t>::max ), that is, as long as
        // it's positive and can be converted to int64_t without risk of UB.
        static inline duration make_pos_double_duration(double n);

        // Creates a normalized duration from an almost-normalized (sec,ticks)
        // pair. sec may be positive or negative.  ticks must be in the range
        // -kTicksPerSecond < *ticks < kTicksPerSecond.  If ticks is negative it
        // will be normalized to a positive value in the resulting duration.
        static constexpr duration make_normalized_duration(int64_t sec, int64_t ticks);


        static int64_t integer_div_duration(bool satq, const duration num, const duration den,
                                            duration *rem);

        static duration make_duration_from_uint128(uint128 u128, bool is_neg);

    private:
        constexpr duration(int64_t hi, uint32_t lo) : _rep_hi(hi), _rep_lo(lo) {}

        friend constexpr duration operator-(duration d);

        [[nodiscard]] uint128 make_uint128_ticks() const;

        template<template<typename> class Operation>
        [[nodiscard]] duration scale_fixed(int64_t r);

        template<template<typename> class Operation>
        [[nodiscard]] duration scale_double(double r) const;

        bool safe_add_rep_hi(double a_hi, double b_hi);

        bool i_div_fast_path(const duration den, int64_t *q,
                             duration *rem) const;

        template<typename Ratio>
        int64_t to_int64(Ratio) const;

        // Fastpath implementations for the 6 common duration units.
        [[nodiscard]] int64_t to_int64(std::nano) const;

        [[nodiscard]] int64_t to_int64(std::micro) const;

        [[nodiscard]] int64_t to_int64(std::milli) const;

        [[nodiscard]] int64_t to_int64(std::ratio<1>) const;

        [[nodiscard]] int64_t to_int64(std::ratio<60>) const;

        [[nodiscard]] int64_t to_int64(std::ratio<3600>) const;

    private:
        int64_t _rep_hi{0};
        uint32_t _rep_lo{0};
        int32_t  _padding{0};
    };

    namespace times_internal {
        FLARE_FORCE_INLINE bool is_valid_divisor(double d) {
            if (std::isnan(d)) return false;
            return d != 0.0;
        }

        // *sec may be positive or negative.  *ticks must be in the range
        // -kTicksPerSecond < *ticks < kTicksPerSecond.  If *ticks is negative it
        // will be normalized to a positive value by adjusting *sec accordingly.
        FLARE_FORCE_INLINE void normalize_ticks(int64_t *sec, int64_t *ticks) {
            if (*ticks < 0) {
                --*sec;
                *ticks += kTicksPerSecond;
            }
        }

        // Convert between int64_t and uint64_t, preserving representation. This
        // allows us to do arithmetic in the unsigned domain, where overflow has
        // well-defined behavior. See operator+=() and operator-=().
        //
        // C99 7.20.1.1.1, as referenced by C++11 18.4.1.2, says, "The typedef
        // name intN_t designates a signed integer type with width N, no padding
        // bits, and a two's complement representation." So, we can convert to
        // and from the corresponding uint64_t value using a bit cast.
        FLARE_FORCE_INLINE uint64_t encode_twos_comp(int64_t v) {
            return flare::base::bit_cast<uint64_t>(v);
        }

        FLARE_FORCE_INLINE int64_t decode_twos_comp(uint64_t v) { return flare::base::bit_cast<int64_t>(v); }

    }

    // Relational Operators
    constexpr bool operator<(duration lhs, duration rhs);

    constexpr bool operator>(duration lhs, duration rhs) { return rhs < lhs; }

    constexpr bool operator>=(duration lhs, duration rhs) { return !(lhs < rhs); }

    constexpr bool operator<=(duration lhs, duration rhs) { return !(rhs < lhs); }

    constexpr bool operator==(duration lhs, duration rhs);

    constexpr bool operator!=(duration lhs, duration rhs) { return !(lhs == rhs); }

    FLARE_FORCE_INLINE duration operator+(duration lhs, duration rhs) { return lhs += rhs; }

    FLARE_FORCE_INLINE duration operator-(duration lhs, duration rhs) { return lhs -= rhs; }


    // Multiplicative Operators
    // Integer operands must be representable as int64_t.
    template<typename T>
    duration operator*(duration lhs, T rhs) {
        return lhs *= rhs;
    }

    template<typename T>
    duration operator*(T lhs, duration rhs) {
        return rhs *= lhs;
    }

    template<typename T>
    duration operator/(duration lhs, T rhs) {
        return lhs /= rhs;
    }

    FLARE_FORCE_INLINE int64_t operator/(duration lhs, duration rhs) {
        return duration::integer_div_duration(lhs, rhs,
                                              &lhs);  // trunc towards zero
    }

    FLARE_FORCE_INLINE duration operator%(duration lhs, duration rhs) { return lhs %= rhs; }

    constexpr bool operator<(duration lhs, duration rhs) {
        return duration::get_rep_hi(lhs) != duration::get_rep_hi(rhs)
               ? duration::get_rep_hi(lhs) < duration::get_rep_hi(rhs)
               : duration::get_rep_hi(lhs) ==
                 (std::numeric_limits<int64_t>::min)()
                 ? duration::get_rep_lo(lhs) + 1 <
                   duration::get_rep_lo(rhs) + 1
                 : duration::get_rep_lo(lhs) <
                   duration::get_rep_lo(rhs);
    }

    constexpr bool operator==(duration lhs, duration rhs) {
        return duration::get_rep_hi(lhs) == duration::get_rep_hi(rhs) &&
               duration::get_rep_lo(lhs) == duration::get_rep_lo(rhs);
    }

    // integer_div_duration()
    //
    // Divides a numerator `duration` by a denominator `duration`, returning the
    // quotient and remainder. The remainder always has the same sign as the
    // numerator. The returned quotient and remainder respect the identity:
    //
    //   numerator = denominator * quotient + remainder
    //
    // Returned quotients are capped to the range of `int64_t`, with the difference
    // spilling into the remainder to uphold the above identity. This means that the
    // remainder returned could differ from the remainder returned by
    // `duration::operator%` for huge quotients.
    //
    // See also the notes on `infinite_duration()` below regarding the behavior of
    // division involving zero and infinite durations.
    //
    // Example:
    //
    //   constexpr flare::duration a =
    //       flare::seconds(std::numeric_limits<int64_t>::max());  // big
    //   constexpr flare::duration b = flare::nanoseconds(1);       // small
    //
    //   flare::duration rem = a % b;
    //   // rem == flare::zero_duration()
    //
    //   // Here, q would overflow int64_t, so rem accounts for the difference.
    //   int64_t q = flare::integer_div_duration(a, b, &rem);
    //   // q == std::numeric_limits<int64_t>::max(), rem == a - b * q
    FLARE_FORCE_INLINE int64_t duration::integer_div_duration(duration num, duration den, duration *rem) {
        return duration::integer_div_duration(true, num, den,
                                              rem);  // trunc towards zero
    }


    // zero_duration()
    //
    // Returns a zero-length duration. This function behaves just like the default
    // constructor, but the name helps make the semantics clear at call sites.
    constexpr duration zero_duration() { return duration(); }

    // abs_duration()
    //
    // Returns the absolute value of a duration.
    FLARE_FORCE_INLINE duration abs_duration(duration d) {
        return (d < zero_duration()) ? -d : d;
    }


    // infinite_duration()
    //
    // Returns an infinite `duration`.  To get a `duration` representing negative
    // infinity, use `-infinite_duration()`.
    //
    // duration arithmetic overflows to +/- infinity and saturates. In general,
    // arithmetic with `duration` infinities is similar to IEEE 754 infinities
    // except where IEEE 754 NaN would be involved, in which case +/-
    // `infinite_duration()` is used in place of a "nan" duration.
    //
    // Examples:
    //
    //   constexpr flare::duration inf = flare::infinite_duration();
    //   const flare::duration d = ... any finite duration ...
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
    //   INT64_MAX == d / flare::zero_duration()
    //
    // The examples involving the `/` operator above also apply to `integer_div_duration()`
    // and `float_div_duration()`.
    constexpr duration infinite_duration() {
        return duration::infinite_future();
    }


    // Output stream operator.
    FLARE_FORCE_INLINE std::ostream &operator<<(std::ostream &os, duration d) {
        return os << d.format_duration();
    }

    // parse_duration()
    //
    // Parses a duration string consisting of a possibly signed sequence of
    // decimal numbers, each with an optional fractional part and a unit
    // suffix.  The valid suffixes are "ns", "us" "ms", "s", "m", and "h".
    // Simple examples include "300ms", "-1.5h", and "2h45m".  Parses "0" as
    // `zero_duration()`. Parses "inf" and "-inf" as +/- `infinite_duration()`.
    bool parse_duration(const std::string &dur_string, duration *d);

    constexpr duration operator-(duration d) {
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
        return duration::get_rep_lo(d) == 0
               ? duration::get_rep_hi(d) ==
                 (std::numeric_limits<int64_t>::min)()
                 ? infinite_duration()
                 : duration::make_duration(-duration::get_rep_hi(d))
               : d.is_infinite_duration()
                 ? duration::opposite_infinity(d)
                 : duration::make_duration(
                                times_internal::negate_and_subtract_one(
                                        duration::get_rep_hi(d)),
                                times_internal::kTicksPerSecond -
                                duration::get_rep_lo(d));
    }


    // private static member functions
    constexpr duration duration::make_duration(int64_t hi, uint32_t lo) {
        return duration(hi, lo);
    }

    constexpr duration duration::make_duration(int64_t hi, int64_t lo) {
        return make_duration(hi, static_cast<uint32_t>(lo));
    }

    inline duration duration::make_pos_double_duration(double n) {
        const int64_t int_secs = static_cast<int64_t>(n);
        const uint32_t ticks =
                static_cast<uint32_t>((n - int_secs) * times_internal::kTicksPerSecond + 0.5);
        return ticks < times_internal::kTicksPerSecond
               ? make_duration(int_secs, ticks)
               : make_duration(int_secs + 1, ticks - times_internal::kTicksPerSecond);
    }


    constexpr duration duration::make_normalized_duration(int64_t sec, int64_t ticks) {
        return (ticks < 0) ? make_duration(sec - 1, ticks + times_internal::kTicksPerSecond)
                           : make_duration(sec, ticks);
    }

// Makes a uint128 count of ticks out of the absolute value of the duration.
    FLARE_FORCE_INLINE uint128 duration::make_uint128_ticks() const {
        int64_t rep_hi = _rep_hi;
        uint32_t rep_lo = _rep_lo;
        if (rep_hi < 0) {
            ++rep_hi;
            rep_hi = -rep_hi;
            rep_lo = times_internal::kTicksPerSecond - rep_lo;
        }
        uint128 u128 = static_cast<uint64_t>(rep_hi);
        u128 *= static_cast<uint64_t>(times_internal::kTicksPerSecond);
        u128 += rep_lo;
        return u128;
    }

// Breaks a uint128 of ticks into a duration.
    FLARE_FORCE_INLINE duration duration::make_duration_from_uint128(uint128 u128, bool is_neg) {
        int64_t rep_hi;
        uint32_t rep_lo;
        const uint64_t h64 = uint128_high64(u128);
        const uint64_t l64 = uint128_low64(u128);
        if (h64 == 0) {  // fastpath
            const uint64_t hi = l64 / times_internal::kTicksPerSecond;
            rep_hi = static_cast<int64_t>(hi);
            rep_lo = static_cast<uint32_t>(l64 - hi * times_internal::kTicksPerSecond);
        } else {
            // kMaxRepHi64 is the high 64 bits of (2^63 * kTicksPerSecond).
            // Any positive tick count whose high 64 bits are >= kMaxRepHi64
            // is not representable as a duration.  A negative tick count can
            // have its high 64 bits == kMaxRepHi64 but only when the low 64
            // bits are all zero, otherwise it is not representable either.
            const uint64_t kMaxRepHi64 = 0x77359400UL;
            if (h64 >= kMaxRepHi64) {
                if (is_neg && h64 == kMaxRepHi64 && l64 == 0) {
                    // Avoid trying to represent -kint64min below.
                    return make_duration(std::numeric_limits<int64_t>::min());
                }
                return is_neg ? -infinite_duration() : infinite_duration();
            }
            const uint128 kTicksPerSecond128 = static_cast<uint64_t>(times_internal::kTicksPerSecond);
            const uint128 hi = u128 / kTicksPerSecond128;
            rep_hi = static_cast<int64_t>(uint128_low64(hi));
            rep_lo =
                    static_cast<uint32_t>(uint128_low64(u128 - hi * kTicksPerSecond128));
        }
        if (is_neg) {
            rep_hi = -rep_hi;
            if (rep_lo != 0) {
                --rep_hi;
                rep_lo = times_internal::kTicksPerSecond - rep_lo;
            }
        }
        return make_duration(rep_hi, rep_lo);
    }

    constexpr duration duration::opposite_infinity(duration d) {
        return get_rep_hi(d) < 0
               ? make_duration((std::numeric_limits<int64_t>::max)(), ~0U)
               : make_duration((std::numeric_limits<int64_t>::min)(), ~0U);
    }

    constexpr duration duration::universal_duration() {
        return make_duration(-24 * 719162 * int64_t{3600}, 0U);
    }

    template<std::intmax_t N>
    constexpr duration duration::from_int64(int64_t v, std::ratio<1, N>) {
        static_assert(0 < N && N <= 1000 * 1000 * 1000, "Unsupported ratio");
        // Subsecond ratios cannot overflow.
        return make_normalized_duration(
                v / N, v % N * times_internal::kTicksPerNanosecond * 1000 * 1000 * 1000 / N);
    }

    constexpr duration duration::infinite_future() {
        return make_duration((std::numeric_limits<int64_t>::max)(), ~0U);
    }

    constexpr duration duration::infinite_pass() {
        return make_duration((std::numeric_limits<int64_t>::min)(), ~0U);
    }

    constexpr duration duration::from_int64(int64_t v, std::ratio<60>) {
        return (v <= (std::numeric_limits<int64_t>::max)() / 60 &&
                v >= (std::numeric_limits<int64_t>::min)() / 60)
               ? make_duration(v * 60)
               : v > 0 ? infinite_duration() : -infinite_duration();
    }

    constexpr duration duration::from_int64(int64_t v, std::ratio<3600>) {
        return (v <= (std::numeric_limits<int64_t>::max)() / 3600 &&
                v >= (std::numeric_limits<int64_t>::min)() / 3600)
               ? make_duration(v * 3600)
               : v > 0 ? infinite_duration() : -infinite_duration();
    }

// Converts a std::chrono::duration to an flare::duration.
    template<typename Rep, typename Period>
    constexpr duration duration::from_chrono_internal(const std::chrono::duration<Rep, Period> &d) {
        static_assert(times_internal::is_valid_rep64<Rep>(0), "duration::rep is invalid");
        return from_int64(int64_t{d.count()}, Period{});
    }


    constexpr duration duration::from_chrono(const std::chrono::nanoseconds &d) {
        return from_chrono_internal(d);
    }

    constexpr duration duration::from_chrono(const std::chrono::microseconds &d) {
        return from_chrono_internal(d);
    }

    constexpr duration duration::from_chrono(const std::chrono::milliseconds &d) {
        return from_chrono_internal(d);
    }

    constexpr duration duration::from_chrono(const std::chrono::seconds &d) {
        return from_chrono_internal(d);
    }

    constexpr duration duration::from_chrono(const std::chrono::minutes &d) {
        return from_chrono_internal(d);
    }

    constexpr duration duration::from_chrono(const std::chrono::hours &d) {
        return from_chrono_internal(d);
    }


    // Note: The overflow detection in this function is done using greater/less *or
    // equal* because kint64max/min is too large to be represented exactly in a
    // double (which only has 53 bits of precision). In order to avoid assigning to
    // rep->hi a double value that is too large for an int64_t (and therefore is
    // undefined), we must consider computations that equal kint64max/min as a
    // double as overflow cases.
    FLARE_FORCE_INLINE bool duration::safe_add_rep_hi(double a_hi, double b_hi) {
        double c = a_hi + b_hi;
        if (c >= static_cast<double>(times_internal::kint64max)) {
            *this = infinite_duration();
            return false;
        }
        if (c <= static_cast<double>(times_internal::kint64min)) {
            *this = -infinite_duration();
            return false;
        }
        *this = make_duration(c, _rep_lo);
        return true;
    }

    // Scales (i.e., multiplies or divides, depending on the Operation template)
    // the duration d by the int64_t r.
    template<template<typename> class Operation>
    FLARE_FORCE_INLINE duration duration::scale_fixed(int64_t r) {
        const uint128 a = make_uint128_ticks();
        const uint128 b = make_uint128(r);
        const uint128 q = Operation<uint128>()(a, b);
        const bool is_neg = (_rep_hi < 0) != (r < 0);
        return make_duration_from_uint128(q, is_neg);
    }

    // Scales (i.e., multiplies or divides, depending on the Operation template)
    // the duration d by the double r.
    template<template<typename> class Operation>
    FLARE_FORCE_INLINE duration duration::scale_double(double r) const {
        Operation<double> op;
        double hi_doub = op(_rep_hi, r);
        double lo_doub = op(_rep_lo, r);

        double hi_int = 0;
        double hi_frac = std::modf(hi_doub, &hi_int);

        // Moves hi's fractional bits to lo.
        lo_doub /= times_internal::kTicksPerSecond;
        lo_doub += hi_frac;

        double lo_int = 0;
        double lo_frac = std::modf(lo_doub, &lo_int);
        // Rolls lo into hi if necessary.
        int64_t lo64 = flare::base::round(lo_frac * times_internal::kTicksPerSecond);
        duration ans;
        if (!ans.safe_add_rep_hi(hi_int, lo_int)) {
            return ans;
        }
        int64_t hi64 = ans._rep_hi;
        if (!ans.safe_add_rep_hi(hi64, lo64 / times_internal::kTicksPerSecond)) {
            return ans;
        }
        hi64 = ans._rep_hi;
        lo64 %= times_internal::kTicksPerSecond;
        times_internal::normalize_ticks(&hi64, &lo64);
        return make_duration(hi64, lo64);
    }

    // Tries to divide num by den as fast as possible by looking for common, easy
    // cases. If the division was done, the quotient is in *q and the remainder is
    // in *rem and true will be returned.
    FLARE_FORCE_INLINE bool duration::i_div_fast_path(const duration den, int64_t *q,
                                                      duration *rem) const {
        // Bail if num or den is an infinity.
        if (is_infinite_duration() ||
            den.is_infinite_duration())
            return false;

        int64_t num_hi = _rep_hi;
        uint32_t num_lo = _rep_lo;
        int64_t den_hi = den._rep_hi;
        uint32_t den_lo = den._rep_lo;

        if (den_hi == 0 && den_lo == times_internal::kTicksPerNanosecond) {
            // Dividing by 1ns
            if (num_hi >= 0 && num_hi < (times_internal::kint64max - times_internal::kTicksPerSecond) / 1000000000) {
                *q = num_hi * 1000000000 + num_lo / times_internal::kTicksPerNanosecond;
                *rem = make_duration(0, num_lo % den_lo);
                return true;
            }
        } else if (den_hi == 0 && den_lo == 100 * times_internal::kTicksPerNanosecond) {
            // Dividing by 100ns (common when converting to Universal time)
            if (num_hi >= 0 && num_hi < (times_internal::kint64max - times_internal::kTicksPerSecond) / 10000000) {
                *q = num_hi * 10000000 + num_lo / (100 * times_internal::kTicksPerNanosecond);
                *rem = make_duration(0, num_lo % den_lo);
                return true;
            }
        } else if (den_hi == 0 && den_lo == 1000 * times_internal::kTicksPerNanosecond) {
            // Dividing by 1us
            if (num_hi >= 0 && num_hi < (times_internal::kint64max - times_internal::kTicksPerSecond) / 1000000) {
                *q = num_hi * 1000000 + num_lo / (1000 * times_internal::kTicksPerNanosecond);
                *rem = make_duration(0, num_lo % den_lo);
                return true;
            }
        } else if (den_hi == 0 && den_lo == 1000000 * times_internal::kTicksPerNanosecond) {
            // Dividing by 1ms
            if (num_hi >= 0 && num_hi < (times_internal::kint64max - times_internal::kTicksPerSecond) / 1000) {
                *q = num_hi * 1000 + num_lo / (1000000 * times_internal::kTicksPerNanosecond);
                *rem = make_duration(0, num_lo % den_lo);
                return true;
            }
        } else if (den_hi > 0 && den_lo == 0) {
            // Dividing by positive multiple of 1s
            if (num_hi >= 0) {
                if (den_hi == 1) {
                    *q = num_hi;
                    *rem = make_duration(0, num_lo);
                    return true;
                }
                *q = num_hi / den_hi;
                *rem = make_duration(num_hi % den_hi, num_lo);
                return true;
            }
            if (num_lo != 0) {
                num_hi += 1;
            }
            int64_t quotient = num_hi / den_hi;
            int64_t rem_sec = num_hi % den_hi;
            if (rem_sec > 0) {
                rem_sec -= den_hi;
                quotient += 1;
            }
            if (num_lo != 0) {
                rem_sec -= 1;
            }
            *q = quotient;
            *rem = make_duration(rem_sec, num_lo);
            return true;
        }

        return false;
    }


    constexpr duration duration::nanoseconds(int64_t n) {
        return from_int64(n, std::nano{});
    }

    constexpr duration duration::microseconds(int64_t n) {
        return from_int64(n, std::micro{});
    }

    constexpr duration duration::milliseconds(int64_t n) {
        return from_int64(n, std::milli{});
    }

    constexpr duration duration::seconds(int64_t n) {
        return from_int64(n, std::ratio<1>{});
    }

    constexpr duration duration::minutes(int64_t n) {
        return from_int64(n, std::ratio<60>{});
    }

    constexpr duration duration::hours(int64_t n) {
        return from_int64(n, std::ratio<3600>{});
    }

    template<typename Ratio>
    int64_t to_int64(duration d, Ratio) {
        // Note: This may be used on MSVC, which may have a system_clock period of
        // std::ratio<1, 10 * 1000 * 1000>
        return to_int64_seconds(d * Ratio::den / Ratio::num);
    }
// Fastpath implementations for the 6 common duration units.
    FLARE_FORCE_INLINE int64_t duration::to_int64(std::nano) const {
        return to_int64_nanoseconds();
    }

    FLARE_FORCE_INLINE int64_t duration::to_int64(std::micro) const {
        return to_int64_microseconds();
    }

    FLARE_FORCE_INLINE int64_t duration::to_int64(std::milli) const {
        return to_int64_milliseconds();
    }

    FLARE_FORCE_INLINE int64_t duration::to_int64(std::ratio<1>) const {
        return to_int64_seconds();
    }

    FLARE_FORCE_INLINE int64_t duration::to_int64(std::ratio<60>) const {
        return to_int64_minutes();
    }

    FLARE_FORCE_INLINE int64_t duration::to_int64(std::ratio<3600>) const {
        return to_int64_hours();
    }

// Converts an flare::duration to a chrono duration of type T.
    template<typename T>
    T duration::to_chrono_duration() const {
        using Rep = typename T::rep;
        using Period = typename T::period;
        static_assert(times_internal::is_valid_rep64<Rep>(0), "duration::rep is invalid");
        if (is_infinite_duration())
            return *this < zero_duration() ? (T::min)() : (T::max)();
        const auto v = to_int64(Period{});
        if (v > (std::numeric_limits<Rep>::max)())
            return (T::max)();
        if (v < (std::numeric_limits<Rep>::min)())
            return (T::min)();
        return T{v};
    }


}  // namespace flare

#endif  // FLARE_TIMES_DURATION_H_
