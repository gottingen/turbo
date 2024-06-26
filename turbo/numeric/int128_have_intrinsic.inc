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

// This file contains :int128 implementation details that depend on internal
// representation when TURBO_HAVE_INTRINSIC_INT128 is defined. This file is
// included by int128.h and relies on TURBO_INTERNAL_WCHAR_T being defined.

namespace int128_internal {

// Casts from unsigned to signed while preserving the underlying binary
// representation.
constexpr __int128 BitCastToSigned(unsigned __int128 v) {
  // Casting an unsigned integer to a signed integer of the same
  // width is implementation defined behavior if the source value would not fit
  // in the destination type. We step around it with a roundtrip bitwise not
  // operation to make sure this function remains constexpr. Clang and GCC
  // optimize this to a no-op on x86-64.
  return v & (static_cast<unsigned __int128>(1) << 127)
             ? ~static_cast<__int128>(~v)
             : static_cast<__int128>(v);
}

}  // namespace int128_internal

inline int128& int128::operator=(__int128 v) {
  v_ = v;
  return *this;
}

constexpr uint64_t Int128Low64(int128 v) {
  return static_cast<uint64_t>(v.v_ & ~uint64_t{0});
}

constexpr int64_t Int128High64(int128 v) {
  // Initially cast to unsigned to prevent a right shift on a negative value.
  return int128_internal::BitCastToSigned(
      static_cast<uint64_t>(static_cast<unsigned __int128>(v.v_) >> 64));
}

constexpr int128::int128(int64_t high, uint64_t low)
    // Initially cast to unsigned to prevent a left shift that overflows.
    : v_(int128_internal::BitCastToSigned(static_cast<unsigned __int128>(high)
                                           << 64) |
         low) {}


constexpr int128::int128(int v) : v_{v} {}

constexpr int128::int128(long v) : v_{v} {}       // NOLINT(runtime/int)

constexpr int128::int128(long long v) : v_{v} {}  // NOLINT(runtime/int)

constexpr int128::int128(__int128 v) : v_{v} {}

constexpr int128::int128(unsigned int v) : v_{v} {}

constexpr int128::int128(unsigned long v) : v_{v} {}  // NOLINT(runtime/int)

// NOLINTNEXTLINE(runtime/int)
constexpr int128::int128(unsigned long long v) : v_{v} {}

constexpr int128::int128(unsigned __int128 v) : v_{static_cast<__int128>(v)} {}

inline int128::int128(float v) {
  v_ = static_cast<__int128>(v);
}

inline int128::int128(double v) {
  v_ = static_cast<__int128>(v);
}

inline int128::int128(long double v) {
  v_ = static_cast<__int128>(v);
}

constexpr int128::int128(uint128 v) : v_{static_cast<__int128>(v)} {}

constexpr int128::operator bool() const { return static_cast<bool>(v_); }

constexpr int128::operator char() const { return static_cast<char>(v_); }

constexpr int128::operator signed char() const {
  return static_cast<signed char>(v_);
}

constexpr int128::operator unsigned char() const {
  return static_cast<unsigned char>(v_);
}

constexpr int128::operator char16_t() const {
  return static_cast<char16_t>(v_);
}

constexpr int128::operator char32_t() const {
  return static_cast<char32_t>(v_);
}

constexpr int128::operator TURBO_INTERNAL_WCHAR_T() const {
  return static_cast<TURBO_INTERNAL_WCHAR_T>(v_);
}

constexpr int128::operator short() const {  // NOLINT(runtime/int)
  return static_cast<short>(v_);            // NOLINT(runtime/int)
}

constexpr int128::operator unsigned short() const {  // NOLINT(runtime/int)
  return static_cast<unsigned short>(v_);            // NOLINT(runtime/int)
}

constexpr int128::operator int() const {
  return static_cast<int>(v_);
}

constexpr int128::operator unsigned int() const {
  return static_cast<unsigned int>(v_);
}

constexpr int128::operator long() const {  // NOLINT(runtime/int)
  return static_cast<long>(v_);            // NOLINT(runtime/int)
}

constexpr int128::operator unsigned long() const {  // NOLINT(runtime/int)
  return static_cast<unsigned long>(v_);            // NOLINT(runtime/int)
}

constexpr int128::operator long long() const {  // NOLINT(runtime/int)
  return static_cast<long long>(v_);            // NOLINT(runtime/int)
}

constexpr int128::operator unsigned long long() const {  // NOLINT(runtime/int)
  return static_cast<unsigned long long>(v_);            // NOLINT(runtime/int)
}

constexpr int128::operator __int128() const { return v_; }

constexpr int128::operator unsigned __int128() const {
  return static_cast<unsigned __int128>(v_);
}

// Clang on PowerPC sometimes produces incorrect __int128 to floating point
// conversions. In that case, we do the conversion with a similar implementation
// to the conversion operators in int128_no_intrinsic.inc.
#if defined(__clang__) && !defined(__ppc64__)
inline int128::operator float() const { return static_cast<float>(v_); }

