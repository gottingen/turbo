
/****************************************************************
 * Copyright (c) 2022, liyinbin
 * All rights reserved.
 * Author by liyinbin (jeff.li) lijippy@163.com
 *****************************************************************/

#ifndef FLARE_BASE_INT128_H_
#define FLARE_BASE_INT128_H_

#include <cassert>
#include <cmath>
#include <cstdint>
#include <cstring>
#include <iosfwd>
#include <limits>
#include <utility>
#include "flare/base/profile.h"

#if defined(_MSC_VER)
// In very old versions of MSVC and when the /Zc:wchar_t flag is off, wchar_t is
// a typedef for unsigned short.  Otherwise wchar_t is mapped to the __wchar_t
// builtin type.  We need to make sure not to define operator wchar_t()
// alongside operator unsigned short() in these instances.
#define FLARE_INTERNAL_WCHAR_T __wchar_t
#if defined(_M_X64)
#include <intrin.h>
#pragma intrinsic(_umul128)
#endif  // defined(_M_X64)
#else   // defined(_MSC_VER)
#define FLARE_INTERNAL_WCHAR_T wchar_t
#endif  // defined(_MSC_VER)

namespace flare {

    class int128;

    // uint128
    //
    // An unsigned 128-bit integer type. The API is meant to mimic an intrinsic type
    // as closely as is practical, including exhibiting undefined behavior in
    // analogous cases (e.g. division by zero). This type is intended to be a
    // drop-in replacement once C++ supports an intrinsic `uint128_t` type; when
    // that occurs, existing well-behaved uses of `uint128` will continue to work
    // using that new type.
    //
    // Note: code written with this type will continue to compile once `uint128_t`
    // is introduced, provided the replacement helper functions
    // `Uint128(Low|High)64()` and `make_uint128()` are made.
    //
    // A `uint128` supports the following:
    //
    //   * Implicit construction from integral types
    //   * Explicit conversion to integral types
    //
    // Additionally, if your compiler supports `__int128`, `uint128` is
    // interoperable with that type. (flare checks for this compatibility through
    // the `FLARE_HAVE_INTRINSIC_INT128` macro.)
    //
    // However, a `uint128` differs from intrinsic integral types in the following
    // ways:
    //
    //   * Errors on implicit conversions that do not preserve value (such as
    //     loss of precision when converting to float values).
    //   * Requires explicit construction from and conversion to floating point
    //     types.
    //   * Conversion to integral types requires an explicit static_cast() to
    //     mimic use of the `-Wnarrowing` compiler flag.
    //   * The alignment requirement of `uint128` may differ from that of an
    //     intrinsic 128-bit integer type depending on platform and build
    //     configuration.
    //
    // Example:
    //
    //     float y = flare::uint128_max();  // Error. uint128 cannot be implicitly
    //                                    // converted to float.
    //
    //     flare::uint128 v;
    //     uint64_t i = v;                         // Error
    //     uint64_t i = static_cast<uint64_t>(v);  // OK
    //
    class
#if defined(FLARE_HAVE_INTRINSIC_INT128)
    alignas(unsigned __int128)
#endif  // FLARE_HAVE_INTRINSIC_INT128
    uint128 {
    public:
        uint128() = default;

        // Constructors from arithmetic types
        constexpr uint128(int v);                 // NOLINT(runtime/explicit)
        constexpr uint128(unsigned int v);        // NOLINT(runtime/explicit)
        constexpr uint128(long v);                // NOLINT(runtime/int)
        constexpr uint128(unsigned long v);       // NOLINT(runtime/int)
        constexpr uint128(long long v);           // NOLINT(runtime/int)
        constexpr uint128(unsigned long long v);  // NOLINT(runtime/int)
#ifdef FLARE_HAVE_INTRINSIC_INT128

        constexpr uint128(__int128 v);           // NOLINT(runtime/explicit)
        constexpr uint128(unsigned __int128 v);  // NOLINT(runtime/explicit)
#endif  // FLARE_HAVE_INTRINSIC_INT128

        constexpr uint128(int128 v);  // NOLINT(runtime/explicit)
        explicit uint128(float v);

        explicit uint128(double v);

        explicit uint128(long double v);

        // Assignment operators from arithmetic types
        uint128 &operator=(int v);

        uint128 &operator=(unsigned int v);

        uint128 &operator=(long v);                // NOLINT(runtime/int)
        uint128 &operator=(unsigned long v);       // NOLINT(runtime/int)
        uint128 &operator=(long long v);           // NOLINT(runtime/int)
        uint128 &operator=(unsigned long long v);  // NOLINT(runtime/int)
#ifdef FLARE_HAVE_INTRINSIC_INT128

        uint128 &operator=(__int128 v);

        uint128 &operator=(unsigned __int128 v);

#endif  // FLARE_HAVE_INTRINSIC_INT128

        uint128 &operator=(int128 v);

        // Conversion operators to other arithmetic types
        constexpr explicit operator bool() const;

        constexpr explicit operator char() const;

        constexpr explicit operator signed char() const;

        constexpr explicit operator unsigned char() const;

        constexpr explicit operator char16_t() const;

        constexpr explicit operator char32_t() const;

        constexpr explicit operator FLARE_INTERNAL_WCHAR_T() const;

        constexpr explicit operator short() const;  // NOLINT(runtime/int)
        // NOLINTNEXTLINE(runtime/int)
        constexpr explicit operator unsigned short() const;

        constexpr explicit operator int() const;

        constexpr explicit operator unsigned int() const;

        constexpr explicit operator long() const;  // NOLINT(runtime/int)
        // NOLINTNEXTLINE(runtime/int)
        constexpr explicit operator unsigned long() const;

        // NOLINTNEXTLINE(runtime/int)
        constexpr explicit operator long long() const;

        // NOLINTNEXTLINE(runtime/int)
        constexpr explicit operator unsigned long long() const;

#ifdef FLARE_HAVE_INTRINSIC_INT128

        constexpr explicit operator __int128() const;

        constexpr explicit operator unsigned __int128() const;

#endif  // FLARE_HAVE_INTRINSIC_INT128

        explicit operator float() const;

        explicit operator double() const;

        explicit operator long double() const;

        // Trivial copy constructor, assignment operator and destructor.

        // Arithmetic operators.
        uint128 &operator+=(uint128 other);

        uint128 &operator-=(uint128 other);

        uint128 &operator*=(uint128 other);

        // Long division/modulo for uint128.
        uint128 &operator/=(uint128 other);

        uint128 &operator%=(uint128 other);

        uint128 operator++(int);

        uint128 operator--(int);

        uint128 &operator<<=(int);

        uint128 &operator>>=(int);

        uint128 &operator&=(uint128 other);

        uint128 &operator|=(uint128 other);

        uint128 &operator^=(uint128 other);

        uint128 &operator++();

        uint128 &operator--();

        // uint128_low64()
        //
        // Returns the lower 64-bit value of a `uint128` value.
        friend constexpr uint64_t uint128_low64(uint128 v);

        // uint128_high64()
        //
        // Returns the higher 64-bit value of a `uint128` value.
        friend constexpr uint64_t uint128_high64(uint128 v);

        // MakeUInt128()
        //
        // Constructs a `uint128` numeric value from two 64-bit unsigned integers.
        // Note that this factory function is the only way to construct a `uint128`
        // from integer values greater than 2^64.
        //
        // Example:
        //
        //   flare::uint128 big = flare::make_uint128(1, 0);
        friend constexpr uint128 make_uint128(uint64_t high, uint64_t low);

        // uint128_max()
        //
        // Returns the highest value for a 128-bit unsigned integer.
        friend constexpr uint128 uint128_max();

        // Support for flare::hash.
        template<typename H>
        friend H flare_hash_value(H h, uint128 v) {
            return H::combine(std::move(h), uint128_high64(v), uint128_low64(v));
        }

    private:
        constexpr uint128(uint64_t high, uint64_t low);

        // TODO(strel) Update implementation to use __int128 once all users of
        // uint128 are fixed to not depend on alignof(uint128) == 8. Also add
        // alignas(16) to class definition to keep alignment consistent across
        // platforms.
#if defined(FLARE_SYSTEM_LITTLE_ENDIAN)
        uint64_t lo_;
        uint64_t hi_;
#elif defined(FLARE_SYSTEM_BIG_ENDIAN)
        uint64_t hi_;
        uint64_t lo_;
#else  // byte order
#error "Unsupported byte order: must be little-endian or big-endian."
#endif  // byte order
    };

// Prefer to use the constexpr `uint128_max()`.
//
// TODO(flare-team) deprecate kuint128max once migration tool is released.
    extern const uint128 kuint128max;

// allow uint128 to be logged
    std::ostream &operator<<(std::ostream &os, uint128 v);

// TODO(strel) add operator>>(std::istream&, uint128)

