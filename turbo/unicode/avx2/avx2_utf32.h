// Copyright 2023 The Elastic-AI Authors.
// part of Elastic AI Search
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

#ifndef TURBO_UNICODE_AVX2_UTF32_H_
#define TURBO_UNICODE_AVX2_UTF32_H_

#include "turbo/unicode/scalar/validate.h"
#include "turbo/unicode/simd/fwd.h"
#include "turbo/base/bits.h"
#include "turbo/unicode/avx2/engine.h"
#include "turbo/unicode/avx2/simd.h"
#include "turbo/unicode/simd/utf16_to_utf8_tables.h"
#include "turbo/unicode/simd/utf8_to_utf16_tables.h"

namespace turbo::unicode::simd {

    inline std::pair<const char32_t*, char*> avx2_convert_utf32_to_utf8(const char32_t* buf, size_t len, char* utf8_output) {
        const char32_t* end = buf + len;
        const __m256i v_0000 = _mm256_setzero_si256();
        const __m256i v_ffff0000 = _mm256_set1_epi32((uint32_t)0xffff0000);
        const __m256i v_ff80 = _mm256_set1_epi16((uint16_t)0xff80);
        const __m256i v_f800 = _mm256_set1_epi16((uint16_t)0xf800);
        const __m256i v_c080 = _mm256_set1_epi16((uint16_t)0xc080);
        const __m256i v_7fffffff = _mm256_set1_epi32((uint32_t)0x7fffffff);
        __m256i running_max = _mm256_setzero_si256();
        __m256i forbidden_bytemask = _mm256_setzero_si256();

        const size_t safety_margin = 12; // to avoid overruns, see issue https://github.com/simdutf/simdutf/issues/92

        while (buf + 16 + safety_margin <= end) {
            __m256i in = _mm256_loadu_si256((__m256i*)buf);
            __m256i nextin = _mm256_loadu_si256((__m256i*)buf+1);
            running_max = _mm256_max_epu32(_mm256_max_epu32(in, running_max), nextin);

            // Pack 32-bit UTF-32 code units to 16-bit UTF-16 code units with unsigned saturation
            __m256i in_16 = _mm256_packus_epi32(_mm256_and_si256(in, v_7fffffff), _mm256_and_si256(nextin, v_7fffffff));
            in_16 = _mm256_permute4x64_epi64(in_16, 0b11011000);

            // Try to apply UTF-16 => UTF-8 routine on 256 bits (haswell/avx2_convert_utf16_to_utf8.cpp)

            if(_mm256_testz_si256(in_16, v_ff80)) { // ASCII fast path!!!!
                // 1. pack the bytes
                const __m128i utf8_packed = _mm_packus_epi16(_mm256_castsi256_si128(in_16),_mm256_extractf128_si256(in_16,1));
                // 2. store (16 bytes)
                _mm_storeu_si128((__m128i*)utf8_output, utf8_packed);
                // 3. adjust pointers
                buf += 16;
                utf8_output += 16;
                continue; // we are done for this round!
            }
            // no bits set above 7th bit
            const __m256i one_byte_bytemask = _mm256_cmpeq_epi16(_mm256_and_si256(in_16, v_ff80), v_0000);
            const uint32_t one_byte_bitmask = static_cast<uint32_t>(_mm256_movemask_epi8(one_byte_bytemask));

            // no bits set above 11th bit
            const __m256i one_or_two_bytes_bytemask = _mm256_cmpeq_epi16(_mm256_and_si256(in_16, v_f800), v_0000);
            const uint32_t one_or_two_bytes_bitmask = static_cast<uint32_t>(_mm256_movemask_epi8(one_or_two_bytes_bytemask));
            if (one_or_two_bytes_bitmask == 0xffffffff) {
                // 1. prepare 2-byte values
                // input 16-bit word : [0000|0aaa|aabb|bbbb] x 8
                // expected output   : [110a|aaaa|10bb|bbbb] x 8
                const __m256i v_1f00 = _mm256_set1_epi16((int16_t)0x1f00);
                const __m256i v_003f = _mm256_set1_epi16((int16_t)0x003f);

                // t0 = [000a|aaaa|bbbb|bb00]
                const __m256i t0 = _mm256_slli_epi16(in_16, 2);
                // t1 = [000a|aaaa|0000|0000]
                const __m256i t1 = _mm256_and_si256(t0, v_1f00);
                // t2 = [0000|0000|00bb|bbbb]
                const __m256i t2 = _mm256_and_si256(in_16, v_003f);
                // t3 = [000a|aaaa|00bb|bbbb]
                const __m256i t3 = _mm256_or_si256(t1, t2);
                // t4 = [110a|aaaa|10bb|bbbb]
                const __m256i t4 = _mm256_or_si256(t3, v_c080);

                // 2. merge ASCII and 2-byte codewords
                const __m256i utf8_unpacked = _mm256_blendv_epi8(t4, in_16, one_byte_bytemask);

                // 3. prepare bitmask for 8-bit lookup
                const uint32_t M0 = one_byte_bitmask & 0x55555555;
                const uint32_t M1 = M0 >> 7;
                const uint32_t M2 = (M1 | M0)  & 0x00ff00ff;
                // 4. pack the bytes

                const uint8_t* row = &utf16_to_utf8::pack_1_2_utf8_bytes[uint8_t(M2)][0];
                const uint8_t* row_2 = &utf16_to_utf8::pack_1_2_utf8_bytes[uint8_t(M2>>16)][0];

                const __m128i shuffle = _mm_loadu_si128((__m128i*)(row + 1));
                const __m128i shuffle_2 = _mm_loadu_si128((__m128i*)(row_2 + 1));

                const __m256i utf8_packed = _mm256_shuffle_epi8(utf8_unpacked, _mm256_setr_m128i(shuffle,shuffle_2));
                // 5. store bytes
                _mm_storeu_si128((__m128i*)utf8_output, _mm256_castsi256_si128(utf8_packed));
                utf8_output += row[0];
                _mm_storeu_si128((__m128i*)utf8_output, _mm256_extractf128_si256(utf8_packed,1));
                utf8_output += row_2[0];

                // 6. adjust pointers
                buf += 16;
                continue;
            }
            // Must check for overflow in packing
            const __m256i saturation_bytemask = _mm256_cmpeq_epi32(_mm256_and_si256(_mm256_or_si256(in, nextin), v_ffff0000), v_0000);
            const uint32_t saturation_bitmask = static_cast<uint32_t>(_mm256_movemask_epi8(saturation_bytemask));
            if (saturation_bitmask == 0xffffffff) {
                // case: code units from register produce either 1, 2 or 3 UTF-8 bytes
                const __m256i v_d800 = _mm256_set1_epi16((uint16_t)0xd800);
                forbidden_bytemask = _mm256_or_si256(forbidden_bytemask, _mm256_cmpeq_epi16(_mm256_and_si256(in_16, v_f800), v_d800));

                const __m256i dup_even = _mm256_setr_epi16(0x0000, 0x0202, 0x0404, 0x0606,
                                                           0x0808, 0x0a0a, 0x0c0c, 0x0e0e,
                                                           0x0000, 0x0202, 0x0404, 0x0606,
                                                           0x0808, 0x0a0a, 0x0c0c, 0x0e0e);

                /* In this branch we handle three cases:
                  1. [0000|0000|0ccc|cccc] => [0ccc|cccc]                           - single UFT-8 byte
                  2. [0000|0bbb|bbcc|cccc] => [110b|bbbb], [10cc|cccc]              - two UTF-8 bytes
                  3. [aaaa|bbbb|bbcc|cccc] => [1110|aaaa], [10bb|bbbb], [10cc|cccc] - three UTF-8 bytes

                  We expand the input word (16-bit) into two code units (32-bit), thus
                  we have room for four bytes. However, we need five distinct bit
                  layouts. Note that the last byte in cases #2 and #3 is the same.

                  We precompute byte 1 for case #1 and the common byte for cases #2 & #3
                  in register t2.

                  We precompute byte 1 for case #3 and -- **conditionally** -- precompute
                  either byte 1 for case #2 or byte 2 for case #3. Note that they
                  differ by exactly one bit.

                  Finally from these two code units we build proper UTF-8 sequence, taking
                  into account the case (i.e, the number of bytes to write).
                */
                /**
                 * Given [aaaa|bbbb|bbcc|cccc] our goal is to produce:
                 * t2 => [0ccc|cccc] [10cc|cccc]
                 * s4 => [1110|aaaa] ([110b|bbbb] OR [10bb|bbbb])
                 */
#define simdutf_vec(x) _mm256_set1_epi16(static_cast<uint16_t>(x))
                // [aaaa|bbbb|bbcc|cccc] => [bbcc|cccc|bbcc|cccc]
                const __m256i t0 = _mm256_shuffle_epi8(in_16, dup_even);
                // [bbcc|cccc|bbcc|cccc] => [00cc|cccc|0bcc|cccc]
                const __m256i t1 = _mm256_and_si256(t0, simdutf_vec(0b0011111101111111));
                // [00cc|cccc|0bcc|cccc] => [10cc|cccc|0bcc|cccc]
                const __m256i t2 = _mm256_or_si256 (t1, simdutf_vec(0b1000000000000000));

                // [aaaa|bbbb|bbcc|cccc] =>  [0000|aaaa|bbbb|bbcc]
                const __m256i s0 = _mm256_srli_epi16(in_16, 4);
                // [0000|aaaa|bbbb|bbcc] => [0000|aaaa|bbbb|bb00]
                const __m256i s1 = _mm256_and_si256(s0, simdutf_vec(0b0000111111111100));
                // [0000|aaaa|bbbb|bb00] => [00bb|bbbb|0000|aaaa]
                const __m256i s2 = _mm256_maddubs_epi16(s1, simdutf_vec(0x0140));
                // [00bb|bbbb|0000|aaaa] => [11bb|bbbb|1110|aaaa]
                const __m256i s3 = _mm256_or_si256(s2, simdutf_vec(0b1100000011100000));
                const __m256i m0 = _mm256_andnot_si256(one_or_two_bytes_bytemask, simdutf_vec(0b0100000000000000));
                const __m256i s4 = _mm256_xor_si256(s3, m0);
#undef simdutf_vec

                // 4. expand code units 16-bit => 32-bit
                const __m256i out0 = _mm256_unpacklo_epi16(t2, s4);
                const __m256i out1 = _mm256_unpackhi_epi16(t2, s4);

                // 5. compress 32-bit code units into 1, 2 or 3 bytes -- 2 x shuffle
                const uint32_t mask = (one_byte_bitmask & 0x55555555) |
                                      (one_or_two_bytes_bitmask & 0xaaaaaaaa);
                // Due to the wider registers, the following path is less likely to be useful.
                /*if(mask == 0) {
                  // We only have three-byte code units. Use fast path.
                  const __m256i shuffle = _mm256_setr_epi8(2,3,1,6,7,5,10,11,9,14,15,13,-1,-1,-1,-1, 2,3,1,6,7,5,10,11,9,14,15,13,-1,-1,-1,-1);
                  const __m256i utf8_0 = _mm256_shuffle_epi8(out0, shuffle);
                  const __m256i utf8_1 = _mm256_shuffle_epi8(out1, shuffle);
                  _mm_storeu_si128((__m128i*)utf8_output, _mm256_castsi256_si128(utf8_0));
                  utf8_output += 12;
                  _mm_storeu_si128((__m128i*)utf8_output, _mm256_castsi256_si128(utf8_1));
                  utf8_output += 12;
                  _mm_storeu_si128((__m128i*)utf8_output, _mm256_extractf128_si256(utf8_0,1));
                  utf8_output += 12;
                  _mm_storeu_si128((__m128i*)utf8_output, _mm256_extractf128_si256(utf8_1,1));
                  utf8_output += 12;
                  buf += 16;
                  continue;
                }*/
                const uint8_t mask0 = uint8_t(mask);
                const uint8_t* row0 = &utf16_to_utf8::pack_1_2_3_utf8_bytes[mask0][0];
                const __m128i shuffle0 = _mm_loadu_si128((__m128i*)(row0 + 1));
                const __m128i utf8_0 = _mm_shuffle_epi8(_mm256_castsi256_si128(out0), shuffle0);

                const uint8_t mask1 = static_cast<uint8_t>(mask >> 8);
                const uint8_t* row1 = &utf16_to_utf8::pack_1_2_3_utf8_bytes[mask1][0];
                const __m128i shuffle1 = _mm_loadu_si128((__m128i*)(row1 + 1));
                const __m128i utf8_1 = _mm_shuffle_epi8(_mm256_castsi256_si128(out1), shuffle1);

                const uint8_t mask2 = static_cast<uint8_t>(mask >> 16);
                const uint8_t* row2 = &utf16_to_utf8::pack_1_2_3_utf8_bytes[mask2][0];
                const __m128i shuffle2 = _mm_loadu_si128((__m128i*)(row2 + 1));
                const __m128i utf8_2 = _mm_shuffle_epi8(_mm256_extractf128_si256(out0,1), shuffle2);


                const uint8_t mask3 = static_cast<uint8_t>(mask >> 24);
                const uint8_t* row3 = &utf16_to_utf8::pack_1_2_3_utf8_bytes[mask3][0];
                const __m128i shuffle3 = _mm_loadu_si128((__m128i*)(row3 + 1));
                const __m128i utf8_3 = _mm_shuffle_epi8(_mm256_extractf128_si256(out1,1), shuffle3);

                _mm_storeu_si128((__m128i*)utf8_output, utf8_0);
                utf8_output += row0[0];
                _mm_storeu_si128((__m128i*)utf8_output, utf8_1);
                utf8_output += row1[0];
                _mm_storeu_si128((__m128i*)utf8_output, utf8_2);
                utf8_output += row2[0];
                _mm_storeu_si128((__m128i*)utf8_output, utf8_3);
                utf8_output += row3[0];
                buf += 16;
            } else {
                // case: at least one 32-bit word is larger than 0xFFFF <=> it will produce four UTF-8 bytes.
                // Let us do a scalar fallback.
                // It may seem wasteful to use scalar code, but being efficient with SIMD
                // may require large, non-trivial tables?
                size_t forward = 15;
                size_t k = 0;
                if(size_t(end - buf) < forward + 1) { forward = size_t(end - buf - 1);}
                for(; k < forward; k++) {
                    uint32_t word = buf[k];
                    if((word & 0xFFFFFF80)==0) {  // 1-byte (ASCII)
                        *utf8_output++ = char(word);
                    } else if((word & 0xFFFFF800)==0) { // 2-byte
                        *utf8_output++ = char((word>>6) | 0b11000000);
                        *utf8_output++ = char((word & 0b111111) | 0b10000000);
                    } else if((word & 0xFFFF0000 )==0) {  // 3-byte
                        if (word >= 0xD800 && word <= 0xDFFF) { return std::make_pair(nullptr, utf8_output); }
                        *utf8_output++ = char((word>>12) | 0b11100000);
                        *utf8_output++ = char(((word>>6) & 0b111111) | 0b10000000);
                        *utf8_output++ = char((word & 0b111111) | 0b10000000);
                    } else {  // 4-byte
                        if (word > 0x10FFFF) { return std::make_pair(nullptr, utf8_output); }
                        *utf8_output++ = char((word>>18) | 0b11110000);
                        *utf8_output++ = char(((word>>12) & 0b111111) | 0b10000000);
                        *utf8_output++ = char(((word>>6) & 0b111111) | 0b10000000);
                        *utf8_output++ = char((word & 0b111111) | 0b10000000);
                    }
                }
                buf += k;
            }
        } // while

        // check for invalid input
        const __m256i v_10ffff = _mm256_set1_epi32((uint32_t)0x10ffff);
        if(static_cast<uint32_t>(_mm256_movemask_epi8(_mm256_cmpeq_epi32(_mm256_max_epu32(running_max, v_10ffff), v_10ffff))) != 0xffffffff) {
            return std::make_pair(nullptr, utf8_output);
        }

        if (static_cast<uint32_t>(_mm256_movemask_epi8(forbidden_bytemask)) != 0) { return std::make_pair(nullptr, utf8_output); }

        return std::make_pair(buf, utf8_output);
    }


