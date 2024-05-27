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

#include <turbo/strings/internal/cordz_functions.h>

#include <atomic>
#include <cmath>
#include <limits>
#include <random>

#include <turbo/base/attributes.h>
#include <turbo/base/config.h>
#include <turbo/base/internal/raw_logging.h>
#include <turbo/profiling/internal/exponential_biased.h>

namespace turbo {
TURBO_NAMESPACE_BEGIN
namespace cord_internal {
namespace {

// The average interval until the next sample. A value of 0 disables profiling
// while a value of 1 will profile all Cords.
std::atomic<int> g_cordz_mean_interval(50000);

}  // namespace

#ifdef TURBO_INTERNAL_CORDZ_ENABLED

// Special negative 'not initialized' per thread value for cordz_next_sample.
static constexpr int64_t kInitCordzNextSample = -1;

TURBO_CONST_INIT thread_local SamplingState cordz_next_sample = {
    kInitCordzNextSample, 1};

// kIntervalIfDisabled is the number of profile-eligible events need to occur
// before the code will confirm that cordz is still disabled.
constexpr int64_t kIntervalIfDisabled = 1 << 16;

TURBO_ATTRIBUTE_NOINLINE int64_t
cordz_should_profile_slow(SamplingState& state) {

  thread_local turbo::profiling_internal::ExponentialBiased
      exponential_biased_generator;
  int32_t mean_interval = get_cordz_mean_interval();

  // Check if we disabled profiling. If so, set the next sample to a "large"
  // number to minimize the overhead of the should_profile codepath.
  if (mean_interval <= 0) {
    state = {kIntervalIfDisabled, kIntervalIfDisabled};
    return 0;
  }

  // Check if we're always sampling.
  if (mean_interval == 1) {
    state = {1, 1};
    return 1;
  }

  if (cordz_next_sample.next_sample <= 0) {
    // If first check on current thread, check cordz_should_profile()
    // again using the created (initial) stride in cordz_next_sample.
    const bool initialized =
        cordz_next_sample.next_sample != kInitCordzNextSample;
    auto old_stride = state.sample_stride;
    auto stride = exponential_biased_generator.GetStride(mean_interval);
    state = {stride, stride};
    bool should_sample = initialized || cordz_should_profile() > 0;
    return should_sample ? old_stride : 0;
  }

  --state.next_sample;
  return 0;
}

void cordz_set_next_sample_for_testing(int64_t next_sample) {
  cordz_next_sample = {next_sample, next_sample};
}

#endif  // TURBO_INTERNAL_CORDZ_ENABLED

int32_t get_cordz_mean_interval() {
  return g_cordz_mean_interval.load(std::memory_order_acquire);
}

void set_cordz_mean_interval(int32_t mean_interval) {
  g_cordz_mean_interval.store(mean_interval, std::memory_order_release);
}

}  // namespace cord_internal
TURBO_NAMESPACE_END
}  // namespace turbo