    constexpr uint128 uint128_max() {
        return uint128((std::numeric_limits<uint64_t>::max)(),
                       (std::numeric_limits<uint64_t>::max)());
    }

}  // namespace flare::debugging

// Specialized numeric_limits for uint128.
namespace std {
    template<>
    class numeric_limits<flare::uint128> {
    public:
        static constexpr bool is_specialized = true;
        static constexpr bool is_signed = false;
        static constexpr bool is_integer = true;
        static constexpr bool is_exact = true;
        static constexpr bool has_infinity = false;
        static constexpr bool has_quiet_NaN = false;
        static constexpr bool has_signaling_NaN = false;
        static constexpr float_denorm_style has_denorm = denorm_absent;
        static constexpr bool has_denorm_loss = false;
        static constexpr float_round_style round_style = round_toward_zero;
        static constexpr bool is_iec559 = false;
        static constexpr bool is_bounded = true;
        static constexpr bool is_modulo = true;
        static constexpr int digits = 128;
        static constexpr int digits10 = 38;
        static constexpr int max_digits10 = 0;
        static constexpr int radix = 2;
        static constexpr int min_exponent = 0;
        static constexpr int min_exponent10 = 0;
        static constexpr int max_exponent = 0;
        static constexpr int max_exponent10 = 0;
#ifdef FLARE_HAVE_INTRINSIC_INT128
        static constexpr bool traps = numeric_limits<unsigned __int128>::traps;
#else   // FLARE_HAVE_INTRINSIC_INT128
        static constexpr bool traps = numeric_limits<uint64_t>::traps;
#endif  // FLARE_HAVE_INTRINSIC_INT128
        static constexpr bool tinyness_before = false;

        static constexpr flare::uint128 (min)() { return 0; }

        static constexpr flare::uint128 lowest() { return 0; }

