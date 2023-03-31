// Copyright 2023 The Turbo Authors.
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

#ifndef SIMDUTF_HASWELL_BITMANIPULATION_H
#define SIMDUTF_HASWELL_BITMANIPULATION_H

namespace turbo {
namespace TURBO_UNICODE_IMPLEMENTATION {
namespace {

#ifdef SIMDUTF_REGULAR_VISUAL_STUDIO
TURBO_FORCE_INLINE unsigned size_t count_ones(uint64_t input_num) {
  // note: we do not support legacy 32-bit Windows
  return static_cast<size_t>(__popcnt64(input_num));// Visual Studio wants two underscores
}
#else
TURBO_FORCE_INLINE size_t count_ones(uint64_t input_num) {
  return static_cast<size_t>(_popcnt64(input_num));
}
#endif

} // unnamed namespace
} // namespace TURBO_UNICODE_IMPLEMENTATION
} // namespace turbo

#endif // SIMDUTF_HASWELL_BITMANIPULATION_H
