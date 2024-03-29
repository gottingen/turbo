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
//

#include "turbo/unicode/scalar/utf8_convert.h"
#include "turbo/unicode/simd/fwd.h"
#include "turbo/base/bits.h"
#include "turbo/platform/port.h"

#if TURBO_WITH_AVX2
#include "turbo/unicode/avx2/convert_masked.h"
#endif

namespace turbo::unicode::simd::utf8_to_utf32 {

    template<typename Engine>
    TURBO_FORCE_INLINE size_t convert_valid(const char *input, size_t size,
                                            char32_t *utf32_output) noexcept {
        size_t pos = 0;
        char32_t *start{utf32_output};
        const size_t safety_margin = 16; // to avoid overruns!
        while (pos + 64 + safety_margin <= size) {
            simd8x64<int8_t, Engine> in(reinterpret_cast<const int8_t *>(input + pos));
            if (in.is_ascii()) {
                in.store_ascii_as_utf32(utf32_output);
                utf32_output += 64;
                pos += 64;
            } else {
                // -65 is 0b10111111 in two-complement's, so largest possible continuation byte
                uint64_t utf8_continuation_mask = in.lt(-65 + 1);
                uint64_t utf8_leading_mask = ~utf8_continuation_mask;
                uint64_t utf8_end_of_code_point_mask = utf8_leading_mask >> 1;
                size_t max_starting_point = (pos + 64) - 12;
                while (pos < max_starting_point) {
                    size_t consumed = turbo::unicode::simd::convert_masked_utf8_to_utf32(input + pos,
                                                                                         utf8_end_of_code_point_mask,
                                                                                         utf32_output);
                    pos += consumed;
                    utf8_end_of_code_point_mask >>= consumed;
                }
            }
        }
        utf32_output += turbo::unicode::utf8_to_utf32::convert_valid(input + pos, size - pos, utf32_output);
        return utf32_output - start;
    }


} // namespace turbo::unicode::simd::utf8_to_utf32