        static constexpr flare::uint128 (max)() { return flare::uint128_max(); }

        static constexpr flare::uint128 epsilon() { return 0; }

        static constexpr flare::uint128 round_error() { return 0; }

        static constexpr flare::uint128 infinity() { return 0; }

        static constexpr flare::uint128 quiet_NaN() { return 0; }

        static constexpr flare::uint128 signaling_NaN() { return 0; }

        static constexpr flare::uint128 denorm_min() { return 0; }
    };
}  // namespace std

namespace flare {


    // int128
    //
    // A signed 128-bit integer type. The API is meant to mimic an intrinsic
    // integral type as closely as is practical, including exhibiting undefined
    // behavior in analogous cases (e.g. division by zero).
    //
    // An `int128` supports the following:
    //
    //   * Implicit construction from integral types
    //   * Explicit conversion to integral types
    //
    // However, an `int128` differs from intrinsic integral types in the following
    // ways:
    //
    //   * It is not implicitly convertible to other integral types.
    //   * Requires explicit construction from and conversion to floating point
    //     types.

    // Additionally, if your compiler supports `__int128`, `int128` is
    // interoperable with that type. (flare checks for this compatibility through
    // the `FLARE_HAVE_INTRINSIC_INT128` macro.)
    //
    // The design goal for `int128` is that it will be compatible with a future
    // `int128_t`, if that type becomes a part of the standard.
    //
    // Example:
    //
    //     float y = flare::int128(17);  // Error. int128 cannot be implicitly
    //                                  // converted to float.
    //
    //     flare::int128 v;
    //     int64_t i = v;                        // Error
    //     int64_t i = static_cast<int64_t>(v);  // OK
    //
    class int128 {
    public:
        int128() = default;

        // Constructors from arithmetic types
        constexpr int128(int v);                 // NOLINT(runtime/explicit)
        constexpr int128(unsigned int v);        // NOLINT(runtime/explicit)
        constexpr int128(long v);                // NOLINT(runtime/int)
        constexpr int128(unsigned long v);       // NOLINT(runtime/int)
        constexpr int128(long long v);           // NOLINT(runtime/int)
        constexpr int128(unsigned long long v);  // NOLINT(runtime/int)
#ifdef FLARE_HAVE_INTRINSIC_INT128

        constexpr int128(__int128 v);  // NOLINT(runtime/explicit)
        constexpr explicit int128(unsigned __int128 v);

#endif  // FLARE_HAVE_INTRINSIC_INT128

        constexpr explicit int128(uint128 v);

        explicit int128(float v);

        explicit int128(double v);

        explicit int128(long double v);

        // Assignment operators from arithmetic types
        int128 &operator=(int v);

        int128 &operator=(unsigned int v);

        int128 &operator=(long v);                // NOLINT(runtime/int)
        int128 &operator=(unsigned long v);       // NOLINT(runtime/int)
        int128 &operator=(long long v);           // NOLINT(runtime/int)
        int128 &operator=(unsigned long long v);  // NOLINT(runtime/int)
#ifdef FLARE_HAVE_INTRINSIC_INT128

        int128 &operator=(__int128 v);

#endif  // FLARE_HAVE_INTRINSIC_INT128

        // Conversion operators to other arithmetic types
        constexpr explicit operator bool() const;

        constexpr explicit operator char() const;

        constexpr explicit operator signed char() const;

        constexpr explicit operator unsigned char() const;

        constexpr explicit operator char16_t() const;

        constexpr explicit operator char32_t() const;

        constexpr explicit operator FLARE_INTERNAL_WCHAR_T() const;

        constexpr explicit operator short() const;  // NOLINT(runtime/int)
        // NOLINTNEXTLINE(runtime/int)
        constexpr explicit operator unsigned short() const;

        constexpr explicit operator int() const;

        constexpr explicit operator unsigned int() const;

        constexpr explicit operator long() const;  // NOLINT(runtime/int)
        // NOLINTNEXTLINE(runtime/int)
        constexpr explicit operator unsigned long() const;

        // NOLINTNEXTLINE(runtime/int)
        constexpr explicit operator long long() const;

        // NOLINTNEXTLINE(runtime/int)
        constexpr explicit operator unsigned long long() const;

#ifdef FLARE_HAVE_INTRINSIC_INT128

        constexpr explicit operator __int128() const;

        constexpr explicit operator unsigned __int128() const;

#endif  // FLARE_HAVE_INTRINSIC_INT128

        explicit operator float() const;

        explicit operator double() const;

        explicit operator long double() const;

        // Trivial copy constructor, assignment operator and destructor.

        // Arithmetic operators
        int128 &operator+=(int128 other);

        int128 &operator-=(int128 other);

        int128 &operator*=(int128 other);

        int128 &operator/=(int128 other);

        int128 &operator%=(int128 other);

        int128 operator++(int);  // postfix increment: i++
        int128 operator--(int);  // postfix decrement: i--
        int128 &operator++();    // prefix increment:  ++i
        int128 &operator--();    // prefix decrement:  --i
        int128 &operator&=(int128 other);

        int128 &operator|=(int128 other);

        int128 &operator^=(int128 other);

        int128 &operator<<=(int amount);

        int128 &operator>>=(int amount);

        // int128_low64()
        //
        // Returns the lower 64-bit value of a `int128` value.
        friend constexpr uint64_t int128_low64(int128 v);

