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


#ifndef TURBO_UNICODE_SCALAR_VALIDATE_H_
#define TURBO_UNICODE_SCALAR_VALIDATE_H_

#include <cstddef>
#include <cstdint>
#include <cstring>
#include "turbo/unicode/encoding_types.h"
#include "turbo/unicode/error.h"

namespace turbo::unicode::ascii {

    inline TURBO_MUST_USE_RESULT bool validate(const char *buf, size_t len) noexcept {
        const uint8_t *data = reinterpret_cast<const uint8_t *>(buf);
        uint64_t pos = 0;
        // process in blocks of 16 bytes when possible
        for (; pos + 16 < len; pos += 16) {
            uint64_t v1;
            std::memcpy(&v1, data + pos, sizeof(uint64_t));
            uint64_t v2;
            std::memcpy(&v2, data + pos + sizeof(uint64_t), sizeof(uint64_t));
            uint64_t v{v1 | v2};
            if ((v & 0x8080808080808080) != 0) { return false; }
        }
        // process the tail byte-by-byte
        for (; pos < len; pos++) {
            if (data[pos] >= 0b10000000) { return false; }
        }
        return true;
    }

    inline TURBO_MUST_USE_RESULT result validate_with_errors(const char *buf, size_t len) noexcept {
        const uint8_t *data = reinterpret_cast<const uint8_t *>(buf);
        size_t pos = 0;
        // process in blocks of 16 bytes when possible
        for (; pos + 16 < len; pos += 16) {
            uint64_t v1;
            std::memcpy(&v1, data + pos, sizeof(uint64_t));
            uint64_t v2;
            std::memcpy(&v2, data + pos + sizeof(uint64_t), sizeof(uint64_t));
            uint64_t v{v1 | v2};
            if ((v & 0x8080808080808080) != 0) {
                for (; pos < len; pos++) {
                    if (data[pos] >= 0b10000000) { return result(error_code::TOO_LARGE, pos); }
                }
            }
        }
        // process the tail byte-by-byte
        for (; pos < len; pos++) {
            if (data[pos] >= 0b10000000) { return result(error_code::TOO_LARGE, pos); }
        }
        return result(error_code::SUCCESS, pos);
    }
}  // namespace turbo::unicode::ascii

namespace turbo::unicode::utf8 {

    inline bool validate(const char *buf, size_t len) noexcept {
        const uint8_t *data = reinterpret_cast<const uint8_t *>(buf);
        uint64_t pos = 0;
        uint32_t code_point = 0;
        while (pos < len) {
            // check of the next 8 bytes are ascii.
            uint64_t next_pos = pos + 16;
            if (next_pos <= len) { // if it is safe to read 8 more bytes, check that they are ascii
                uint64_t v1;
                std::memcpy(&v1, data + pos, sizeof(uint64_t));
                uint64_t v2;
                std::memcpy(&v2, data + pos + sizeof(uint64_t), sizeof(uint64_t));
                uint64_t v{v1 | v2};
                if ((v & 0x8080808080808080) == 0) {
                    pos = next_pos;
                    continue;
                }
            }
            unsigned char byte = data[pos];

            while (byte < 0b10000000) {
                if (++pos == len) { return true; }
                byte = data[pos];
            }

            if ((byte & 0b11100000) == 0b11000000) {
                next_pos = pos + 2;
                if (next_pos > len) { return false; }
                if ((data[pos + 1] & 0b11000000) != 0b10000000) { return false; }
                // range check
                code_point = (byte & 0b00011111) << 6 | (data[pos + 1] & 0b00111111);
                if ((code_point < 0x80) || (0x7ff < code_point)) { return false; }
            } else if ((byte & 0b11110000) == 0b11100000) {
                next_pos = pos + 3;
                if (next_pos > len) { return false; }
                if ((data[pos + 1] & 0b11000000) != 0b10000000) { return false; }
                if ((data[pos + 2] & 0b11000000) != 0b10000000) { return false; }
                // range check
                code_point = (byte & 0b00001111) << 12 |
                             (data[pos + 1] & 0b00111111) << 6 |
                             (data[pos + 2] & 0b00111111);
                if ((code_point < 0x800) || (0xffff < code_point) ||
                    (0xd7ff < code_point && code_point < 0xe000)) {
                    return false;
                }
            } else if ((byte & 0b11111000) == 0b11110000) { // 0b11110000
                next_pos = pos + 4;
                if (next_pos > len) { return false; }
                if ((data[pos + 1] & 0b11000000) != 0b10000000) { return false; }
                if ((data[pos + 2] & 0b11000000) != 0b10000000) { return false; }
                if ((data[pos + 3] & 0b11000000) != 0b10000000) { return false; }
                // range check
                code_point =
                        (byte & 0b00000111) << 18 | (data[pos + 1] & 0b00111111) << 12 |
                        (data[pos + 2] & 0b00111111) << 6 | (data[pos + 3] & 0b00111111);
                if (code_point <= 0xffff || 0x10ffff < code_point) { return false; }
            } else {
                // we may have a continuation
                return false;
            }
            pos = next_pos;
        }
        return true;
    }

