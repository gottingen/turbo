
/****************************************************************
 * Copyright (c) 2022, liyinbin
 * All rights reserved.
 * Author by liyinbin (jeff.li) lijippy@163.com
 *****************************************************************///
//

// This file contains :int128 implementation details that depend on internal
// representation when FLARE_HAVE_INTRINSIC_INT128 is *not* defined. This file
// is included by int128.h and relies on FLARE_INTERNAL_WCHAR_T being defined.

constexpr uint64_t int128_low64(int128 v) { return v.lo_; }

constexpr int64_t int128_high64(int128 v) { return v.hi_; }

#if defined(FLARE_SYSTEM_LITTLE_ENDIAN)

constexpr int128::int128(int64_t high, uint64_t low) :
    lo_(low), hi_(high) {}

constexpr int128::int128(int v)
    : lo_{static_cast<uint64_t>(v)}, hi_{v < 0 ? ~int64_t{0} : 0} {}
constexpr int128::int128(long v)  // NOLINT(runtime/int)
    : lo_{static_cast<uint64_t>(v)}, hi_{v < 0 ? ~int64_t{0} : 0} {}
constexpr int128::int128(long long v)  // NOLINT(runtime/int)
    : lo_{static_cast<uint64_t>(v)}, hi_{v < 0 ? ~int64_t{0} : 0} {}

constexpr int128::int128(unsigned int v) : lo_{v}, hi_{0} {}
// NOLINTNEXTLINE(runtime/int)
constexpr int128::int128(unsigned long v) : lo_{v}, hi_{0} {}
// NOLINTNEXTLINE(runtime/int)
constexpr int128::int128(unsigned long long v) : lo_{v}, hi_{0} {}

constexpr int128::int128(uint128 v)
    : lo_{uint128_low64(v)}, hi_{static_cast<int64_t>(uint128_high64(v))} {}

#elif defined(FLARE_SYSTEM_BIG_ENDIAN)

constexpr int128::int128(int64_t high, uint64_t low) :
    hi_{high}, lo_{low} {}

constexpr int128::int128(int v)
    : hi_{v < 0 ? ~int64_t{0} : 0}, lo_{static_cast<uint64_t>(v)} {}
constexpr int128::int128(long v)  // NOLINT(runtime/int)
    : hi_{v < 0 ? ~int64_t{0} : 0}, lo_{static_cast<uint64_t>(v)} {}
constexpr int128::int128(long long v)  // NOLINT(runtime/int)
    : hi_{v < 0 ? ~int64_t{0} : 0}, lo_{static_cast<uint64_t>(v)} {}

constexpr int128::int128(unsigned int v) : hi_{0}, lo_{v} {}
// NOLINTNEXTLINE(runtime/int)
constexpr int128::int128(unsigned long v) : hi_{0}, lo_{v} {}
// NOLINTNEXTLINE(runtime/int)
constexpr int128::int128(unsigned long long v) : hi_{0}, lo_{v} {}

constexpr int128::int128(uint128 v)
    : hi_{static_cast<int64_t>(uint128_high64(v))}, lo_{uint128_low64(v)} {}

#else  // byte order
#error "Unsupported byte order: must be little-endian or big-endian."
#endif  // byte order

constexpr int128::operator bool() const { return lo_ || hi_; }

constexpr int128::operator char() const {
    // NOLINTNEXTLINE(runtime/int)
    return static_cast<char>(static_cast<long long>(*this));
}

constexpr int128::operator signed char() const {
    // NOLINTNEXTLINE(runtime/int)
    return static_cast<signed char>(static_cast<long long>(*this));
}

constexpr int128::operator unsigned char() const {
    return static_cast<unsigned char>(lo_);
}

constexpr int128::operator char16_t() const {
    return static_cast<char16_t>(lo_);
}

constexpr int128::operator char32_t() const {
    return static_cast<char32_t>(lo_);
}

constexpr int128::operator FLARE_INTERNAL_WCHAR_T() const {
    // NOLINTNEXTLINE(runtime/int)
    return static_cast<FLARE_INTERNAL_WCHAR_T>(static_cast<long long>(*this));
}