        // int128_high64()
        //
        // Returns the higher 64-bit value of a `int128` value.
        friend constexpr int64_t int128_high64(int128 v);

        // make_int128()
        //
        // Constructs a `int128` numeric value from two 64-bit integers. Note that
        // signedness is conveyed in the upper `high` value.
        //
        //   (flare::int128(1) << 64) * high + low
        //
        // Note that this factory function is the only way to construct a `int128`
        // from integer values greater than 2^64 or less than -2^64.
        //
        // Example:
        //
        //   flare::int128 big = flare::make_int128(1, 0);
        //   flare::int128 big_n = flare::make_int128(-1, 0);
        friend constexpr int128 make_int128(int64_t high, uint64_t low);

        // int128_max()
        //
        // Returns the maximum value for a 128-bit signed integer.
        friend constexpr int128 int128_max();

        // int128_min()
        //
        // Returns the minimum value for a 128-bit signed integer.
        friend constexpr int128 int128_min();

        // Support for flare::hash.
        template<typename H>
        friend H flare_hash_value(H h, int128 v) {
            return H::combine(std::move(h), int128_high64(v), int128_low64(v));
        }

    private:
        constexpr int128(int64_t high, uint64_t low);

#if defined(FLARE_HAVE_INTRINSIC_INT128)
        __int128 v_;
#else  // FLARE_HAVE_INTRINSIC_INT128
#if defined(FLARE_SYSTEM_LITTLE_ENDIAN)
        uint64_t lo_;
        int64_t hi_;
#elif defined(FLARE_SYSTEM_BIG_ENDIAN)
        int64_t hi_;
        uint64_t lo_;
#else  // byte order
#error "Unsupported byte order: must be little-endian or big-endian."
#endif  // byte order
#endif  // FLARE_HAVE_INTRINSIC_INT128
    };

    std::ostream &operator<<(std::ostream &os, int128 v);

// TODO(flare-team) add operator>>(std::istream&, int128)

    constexpr int128 int128_max() {
        return int128((std::numeric_limits<int64_t>::max)(),
                      (std::numeric_limits<uint64_t>::max)());
    }

    constexpr int128 int128_min() {
        return int128((std::numeric_limits<int64_t>::min)(), 0);
    }

}  // namespace flare

// Specialized numeric_limits for int128.
namespace std {
    template<>
    class numeric_limits<flare::int128> {
    public:
        static constexpr bool is_specialized = true;
        static constexpr bool is_signed = true;
        static constexpr bool is_integer = true;
        static constexpr bool is_exact = true;
        static constexpr bool has_infinity = false;
        static constexpr bool has_quiet_NaN = false;
        static constexpr bool has_signaling_NaN = false;
        static constexpr float_denorm_style has_denorm = denorm_absent;
        static constexpr bool has_denorm_loss = false;
        static constexpr float_round_style round_style = round_toward_zero;
        static constexpr bool is_iec559 = false;
        static constexpr bool is_bounded = true;
        static constexpr bool is_modulo = false;
        static constexpr int digits = 127;
        static constexpr int digits10 = 38;
        static constexpr int max_digits10 = 0;
        static constexpr int radix = 2;
        static constexpr int min_exponent = 0;
        static constexpr int min_exponent10 = 0;
        static constexpr int max_exponent = 0;
        static constexpr int max_exponent10 = 0;
#ifdef FLARE_HAVE_INTRINSIC_INT128
        static constexpr bool traps = numeric_limits<__int128>::traps;
#else   // FLARE_HAVE_INTRINSIC_INT128
        static constexpr bool traps = numeric_limits<uint64_t>::traps;
#endif  // FLARE_HAVE_INTRINSIC_INT128
        static constexpr bool tinyness_before = false;

        static constexpr flare::int128 (min)() { return flare::int128_min(); }

        static constexpr flare::int128 lowest() { return flare::int128_min(); }

        static constexpr flare::int128 (max)() { return flare::int128_max(); }

        static constexpr flare::int128 epsilon() { return 0; }

        static constexpr flare::int128 round_error() { return 0; }

        static constexpr flare::int128 infinity() { return 0; }

        static constexpr flare::int128 quiet_NaN() { return 0; }

        static constexpr flare::int128 signaling_NaN() { return 0; }

        static constexpr flare::int128 denorm_min() { return 0; }
    };
}  // namespace std

// --------------------------------------------------------------------------
//                      Implementation details follow
// --------------------------------------------------------------------------
namespace flare {

    constexpr uint128 make_uint128(uint64_t high, uint64_t low) {
        return uint128(high, low);
    }

    FLARE_FORCE_INLINE uint128 make_uint128(int64_t a) {
        uint128 u128 = 0;
        if (a < 0) {
            ++u128;
            ++a;  // Makes it safe to negate 'a'
            a = -a;
        }
        u128 += static_cast<uint64_t>(a);
        return u128;
    }

// Assignment from integer types.

    FLARE_FORCE_INLINE uint128 &uint128::operator=(int v) { return *this = uint128(v); }

    FLARE_FORCE_INLINE uint128 &uint128::operator=(unsigned int v) {
        return *this = uint128(v);
    }

