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
//
// Created by jeff on 23-12-7.
//

#ifndef TURBO_UNICODE_SCALAR_UTF8_CONVERT_H_
#define TURBO_UNICODE_SCALAR_UTF8_CONVERT_H_


#include <cstddef>
#include <cstdint>
#include <cstring>
#include "turbo/unicode/encoding_types.h"
#include "turbo/unicode/error.h"
#include "turbo/base/endian.h"

namespace turbo::unicode::utf8_to_utf16 {


    template<EndianNess big_endian>
    inline size_t convert(const char *buf, size_t len, char16_t *utf16_output) {
        const uint8_t *data = reinterpret_cast<const uint8_t *>(buf);
        size_t pos = 0;
        char16_t *start{utf16_output};
        while (pos < len) {
            // try to convert the next block of 16 ASCII bytes
            if (pos + 16 <= len) { // if it is safe to read 16 more bytes, check that they are ascii
                uint64_t v1;
                ::memcpy(&v1, data + pos, sizeof(uint64_t));
                uint64_t v2;
                ::memcpy(&v2, data + pos + sizeof(uint64_t), sizeof(uint64_t));
                uint64_t v{v1 | v2};
                if ((v & 0x8080808080808080) == 0) {
                    size_t final_pos = pos + 16;
                    while (pos < final_pos) {
                        *utf16_output++ = !match_system(big_endian) ? char16_t(gbswap_16(buf[pos])) : char16_t(
                                buf[pos]);
                        pos++;
                    }
                    continue;
                }
            }

            uint8_t leading_byte = data[pos]; // leading byte
            if (leading_byte < 0b10000000) {
                // converting one ASCII byte !!!
                *utf16_output++ = !match_system(big_endian) ? char16_t(gbswap_16(leading_byte)) : char16_t(
                        leading_byte);
                pos++;
            } else if ((leading_byte & 0b11100000) == 0b11000000) {
                // We have a two-byte UTF-8, it should become
                // a single UTF-16 word.
                if (pos + 1 >= len) { return 0; } // minimal bound checking
                if ((data[pos + 1] & 0b11000000) != 0b10000000) { return 0; }
                // range check
                uint32_t code_point = (leading_byte & 0b00011111) << 6 | (data[pos + 1] & 0b00111111);
                if (code_point < 0x80 || 0x7ff < code_point) { return 0; }
                if (!match_system(big_endian)) {
                    code_point = uint32_t(gbswap_16(uint16_t(code_point)));
                }
                *utf16_output++ = char16_t(code_point);
                pos += 2;
            } else if ((leading_byte & 0b11110000) == 0b11100000) {
                // We have a three-byte UTF-8, it should become
                // a single UTF-16 word.
                if (pos + 2 >= len) { return 0; } // minimal bound checking

                if ((data[pos + 1] & 0b11000000) != 0b10000000) { return 0; }
                if ((data[pos + 2] & 0b11000000) != 0b10000000) { return 0; }
                // range check
                uint32_t code_point = (leading_byte & 0b00001111) << 12 |
                                      (data[pos + 1] & 0b00111111) << 6 |
                                      (data[pos + 2] & 0b00111111);
                if (code_point < 0x800 || 0xffff < code_point ||
                    (0xd7ff < code_point && code_point < 0xe000)) {
                    return 0;
                }
                if (!match_system(big_endian)) {
                    code_point = uint32_t(gbswap_16(uint16_t(code_point)));
                }
                *utf16_output++ = char16_t(code_point);
                pos += 3;
            } else if ((leading_byte & 0b11111000) == 0b11110000) { // 0b11110000
                // we have a 4-byte UTF-8 word.
                if (pos + 3 >= len) { return 0; } // minimal bound checking
                if ((data[pos + 1] & 0b11000000) != 0b10000000) { return 0; }
                if ((data[pos + 2] & 0b11000000) != 0b10000000) { return 0; }
                if ((data[pos + 3] & 0b11000000) != 0b10000000) { return 0; }

                // range check
                uint32_t code_point =
                        (leading_byte & 0b00000111) << 18 | (data[pos + 1] & 0b00111111) << 12 |
                        (data[pos + 2] & 0b00111111) << 6 | (data[pos + 3] & 0b00111111);
                if (code_point <= 0xffff || 0x10ffff < code_point) { return 0; }
                code_point -= 0x10000;
                uint16_t high_surrogate = uint16_t(0xD800 + (code_point >> 10));
                uint16_t low_surrogate = uint16_t(0xDC00 + (code_point & 0x3FF));
                if (!match_system(big_endian)) {
                    high_surrogate = gbswap_16(high_surrogate);
                    low_surrogate = gbswap_16(low_surrogate);
                }
                *utf16_output++ = char16_t(high_surrogate);
                *utf16_output++ = char16_t(low_surrogate);
                pos += 4;
            } else {
                return 0;
            }
        }
        return utf16_output - start;
    }

