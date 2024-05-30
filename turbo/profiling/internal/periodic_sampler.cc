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

#include <turbo/profiling/internal/periodic_sampler.h>

#include <atomic>

#include <turbo/profiling/internal/exponential_biased.h>

namespace turbo {
TURBO_NAMESPACE_BEGIN
namespace profiling_internal {

int64_t PeriodicSamplerBase::GetExponentialBiased(int period) noexcept {
  return rng_.GetStride(period);
}

bool PeriodicSamplerBase::SubtleConfirmSample() noexcept {
  int current_period = period();

  // Deal with period case 0 (always off) and 1 (always on)
  if (TURBO_UNLIKELY(current_period < 2)) {
    stride_ = 0;
    return current_period == 1;
  }

  // Check if this is the first call to Sample()
  if (TURBO_UNLIKELY(stride_ == 1)) {
    stride_ = static_cast<uint64_t>(-GetExponentialBiased(current_period));
    if (static_cast<int64_t>(stride_) < -1) {
      ++stride_;
      return false;
    }
  }

  stride_ = static_cast<uint64_t>(-GetExponentialBiased(current_period));
  return true;
}

}  // namespace profiling_internal
TURBO_NAMESPACE_END
}  // namespace turbo