inline int128::operator double() const { return static_cast<double>(v_); }

inline int128::operator long double() const {
  return static_cast<long double>(v_);
}

#else  // Clang on PowerPC

inline int128::operator float() const {
  // We must convert the absolute value and then negate as needed, because
  // floating point types are typically sign-magnitude. Otherwise, the
  // difference between the high and low 64 bits when interpreted as two's
  // complement overwhelms the precision of the mantissa.
  //
  // Also check to make sure we don't negate Int128Min()
  return v_ < 0 && *this != Int128Min()
             ? -static_cast<float>(-*this)
             : static_cast<float>(Int128Low64(*this)) +
                   std::ldexp(static_cast<float>(Int128High64(*this)), 64);
}

inline int128::operator double() const {
  // See comment in int128::operator float() above.
  return v_ < 0 && *this != Int128Min()
             ? -static_cast<double>(-*this)
             : static_cast<double>(Int128Low64(*this)) +
                   std::ldexp(static_cast<double>(Int128High64(*this)), 64);
}

inline int128::operator long double() const {
  // See comment in int128::operator float() above.
  return v_ < 0 && *this != Int128Min()
             ? -static_cast<long double>(-*this)
             : static_cast<long double>(Int128Low64(*this)) +
                   std::ldexp(static_cast<long double>(Int128High64(*this)),
                              64);
}
#endif  // Clang on PowerPC

// Comparison operators.

constexpr bool operator==(int128 lhs, int128 rhs) {
  return static_cast<__int128>(lhs) == static_cast<__int128>(rhs);
}

constexpr bool operator!=(int128 lhs, int128 rhs) {
  return static_cast<__int128>(lhs) != static_cast<__int128>(rhs);
}

constexpr bool operator<(int128 lhs, int128 rhs) {
  return static_cast<__int128>(lhs) < static_cast<__int128>(rhs);
}

constexpr bool operator>(int128 lhs, int128 rhs) {
  return static_cast<__int128>(lhs) > static_cast<__int128>(rhs);
}

constexpr bool operator<=(int128 lhs, int128 rhs) {
  return static_cast<__int128>(lhs) <= static_cast<__int128>(rhs);
}

constexpr bool operator>=(int128 lhs, int128 rhs) {
  return static_cast<__int128>(lhs) >= static_cast<__int128>(rhs);
}

#ifdef __cpp_impl_three_way_comparison
constexpr turbo::strong_ordering operator<=>(int128 lhs, int128 rhs) {
  if (auto lhs_128 = static_cast<__int128>(lhs),
      rhs_128 = static_cast<__int128>(rhs);
      lhs_128 < rhs_128) {
    return turbo::strong_ordering::less;
  } else if (lhs_128 > rhs_128) {
    return turbo::strong_ordering::greater;
  } else {
    return turbo::strong_ordering::equal;
  }
}
#endif

// Unary operators.

constexpr int128 operator-(int128 v) { return -static_cast<__int128>(v); }

constexpr bool operator!(int128 v) { return !static_cast<__int128>(v); }

constexpr int128 operator~(int128 val) { return ~static_cast<__int128>(val); }

// Arithmetic operators.

constexpr int128 operator+(int128 lhs, int128 rhs) {
  return static_cast<__int128>(lhs) + static_cast<__int128>(rhs);
}

constexpr int128 operator-(int128 lhs, int128 rhs) {
  return static_cast<__int128>(lhs) - static_cast<__int128>(rhs);
}

inline int128 operator*(int128 lhs, int128 rhs) {
  return static_cast<__int128>(lhs) * static_cast<__int128>(rhs);
}

inline int128 operator/(int128 lhs, int128 rhs) {
  return static_cast<__int128>(lhs) / static_cast<__int128>(rhs);
}

inline int128 operator%(int128 lhs, int128 rhs) {
  return static_cast<__int128>(lhs) % static_cast<__int128>(rhs);
}

inline int128 int128::operator++(int) {
  int128 tmp(*this);
  ++v_;
  return tmp;
}

inline int128 int128::operator--(int) {
  int128 tmp(*this);
  --v_;
  return tmp;
}

inline int128& int128::operator++() {
  ++v_;
  return *this;
}

inline int128& int128::operator--() {
  --v_;
  return *this;
}

constexpr int128 operator|(int128 lhs, int128 rhs) {
  return static_cast<__int128>(lhs) | static_cast<__int128>(rhs);
}

constexpr int128 operator&(int128 lhs, int128 rhs) {
  return static_cast<__int128>(lhs) & static_cast<__int128>(rhs);
}

constexpr int128 operator^(int128 lhs, int128 rhs) {
  return static_cast<__int128>(lhs) ^ static_cast<__int128>(rhs);
}

constexpr int128 operator<<(int128 lhs, int amount) {
  return static_cast<__int128>(lhs) << amount;
}

constexpr int128 operator>>(int128 lhs, int amount) {
  return static_cast<__int128>(lhs) >> amount;
}