    template<EndianNess big_endian>
    inline UnicodeResult convert_with_errors(const char *buf, size_t len, char16_t *utf16_output) {
        const uint8_t *data = reinterpret_cast<const uint8_t *>(buf);
        size_t pos = 0;
        char16_t *start{utf16_output};
        while (pos < len) {
            // try to convert the next block of 16 ASCII bytes
            if (pos + 16 <= len) { // if it is safe to read 16 more bytes, check that they are ascii
                uint64_t v1;
                ::memcpy(&v1, data + pos, sizeof(uint64_t));
                uint64_t v2;
                ::memcpy(&v2, data + pos + sizeof(uint64_t), sizeof(uint64_t));
                uint64_t v{v1 | v2};
                if ((v & 0x8080808080808080) == 0) {
                    size_t final_pos = pos + 16;
                    while (pos < final_pos) {
                        *utf16_output++ = !match_system(big_endian) ? char16_t(gbswap_16(buf[pos])) : char16_t(
                                buf[pos]);
                        pos++;
                    }
                    continue;
                }
            }
            uint8_t leading_byte = data[pos]; // leading byte
            if (leading_byte < 0b10000000) {
                // converting one ASCII byte !!!
                *utf16_output++ = !match_system(big_endian) ? char16_t(gbswap_16(leading_byte)) : char16_t(
                        leading_byte);
                pos++;
            } else if ((leading_byte & 0b11100000) == 0b11000000) {
                // We have a two-byte UTF-8, it should become
                // a single UTF-16 word.
                if (pos + 1 >= len) { return UnicodeResult{UnicodeError::TOO_SHORT, pos}; } // minimal bound checking
                if ((data[pos + 1] & 0b11000000) != 0b10000000) { return UnicodeResult(UnicodeError::TOO_SHORT, pos); }
                // range check
                uint32_t code_point = (leading_byte & 0b00011111) << 6 | (data[pos + 1] & 0b00111111);
                if (code_point < 0x80 || 0x7ff < code_point) { return UnicodeResult(UnicodeError::OVERLONG, pos); }
                if (!match_system(big_endian)) {
                    code_point = uint32_t(gbswap_16(uint16_t(code_point)));
                }
                *utf16_output++ = char16_t(code_point);
                pos += 2;
            } else if ((leading_byte & 0b11110000) == 0b11100000) {
                // We have a three-byte UTF-8, it should become
                // a single UTF-16 word.
                if (pos + 2 >= len) { return {UnicodeError::TOO_SHORT, pos}; } // minimal bound checking

                if ((data[pos + 1] & 0b11000000) != 0b10000000) { return {UnicodeError::TOO_SHORT, pos}; }
                if ((data[pos + 2] & 0b11000000) != 0b10000000) { return {UnicodeError::TOO_SHORT, pos}; }
                // range check
                uint32_t code_point = (leading_byte & 0b00001111) << 12 |
                                      (data[pos + 1] & 0b00111111) << 6 |
                                      (data[pos + 2] & 0b00111111);
                if ((code_point < 0x800) || (0xffff < code_point)) { return {UnicodeError::OVERLONG, pos}; }
                if (0xd7ff < code_point && code_point < 0xe000) { return {UnicodeError::SURROGATE, pos}; }
                if (!match_system(big_endian)) {
                    code_point = uint32_t(gbswap_16(uint16_t(code_point)));
                }
                *utf16_output++ = char16_t(code_point);
                pos += 3;
            } else if ((leading_byte & 0b11111000) == 0b11110000) { // 0b11110000
                // we have a 4-byte UTF-8 word.
                if (pos + 3 >= len) { return {UnicodeError::TOO_SHORT, pos}; } // minimal bound checking
                if ((data[pos + 1] & 0b11000000) != 0b10000000) { return {UnicodeError::TOO_SHORT, pos}; }
                if ((data[pos + 2] & 0b11000000) != 0b10000000) { return {UnicodeError::TOO_SHORT, pos}; }
                if ((data[pos + 3] & 0b11000000) != 0b10000000) { return {UnicodeError::TOO_SHORT, pos}; }

                // range check
                uint32_t code_point =
                        (leading_byte & 0b00000111) << 18 | (data[pos + 1] & 0b00111111) << 12 |
                        (data[pos + 2] & 0b00111111) << 6 | (data[pos + 3] & 0b00111111);
                if (code_point <= 0xffff) { return {UnicodeError::OVERLONG, pos}; }
                if (0x10ffff < code_point) { return {UnicodeError::TOO_LARGE, pos}; }
                code_point -= 0x10000;
                uint16_t high_surrogate = uint16_t(0xD800 + (code_point >> 10));
                uint16_t low_surrogate = uint16_t(0xDC00 + (code_point & 0x3FF));
                if (!match_system(big_endian)) {
                    high_surrogate = gbswap_16(high_surrogate);
                    low_surrogate = gbswap_16(low_surrogate);
                }
                *utf16_output++ = char16_t(high_surrogate);
                *utf16_output++ = char16_t(low_surrogate);
                pos += 4;
            } else {
                // we either have too many continuation bytes or an invalid leading byte
                if ((leading_byte & 0b11000000) == 0b10000000) { return {UnicodeError::TOO_LONG, pos}; }
                else { return {UnicodeError::HEADER_BITS, pos}; }
            }
        }
        return {UnicodeError::SUCCESS, static_cast<size_t>(utf16_output - start)};
    }