    inline result validate_with_errors(const char *buf, size_t len) noexcept {
        const uint8_t *data = reinterpret_cast<const uint8_t *>(buf);
        size_t pos = 0;
        uint32_t code_point = 0;
        while (pos < len) {
            // check of the next 8 bytes are ascii.
            size_t next_pos = pos + 16;
            if (next_pos <= len) { // if it is safe to read 8 more bytes, check that they are ascii
                uint64_t v1;
                std::memcpy(&v1, data + pos, sizeof(uint64_t));
                uint64_t v2;
                std::memcpy(&v2, data + pos + sizeof(uint64_t), sizeof(uint64_t));
                uint64_t v{v1 | v2};
                if ((v & 0x8080808080808080) == 0) {
                    pos = next_pos;
                    continue;
                }
            }
            unsigned char byte = data[pos];

            while (byte < 0b10000000) {
                if (++pos == len) { return result(error_code::SUCCESS, len); }
                byte = data[pos];
            }

            if ((byte & 0b11100000) == 0b11000000) {
                next_pos = pos + 2;
                if (next_pos > len) { return result(error_code::TOO_SHORT, pos); }
                if ((data[pos + 1] & 0b11000000) != 0b10000000) { return result(error_code::TOO_SHORT, pos); }
                // range check
                code_point = (byte & 0b00011111) << 6 | (data[pos + 1] & 0b00111111);
                if ((code_point < 0x80) || (0x7ff < code_point)) { return result(error_code::OVERLONG, pos); }
            } else if ((byte & 0b11110000) == 0b11100000) {
                next_pos = pos + 3;
                if (next_pos > len) { return result(error_code::TOO_SHORT, pos); }
                if ((data[pos + 1] & 0b11000000) != 0b10000000) { return result(error_code::TOO_SHORT, pos); }
                if ((data[pos + 2] & 0b11000000) != 0b10000000) { return result(error_code::TOO_SHORT, pos); }
                // range check
                code_point = (byte & 0b00001111) << 12 |
                             (data[pos + 1] & 0b00111111) << 6 |
                             (data[pos + 2] & 0b00111111);
                if ((code_point < 0x800) || (0xffff < code_point)) { return result(error_code::OVERLONG, pos); }
                if (0xd7ff < code_point && code_point < 0xe000) { return result(error_code::SURROGATE, pos); }
            } else if ((byte & 0b11111000) == 0b11110000) { // 0b11110000
                next_pos = pos + 4;
                if (next_pos > len) { return result(error_code::TOO_SHORT, pos); }
                if ((data[pos + 1] & 0b11000000) != 0b10000000) { return result(error_code::TOO_SHORT, pos); }
                if ((data[pos + 2] & 0b11000000) != 0b10000000) { return result(error_code::TOO_SHORT, pos); }
                if ((data[pos + 3] & 0b11000000) != 0b10000000) { return result(error_code::TOO_SHORT, pos); }
                // range check
                code_point =
                        (byte & 0b00000111) << 18 | (data[pos + 1] & 0b00111111) << 12 |
                        (data[pos + 2] & 0b00111111) << 6 | (data[pos + 3] & 0b00111111);
                if (code_point <= 0xffff) { return result(error_code::OVERLONG, pos); }
                if (0x10ffff < code_point) { return result(error_code::TOO_LARGE, pos); }
            } else {
                // we either have too many continuation bytes or an invalid leading byte
                if ((byte & 0b11000000) == 0b10000000) { return result(error_code::TOO_LONG, pos); }
                else { return result(error_code::HEADER_BITS, pos); }
            }
            pos = next_pos;
        }
        return result(error_code::SUCCESS, len);
    }

