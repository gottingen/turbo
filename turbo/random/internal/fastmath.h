// Copyright 2022 The Turbo Authors.
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

#ifndef TURBO_RANDOM_INTERNAL_FASTMATH_H_
#define TURBO_RANDOM_INTERNAL_FASTMATH_H_

// This file contains fast math functions (bitwise ops as well as some others)
// which are implementation details of various turbo random number distributions.

#include <cassert>
#include <cmath>
#include <cstdint>

#include "turbo/base/bits.h"

namespace turbo {
TURBO_NAMESPACE_BEGIN
namespace random_internal {

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