    /**
     * When rewind_and_convert_with_errors is called, we are pointing at 'buf' and we have
     * up to len input bytes left, and we encountered some error. It is possible that
     * the error is at 'buf' exactly, but it could also be in the previous bytes  (up to 3 bytes back).
     *
     * prior_bytes indicates how many bytes, prior to 'buf' may belong to the current memory section
     * and can be safely accessed. We prior_bytes to access safely up to three bytes before 'buf'.
     *
     * The caller is responsible to ensure that len > 0.
     *
     * If the error is believed to have occured prior to 'buf', the count value contain in the result
     * will be SIZE_T - 1, SIZE_T - 2, or SIZE_T - 3.
     */
    template<EndianNess endian>
    inline UnicodeResult
    rewind_and_convert_with_errors(size_t prior_bytes, const char *buf, size_t len, char16_t *utf16_output) {
        size_t extra_len{0};
        // We potentially need to go back in time and find a leading byte.
        size_t how_far_back = 3; // 3 bytes in the past + current position
        if (how_far_back >= prior_bytes) { how_far_back = prior_bytes; }
        bool found_leading_bytes{false};
        // important: it is i <= how_far_back and not 'i < how_far_back'.
        for (size_t i = 0; i <= how_far_back; i++) {
            unsigned char byte = buf[-i];
            found_leading_bytes = ((byte & 0b11000000) != 0b10000000);
            if (found_leading_bytes) {
                buf -= i;
                extra_len = i;
                break;
            }
        }
        //
        // It is possible for this function to return a negative count in its result.
        // C++ Standard Section 18.1 defines size_t is in <cstddef> which is described in C Standard as <stddef.h>.
        // C Standard Section 4.1.5 defines size_t as an unsigned integral type of the result of the sizeof operator
        //
        // An unsigned type will simply wrap round arithmetically (well defined).
        //
        if (!found_leading_bytes) {
            // If how_far_back == 3, we may have four consecutive continuation bytes!!!
            // [....] [continuation] [continuation] [continuation] | [buf is continuation]
            // Or we possibly have a stream that does not start with a leading byte.
            return {UnicodeError::TOO_LONG, -how_far_back};
        }
        auto res = convert_with_errors<endian>(buf, len + extra_len, utf16_output);
        if (is_unicode_error(res)) {
            res.count -= extra_len;
        }
        return res;
    }