    FLARE_FORCE_INLINE uint128 &uint128::operator=(long v) {  // NOLINT(runtime/int)
        return *this = uint128(v);
    }

// NOLINTNEXTLINE(runtime/int)
    FLARE_FORCE_INLINE uint128 &uint128::operator=(unsigned long v) {
        return *this = uint128(v);
    }

// NOLINTNEXTLINE(runtime/int)
    FLARE_FORCE_INLINE uint128 &uint128::operator=(long long v) {
        return *this = uint128(v);
    }

// NOLINTNEXTLINE(runtime/int)
    FLARE_FORCE_INLINE uint128 &uint128::operator=(unsigned long long v) {
        return *this = uint128(v);
    }

#ifdef FLARE_HAVE_INTRINSIC_INT128

    FLARE_FORCE_INLINE uint128 &uint128::operator=(__int128 v) {
        return *this = uint128(v);
    }

    FLARE_FORCE_INLINE uint128 &uint128::operator=(unsigned __int128 v) {
        return *this = uint128(v);
    }

#endif  // FLARE_HAVE_INTRINSIC_INT128

    FLARE_FORCE_INLINE uint128 &uint128::operator=(int128 v) {
        return *this = uint128(v);
    }

// Arithmetic operators.

    uint128 operator<<(uint128 lhs, int amount);

    uint128 operator>>(uint128 lhs, int amount);

    uint128 operator+(uint128 lhs, uint128 rhs);

    uint128 operator-(uint128 lhs, uint128 rhs);

    uint128 operator*(uint128 lhs, uint128 rhs);

    uint128 operator/(uint128 lhs, uint128 rhs);

    uint128 operator%(uint128 lhs, uint128 rhs);

    FLARE_FORCE_INLINE uint128 &uint128::operator<<=(int amount) {
        *this = *this << amount;
        return *this;
    }

    FLARE_FORCE_INLINE uint128 &uint128::operator>>=(int amount) {
        *this = *this >> amount;
        return *this;
    }

    FLARE_FORCE_INLINE uint128 &uint128::operator+=(uint128 other) {
        *this = *this + other;
        return *this;
    }

    FLARE_FORCE_INLINE uint128 &uint128::operator-=(uint128 other) {
        *this = *this - other;
        return *this;
    }

    FLARE_FORCE_INLINE uint128 &uint128::operator*=(uint128 other) {
        *this = *this * other;
        return *this;
    }

    FLARE_FORCE_INLINE uint128 &uint128::operator/=(uint128 other) {
        *this = *this / other;
        return *this;
    }

    FLARE_FORCE_INLINE uint128 &uint128::operator%=(uint128 other) {
        *this = *this % other;
        return *this;
    }

    constexpr uint64_t uint128_low64(uint128 v) { return v.lo_; }

    constexpr uint64_t uint128_high64(uint128 v) { return v.hi_; }

// Constructors from integer types.

#if defined(FLARE_SYSTEM_LITTLE_ENDIAN)

    constexpr uint128::uint128(uint64_t high, uint64_t low)
            : lo_{low}, hi_{high} {}

    constexpr uint128::uint128(int v)
            : lo_{static_cast<uint64_t>(v)},
              hi_{v < 0 ? (std::numeric_limits<uint64_t>::max)() : 0} {}

    constexpr uint128::uint128(long v)  // NOLINT(runtime/int)
            : lo_{static_cast<uint64_t>(v)},
              hi_{v < 0 ? (std::numeric_limits<uint64_t>::max)() : 0} {}

    constexpr uint128::uint128(long long v)  // NOLINT(runtime/int)
            : lo_{static_cast<uint64_t>(v)},
              hi_{v < 0 ? (std::numeric_limits<uint64_t>::max)() : 0} {}

    constexpr uint128::uint128(unsigned int v) : lo_{v}, hi_{0} {}

// NOLINTNEXTLINE(runtime/int)
    constexpr uint128::uint128(unsigned long v) : lo_{v}, hi_{0} {}

// NOLINTNEXTLINE(runtime/int)
    constexpr uint128::uint128(unsigned long long v) : lo_{v}, hi_{0} {}

#ifdef FLARE_HAVE_INTRINSIC_INT128

    constexpr uint128::uint128(__int128 v)
            : lo_{static_cast<uint64_t>(v & ~uint64_t{0})},
              hi_{static_cast<uint64_t>(static_cast<unsigned __int128>(v) >> 64)} {}

    constexpr uint128::uint128(unsigned __int128 v)
            : lo_{static_cast<uint64_t>(v & ~uint64_t{0})},
              hi_{static_cast<uint64_t>(v >> 64)} {}

#endif  // FLARE_HAVE_INTRINSIC_INT128

    constexpr uint128::uint128(int128 v)
            : lo_{int128_low64(v)}, hi_{static_cast<uint64_t>(int128_high64(v))} {}

#elif defined(FLARE_SYSTEM_BIG_ENDIAN)

    constexpr uint128::uint128(uint64_t high, uint64_t low)
        : hi_{high}, lo_{low} {}

    constexpr uint128::uint128(int v)
        : hi_{v < 0 ? (std::numeric_limits<uint64_t>::max)() : 0},
          lo_{static_cast<uint64_t>(v)} {}
    constexpr uint128::uint128(long v)  // NOLINT(runtime/int)
        : hi_{v < 0 ? (std::numeric_limits<uint64_t>::max)() : 0},
          lo_{static_cast<uint64_t>(v)} {}
    constexpr uint128::uint128(long long v)  // NOLINT(runtime/int)
        : hi_{v < 0 ? (std::numeric_limits<uint64_t>::max)() : 0},
          lo_{static_cast<uint64_t>(v)} {}