    inline std::pair<UnicodeResult, char*> avx2_convert_utf32_to_utf8_with_errors(const char32_t* buf, size_t len, char* utf8_output) {
        const char32_t* end = buf + len;
        const char32_t* start = buf;

        const __m256i v_0000 = _mm256_setzero_si256();
        const __m256i v_ffff0000 = _mm256_set1_epi32((uint32_t)0xffff0000);
        const __m256i v_ff80 = _mm256_set1_epi16((uint16_t)0xff80);
        const __m256i v_f800 = _mm256_set1_epi16((uint16_t)0xf800);
        const __m256i v_c080 = _mm256_set1_epi16((uint16_t)0xc080);
        const __m256i v_7fffffff = _mm256_set1_epi32((uint32_t)0x7fffffff);
        const __m256i v_10ffff = _mm256_set1_epi32((uint32_t)0x10ffff);

        const size_t safety_margin = 12; // to avoid overruns, see issue https://github.com/simdutf/simdutf/issues/92

        while (buf + 16 + safety_margin <= end) {
            __m256i in = _mm256_loadu_si256((__m256i*)buf);
            __m256i nextin = _mm256_loadu_si256((__m256i*)buf+1);
            // Check for too large input
            const __m256i max_input = _mm256_max_epu32(_mm256_max_epu32(in, nextin), v_10ffff);
            if(static_cast<uint32_t>(_mm256_movemask_epi8(_mm256_cmpeq_epi32(max_input, v_10ffff))) != 0xffffffff) {
                return std::make_pair(UnicodeResult(UnicodeError::TOO_LARGE, buf - start), utf8_output);
            }

            // Pack 32-bit UTF-32 code units to 16-bit UTF-16 code units with unsigned saturation
            __m256i in_16 = _mm256_packus_epi32(_mm256_and_si256(in, v_7fffffff), _mm256_and_si256(nextin, v_7fffffff));
            in_16 = _mm256_permute4x64_epi64(in_16, 0b11011000);

            // Try to apply UTF-16 => UTF-8 routine on 256 bits (haswell/avx2_convert_utf16_to_utf8.cpp)

            if(_mm256_testz_si256(in_16, v_ff80)) { // ASCII fast path!!!!
                // 1. pack the bytes
                const __m128i utf8_packed = _mm_packus_epi16(_mm256_castsi256_si128(in_16),_mm256_extractf128_si256(in_16,1));
                // 2. store (16 bytes)
                _mm_storeu_si128((__m128i*)utf8_output, utf8_packed);
                // 3. adjust pointers
                buf += 16;
                utf8_output += 16;
                continue; // we are done for this round!
            }
            // no bits set above 7th bit
            const __m256i one_byte_bytemask = _mm256_cmpeq_epi16(_mm256_and_si256(in_16, v_ff80), v_0000);
            const uint32_t one_byte_bitmask = static_cast<uint32_t>(_mm256_movemask_epi8(one_byte_bytemask));

            // no bits set above 11th bit
            const __m256i one_or_two_bytes_bytemask = _mm256_cmpeq_epi16(_mm256_and_si256(in_16, v_f800), v_0000);
            const uint32_t one_or_two_bytes_bitmask = static_cast<uint32_t>(_mm256_movemask_epi8(one_or_two_bytes_bytemask));
            if (one_or_two_bytes_bitmask == 0xffffffff) {
                // 1. prepare 2-byte values
                // input 16-bit word : [0000|0aaa|aabb|bbbb] x 8
                // expected output   : [110a|aaaa|10bb|bbbb] x 8
                const __m256i v_1f00 = _mm256_set1_epi16((int16_t)0x1f00);
                const __m256i v_003f = _mm256_set1_epi16((int16_t)0x003f);

                // t0 = [000a|aaaa|bbbb|bb00]
                const __m256i t0 = _mm256_slli_epi16(in_16, 2);
                // t1 = [000a|aaaa|0000|0000]
                const __m256i t1 = _mm256_and_si256(t0, v_1f00);
                // t2 = [0000|0000|00bb|bbbb]
                const __m256i t2 = _mm256_and_si256(in_16, v_003f);
                // t3 = [000a|aaaa|00bb|bbbb]
                const __m256i t3 = _mm256_or_si256(t1, t2);
                // t4 = [110a|aaaa|10bb|bbbb]
                const __m256i t4 = _mm256_or_si256(t3, v_c080);

                // 2. merge ASCII and 2-byte codewords
                const __m256i utf8_unpacked = _mm256_blendv_epi8(t4, in_16, one_byte_bytemask);

                // 3. prepare bitmask for 8-bit lookup
                const uint32_t M0 = one_byte_bitmask & 0x55555555;
                const uint32_t M1 = M0 >> 7;
                const uint32_t M2 = (M1 | M0)  & 0x00ff00ff;
                // 4. pack the bytes

                const uint8_t* row = &utf16_to_utf8::pack_1_2_utf8_bytes[uint8_t(M2)][0];
                const uint8_t* row_2 = &utf16_to_utf8::pack_1_2_utf8_bytes[uint8_t(M2>>16)][0];

                const __m128i shuffle = _mm_loadu_si128((__m128i*)(row + 1));
                const __m128i shuffle_2 = _mm_loadu_si128((__m128i*)(row_2 + 1));

                const __m256i utf8_packed = _mm256_shuffle_epi8(utf8_unpacked, _mm256_setr_m128i(shuffle,shuffle_2));
                // 5. store bytes
                _mm_storeu_si128((__m128i*)utf8_output, _mm256_castsi256_si128(utf8_packed));
                utf8_output += row[0];
                _mm_storeu_si128((__m128i*)utf8_output, _mm256_extractf128_si256(utf8_packed,1));
                utf8_output += row_2[0];

                // 6. adjust pointers
                buf += 16;
                continue;
            }
            // Must check for overflow in packing
            const __m256i saturation_bytemask = _mm256_cmpeq_epi32(_mm256_and_si256(_mm256_or_si256(in, nextin), v_ffff0000), v_0000);
            const uint32_t saturation_bitmask = static_cast<uint32_t>(_mm256_movemask_epi8(saturation_bytemask));
            if (saturation_bitmask == 0xffffffff) {
                // case: code units from register produce either 1, 2 or 3 UTF-8 bytes

                // Check for illegal surrogate code units
                const __m256i v_d800 = _mm256_set1_epi16((uint16_t)0xd800);
                const __m256i forbidden_bytemask = _mm256_cmpeq_epi16(_mm256_and_si256(in_16, v_f800), v_d800);
                if (static_cast<uint32_t>(_mm256_movemask_epi8(forbidden_bytemask)) != 0x0) {
                    return std::make_pair(UnicodeResult(UnicodeError::SURROGATE, buf - start), utf8_output);
                }

                const __m256i dup_even = _mm256_setr_epi16(0x0000, 0x0202, 0x0404, 0x0606,
                                                           0x0808, 0x0a0a, 0x0c0c, 0x0e0e,
                                                           0x0000, 0x0202, 0x0404, 0x0606,
                                                           0x0808, 0x0a0a, 0x0c0c, 0x0e0e);

                /* In this branch we handle three cases:
                  1. [0000|0000|0ccc|cccc] => [0ccc|cccc]                           - single UFT-8 byte
                  2. [0000|0bbb|bbcc|cccc] => [110b|bbbb], [10cc|cccc]              - two UTF-8 bytes
                  3. [aaaa|bbbb|bbcc|cccc] => [1110|aaaa], [10bb|bbbb], [10cc|cccc] - three UTF-8 bytes

                  We expand the input word (16-bit) into two code units (32-bit), thus
                  we have room for four bytes. However, we need five distinct bit
                  layouts. Note that the last byte in cases #2 and #3 is the same.

                  We precompute byte 1 for case #1 and the common byte for cases #2 & #3
                  in register t2.

                  We precompute byte 1 for case #3 and -- **conditionally** -- precompute
                  either byte 1 for case #2 or byte 2 for case #3. Note that they
                  differ by exactly one bit.

                  Finally from these two code units we build proper UTF-8 sequence, taking
                  into account the case (i.e, the number of bytes to write).
                */
                /**
                 * Given [aaaa|bbbb|bbcc|cccc] our goal is to produce:
                 * t2 => [0ccc|cccc] [10cc|cccc]
                 * s4 => [1110|aaaa] ([110b|bbbb] OR [10bb|bbbb])
                 */
#define simdutf_vec(x) _mm256_set1_epi16(static_cast<uint16_t>(x))
                // [aaaa|bbbb|bbcc|cccc] => [bbcc|cccc|bbcc|cccc]
                const __m256i t0 = _mm256_shuffle_epi8(in_16, dup_even);
                // [bbcc|cccc|bbcc|cccc] => [00cc|cccc|0bcc|cccc]
                const __m256i t1 = _mm256_and_si256(t0, simdutf_vec(0b0011111101111111));
                // [00cc|cccc|0bcc|cccc] => [10cc|cccc|0bcc|cccc]
                const __m256i t2 = _mm256_or_si256 (t1, simdutf_vec(0b1000000000000000));

                // [aaaa|bbbb|bbcc|cccc] =>  [0000|aaaa|bbbb|bbcc]
                const __m256i s0 = _mm256_srli_epi16(in_16, 4);
                // [0000|aaaa|bbbb|bbcc] => [0000|aaaa|bbbb|bb00]
                const __m256i s1 = _mm256_and_si256(s0, simdutf_vec(0b0000111111111100));
                // [0000|aaaa|bbbb|bb00] => [00bb|bbbb|0000|aaaa]
                const __m256i s2 = _mm256_maddubs_epi16(s1, simdutf_vec(0x0140));
                // [00bb|bbbb|0000|aaaa] => [11bb|bbbb|1110|aaaa]
                const __m256i s3 = _mm256_or_si256(s2, simdutf_vec(0b1100000011100000));
                const __m256i m0 = _mm256_andnot_si256(one_or_two_bytes_bytemask, simdutf_vec(0b0100000000000000));
                const __m256i s4 = _mm256_xor_si256(s3, m0);
#undef simdutf_vec

                // 4. expand code units 16-bit => 32-bit
                const __m256i out0 = _mm256_unpacklo_epi16(t2, s4);
                const __m256i out1 = _mm256_unpackhi_epi16(t2, s4);

                // 5. compress 32-bit code units into 1, 2 or 3 bytes -- 2 x shuffle
                const uint32_t mask = (one_byte_bitmask & 0x55555555) |
                                      (one_or_two_bytes_bitmask & 0xaaaaaaaa);
                // Due to the wider registers, the following path is less likely to be useful.
                /*if(mask == 0) {
                  // We only have three-byte code units. Use fast path.
                  const __m256i shuffle = _mm256_setr_epi8(2,3,1,6,7,5,10,11,9,14,15,13,-1,-1,-1,-1, 2,3,1,6,7,5,10,11,9,14,15,13,-1,-1,-1,-1);
                  const __m256i utf8_0 = _mm256_shuffle_epi8(out0, shuffle);
                  const __m256i utf8_1 = _mm256_shuffle_epi8(out1, shuffle);
                  _mm_storeu_si128((__m128i*)utf8_output, _mm256_castsi256_si128(utf8_0));
                  utf8_output += 12;
                  _mm_storeu_si128((__m128i*)utf8_output, _mm256_castsi256_si128(utf8_1));
                  utf8_output += 12;
                  _mm_storeu_si128((__m128i*)utf8_output, _mm256_extractf128_si256(utf8_0,1));
                  utf8_output += 12;
                  _mm_storeu_si128((__m128i*)utf8_output, _mm256_extractf128_si256(utf8_1,1));
                  utf8_output += 12;
                  buf += 16;
                  continue;
                }*/
                const uint8_t mask0 = uint8_t(mask);
                const uint8_t* row0 = &utf16_to_utf8::pack_1_2_3_utf8_bytes[mask0][0];
                const __m128i shuffle0 = _mm_loadu_si128((__m128i*)(row0 + 1));
                const __m128i utf8_0 = _mm_shuffle_epi8(_mm256_castsi256_si128(out0), shuffle0);

                const uint8_t mask1 = static_cast<uint8_t>(mask >> 8);
                const uint8_t* row1 = &utf16_to_utf8::pack_1_2_3_utf8_bytes[mask1][0];
                const __m128i shuffle1 = _mm_loadu_si128((__m128i*)(row1 + 1));
                const __m128i utf8_1 = _mm_shuffle_epi8(_mm256_castsi256_si128(out1), shuffle1);

                const uint8_t mask2 = static_cast<uint8_t>(mask >> 16);
                const uint8_t* row2 = &utf16_to_utf8::pack_1_2_3_utf8_bytes[mask2][0];
                const __m128i shuffle2 = _mm_loadu_si128((__m128i*)(row2 + 1));
                const __m128i utf8_2 = _mm_shuffle_epi8(_mm256_extractf128_si256(out0,1), shuffle2);


                const uint8_t mask3 = static_cast<uint8_t>(mask >> 24);
                const uint8_t* row3 = &utf16_to_utf8::pack_1_2_3_utf8_bytes[mask3][0];
                const __m128i shuffle3 = _mm_loadu_si128((__m128i*)(row3 + 1));
                const __m128i utf8_3 = _mm_shuffle_epi8(_mm256_extractf128_si256(out1,1), shuffle3);

                _mm_storeu_si128((__m128i*)utf8_output, utf8_0);
                utf8_output += row0[0];
                _mm_storeu_si128((__m128i*)utf8_output, utf8_1);
                utf8_output += row1[0];
                _mm_storeu_si128((__m128i*)utf8_output, utf8_2);
                utf8_output += row2[0];
                _mm_storeu_si128((__m128i*)utf8_output, utf8_3);
                utf8_output += row3[0];
                buf += 16;
            } else {
                // case: at least one 32-bit word is larger than 0xFFFF <=> it will produce four UTF-8 bytes.
                // Let us do a scalar fallback.
                // It may seem wasteful to use scalar code, but being efficient with SIMD
                // may require large, non-trivial tables?
                size_t forward = 15;
                size_t k = 0;
                if(size_t(end - buf) < forward + 1) { forward = size_t(end - buf - 1);}
                for(; k < forward; k++) {
                    uint32_t word = buf[k];
                    if((word & 0xFFFFFF80)==0) {  // 1-byte (ASCII)
                        *utf8_output++ = char(word);
                    } else if((word & 0xFFFFF800)==0) { // 2-byte
                        *utf8_output++ = char((word>>6) | 0b11000000);
                        *utf8_output++ = char((word & 0b111111) | 0b10000000);
                    } else if((word & 0xFFFF0000 )==0) {  // 3-byte
                        if (word >= 0xD800 && word <= 0xDFFF) { return std::make_pair(UnicodeResult(UnicodeError::SURROGATE, buf - start + k), utf8_output); }
                        *utf8_output++ = char((word>>12) | 0b11100000);
                        *utf8_output++ = char(((word>>6) & 0b111111) | 0b10000000);
                        *utf8_output++ = char((word & 0b111111) | 0b10000000);
                    } else {  // 4-byte
                        if (word > 0x10FFFF) { return std::make_pair(UnicodeResult(UnicodeError::TOO_LARGE, buf - start + k), utf8_output); }
                        *utf8_output++ = char((word>>18) | 0b11110000);
                        *utf8_output++ = char(((word>>12) & 0b111111) | 0b10000000);
                        *utf8_output++ = char(((word>>6) & 0b111111) | 0b10000000);
                        *utf8_output++ = char((word & 0b111111) | 0b10000000);
                    }
                }
                buf += k;
            }
        } // while

        return std::make_pair(UnicodeResult(UnicodeError::SUCCESS, buf - start), utf8_output);
    }