constexpr int128::operator short() const {  // NOLINT(runtime/int)
    // NOLINTNEXTLINE(runtime/int)
    return static_cast<short>(static_cast<long long>(*this));
}

constexpr int128::operator unsigned short() const {  // NOLINT(runtime/int)
    return static_cast<unsigned short>(lo_);           // NOLINT(runtime/int)
}

constexpr int128::operator int() const {
    // NOLINTNEXTLINE(runtime/int)
    return static_cast<int>(static_cast<long long>(*this));
}

constexpr int128::operator unsigned int() const {
    return static_cast<unsigned int>(lo_);
}

constexpr int128::operator long() const {  // NOLINT(runtime/int)
    // NOLINTNEXTLINE(runtime/int)
    return static_cast<long>(static_cast<long long>(*this));
}

constexpr int128::operator unsigned long() const {  // NOLINT(runtime/int)
    return static_cast<unsigned long>(lo_);           // NOLINT(runtime/int)
}

constexpr int128::operator long long() const {  // NOLINT(runtime/int)
    // We don't bother checking the value of hi_. If *this < 0, lo_'s high bit
    // must be set in order for the value to fit into a long long. Conversely, if
    // lo_'s high bit is set, *this must be < 0 for the value to fit.
    return int128_internal::BitCastToSigned(lo_);
}

constexpr int128::operator unsigned long long() const {  // NOLINT(runtime/int)
    return static_cast<unsigned long long>(lo_);           // NOLINT(runtime/int)
}

// Forward declaration for conversion operators to floating point types.
int128 operator-(int128 v);

bool operator!=(int128 lhs, int128 rhs);

FLARE_FORCE_INLINE int128::operator float() const {
    // We must convert the absolute value and then negate as needed, because
    // floating point types are typically sign-magnitude. Otherwise, the
    // difference between the high and low 64 bits when interpreted as two's
    // complement overwhelms the precision of the mantissa.
    //
    // Also check to make sure we don't negate int128_min()
    return hi_ < 0 && *this != int128_min()
           ? -static_cast<float>(-*this)
           : static_cast<float>(lo_) +
             std::ldexp(static_cast<float>(hi_), 64);
}

FLARE_FORCE_INLINE int128::operator double() const {
    // See comment in int128::operator float() above.
    return hi_ < 0 && *this != int128_min()
           ? -static_cast<double>(-*this)
           : static_cast<double>(lo_) +
             std::ldexp(static_cast<double>(hi_), 64);
}

FLARE_FORCE_INLINE int128::operator long double() const {
    // See comment in int128::operator float() above.
    return hi_ < 0 && *this != int128_min()
           ? -static_cast<long double>(-*this)
           : static_cast<long double>(lo_) +
             std::ldexp(static_cast<long double>(hi_), 64);
}

// Comparison operators.

FLARE_FORCE_INLINE bool operator==(int128 lhs, int128 rhs) {
    return (int128_low64(lhs) == int128_low64(rhs) &&
            int128_high64(lhs) == int128_high64(rhs));
}

FLARE_FORCE_INLINE bool operator!=(int128 lhs, int128 rhs) {
    return !(lhs == rhs);
}

FLARE_FORCE_INLINE bool operator<(int128 lhs, int128 rhs) {
    return (int128_high64(lhs) == int128_high64(rhs))
           ? (int128_low64(lhs) < int128_low64(rhs))
           : (int128_high64(lhs) < int128_high64(rhs));
}

FLARE_FORCE_INLINE bool operator>(int128 lhs, int128 rhs) {
    return (int128_high64(lhs) == int128_high64(rhs))
           ? (int128_low64(lhs) > int128_low64(rhs))
           : (int128_high64(lhs) > int128_high64(rhs));
}

FLARE_FORCE_INLINE bool operator<=(int128 lhs, int128 rhs) {
    return !(lhs > rhs);
}

