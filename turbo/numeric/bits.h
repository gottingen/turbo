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
// File: bits.h
// -----------------------------------------------------------------------------
//
// This file contains implementations of C++20's bitwise math functions, as
// defined by:
//
// P0553R4:
//  http://www.open-std.org/jtc1/sc22/wg21/docs/papers/2019/p0553r4.html
// P0556R3:
//  http://www.open-std.org/jtc1/sc22/wg21/docs/papers/2018/p0556r3.html
// P1355R2:
//  http://www.open-std.org/jtc1/sc22/wg21/docs/papers/2019/p1355r2.html
// P1956R1:
//  http://www.open-std.org/jtc1/sc22/wg21/docs/papers/2020/p1956r1.pdf
//
// When using a standard library that implements these functions, we use the
// standard library's implementation.

#ifndef TURBO_NUMERIC_BITS_H_
#define TURBO_NUMERIC_BITS_H_

#include <cstdint>
#include <limits>
#include <type_traits>

#include <turbo/base/config.h>

#if TURBO_INTERNAL_CPLUSPLUS_LANG >= 202002L
#include <bit>
#endif

#include <turbo/base/attributes.h>
#include <turbo/numeric/internal/bits.h>

namespace turbo {
TURBO_NAMESPACE_BEGIN

// https://github.com/llvm/llvm-project/issues/64544
// libc++ had the wrong signature for std::rotl and std::rotr
// prior to libc++ 18.0.
//
#if (defined(__cpp_lib_bitops) && __cpp_lib_bitops >= 201907L) &&     \
    (!defined(_LIBCPP_VERSION) || _LIBCPP_VERSION >= 180000)
using std::rotl;
using std::rotr;

#else

// Rotating functions
template <class T>
TURBO_MUST_USE_RESULT constexpr
    typename std::enable_if<std::is_unsigned<T>::value, T>::type
    rotl(T x, int s) noexcept {
  return numeric_internal::RotateLeft(x, s);
}

template <class T>
TURBO_MUST_USE_RESULT constexpr
    typename std::enable_if<std::is_unsigned<T>::value, T>::type
    rotr(T x, int s) noexcept {
  return numeric_internal::RotateRight(x, s);
}

#endif

// https://github.com/llvm/llvm-project/issues/64544
// libc++ had the wrong signature for std::rotl and std::rotr
// prior to libc++ 18.0.
//
#if (defined(__cpp_lib_bitops) && __cpp_lib_bitops >= 201907L)

using std::countl_one;
using std::countl_zero;
using std::countr_one;
using std::countr_zero;
using std::popcount;

#else

// Counting functions
//
// While these functions are typically constexpr, on some platforms, they may
// not be marked as constexpr due to constraints of the compiler/available
// intrinsics.
template <class T>
TURBO_INTERNAL_CONSTEXPR_CLZ inline
    typename std::enable_if<std::is_unsigned<T>::value, int>::type
    countl_zero(T x) noexcept {
  return numeric_internal::CountLeadingZeroes(x);
}

template <class T>
TURBO_INTERNAL_CONSTEXPR_CLZ inline
    typename std::enable_if<std::is_unsigned<T>::value, int>::type
    countl_one(T x) noexcept {
  // Avoid integer promotion to a wider type
  return countl_zero(static_cast<T>(~x));
}

template <class T>
TURBO_INTERNAL_CONSTEXPR_CTZ inline
    typename std::enable_if<std::is_unsigned<T>::value, int>::type
    countr_zero(T x) noexcept {
  return numeric_internal::CountTrailingZeroes(x);
}

template <class T>
TURBO_INTERNAL_CONSTEXPR_CTZ inline
    typename std::enable_if<std::is_unsigned<T>::value, int>::type
    countr_one(T x) noexcept {
  // Avoid integer promotion to a wider type
  return countr_zero(static_cast<T>(~x));
}

template <class T>
TURBO_INTERNAL_CONSTEXPR_POPCOUNT inline
    typename std::enable_if<std::is_unsigned<T>::value, int>::type
    popcount(T x) noexcept {
  return numeric_internal::Popcount(x);
}

#endif

#if (defined(__cpp_lib_int_pow2) && __cpp_lib_int_pow2 >= 202002L)

using std::bit_ceil;
using std::bit_floor;
using std::bit_width;
using std::has_single_bit;

#else

// Returns: true if x is an integral power of two; false otherwise.
template <class T>
constexpr inline typename std::enable_if<std::is_unsigned<T>::value, bool>::type
has_single_bit(T x) noexcept {
  return x != 0 && (x & (x - 1)) == 0;
}

// Returns: If x == 0, 0; otherwise one plus the base-2 logarithm of x, with any
// fractional part discarded.
template <class T>
TURBO_INTERNAL_CONSTEXPR_CLZ inline
    typename std::enable_if<std::is_unsigned<T>::value, int>::type
    bit_width(T x) noexcept {
  return std::numeric_limits<T>::digits - countl_zero(x);
}

// Returns: If x == 0, 0; otherwise the maximal value y such that
// has_single_bit(y) is true and y <= x.
template <class T>
TURBO_INTERNAL_CONSTEXPR_CLZ inline
    typename std::enable_if<std::is_unsigned<T>::value, T>::type
    bit_floor(T x) noexcept {
  return x == 0 ? 0 : T{1} << (bit_width(x) - 1);
}

// Returns: N, where N is the smallest power of 2 greater than or equal to x.
//
// Preconditions: N is representable as a value of type T.
template <class T>
TURBO_INTERNAL_CONSTEXPR_CLZ inline
    typename std::enable_if<std::is_unsigned<T>::value, T>::type
    bit_ceil(T x) {
  // If T is narrower than unsigned, T{1} << bit_width will be promoted.  We
  // want to force it to wraparound so that bit_ceil of an invalid value are not
  // core constant expressions.
  //
  // BitCeilNonPowerOf2 triggers an overflow in constexpr contexts if we would
  // undergo promotion to unsigned but not fit the result into T without
  // truncation.
  return has_single_bit(x) ? T{1} << (bit_width(x) - 1)
                           : numeric_internal::BitCeilNonPowerOf2(x);
}

#endif

TURBO_NAMESPACE_END
}  // namespace turbo

#endif  // TURBO_NUMERIC_BITS_H_
