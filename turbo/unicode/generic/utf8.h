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

#include "turbo/unicode/scalar/utf8.h"

namespace turbo {
namespace TURBO_UNICODE_IMPLEMENTATION {
namespace {
namespace utf8 {

using namespace simd;

TURBO_FORCE_INLINE size_t count_code_points(const char* in, size_t size) {
    size_t pos = 0;
    size_t count = 0;
    for(;pos + 64 <= size; pos += 64) {
      simd8x64<int8_t> input(reinterpret_cast<const int8_t *>(in + pos));
      uint64_t utf8_continuation_mask = input.lt(-65 + 1);
      count += 64 - count_ones(utf8_continuation_mask);
    }
    return count + scalar::utf8::count_code_points(in + pos, size - pos);
}


TURBO_FORCE_INLINE size_t Utf16LengthFromUtf8(const char* in, size_t size) {
    size_t pos = 0;
    size_t count = 0;
    // This algorithm could no doubt be improved!
    for(;pos + 64 <= size; pos += 64) {
      simd8x64<int8_t> input(reinterpret_cast<const int8_t *>(in + pos));
      uint64_t utf8_continuation_mask = input.lt(-65 + 1);
      // We count one word for anything that is not a continuation (so
      // leading bytes).
      count += 64 - count_ones(utf8_continuation_mask);
      int64_t utf8_4byte = input.gteq_unsigned(240);
      count += count_ones(utf8_4byte);
    }
    return count + scalar::utf8::Utf16LengthFromUtf8(in + pos, size - pos);
}


TURBO_FORCE_INLINE size_t Utf32LengthFromUtf8(const char* in, size_t size) {
    return count_code_points(in, size);
}
} // utf8 namespace
} // unnamed namespace
} // namespace TURBO_UNICODE_IMPLEMENTATION
} // namespace turbo