    template <EndianNess big_endian>
    inline std::pair<const char32_t*, char16_t*> avx2_convert_utf32_to_utf16(const char32_t* buf, size_t len, char16_t* utf16_output) {
        const char32_t* end = buf + len;

        const size_t safety_margin = 12; // to avoid overruns, see issue https://github.com/simdutf/simdutf/issues/92
        __m256i forbidden_bytemask = _mm256_setzero_si256();


        while (buf + 8 + safety_margin <= end) {
            __m256i in = _mm256_loadu_si256((__m256i*)buf);

            const __m256i v_00000000 = _mm256_setzero_si256();
            const __m256i v_ffff0000 = _mm256_set1_epi32((int32_t)0xffff0000);

            // no bits set above 16th bit <=> can pack to UTF16 without surrogate pairs
            const __m256i saturation_bytemask = _mm256_cmpeq_epi32(_mm256_and_si256(in, v_ffff0000), v_00000000);
            const uint32_t saturation_bitmask = static_cast<uint32_t>(_mm256_movemask_epi8(saturation_bytemask));

            if (saturation_bitmask == 0xffffffff) {
                const __m256i v_f800 = _mm256_set1_epi32((uint32_t)0xf800);
                const __m256i v_d800 = _mm256_set1_epi32((uint32_t)0xd800);
                forbidden_bytemask = _mm256_or_si256(forbidden_bytemask, _mm256_cmpeq_epi32(_mm256_and_si256(in, v_f800), v_d800));

                __m128i utf16_packed = _mm_packus_epi32(_mm256_castsi256_si128(in),_mm256_extractf128_si256(in,1));
                if (turbo::is_big_endian(big_endian)) {
                    const __m128i swap = _mm_setr_epi8(1, 0, 3, 2, 5, 4, 7, 6, 9, 8, 11, 10, 13, 12, 15, 14);
                    utf16_packed = _mm_shuffle_epi8(utf16_packed, swap);
                }
                _mm_storeu_si128((__m128i*)utf16_output, utf16_packed);
                utf16_output += 8;
                buf += 8;
            } else {
                size_t forward = 7;
                size_t k = 0;
                if(size_t(end - buf) < forward + 1) { forward = size_t(end - buf - 1);}
                for(; k < forward; k++) {
                    uint32_t word = buf[k];
                    if((word & 0xFFFF0000)==0) {
                        // will not generate a surrogate pair
                        if (word >= 0xD800 && word <= 0xDFFF) { return std::make_pair(nullptr, utf16_output); }
                        *utf16_output++ = turbo::is_big_endian(big_endian) ? char16_t((uint16_t(word) >> 8) | (uint16_t(word) << 8)) : char16_t(word);
                    } else {
                        // will generate a surrogate pair
                        if (word > 0x10FFFF) { return std::make_pair(nullptr, utf16_output); }
                        word -= 0x10000;
                        uint16_t high_surrogate = uint16_t(0xD800 + (word >> 10));
                        uint16_t low_surrogate = uint16_t(0xDC00 + (word & 0x3FF));
                        if (turbo::is_big_endian(big_endian)) {
                            high_surrogate = uint16_t((high_surrogate >> 8) | (high_surrogate << 8));
                            low_surrogate = uint16_t((low_surrogate >> 8) | (low_surrogate << 8));
                        }
                        *utf16_output++ = char16_t(high_surrogate);
                        *utf16_output++ = char16_t(low_surrogate);
                    }
                }
                buf += k;
            }
        }

        // check for invalid input
        if (static_cast<uint32_t>(_mm256_movemask_epi8(forbidden_bytemask)) != 0) { return std::make_pair(nullptr, utf16_output); }

        return std::make_pair(buf, utf16_output);
    }


