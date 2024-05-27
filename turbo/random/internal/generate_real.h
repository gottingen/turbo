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

#ifndef TURBO_RANDOM_INTERNAL_GENERATE_REAL_H_
#define TURBO_RANDOM_INTERNAL_GENERATE_REAL_H_

// This file contains some implementation details which are used by one or more
// of the turbo random number distributions.

#include <cstdint>
#include <cstring>
#include <limits>
#include <type_traits>

#include <turbo/meta/type_traits.h>
#include <turbo/numeric/bits.h>
#include <turbo/random/internal/fastmath.h>
#include <turbo/random/internal/traits.h>

namespace turbo {
TURBO_NAMESPACE_BEGIN
namespace random_internal {

// Tristate tag types controlling the output of GenerateRealFromBits.
struct GeneratePositiveTag {};
struct GenerateNegativeTag {};
struct GenerateSignedTag {};

// GenerateRealFromBits generates a single real value from a single 64-bit
// `bits` with template fields controlling the output.
//
// The `SignedTag` parameter controls whether positive, negative,
// or either signed/unsigned may be returned.
//   When SignedTag == GeneratePositiveTag, range is U(0, 1)
//   When SignedTag == GenerateNegativeTag, range is U(-1, 0)
//   When SignedTag == GenerateSignedTag, range is U(-1, 1)
//
// When the `IncludeZero` parameter is true, the function may return 0 for some
// inputs, otherwise it never returns 0.
//
// When a value in U(0,1) is required, use:
//   GenerateRealFromBits<double, PositiveValueT, true>;
//
// When a value in U(-1,1) is required, use:
//   GenerateRealFromBits<double, SignedValueT, false>;
//
//   This generates more distinct values than the mathematical equivalent
//   `U(0, 1) * 2.0 - 1.0`.
//
// Scaling the result by powers of 2 (and avoiding a multiply) is also possible:
//   GenerateRealFromBits<double>(..., -1);  => U(0, 0.5)
//   GenerateRealFromBits<double>(..., 1);   => U(0, 2)
//
template <typename RealType,  // Real type, either float or double.
          typename SignedTag = GeneratePositiveTag,  // Whether a positive,
                                                     // negative, or signed
                                                     // value is generated.
          bool IncludeZero = true>
inline RealType GenerateRealFromBits(uint64_t bits, int exp_bias = 0) {
  using real_type = RealType;
  using uint_type = turbo::conditional_t<std::is_same<real_type, float>::value,
                                        uint32_t, uint64_t>;

  static_assert(
      (std::is_same<double, real_type>::value ||
       std::is_same<float, real_type>::value),
      "GenerateRealFromBits must be parameterized by either float or double.");

  static_assert(sizeof(uint_type) == sizeof(real_type),
                "Mismatched unsigned and real types.");

  static_assert((std::numeric_limits<real_type>::is_iec559 &&
                 std::numeric_limits<real_type>::radix == 2),
                "RealType representation is not IEEE 754 binary.");

  static_assert((std::is_same<SignedTag, GeneratePositiveTag>::value ||
                 std::is_same<SignedTag, GenerateNegativeTag>::value ||
                 std::is_same<SignedTag, GenerateSignedTag>::value),
                "");

  static constexpr int kExp = std::numeric_limits<real_type>::digits - 1;
  static constexpr uint_type kMask = (static_cast<uint_type>(1) << kExp) - 1u;
  static constexpr int kUintBits = sizeof(uint_type) * 8;

  int exp = exp_bias + int{std::numeric_limits<real_type>::max_exponent - 2};

  // Determine the sign bit.
  // Depending on the SignedTag, this may use the left-most bit
  // or it may be a constant value.
  uint_type sign = std::is_same<SignedTag, GenerateNegativeTag>::value
                       ? (static_cast<uint_type>(1) << (kUintBits - 1))
                       : 0;
  if (std::is_same<SignedTag, GenerateSignedTag>::value) {
    if (std::is_same<uint_type, uint64_t>::value) {
      sign = bits & uint64_t{0x8000000000000000};
    }
    if (std::is_same<uint_type, uint32_t>::value) {
      const uint64_t tmp = bits & uint64_t{0x8000000000000000};
      sign = static_cast<uint32_t>(tmp >> 32);
    }
    // adjust the bits and the exponent to account for removing
    // the leading bit.
    bits = bits & uint64_t{0x7FFFFFFFFFFFFFFF};
    exp++;
  }
  if (IncludeZero) {
    if (bits == 0u) return 0;
  }

  // Number of leading zeros is mapped to the exponent: 2^-clz
  // bits is 0..01xxxxxx. After shifting, we're left with 1xxx...0..0
  int clz = countl_zero(bits);
  bits <<= (IncludeZero ? clz : (clz & 63));  // remove 0-bits.
  exp -= clz;                                 // set the exponent.
  bits >>= (63 - kExp);

  // Construct the 32-bit or 64-bit IEEE 754 floating-point value from
  // the individual fields: sign, exp, mantissa(bits).
  uint_type val = sign | (static_cast<uint_type>(exp) << kExp) |
                  (static_cast<uint_type>(bits) & kMask);

  // bit_cast to the output-type
  real_type result;
  memcpy(static_cast<void*>(&result), static_cast<const void*>(&val),
         sizeof(result));
  return result;
}

}  // namespace random_internal
TURBO_NAMESPACE_END
}  // namespace turbo

#endif  // TURBO_RANDOM_INTERNAL_GENERATE_REAL_H_