    constexpr uint128::uint128(unsigned int v) : hi_{0}, lo_{v} {}
    // NOLINTNEXTLINE(runtime/int)
    constexpr uint128::uint128(unsigned long v) : hi_{0}, lo_{v} {}
    // NOLINTNEXTLINE(runtime/int)
    constexpr uint128::uint128(unsigned long long v) : hi_{0}, lo_{v} {}

#ifdef FLARE_HAVE_INTRINSIC_INT128
    constexpr uint128::uint128(__int128 v)
        : hi_{static_cast<uint64_t>(static_cast<unsigned __int128>(v) >> 64)},
          lo_{static_cast<uint64_t>(v & ~uint64_t{0})} {}
    constexpr uint128::uint128(unsigned __int128 v)
        : hi_{static_cast<uint64_t>(v >> 64)},
          lo_{static_cast<uint64_t>(v & ~uint64_t{0})} {}
#endif  // FLARE_HAVE_INTRINSIC_INT128

    constexpr uint128::uint128(int128 v)
        : hi_{static_cast<uint64_t>(int128_high64(v))}, lo_{int128_low64(v)} {}

#else  // byte order
#error "Unsupported byte order: must be little-endian or big-endian."
#endif  // byte order

// Conversion operators to integer types.

    constexpr uint128::operator bool() const { return lo_ || hi_; }

    constexpr uint128::operator char() const { return static_cast<char>(lo_); }

    constexpr uint128::operator signed char() const {
        return static_cast<signed char>(lo_);
    }

    constexpr uint128::operator unsigned char() const {
        return static_cast<unsigned char>(lo_);
    }

    constexpr uint128::operator char16_t() const {
        return static_cast<char16_t>(lo_);
    }

    constexpr uint128::operator char32_t() const {
        return static_cast<char32_t>(lo_);
    }

    constexpr uint128::operator FLARE_INTERNAL_WCHAR_T() const {
        return static_cast<FLARE_INTERNAL_WCHAR_T>(lo_);
    }

// NOLINTNEXTLINE(runtime/int)
    constexpr uint128::operator short() const { return static_cast<short>(lo_); }

    constexpr uint128::operator unsigned short() const {  // NOLINT(runtime/int)
        return static_cast<unsigned short>(lo_);            // NOLINT(runtime/int)
    }

    constexpr uint128::operator int() const { return static_cast<int>(lo_); }

    constexpr uint128::operator unsigned int() const {
        return static_cast<unsigned int>(lo_);
    }

// NOLINTNEXTLINE(runtime/int)
    constexpr uint128::operator long() const { return static_cast<long>(lo_); }

    constexpr uint128::operator unsigned long() const {  // NOLINT(runtime/int)
        return static_cast<unsigned long>(lo_);            // NOLINT(runtime/int)
    }

    constexpr uint128::operator long long() const {  // NOLINT(runtime/int)
        return static_cast<long long>(lo_);            // NOLINT(runtime/int)
    }

    constexpr uint128::operator unsigned long long() const {  // NOLINT(runtime/int)
        return static_cast<unsigned long long>(lo_);            // NOLINT(runtime/int)
    }

#ifdef FLARE_HAVE_INTRINSIC_INT128

    constexpr uint128::operator __int128() const {
        return (static_cast<__int128>(hi_) << 64) + lo_;
    }

    constexpr uint128::operator unsigned __int128() const {
        return (static_cast<unsigned __int128>(hi_) << 64) + lo_;
    }

#endif  // FLARE_HAVE_INTRINSIC_INT128

// Conversion operators to floating point types.

    FLARE_FORCE_INLINE uint128::operator float() const {
        return static_cast<float>(lo_) + std::ldexp(static_cast<float>(hi_), 64);
    }

    FLARE_FORCE_INLINE uint128::operator double() const {
        return static_cast<double>(lo_) + std::ldexp(static_cast<double>(hi_), 64);
    }

    FLARE_FORCE_INLINE uint128::operator long double() const {
        return static_cast<long double>(lo_) +
               std::ldexp(static_cast<long double>(hi_), 64);
    }

// Comparison operators.

    FLARE_FORCE_INLINE bool operator==(uint128 lhs, uint128 rhs) {
        return (uint128_low64(lhs) == uint128_low64(rhs) &&
                uint128_high64(lhs) == uint128_high64(rhs));
    }

    FLARE_FORCE_INLINE bool operator!=(uint128 lhs, uint128 rhs) {
        return !(lhs == rhs);
    }

    FLARE_FORCE_INLINE bool operator<(uint128 lhs, uint128 rhs) {
        return (uint128_high64(lhs) == uint128_high64(rhs))
               ? (uint128_low64(lhs) < uint128_low64(rhs))
               : (uint128_high64(lhs) < uint128_high64(rhs));
    }

    FLARE_FORCE_INLINE bool operator>(uint128 lhs, uint128 rhs) {
        return (uint128_high64(lhs) == uint128_high64(rhs))
               ? (uint128_low64(lhs) > uint128_low64(rhs))
               : (uint128_high64(lhs) > uint128_high64(rhs));
    }