    template <EndianNess big_endian>
    inline size_t convert_valid(const char* buf, size_t len, char16_t* utf16_output) {
        const uint8_t *data = reinterpret_cast<const uint8_t *>(buf);
        size_t pos = 0;
        char16_t* start{utf16_output};
        while (pos < len) {
            // try to convert the next block of 8 ASCII bytes
            if (pos + 8 <= len) { // if it is safe to read 8 more bytes, check that they are ascii
                uint64_t v;
                ::memcpy(&v, data + pos, sizeof(uint64_t));
                if ((v & 0x8080808080808080) == 0) {
                    size_t final_pos = pos + 8;
                    while(pos < final_pos) {
                        *utf16_output++ = !match_system(big_endian) ? char16_t(gbswap_16(buf[pos])) : char16_t(buf[pos]);
                        pos++;
                    }
                    continue;
                }
            }
            uint8_t leading_byte = data[pos]; // leading byte
            if (leading_byte < 0b10000000) {
                // converting one ASCII byte !!!
                *utf16_output++ = !match_system(big_endian) ? char16_t(gbswap_16(leading_byte)) : char16_t(leading_byte);
                pos++;
            } else if ((leading_byte & 0b11100000) == 0b11000000) {
                // We have a two-byte UTF-8, it should become
                // a single UTF-16 word.
                if(pos + 1 >= len) { break; } // minimal bound checking
                uint16_t code_point = uint16_t(((leading_byte &0b00011111) << 6) | (data[pos + 1] &0b00111111));
                if (!match_system(big_endian)) {
                    code_point = gbswap_16(uint16_t(code_point));
                }
                *utf16_output++ = char16_t(code_point);
                pos += 2;
            } else if ((leading_byte & 0b11110000) == 0b11100000) {
                // We have a three-byte UTF-8, it should become
                // a single UTF-16 word.
                if(pos + 2 >= len) { break; } // minimal bound checking
                uint16_t code_point = uint16_t(((leading_byte &0b00001111) << 12) | ((data[pos + 1] &0b00111111) << 6) | (data[pos + 2] &0b00111111));
                if (!match_system(big_endian)) {
                    code_point = gbswap_16(uint16_t(code_point));
                }
                *utf16_output++ = char16_t(code_point);
                pos += 3;
            } else if ((leading_byte & 0b11111000) == 0b11110000) { // 0b11110000
                // we have a 4-byte UTF-8 word.
                if(pos + 3 >= len) { break; } // minimal bound checking
                uint32_t code_point = ((leading_byte & 0b00000111) << 18 )| ((data[pos + 1] &0b00111111) << 12)
                                      | ((data[pos + 2] &0b00111111) << 6) | (data[pos + 3] &0b00111111);
                code_point -= 0x10000;
                uint16_t high_surrogate = uint16_t(0xD800 + (code_point >> 10));
                uint16_t low_surrogate = uint16_t(0xDC00 + (code_point & 0x3FF));
                if (!match_system(big_endian)) {
                    high_surrogate = gbswap_16(high_surrogate);
                    low_surrogate = gbswap_16(low_surrogate);
                }
                *utf16_output++ = char16_t(high_surrogate);
                *utf16_output++ = char16_t(low_surrogate);
                pos += 4;
            } else {
                // we may have a continuation but we do not do error checking
                return 0;
            }
        }
        return utf16_output - start;
    }

}  // namespace turbo::unicode::utf8_to_utf16

namespace turbo::unicode::utf8_to_utf32 {