FLARE_FORCE_INLINE bool operator>=(int128 lhs, int128 rhs) {
    return !(lhs < rhs);
}

// Unary operators.

FLARE_FORCE_INLINE int128 operator-(int128 v) {
    int64_t hi = ~int128_high64(v);
    uint64_t lo = ~int128_low64(v) + 1;
    if (lo == 0)
        ++hi;  // carry
    return make_int128(hi, lo);
}

FLARE_FORCE_INLINE bool operator!(int128v) {
    return !int128_low64(v) && !int128_high64(v);
}

FLARE_FORCE_INLINE int128 operator~(int128 val) {
    return make_int128(~int128_high64(val), ~int128_low64(val));
}

// Arithmetic operators.

FLARE_FORCE_INLINE int128 operator+(int128 lhs, int128 rhs) {
    int128 result = make_int128(int128_high64(lhs) + int128_high64(rhs),
                            int128_low64(lhs) + int128_low64(rhs));
    if (int128_low64(result) < int128_low64(lhs)) {  // check for carry
        return make_int128(int128_high64(result) + 1, int128_low64(result));
    }
    return result;
}

FLARE_FORCE_INLINE int128 operator-(int128 lhs, int128 rhs) {
    int128 result = make_int128(int128_high64(lhs) - int128_high64(rhs),
                            int128_low64(lhs) - int128_low64(rhs));
    if (int128_low64(lhs) < int128_low64(rhs)) {  // check for carry
        return make_int128(int128_high64(result) - 1, int128_low64(result));
    }
    return result;
}

FLARE_FORCE_INLINE int128 operator*(int128 lhs, int128 rhs) {
    uint128 result = uint128(lhs) * rhs;
    return make_int128(int128_internal::BitCastToSigned(uint128_high64(result)), uint128_low64(result));
}

FLARE_FORCE_INLINE int128 int128::operator++(int) {
    int128 tmp(*this);
    *this += 1;
    return tmp;
}

FLARE_FORCE_INLINE int128

int128::operator--(int) {
    int128 tmp(*this);
    *this -= 1;
    return tmp;
}

FLARE_FORCE_INLINE int128 &int128::operator++() {
    *this += 1;
    return *this;
}

FLARE_FORCE_INLINE int128 &int128::operator--() {
    *this -= 1;
    return *this;
}

inline int128 operator|(int128 lhs, int128 rhs) {
    return make_int128(int128_high64(lhs) |
            int128_high64(rhs), int128_low64(lhs) | int128_low64(rhs));
}

FLARE_FORCE_INLINE int128 operator&(int128 lhs, int128 rhs) {
    return make_int128(int128_high64(lhs) &
        int128_high64(rhs), int128_low64(lhs) &
        int128_low64(rhs));
}

FLARE_FORCE_INLINE int128 operator^(int128 lhs, int128 rhs) {
    return make_int128(int128_high64(lhs) ^
            int128_high64(rhs), int128_low64(lhs) ^
            int128_low64(rhs));
}

FLARE_FORCE_INLINE int128 operator<<(int128 lhs, int amount) {
    // uint64_t shifts of >= 64 are undefined, so we need some special-casing.
    if (amount < 64) {
        if (amount != 0) {
            return make_int128((int128_high64(lhs)<< amount) |
                static_cast<int64_t>(int128_low64(lhs)>> (64 - amount)),int128_low64(lhs)<< amount);
        }
        return lhs;
    }

    return make_int128(static_cast<int64_t>(int128_low64(lhs)<< (amount - 64)), 0);
}

FLARE_FORCE_INLINE int128 operator>>(int128 lhs, int amount) {
    // uint64_t shifts of >= 64 are undefined, so we need some special-casing.
    if (amount < 64) {
        if (amount != 0) {
            return make_int128(int128_high64(lhs) >> amount,(int128_low64(lhs)>> amount) |
                (static_cast<uint64_t>(int128_high64(lhs)) << (64 - amount)));
        }
        return lhs;
    }
    return make_int128(0,static_cast<uint64_t>(int128_high64(lhs)>> (amount - 64)));
}