    // Finds the previous leading byte and validates with errors from there
    // Used to pinpoint the location of an error when an invalid chunk is detected
    inline TURBO_MUST_USE_RESULT result

    rewind_and_validate_with_errors(const char *buf, size_t len) noexcept {
        size_t extra_len{0};
        // A leading byte cannot be further than 4 bytes away
        for (int i = 0; i < 5; i++) {
            unsigned char byte = *buf;
            if ((byte & 0b11000000) != 0b10000000) {
                break;
            } else {
                buf--;
                extra_len++;
            }
        }

        result res = validate_with_errors(buf, len + extra_len);
        res.count -= extra_len;
        return res;
    }

    inline size_t count_code_points(const char *buf, size_t len) {
        const int8_t *p = reinterpret_cast<const int8_t *>(buf);
        size_t counter{0};
        for (size_t i = 0; i < len; i++) {
            // -65 is 0b10111111, anything larger in two-complement's should start a new code point.
            if (p[i] > -65) { counter++; }
        }
        return counter;
    }

    inline size_t Utf16LengthFromUtf8(const char *buf, size_t len) {
        const int8_t *p = reinterpret_cast<const int8_t *>(buf);
        size_t counter{0};
        for (size_t i = 0; i < len; i++) {
            if (p[i] > -65) { counter++; }
            if (uint8_t(p[i]) >= 240) { counter++; }
        }
        return counter;
    }

}  // namespace turbo::unicode::utf8

namespace turbo::unicode::utf16 {


    template<EndianNess big_endian>
    inline TURBO_MUST_USE_RESULT bool validate(const char16_t *buf, size_t len) noexcept {
        const uint16_t *data = reinterpret_cast<const uint16_t *>(buf);
        uint64_t pos = 0;
        while (pos < len) {
            uint16_t word = !match_system(big_endian) ? gbswap_16(data[pos]) : data[pos];
            if ((word & 0xF800) == 0xD800) {
                if (pos + 1 >= len) { return false; }
                uint16_t diff = uint16_t(word - 0xD800);
                if (diff > 0x3FF) { return false; }
                uint16_t next_word = !match_system(big_endian) ? gbswap_16(data[pos + 1]) : data[pos + 1];
                uint16_t diff2 = uint16_t(next_word - 0xDC00);
                if (diff2 > 0x3FF) { return false; }
                pos += 2;
            } else {
                pos++;
            }
        }
        return true;
    }

    template<EndianNess big_endian>
    inline TURBO_MUST_USE_RESULT result validate_with_errors(const char16_t *buf, size_t len) noexcept {
        const uint16_t *data = reinterpret_cast<const uint16_t *>(buf);
        size_t pos = 0;
        while (pos < len) {
            uint16_t word = !match_system(big_endian) ? gbswap_16(data[pos]) : data[pos];
            if ((word & 0xF800) == 0xD800) {
                if (pos + 1 >= len) { return result(error_code::SURROGATE, pos); }
                uint16_t diff = uint16_t(word - 0xD800);
                if (diff > 0x3FF) { return result(error_code::SURROGATE, pos); }
                uint16_t next_word = !match_system(big_endian) ? gbswap_16(data[pos + 1]) : data[pos + 1];
                uint16_t diff2 = uint16_t(next_word - 0xDC00);
                if (diff2 > 0x3FF) { return result(error_code::SURROGATE, pos); }
                pos += 2;
            } else {
                pos++;
            }
        }
        return result(error_code::SUCCESS, pos);
    }

    template<EndianNess big_endian>
    inline size_t count_code_points(const char16_t *buf, size_t len) {
        // We are not BOM aware.
        const uint16_t *p = reinterpret_cast<const uint16_t *>(buf);
        size_t counter{0};
        for (size_t i = 0; i < len; i++) {
            uint16_t word = !match_system(big_endian) ? gbswap_16(p[i]) : p[i];
            counter += ((word & 0xFC00) != 0xDC00);
        }
        return counter;
    }