    inline size_t convert(const char* buf, size_t len, char32_t* utf32_output) {
        const uint8_t *data = reinterpret_cast<const uint8_t *>(buf);
        size_t pos = 0;
        char32_t* start{utf32_output};
        while (pos < len) {
            // try to convert the next block of 16 ASCII bytes
            if (pos + 16 <= len) { // if it is safe to read 16 more bytes, check that they are ascii
                uint64_t v1;
                ::memcpy(&v1, data + pos, sizeof(uint64_t));
                uint64_t v2;
                ::memcpy(&v2, data + pos + sizeof(uint64_t), sizeof(uint64_t));
                uint64_t v{v1 | v2};
                if ((v & 0x8080808080808080) == 0) {
                    size_t final_pos = pos + 16;
                    while(pos < final_pos) {
                        *utf32_output++ = char32_t(buf[pos]);
                        pos++;
                    }
                    continue;
                }
            }
            uint8_t leading_byte = data[pos]; // leading byte
            if (leading_byte < 0b10000000) {
                // converting one ASCII byte !!!
                *utf32_output++ = char32_t(leading_byte);
                pos++;
            } else if ((leading_byte & 0b11100000) == 0b11000000) {
                // We have a two-byte UTF-8
                if(pos + 1 >= len) { return 0; } // minimal bound checking
                if ((data[pos + 1] & 0b11000000) != 0b10000000) { return 0; }
                // range check
                uint32_t code_point = (leading_byte & 0b00011111) << 6 | (data[pos + 1] & 0b00111111);
                if (code_point < 0x80 || 0x7ff < code_point) { return 0; }
                *utf32_output++ = char32_t(code_point);
                pos += 2;
            } else if ((leading_byte & 0b11110000) == 0b11100000) {
                // We have a three-byte UTF-8
                if(pos + 2 >= len) { return 0; } // minimal bound checking

                if ((data[pos + 1] & 0b11000000) != 0b10000000) { return 0; }
                if ((data[pos + 2] & 0b11000000) != 0b10000000) { return 0; }
                // range check
                uint32_t code_point = (leading_byte & 0b00001111) << 12 |
                                      (data[pos + 1] & 0b00111111) << 6 |
                                      (data[pos + 2] & 0b00111111);
                if (code_point < 0x800 || 0xffff < code_point ||
                    (0xd7ff < code_point && code_point < 0xe000)) {
                    return 0;
                }
                *utf32_output++ = char32_t(code_point);
                pos += 3;
            } else if ((leading_byte & 0b11111000) == 0b11110000) { // 0b11110000
                // we have a 4-byte UTF-8 word.
                if(pos + 3 >= len) { return 0; } // minimal bound checking
                if ((data[pos + 1] & 0b11000000) != 0b10000000) { return 0; }
                if ((data[pos + 2] & 0b11000000) != 0b10000000) { return 0; }
                if ((data[pos + 3] & 0b11000000) != 0b10000000) { return 0; }

                // range check
                uint32_t code_point =
                        (leading_byte & 0b00000111) << 18 | (data[pos + 1] & 0b00111111) << 12 |
                        (data[pos + 2] & 0b00111111) << 6 | (data[pos + 3] & 0b00111111);
                if (code_point <= 0xffff || 0x10ffff < code_point) { return 0; }
                *utf32_output++ = char32_t(code_point);
                pos += 4;
            } else {
                return 0;
            }
        }
        return utf32_output - start;
    }