    template <EndianNess big_endian>
    std::pair<UnicodeResult, char16_t*> avx2_convert_utf32_to_utf16_with_errors(const char32_t* buf, size_t len, char16_t* utf16_output) {
        const char32_t* start = buf;
        const char32_t* end = buf + len;

        const size_t safety_margin = 12; // to avoid overruns, see issue https://github.com/simdutf/simdutf/issues/92

        while (buf + 8 + safety_margin <= end) {
            __m256i in = _mm256_loadu_si256((__m256i*)buf);

            const __m256i v_00000000 = _mm256_setzero_si256();
            const __m256i v_ffff0000 = _mm256_set1_epi32((int32_t)0xffff0000);

            // no bits set above 16th bit <=> can pack to UTF16 without surrogate pairs
            const __m256i saturation_bytemask = _mm256_cmpeq_epi32(_mm256_and_si256(in, v_ffff0000), v_00000000);
            const uint32_t saturation_bitmask = static_cast<uint32_t>(_mm256_movemask_epi8(saturation_bytemask));

            if (saturation_bitmask == 0xffffffff) {
                const __m256i v_f800 = _mm256_set1_epi32((uint32_t)0xf800);
                const __m256i v_d800 = _mm256_set1_epi32((uint32_t)0xd800);
                const __m256i forbidden_bytemask = _mm256_cmpeq_epi32(_mm256_and_si256(in, v_f800), v_d800);
                if (static_cast<uint32_t>(_mm256_movemask_epi8(forbidden_bytemask)) != 0x0) {
                    return std::make_pair(UnicodeResult(UnicodeError::SURROGATE, buf - start), utf16_output);
                }

                __m128i utf16_packed = _mm_packus_epi32(_mm256_castsi256_si128(in),_mm256_extractf128_si256(in,1));
                if (turbo::is_big_endian(big_endian)) {
                    const __m128i swap = _mm_setr_epi8(1, 0, 3, 2, 5, 4, 7, 6, 9, 8, 11, 10, 13, 12, 15, 14);
                    utf16_packed = _mm_shuffle_epi8(utf16_packed, swap);
                }
                _mm_storeu_si128((__m128i*)utf16_output, utf16_packed);
                utf16_output += 8;
                buf += 8;
            } else {
                size_t forward = 7;
                size_t k = 0;
                if(size_t(end - buf) < forward + 1) { forward = size_t(end - buf - 1);}
                for(; k < forward; k++) {
                    uint32_t word = buf[k];
                    if((word & 0xFFFF0000)==0) {
                        // will not generate a surrogate pair
                        if (word >= 0xD800 && word <= 0xDFFF) { return std::make_pair(UnicodeResult(UnicodeError::SURROGATE, buf - start + k), utf16_output); }
                        *utf16_output++ = turbo::is_big_endian(big_endian) ? char16_t((uint16_t(word) >> 8) | (uint16_t(word) << 8)) : char16_t(word);
                    } else {
                        // will generate a surrogate pair
                        if (word > 0x10FFFF) { return std::make_pair(UnicodeResult(UnicodeError::TOO_LARGE, buf - start + k), utf16_output); }
                        word -= 0x10000;
                        uint16_t high_surrogate = uint16_t(0xD800 + (word >> 10));
                        uint16_t low_surrogate = uint16_t(0xDC00 + (word & 0x3FF));
                        if (turbo::is_big_endian(big_endian)) {
                            high_surrogate = uint16_t((high_surrogate >> 8) | (high_surrogate << 8));
                            low_surrogate = uint16_t((low_surrogate >> 8) | (low_surrogate << 8));
                        }
                        *utf16_output++ = char16_t(high_surrogate);
                        *utf16_output++ = char16_t(low_surrogate);
                    }
                }
                buf += k;
            }
        }

        return std::make_pair(UnicodeResult(UnicodeError::SUCCESS, buf - start), utf16_output);
    }
    
}  // namespace turbo::unicode::simd
#endif  // TURBO_UNICODE_AVX2_UTF32_H_
