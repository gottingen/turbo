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

#ifndef TURBO_UNICODE_SCALAR_UTF32_CONVERT_H_
#define TURBO_UNICODE_SCALAR_UTF32_CONVERT_H_

#include <cstddef>
#include <cstdint>
#include <cstring>
#include "turbo/unicode/encoding_types.h"
#include "turbo/unicode/error.h"
#include "turbo/base/endian.h"

namespace turbo::unicode::utf32_to_utf8 {

    inline size_t convert(const char32_t *buf, size_t len, char *utf8_output) {
        const uint32_t *data = reinterpret_cast<const uint32_t *>(buf);
        size_t pos = 0;
        char *start{utf8_output};
        while (pos < len) {
            // try to convert the next block of 2 ASCII characters
            if (pos + 2 <= len) { // if it is safe to read 8 more bytes, check that they are ascii
                uint64_t v;
                ::memcpy(&v, data + pos, sizeof(uint64_t));
                if ((v & 0xFFFFFF80FFFFFF80) == 0) {
                    *utf8_output++ = char(buf[pos]);
                    *utf8_output++ = char(buf[pos + 1]);
                    pos += 2;
                    continue;
                }
            }
            uint32_t word = data[pos];
            if ((word & 0xFFFFFF80) == 0) {
                // will generate one UTF-8 bytes
                *utf8_output++ = char(word);
                pos++;
            } else if ((word & 0xFFFFF800) == 0) {
                // will generate two UTF-8 bytes
                // we have 0b110XXXXX 0b10XXXXXX
                *utf8_output++ = char((word >> 6) | 0b11000000);
                *utf8_output++ = char((word & 0b111111) | 0b10000000);
                pos++;
            } else if ((word & 0xFFFF0000) == 0) {
                // will generate three UTF-8 bytes
                // we have 0b1110XXXX 0b10XXXXXX 0b10XXXXXX
                if (word >= 0xD800 && word <= 0xDFFF) { return 0; }
                *utf8_output++ = char((word >> 12) | 0b11100000);
                *utf8_output++ = char(((word >> 6) & 0b111111) | 0b10000000);
                *utf8_output++ = char((word & 0b111111) | 0b10000000);
                pos++;
            } else {
                // will generate four UTF-8 bytes
                // we have 0b11110XXX 0b10XXXXXX 0b10XXXXXX 0b10XXXXXX
                if (word > 0x10FFFF) { return 0; }
                *utf8_output++ = char((word >> 18) | 0b11110000);
                *utf8_output++ = char(((word >> 12) & 0b111111) | 0b10000000);
                *utf8_output++ = char(((word >> 6) & 0b111111) | 0b10000000);
                *utf8_output++ = char((word & 0b111111) | 0b10000000);
                pos++;
            }
        }
        return utf8_output - start;
    }

    inline result convert_with_errors(const char32_t *buf, size_t len, char *utf8_output) {
        const uint32_t *data = reinterpret_cast<const uint32_t *>(buf);
        size_t pos = 0;
        char *start{utf8_output};
        while (pos < len) {
            // try to convert the next block of 2 ASCII characters
            if (pos + 2 <= len) { // if it is safe to read 8 more bytes, check that they are ascii
                uint64_t v;
                ::memcpy(&v, data + pos, sizeof(uint64_t));
                if ((v & 0xFFFFFF80FFFFFF80) == 0) {
                    *utf8_output++ = char(buf[pos]);
                    *utf8_output++ = char(buf[pos + 1]);
                    pos += 2;
                    continue;
                }
            }
            uint32_t word = data[pos];
            if ((word & 0xFFFFFF80) == 0) {
                // will generate one UTF-8 bytes
                *utf8_output++ = char(word);
                pos++;
            } else if ((word & 0xFFFFF800) == 0) {
                // will generate two UTF-8 bytes
                // we have 0b110XXXXX 0b10XXXXXX
                *utf8_output++ = char((word >> 6) | 0b11000000);
                *utf8_output++ = char((word & 0b111111) | 0b10000000);
                pos++;
            } else if ((word & 0xFFFF0000) == 0) {
                // will generate three UTF-8 bytes
                // we have 0b1110XXXX 0b10XXXXXX 0b10XXXXXX
                if (word >= 0xD800 && word <= 0xDFFF) { return result(error_code::SURROGATE, pos); }
                *utf8_output++ = char((word >> 12) | 0b11100000);
                *utf8_output++ = char(((word >> 6) & 0b111111) | 0b10000000);
                *utf8_output++ = char((word & 0b111111) | 0b10000000);
                pos++;
            } else {
                // will generate four UTF-8 bytes
                // we have 0b11110XXX 0b10XXXXXX 0b10XXXXXX 0b10XXXXXX
                if (word > 0x10FFFF) { return result(error_code::TOO_LARGE, pos); }
                *utf8_output++ = char((word >> 18) | 0b11110000);
                *utf8_output++ = char(((word >> 12) & 0b111111) | 0b10000000);
                *utf8_output++ = char(((word >> 6) & 0b111111) | 0b10000000);
                *utf8_output++ = char((word & 0b111111) | 0b10000000);
                pos++;
            }
        }
        return result(error_code::SUCCESS, utf8_output - start);
    }

