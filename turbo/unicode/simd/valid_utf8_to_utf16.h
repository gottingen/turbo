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


namespace turbo::unicode::simd::utf8_to_utf16 {

    template<EndianNess endian, typename Engine>
    TURBO_FORCE_INLINE size_t convert_valid(const char *input, size_t size,
                                            char16_t *utf16_output) noexcept {
        // The implementation is not specific to haswell and should be moved to the generic directory.
        size_t pos = 0;
        char16_t *start{utf16_output};
        const size_t safety_margin = 16; // to avoid overruns!
        while (pos + 64 + safety_margin <= size) {
            // this loop could be unrolled further. For example, we could process the mask
            // far more than 64 bytes.
            simd8x64<int8_t, Engine> in(reinterpret_cast<const int8_t *>(input + pos));
            if (in.is_ascii()) {
                in.template store_ascii_as_utf16<endian>(utf16_output);
                utf16_output += 64;
                pos += 64;
            } else {
                // Slow path. We hope that the compiler will recognize that this is a slow path.
                // Anything that is not a continuation mask is a 'leading byte', that is, the
                // start of a new code point.
                uint64_t utf8_continuation_mask = in.lt(-65 + 1);
                // -65 is 0b10111111 in two-complement's, so largest possible continuation byte
                uint64_t utf8_leading_mask = ~utf8_continuation_mask;
                // The *start* of code points is not so useful, rather, we want the *end* of code points.
                uint64_t utf8_end_of_code_point_mask = utf8_leading_mask >> 1;
                // We process in blocks of up to 12 bytes except possibly
                // for fast paths which may process up to 16 bytes. For the
                // slow path to work, we should have at least 12 input bytes left.
                size_t max_starting_point = (pos + 64) - 12;
                // Next loop is going to run at least five times when using solely
                // the slow/regular path, and at least four times if there are fast paths.
                while (pos < max_starting_point) {
                    // Performance note: our ability to compute 'consumed' and
                    // then shift and recompute is critical. If there is a
                    // latency of, say, 4 cycles on getting 'consumed', then
                    // the inner loop might have a total latency of about 6 cycles.
                    // Yet we process between 6 to 12 inputs bytes, thus we get
                    // a speed limit between 1 cycle/byte and 0.5 cycle/byte
                    // for this section of the code. Hence, there is a limit
                    // to how much we can further increase this latency before
                    // it seriously harms performance.
                    //
                    // Thus we may allow convert_masked_utf8_to_utf16 to process
                    // more bytes at a time under a fast-path mode where 16 bytes
                    // are consumed at once (e.g., when encountering ASCII).
                    size_t consumed = convert_masked_utf8_to_utf16<endian>(input + pos,
                                                                           utf8_end_of_code_point_mask, utf16_output);
                    pos += consumed;
                    utf8_end_of_code_point_mask >>= consumed;
                }
                // At this point there may remain between 0 and 12 bytes in the
                // 64-byte block. These bytes will be processed again. So we have an
                // 80% efficiency (in the worst case). In practice we expect an
                // 85% to 90% efficiency.
            }
        }
        utf16_output += turbo::unicode::utf8_to_utf16::convert_valid<endian>(input + pos, size - pos, utf16_output);
        return utf16_output - start;
    }

}  // namespace turbo::unicode::simd::utf8_to_utf16