    FLARE_FORCE_INLINE bool operator<=(uint128 lhs, uint128 rhs) {
        return (uint128_high64(lhs) == uint128_high64(rhs))
               ? (uint128_low64(lhs) <= uint128_low64(rhs))
               : (uint128_high64(lhs) <= uint128_high64(rhs));
    }

    FLARE_FORCE_INLINE bool operator>=(uint128 lhs, uint128 rhs) {
        return (uint128_high64(lhs) == uint128_high64(rhs))
               ? (uint128_low64(lhs) >= uint128_low64(rhs))
               : (uint128_high64(lhs) >= uint128_high64(rhs));
    }

// Unary operators.

    FLARE_FORCE_INLINE uint128 operator-(uint128 val) {
        uint64_t hi = ~uint128_high64(val);
        uint64_t lo = ~uint128_low64(val) + 1;
        if (lo == 0)
            ++hi;  // carry
        return make_uint128(hi, lo);
    }

    FLARE_FORCE_INLINE bool operator!(uint128 val) {
        return !uint128_high64(val) && !uint128_low64(val);
    }

// Logical operators.

    FLARE_FORCE_INLINE uint128 operator~(uint128 val) {
        return make_uint128(~uint128_high64(val), ~uint128_low64(val));
    }

    FLARE_FORCE_INLINE uint128 operator|(uint128 lhs, uint128 rhs) {
        return make_uint128(uint128_high64(lhs) | uint128_high64(rhs),
                            uint128_low64(lhs) | uint128_low64(rhs));
    }

    FLARE_FORCE_INLINE uint128 operator&(uint128 lhs, uint128 rhs) {
        return make_uint128(uint128_high64(lhs) & uint128_high64(rhs),
                            uint128_low64(lhs) & uint128_low64(rhs));
    }

    FLARE_FORCE_INLINE uint128 operator^(uint128 lhs, uint128 rhs) {
        return make_uint128(uint128_high64(lhs) ^ uint128_high64(rhs),
                            uint128_low64(lhs) ^ uint128_low64(rhs));
    }

    FLARE_FORCE_INLINE uint128 &uint128::operator|=(uint128 other) {
        hi_ |= other.hi_;
        lo_ |= other.lo_;
        return *this;
    }

    FLARE_FORCE_INLINE uint128 &uint128::operator&=(uint128 other) {
        hi_ &= other.hi_;
        lo_ &= other.lo_;
        return *this;
    }

    FLARE_FORCE_INLINE uint128 &uint128::operator^=(uint128 other) {
        hi_ ^= other.hi_;
        lo_ ^= other.lo_;
        return *this;
    }

// Arithmetic operators.

    FLARE_FORCE_INLINE uint128 operator<<(uint128 lhs, int amount) {
        // uint64_t shifts of >= 64 are undefined, so we will need some
        // special-casing.
        if (amount < 64) {
            if (amount != 0) {
                return make_uint128(
                        (uint128_high64(lhs) << amount) | (uint128_low64(lhs) >> (64 - amount)),
                        uint128_low64(lhs) << amount);
            }
            return lhs;
        }
        return make_uint128(uint128_low64(lhs) << (amount - 64), 0);
    }

    FLARE_FORCE_INLINE uint128 operator>>(uint128 lhs, int amount) {
        // uint64_t shifts of >= 64 are undefined, so we will need some
        // special-casing.
        if (amount < 64) {
            if (amount != 0) {
                return make_uint128(uint128_high64(lhs) >> amount,
                                    (uint128_low64(lhs) >> amount) |
                                    (uint128_high64(lhs) << (64 - amount)));
            }
            return lhs;
        }
        return make_uint128(0, uint128_high64(lhs) >> (amount - 64));
    }

    FLARE_FORCE_INLINE uint128 operator+(uint128 lhs, uint128 rhs) {
        uint128 result = make_uint128(uint128_high64(lhs) + uint128_high64(rhs),
                                      uint128_low64(lhs) + uint128_low64(rhs));
        if (uint128_low64(result) < uint128_low64(lhs)) {  // check for carry
            return make_uint128(uint128_high64(result) + 1, uint128_low64(result));
        }
        return result;
    }

    FLARE_FORCE_INLINE uint128 operator-(uint128 lhs, uint128 rhs) {
        uint128 result = make_uint128(uint128_high64(lhs) - uint128_high64(rhs),
                                      uint128_low64(lhs) - uint128_low64(rhs));
        if (uint128_low64(lhs) < uint128_low64(rhs)) {  // check for carry
            return make_uint128(uint128_high64(result) - 1, uint128_low64(result));
        }
        return result;
    }

    FLARE_FORCE_INLINE uint128 operator*(uint128 lhs, uint128 rhs) {
#if defined(FLARE_HAVE_INTRINSIC_INT128)
        // TODO(strel) Remove once alignment issues are resolved and unsigned __int128
        // can be used for uint128 storage.
        return static_cast<unsigned __int128>(lhs) *
               static_cast<unsigned __int128>(rhs);
#elif defined(_MSC_VER) && defined(_M_X64)
        uint64_t carry;
        uint64_t low = _umul128(uint128_low64(lhs), uint128_low64(rhs), &carry);
        return make_uint128(uint128_low64(lhs) * uint128_high64(rhs) +
                               uint128_high64(lhs) * uint128_low64(rhs) + carry,
                           low);
#else   // FLARE_HAVE_INTRINSIC128
        uint64_t a32 = uint128_low64(lhs) >> 32;
        uint64_t a00 = uint128_low64(lhs) & 0xffffffff;
        uint64_t b32 = uint128_low64(rhs) >> 32;
        uint64_t b00 = uint128_low64(rhs) & 0xffffffff;
        uint128 result =
            make_uint128(uint128_high64(lhs) * uint128_low64(rhs) +
                            uint128_low64(lhs) * uint128_high64(rhs) + a32 * b32,
                        a00 * b00);
        result += uint128(a32 * b00) << 32;
        result += uint128(a00 * b32) << 32;
        return result;
#endif  // FLARE_HAVE_INTRINSIC128
    }

// Increment/decrement operators.