    inline UnicodeResult convert_with_errors(const char* buf, size_t len, char32_t* utf32_output) {
        const uint8_t *data = reinterpret_cast<const uint8_t *>(buf);
        size_t pos = 0;
        char32_t* start{utf32_output};
        while (pos < len) {
            // try to convert the next block of 16 ASCII bytes
            if (pos + 16 <= len) { // if it is safe to read 16 more bytes, check that they are ascii
                uint64_t v1;
                ::memcpy(&v1, data + pos, sizeof(uint64_t));
                uint64_t v2;
                ::memcpy(&v2, data + pos + sizeof(uint64_t), sizeof(uint64_t));
                uint64_t v{v1 | v2};
                if ((v & 0x8080808080808080) == 0) {
                    size_t final_pos = pos + 16;
                    while(pos < final_pos) {
                        *utf32_output++ = char32_t(buf[pos]);
                        pos++;
                    }
                    continue;
                }
            }
            uint8_t leading_byte = data[pos]; // leading byte
            if (leading_byte < 0b10000000) {
                // converting one ASCII byte !!!
                *utf32_output++ = char32_t(leading_byte);
                pos++;
            } else if ((leading_byte & 0b11100000) == 0b11000000) {
                // We have a two-byte UTF-8
                if(pos + 1 >= len) { return {UnicodeError::TOO_SHORT, pos}; } // minimal bound checking
                if ((data[pos + 1] & 0b11000000) != 0b10000000) { return {UnicodeError::TOO_SHORT, pos}; }
                // range check
                uint32_t code_point = (leading_byte & 0b00011111) << 6 | (data[pos + 1] & 0b00111111);
                if (code_point < 0x80 || 0x7ff < code_point) { return {UnicodeError::OVERLONG, pos}; }
                *utf32_output++ = char32_t(code_point);
                pos += 2;
            } else if ((leading_byte & 0b11110000) == 0b11100000) {
                // We have a three-byte UTF-8
                if(pos + 2 >= len) { return {UnicodeError::TOO_SHORT, pos}; } // minimal bound checking

                if ((data[pos + 1] & 0b11000000) != 0b10000000) { return {UnicodeError::TOO_SHORT, pos}; }
                if ((data[pos + 2] & 0b11000000) != 0b10000000) { return {UnicodeError::TOO_SHORT, pos}; }
                // range check
                uint32_t code_point = (leading_byte & 0b00001111) << 12 |
                                      (data[pos + 1] & 0b00111111) << 6 |
                                      (data[pos + 2] & 0b00111111);
                if (code_point < 0x800 || 0xffff < code_point) { return {UnicodeError::OVERLONG, pos}; }
                if (0xd7ff < code_point && code_point < 0xe000) { return {UnicodeError::SURROGATE, pos}; }
                *utf32_output++ = char32_t(code_point);
                pos += 3;
            } else if ((leading_byte & 0b11111000) == 0b11110000) { // 0b11110000
                // we have a 4-byte UTF-8 word.
                if(pos + 3 >= len) { return {UnicodeError::TOO_SHORT, pos}; } // minimal bound checking
                if ((data[pos + 1] & 0b11000000) != 0b10000000) { return {UnicodeError::TOO_SHORT, pos};}
                if ((data[pos + 2] & 0b11000000) != 0b10000000) { return {UnicodeError::TOO_SHORT, pos}; }
                if ((data[pos + 3] & 0b11000000) != 0b10000000) { return {UnicodeError::TOO_SHORT, pos}; }

                // range check
                uint32_t code_point =
                        (leading_byte & 0b00000111) << 18 | (data[pos + 1] & 0b00111111) << 12 |
                        (data[pos + 2] & 0b00111111) << 6 | (data[pos + 3] & 0b00111111);
                if (code_point <= 0xffff) { return {UnicodeError::OVERLONG, pos}; }
                if (0x10ffff < code_point) { return {UnicodeError::TOO_LARGE, pos}; }
                *utf32_output++ = char32_t(code_point);
                pos += 4;
            } else {
                // we either have too many continuation bytes or an invalid leading byte
                if ((leading_byte & 0b11000000) == 0b10000000) { return {UnicodeError::TOO_LONG, pos}; }
                else { return {UnicodeError::HEADER_BITS, pos}; }
            }
        }
        return {UnicodeError::SUCCESS, static_cast<size_t>(utf32_output - start)};
    }

