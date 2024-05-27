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

#include <turbo/profiling/internal/exponential_biased.h>

#include <stdint.h>

#include <algorithm>
#include <atomic>
#include <cmath>
#include <limits>

#include <turbo/base/attributes.h>
#include <turbo/base/optimization.h>

namespace turbo {
TURBO_NAMESPACE_BEGIN
namespace profiling_internal {

// The algorithm generates a random number between 0 and 1 and applies the
// inverse cumulative distribution function for an exponential. Specifically:
// Let m be the inverse of the sample period, then the probability
// distribution function is m*exp(-mx) so the CDF is
// p = 1 - exp(-mx), so
// q = 1 - p = exp(-mx)
// log_e(q) = -mx
// -log_e(q)/m = x
// log_2(q) * (-log_e(2) * 1/m) = x
// In the code, q is actually in the range 1 to 2**26, hence the -26 below
int64_t ExponentialBiased::GetSkipCount(int64_t mean) {
  if (TURBO_PREDICT_FALSE(!initialized_)) {
    Initialize();
  }

  uint64_t rng = NextRandom(rng_);
  rng_ = rng;

  // Take the top 26 bits as the random number
  // (This plus the 1<<58 sampling bound give a max possible step of
  // 5194297183973780480 bytes.)
  // The uint32_t cast is to prevent a (hard-to-reproduce) NAN
  // under piii debug for some binaries.
  double q = static_cast<uint32_t>(rng >> (kPrngNumBits - 26)) + 1.0;
  // Put the computed p-value through the CDF of a geometric.
  double interval = bias_ + (std::log2(q) - 26) * (-std::log(2.0) * mean);
  // Very large values of interval overflow int64_t. To avoid that, we will
  // cheat and clamp any huge values to (int64_t max)/2. This is a potential
  // source of bias, but the mean would need to be such a large value that it's
  // not likely to come up. For example, with a mean of 1e18, the probability of
  // hitting this condition is about 1/1000. For a mean of 1e17, standard
  // calculators claim that this event won't happen.
  if (interval > static_cast<double>(std::numeric_limits<int64_t>::max() / 2)) {
    // Assume huge values are bias neutral, retain bias for next call.
    return std::numeric_limits<int64_t>::max() / 2;
  }
  double value = std::rint(interval);
  bias_ = interval - value;
  return value;
}

int64_t ExponentialBiased::GetStride(int64_t mean) {
  return GetSkipCount(mean - 1) + 1;
}

void ExponentialBiased::Initialize() {
  // We don't get well distributed numbers from `this` so we call NextRandom() a
  // bunch to mush the bits around. We use a global_rand to handle the case
  // where the same thread (by memory address) gets created and destroyed
  // repeatedly.
  TURBO_CONST_INIT static std::atomic<uint32_t> global_rand(0);
  uint64_t r = reinterpret_cast<uint64_t>(this) +
               global_rand.fetch_add(1, std::memory_order_relaxed);
  for (int i = 0; i < 20; ++i) {
    r = NextRandom(r);
  }
  rng_ = r;
  initialized_ = true;
}

}  // namespace profiling_internal
TURBO_NAMESPACE_END
}  // namespace turbo