    FLARE_FORCE_INLINE uint128 uint128::operator++(int) {
        uint128 tmp(*this);
        *this += 1;
        return tmp;
    }

    FLARE_FORCE_INLINE uint128 uint128::operator--(int) {
        uint128 tmp(*this);
        *this -= 1;
        return tmp;
    }

    FLARE_FORCE_INLINE uint128 &uint128::operator++() {
        *this += 1;
        return *this;
    }

    FLARE_FORCE_INLINE uint128 &uint128::operator--() {
        *this -= 1;
        return *this;
    }

    constexpr int128 make_int128(int64_t high, uint64_t low) {
        return int128(high, low);
    }

// Assignment from integer types.
    FLARE_FORCE_INLINE int128 &int128::operator=(int v) {
        return *this = int128(v);
    }

    FLARE_FORCE_INLINE int128 &int128::operator=(unsigned int v) {
        return *this = int128(v);
    }

    FLARE_FORCE_INLINE int128 &int128::operator=(long v) {  // NOLINT(runtime/int)
        return *this = int128(v);
    }

// NOLINTNEXTLINE(runtime/int)
    FLARE_FORCE_INLINE int128 &int128::operator=(unsigned long v) {
        return *this = int128(v);
    }

// NOLINTNEXTLINE(runtime/int)
    FLARE_FORCE_INLINE int128 &int128::operator=(long long v) {
        return *this = int128(v);
    }

// NOLINTNEXTLINE(runtime/int)
    FLARE_FORCE_INLINE int128 &int128::operator=(unsigned long long v) {
        return *this = int128(v);
    }

// Arithmetic operators.

    int128 operator+(int128 lhs, int128 rhs);

    int128 operator-(int128 lhs, int128 rhs);

    int128 operator*(int128 lhs, int128 rhs);

    int128 operator/(int128 lhs, int128 rhs);

    int128 operator%(int128 lhs, int128 rhs);

    int128 operator|(int128 lhs, int128 rhs);

    int128 operator&(int128 lhs, int128 rhs);

    int128 operator^(int128 lhs, int128 rhs);

    int128 operator<<(int128 lhs, int amount);

    int128 operator>>(int128 lhs, int amount);

    FLARE_FORCE_INLINE int128 &int128::operator+=(int128 other) {
        *this = *this + other;
        return *this;
    }

    FLARE_FORCE_INLINE int128 &int128::operator-=(int128 other) {
        *this = *this - other;
        return *this;
    }

    FLARE_FORCE_INLINE int128 &int128::operator*=(int128 other) {
        *this = *this * other;
        return *this;
    }

    FLARE_FORCE_INLINE int128 &int128::operator/=(int128 other) {
        *this = *this / other;
        return *this;
    }

    FLARE_FORCE_INLINE int128 &int128::operator%=(int128 other) {
        *this = *this % other;
        return *this;
    }

    FLARE_FORCE_INLINE int128 &int128::operator|=(int128 other) {
        *this = *this | other;
        return *this;
    }

    FLARE_FORCE_INLINE int128 &int128::operator&=(int128 other) {
        *this = *this & other;
        return *this;
    }

    FLARE_FORCE_INLINE int128 &int128::operator^=(int128 other) {
        *this = *this ^ other;
        return *this;
    }

    FLARE_FORCE_INLINE int128 &int128::operator<<=(int amount) {
        *this = *this << amount;
        return *this;
    }

    FLARE_FORCE_INLINE int128 &int128::operator>>=(int amount) {
        *this = *this >> amount;
        return *this;
    }

    namespace int128_internal {

        // Casts from unsigned to signed while preserving the underlying binary
        // representation.
        constexpr int64_t BitCastToSigned(uint64_t v) {
            // Casting an unsigned integer to a signed integer of the same
            // width is implementation defined behavior if the source value would not fit
            // in the destination type. We step around it with a roundtrip bitwise not
            // operation to make sure this function remains constexpr. Clang, GCC, and
            // MSVC optimize this to a no-op on x86-64.
            return v & (uint64_t{1} << 63) ? ~static_cast<int64_t>(~v)
                                           : static_cast<int64_t>(v);
        }

    }  // namespace int128_internal

#if defined(FLARE_HAVE_INTRINSIC_INT128)

#include "flare/base/int128_have_intrinsic.h"  // IWYU pragma: export

#else  // FLARE_HAVE_INTRINSIC_INT128
#include "flare/base/int128_no_intrinsic.h"  // IWYU pragma: export
#endif  // FLARE_HAVE_INTRINSIC_INT128


}  // namespace flare

#undef FLARE_INTERNAL_WCHAR_T

#endif  // FLARE_BASE_INT128_H_
