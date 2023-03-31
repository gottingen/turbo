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

#ifndef SIMDUTF_ARM64_BITMANIPULATION_H
#define SIMDUTF_ARM64_BITMANIPULATION_H

namespace turbo {
namespace TURBO_UNICODE_IMPLEMENTATION {
namespace {

/* result might be undefined when input_num is zero */
TURBO_FORCE_INLINE int count_ones(uint64_t input_num) {
   return vaddv_u8(vcnt_u8(vcreate_u8(input_num)));
}

} // unnamed namespace
} // namespace TURBO_UNICODE_IMPLEMENTATION
} // namespace turbo

#endif // SIMDUTF_ARM64_BITMANIPULATION_H