    /**
     * When rewind_and_convert_with_errors is called, we are pointing at 'buf' and we have
     * up to len input bytes left, and we encountered some error. It is possible that
     * the error is at 'buf' exactly, but it could also be in the previous bytes location (up to 3 bytes back).
     *
     * prior_bytes indicates how many bytes, prior to 'buf' may belong to the current memory section
     * and can be safely accessed. We prior_bytes to access safely up to three bytes before 'buf'.
     *
     * The caller is responsible to ensure that len > 0.
     *
     * If the error is believed to have occured prior to 'buf', the count value contain in the result
     * will be SIZE_T - 1, SIZE_T - 2, or SIZE_T - 3.
     */
    inline UnicodeResult rewind_and_convert_with_errors(size_t prior_bytes, const char* buf, size_t len, char32_t* utf32_output) {
        size_t extra_len{0};
        // We potentially need to go back in time and find a leading byte.
        size_t how_far_back = 3; // 3 bytes in the past + current position
        if(how_far_back > prior_bytes) { how_far_back = prior_bytes; }
        bool found_leading_bytes{false};
        // important: it is i <= how_far_back and not 'i < how_far_back'.
        for(size_t i = 0; i <= how_far_back; i++) {
            unsigned char byte = buf[-i];
            found_leading_bytes = ((byte & 0b11000000) != 0b10000000);
            if(found_leading_bytes) {
                buf -= i;
                extra_len = i;
                break;
            }
        }
        //
        // It is possible for this function to return a negative count in its result.
        // C++ Standard Section 18.1 defines size_t is in <cstddef> which is described in C Standard as <stddef.h>.
        // C Standard Section 4.1.5 defines size_t as an unsigned integral type of the result of the sizeof operator
        //
        // An unsigned type will simply wrap round arithmetically (well defined).
        //
        if(!found_leading_bytes) {
            // If how_far_back == 3, we may have four consecutive continuation bytes!!!
            // [....] [continuation] [continuation] [continuation] | [buf is continuation]
            // Or we possibly have a stream that does not start with a leading byte.
            return {UnicodeError::TOO_LONG, -how_far_back};
        }

        auto res = convert_with_errors(buf, len + extra_len, utf32_output);
        if (is_unicode_error(res)) {
            res.count -= extra_len;
        }
        return res;
    }

    inline size_t convert_valid(const char* buf, size_t len, char32_t* utf32_output) {
        const uint8_t *data = reinterpret_cast<const uint8_t *>(buf);
        size_t pos = 0;
        char32_t* start{utf32_output};
        while (pos < len) {
            // try to convert the next block of 8 ASCII bytes
            if (pos + 8 <= len) { // if it is safe to read 8 more bytes, check that they are ascii
                uint64_t v;
                ::memcpy(&v, data + pos, sizeof(uint64_t));
                if ((v & 0x8080808080808080) == 0) {
                    size_t final_pos = pos + 8;
                    while(pos < final_pos) {
                        *utf32_output++ = char32_t(buf[pos]);
                        pos++;
                    }
                    continue;
                }
            }
            uint8_t leading_byte = data[pos]; // leading byte
            if (leading_byte < 0b10000000) {
                // converting one ASCII byte !!!
                *utf32_output++ = char32_t(leading_byte);
                pos++;
            } else if ((leading_byte & 0b11100000) == 0b11000000) {
                // We have a two-byte UTF-8
                if(pos + 1 >= len) { break; } // minimal bound checking
                *utf32_output++ = char32_t(((leading_byte &0b00011111) << 6) | (data[pos + 1] &0b00111111));
                pos += 2;
            } else if ((leading_byte & 0b11110000) == 0b11100000) {
                // We have a three-byte UTF-8
                if(pos + 2 >= len) { break; } // minimal bound checking
                *utf32_output++ = char32_t(((leading_byte &0b00001111) << 12) | ((data[pos + 1] &0b00111111) << 6) | (data[pos + 2] &0b00111111));
                pos += 3;
            } else if ((leading_byte & 0b11111000) == 0b11110000) { // 0b11110000
                // we have a 4-byte UTF-8 word.
                if(pos + 3 >= len) { break; } // minimal bound checking
                uint32_t code_word = ((leading_byte & 0b00000111) << 18 )| ((data[pos + 1] &0b00111111) << 12)
                                     | ((data[pos + 2] &0b00111111) << 6) | (data[pos + 3] &0b00111111);
                *utf32_output++ = char32_t(code_word);
                pos += 4;
            } else {
                // we may have a continuation but we do not do error checking
                return 0;
            }
        }
        return utf32_output - start;
    }

}  // namespace turbo::unicode::utf8_to_utf32
#endif  // TURBO_UNICODE_SCALAR_UTF8_CONVERT_H_
