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

#ifndef TURBO_RANDOM_INTERNAL_FASTMATH_H_
#define TURBO_RANDOM_INTERNAL_FASTMATH_H_

// This file contains fast math functions (bitwise ops as well as some others)
// which are implementation details of various turbo random number distributions.

#include <cassert>
#include <cmath>
#include <cstdint>

#include <turbo/numeric/bits.h>

namespace turbo {
TURBO_NAMESPACE_BEGIN
namespace random_internal {

// Compute log2(n) using integer operations.
// While std::log2 is more accurate than std::log(n) / std::log(2), for
// very large numbers--those close to std::numeric_limits<uint64_t>::max() - 2,
// for instance--std::log2 rounds up rather than down, which introduces
// definite skew in the results.
inline int IntLog2Floor(uint64_t n) {
  return (n <= 1) ? 0 : (63 - countl_zero(n));
}
inline int IntLog2Ceil(uint64_t n) {
  return (n <= 1) ? 0 : (64 - countl_zero(n - 1));
}

inline double StirlingLogFactorial(double n) {
  assert(n >= 1);
  // Using Stirling's approximation.
  constexpr double kLog2PI = 1.83787706640934548356;
  const double logn = std::log(n);
  const double ninv = 1.0 / static_cast<double>(n);
  return n * logn - n + 0.5 * (kLog2PI + logn) + (1.0 / 12.0) * ninv -
         (1.0 / 360.0) * ninv * ninv * ninv;
}

}  // namespace random_internal
TURBO_NAMESPACE_END
}  // namespace turbo

#endif  // TURBO_RANDOM_INTERNAL_FASTMATH_H_
