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

#ifndef TURBO_STRINGS_INTERNAL_CORDZ_FUNCTIONS_H_
#define TURBO_STRINGS_INTERNAL_CORDZ_FUNCTIONS_H_

#include <stdint.h>

#include <turbo/base/attributes.h>
#include <turbo/base/config.h>
#include <turbo/base/optimization.h>

namespace turbo {
TURBO_NAMESPACE_BEGIN
namespace cord_internal {

// Returns the current sample rate. This represents the average interval
// between samples.
int32_t get_cordz_mean_interval();

// Sets the sample rate with the average interval between samples.
void set_cordz_mean_interval(int32_t mean_interval);

// Cordz is only enabled on Linux with thread_local support.
#if defined(TURBO_INTERNAL_CORDZ_ENABLED)
#error TURBO_INTERNAL_CORDZ_ENABLED cannot be set directly
#elif defined(__linux__) && defined(TURBO_HAVE_THREAD_LOCAL)
#define TURBO_INTERNAL_CORDZ_ENABLED 1
#endif

#ifdef TURBO_INTERNAL_CORDZ_ENABLED

struct SamplingState {
  int64_t next_sample;
  int64_t sample_stride;
};

// cordz_next_sample is the number of events until the next sample event. If
// the value is 1 or less, the code will check on the next event if cordz is
// enabled, and if so, will sample the Cord. cordz is only enabled when we can
// use thread locals.
TURBO_CONST_INIT extern thread_local SamplingState cordz_next_sample;

// Determines if the next sample should be profiled.
// Returns:
//   0: Do not sample
//  >0: Sample with the stride of the last sampling period
int64_t cordz_should_profile_slow(SamplingState& state);

// Determines if the next sample should be profiled.
// Returns:
//   0: Do not sample
//  >0: Sample with the stride of the last sampling period
inline int64_t cordz_should_profile() {
  if (TURBO_LIKELY(cordz_next_sample.next_sample > 1)) {
    cordz_next_sample.next_sample--;
    return 0;
  }
  return cordz_should_profile_slow(cordz_next_sample);
}

// Sets the interval until the next sample (for testing only)
void cordz_set_next_sample_for_testing(int64_t next_sample);

#else  // TURBO_INTERNAL_CORDZ_ENABLED

inline int64_t cordz_should_profile() { return 0; }
inline void cordz_set_next_sample_for_testing(int64_t) {}

#endif  // TURBO_INTERNAL_CORDZ_ENABLED

}  // namespace cord_internal
TURBO_NAMESPACE_END
}  // namespace turbo

#endif  // TURBO_STRINGS_INTERNAL_CORDZ_FUNCTIONS_H_