    template<EndianNess big_endian>
    inline size_t Utf8LengthFromUtf16(const char16_t *buf, size_t len) {
        // We are not BOM aware.
        const uint16_t *p = reinterpret_cast<const uint16_t *>(buf);
        size_t counter{0};
        for (size_t i = 0; i < len; i++) {
            uint16_t word = !match_system(big_endian) ? gbswap_16(p[i]) : p[i];
            /** ASCII **/
            if (word <= 0x7F) { counter++; }
                /** two-byte **/
            else if (word <= 0x7FF) { counter += 2; }
                /** three-byte **/
            else if ((word <= 0xD7FF) || (word >= 0xE000)) { counter += 3; }
                /** surrogates -- 4 bytes **/
            else { counter += 2; }
        }
        return counter;
    }

    template<EndianNess big_endian>
    inline size_t Utf32LengthFromUtf16(const char16_t *buf, size_t len) {
        // We are not BOM aware.
        const uint16_t *p = reinterpret_cast<const uint16_t *>(buf);
        size_t counter{0};
        for (size_t i = 0; i < len; i++) {
            uint16_t word = !match_system(big_endian) ? gbswap_16(p[i]) : p[i];
            counter += ((word & 0xFC00) != 0xDC00);
        }
        return counter;
    }

    TURBO_FORCE_INLINE void ChangeEndiannessUtf16(const char16_t *in, size_t size, char16_t *out) {
        const uint16_t *input = reinterpret_cast<const uint16_t *>(in);
        uint16_t *output = reinterpret_cast<uint16_t *>(out);
        for (size_t i = 0; i < size; i++) {
            *output++ = uint16_t(input[i] >> 8 | input[i] << 8);
        }
    }


}  // namespace turbo::unicode::utf16

namespace turbo::unicode::utf32 {

    inline TURBO_MUST_USE_RESULT bool validate(const char32_t *buf, size_t len) noexcept {
        const uint32_t *data = reinterpret_cast<const uint32_t *>(buf);
        uint64_t pos = 0;
        for (; pos < len; pos++) {
            uint32_t word = data[pos];
            if (word > 0x10FFFF || (word >= 0xD800 && word <= 0xDFFF)) {
                return false;
            }
        }
        return true;
    }

    inline TURBO_MUST_USE_RESULT result validate_with_errors(const char32_t *buf, size_t len) noexcept {
        const uint32_t *data = reinterpret_cast<const uint32_t *>(buf);
        size_t pos = 0;
        for (; pos < len; pos++) {
            uint32_t word = data[pos];
            if (word > 0x10FFFF) {
                return result(error_code::TOO_LARGE, pos);
            }
            if (word >= 0xD800 && word <= 0xDFFF) {
                return result(error_code::SURROGATE, pos);
            }
        }
        return result(error_code::SUCCESS, pos);
    }

    inline size_t Utf8LengthFromUtf32(const char32_t *buf, size_t len) {
        // We are not BOM aware.
        const uint32_t *p = reinterpret_cast<const uint32_t *>(buf);
        size_t counter{0};
        for (size_t i = 0; i < len; i++) {
            /** ASCII **/
            if (p[i] <= 0x7F) { counter++; }
                /** two-byte **/
            else if (p[i] <= 0x7FF) { counter += 2; }
                /** three-byte **/
            else if (p[i] <= 0xFFFF) { counter += 3; }
                /** four-bytes **/
            else { counter += 4; }
        }
        return counter;
    }

    inline size_t Utf16LengthFromUtf32(const char32_t *buf, size_t len) {
        // We are not BOM aware.
        const uint32_t *p = reinterpret_cast<const uint32_t *>(buf);
        size_t counter{0};
        for (size_t i = 0; i < len; i++) {
            /** non-surrogate word **/
            if (p[i] <= 0xFFFF) { counter++; }
                /** surrogate pair **/
            else { counter += 2; }
        }
        return counter;
    }


}  // namespace turbo::unicode::utf32


#endif  // TURBO_UNICODE_SCALAR_VALIDATE_H_