    inline size_t convert_valid(const char32_t *buf, size_t len, char *utf8_output) {
        const uint32_t *data = reinterpret_cast<const uint32_t *>(buf);
        size_t pos = 0;
        char *start{utf8_output};
        while (pos < len) {
            // try to convert the next block of 2 ASCII characters
            if (pos + 2 <= len) { // if it is safe to read 8 more bytes, check that they are ascii
                uint64_t v;
                ::memcpy(&v, data + pos, sizeof(uint64_t));
                if ((v & 0xFFFFFF80FFFFFF80) == 0) {
                    *utf8_output++ = char(buf[pos]);
                    *utf8_output++ = char(buf[pos + 1]);
                    pos += 2;
                    continue;
                }
            }
            uint32_t word = data[pos];
            if ((word & 0xFFFFFF80) == 0) {
                // will generate one UTF-8 bytes
                *utf8_output++ = char(word);
                pos++;
            } else if ((word & 0xFFFFF800) == 0) {
                // will generate two UTF-8 bytes
                // we have 0b110XXXXX 0b10XXXXXX
                *utf8_output++ = char((word >> 6) | 0b11000000);
                *utf8_output++ = char((word & 0b111111) | 0b10000000);
                pos++;
            } else if ((word & 0xFFFF0000) == 0) {
                // will generate three UTF-8 bytes
                // we have 0b1110XXXX 0b10XXXXXX 0b10XXXXXX
                *utf8_output++ = char((word >> 12) | 0b11100000);
                *utf8_output++ = char(((word >> 6) & 0b111111) | 0b10000000);
                *utf8_output++ = char((word & 0b111111) | 0b10000000);
                pos++;
            } else {
                // will generate four UTF-8 bytes
                // we have 0b11110XXX 0b10XXXXXX 0b10XXXXXX 0b10XXXXXX
                *utf8_output++ = char((word >> 18) | 0b11110000);
                *utf8_output++ = char(((word >> 12) & 0b111111) | 0b10000000);
                *utf8_output++ = char(((word >> 6) & 0b111111) | 0b10000000);
                *utf8_output++ = char((word & 0b111111) | 0b10000000);
                pos++;
            }
        }
        return utf8_output - start;
    }
}  // namespace turbo::unicode::utf32_to_utf8

namespace turbo::unicode::utf32_to_utf16 {

    template<EndianNess big_endian>
    inline size_t convert(const char32_t *buf, size_t len, char16_t *utf16_output) {
        const uint32_t *data = reinterpret_cast<const uint32_t *>(buf);
        size_t pos = 0;
        char16_t *start{utf16_output};
        while (pos < len) {
            uint32_t word = data[pos];
            if ((word & 0xFFFF0000) == 0) {
                if (word >= 0xD800 && word <= 0xDFFF) { return 0; }
                // will not generate a surrogate pair
                *utf16_output++ = !match_system(big_endian) ? char16_t(gbswap_16(uint16_t(word))) : char16_t(word);
            } else {
                // will generate a surrogate pair
                if (word > 0x10FFFF) { return 0; }
                word -= 0x10000;
                uint16_t high_surrogate = uint16_t(0xD800 + (word >> 10));
                uint16_t low_surrogate = uint16_t(0xDC00 + (word & 0x3FF));
                if (!match_system(big_endian)) {
                    high_surrogate = gbswap_16(high_surrogate);
                    low_surrogate = gbswap_16(low_surrogate);
                }
                *utf16_output++ = char16_t(high_surrogate);
                *utf16_output++ = char16_t(low_surrogate);
            }
            pos++;
        }
        return utf16_output - start;
    }

    template<EndianNess big_endian>
    inline result convert_with_errors(const char32_t *buf, size_t len, char16_t *utf16_output) {
        const uint32_t *data = reinterpret_cast<const uint32_t *>(buf);
        size_t pos = 0;
        char16_t *start{utf16_output};
        while (pos < len) {
            uint32_t word = data[pos];
            if ((word & 0xFFFF0000) == 0) {
                if (word >= 0xD800 && word <= 0xDFFF) { return result(error_code::SURROGATE, pos); }
                // will not generate a surrogate pair
                *utf16_output++ = !match_system(big_endian) ? char16_t(gbswap_16(uint16_t(word))) : char16_t(word);
            } else {
                // will generate a surrogate pair
                if (word > 0x10FFFF) { return result(error_code::TOO_LARGE, pos); }
                word -= 0x10000;
                uint16_t high_surrogate = uint16_t(0xD800 + (word >> 10));
                uint16_t low_surrogate = uint16_t(0xDC00 + (word & 0x3FF));
                if (!match_system(big_endian)) {
                    high_surrogate = gbswap_16(high_surrogate);
                    low_surrogate = gbswap_16(low_surrogate);
                }
                *utf16_output++ = char16_t(high_surrogate);
                *utf16_output++ = char16_t(low_surrogate);
            }
            pos++;
        }
        return result(error_code::SUCCESS, utf16_output - start);
    }


    template <EndianNess big_endian>
    inline size_t convert_valid(const char32_t* buf, size_t len, char16_t* utf16_output) {
        const uint32_t *data = reinterpret_cast<const uint32_t *>(buf);
        size_t pos = 0;
        char16_t* start{utf16_output};
        while (pos < len) {
            uint32_t word = data[pos];
            if((word & 0xFFFF0000)==0) {
                // will not generate a surrogate pair
                *utf16_output++ = !match_system(big_endian) ? char16_t(gbswap_16(uint16_t(word))) : char16_t(word);
                pos++;
            } else {
                // will generate a surrogate pair
                word -= 0x10000;
                uint16_t high_surrogate = uint16_t(0xD800 + (word >> 10));
                uint16_t low_surrogate = uint16_t(0xDC00 + (word & 0x3FF));
                if (!match_system(big_endian)) {
                    high_surrogate = gbswap_16(high_surrogate);
                    low_surrogate = gbswap_16(low_surrogate);
                }
                *utf16_output++ = char16_t(high_surrogate);
                *utf16_output++ = char16_t(low_surrogate);
                pos++;
            }
        }
        return utf16_output - start;
    }

}  // namespace turbo::unicode::utf32_to_utf16

#endif  // TURBO_UNICODE_SCALAR_UTF32_CONVERT_H_
