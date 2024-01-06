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

#ifndef TURBO_UNICODE_SCALAR_CONVERTER_H_
#define TURBO_UNICODE_SCALAR_CONVERTER_H_

#include "turbo/unicode/fwd.h"
#include "turbo/unicode/scalar/engine.h"
#include "turbo/unicode/scalar/validate.h"
#include "turbo/unicode/scalar/utf8_convert.h"
#include "turbo/unicode/scalar/utf16_convert.h"
#include "turbo/unicode/scalar/utf32_convert.h"

namespace turbo::unicode {

    template<>
    struct Converter<scalar_engine> {
        [[nodiscard]] turbo::EncodingType auto_detect_encoding(const char *input, size_t length) noexcept;

        [[nodiscard]] static inline int detect_encodings(const char *input, size_t length) noexcept;

        [[nodiscard]] static inline bool validate_utf8(const char *buf, size_t len) noexcept;

        [[nodiscard]] static inline UnicodeResult validate_utf8_with_errors(const char *buf, size_t len) noexcept;

        [[nodiscard]] static inline bool validate_ascii(const char *buf, size_t len) noexcept;

        [[nodiscard]] static inline UnicodeResult validate_ascii_with_errors(const char *buf, size_t len) noexcept;

        [[nodiscard]] static inline bool validate_utf16le(const char16_t *buf, size_t len) noexcept;

        [[nodiscard]] static inline bool validate_utf16be(const char16_t *buf, size_t len) noexcept;

        [[nodiscard]] static inline UnicodeResult validate_utf16le_with_errors(const char16_t *buf, size_t len) noexcept;

        [[nodiscard]] static inline UnicodeResult validate_utf16be_with_errors(const char16_t *buf, size_t len) noexcept;

        [[nodiscard]] static inline bool validate_utf32(const char32_t *buf, size_t len) noexcept;

        [[nodiscard]] static inline UnicodeResult validate_utf32_with_errors(const char32_t *buf, size_t len) noexcept;

        [[nodiscard]] static inline size_t
        convert_utf8_to_utf16le(const char *buf, size_t len, char16_t *utf16_output) noexcept;

        [[nodiscard]] static inline size_t
        convert_utf8_to_utf16be(const char *buf, size_t len, char16_t *utf16_output) noexcept;

        [[nodiscard]] static inline UnicodeResult
        convert_utf8_to_utf16le_with_errors(const char *buf, size_t len, char16_t *utf16_output) noexcept;

        [[nodiscard]] static inline UnicodeResult
        convert_utf8_to_utf16be_with_errors(const char *buf, size_t len, char16_t *utf16_output) noexcept;

        [[nodiscard]] static inline size_t
        convert_valid_utf8_to_utf16le(const char *buf, size_t len, char16_t *utf16_buffer) noexcept;

        [[nodiscard]] static inline size_t
        convert_valid_utf8_to_utf16be(const char *buf, size_t len, char16_t *utf16_buffer) noexcept;

        [[nodiscard]] static inline size_t
        convert_utf8_to_utf32(const char *buf, size_t len, char32_t *utf32_output) noexcept;

        [[nodiscard]] static inline UnicodeResult
        convert_utf8_to_utf32_with_errors(const char *buf, size_t len, char32_t *utf32_output) noexcept;

        [[nodiscard]] static inline size_t
        convert_valid_utf8_to_utf32(const char *buf, size_t len, char32_t *utf32_buffer) noexcept;

        [[nodiscard]] static inline size_t
        convert_utf16le_to_utf8(const char16_t *buf, size_t len, char *utf8_buffer) noexcept;

        [[nodiscard]] static inline size_t
        convert_utf16be_to_utf8(const char16_t *buf, size_t len, char *utf8_buffer) noexcept;

        [[nodiscard]] static inline UnicodeResult
        convert_utf16le_to_utf8_with_errors(const char16_t *buf, size_t len, char *utf8_buffer) noexcept;

        [[nodiscard]] static inline UnicodeResult
        convert_utf16be_to_utf8_with_errors(const char16_t *buf, size_t len, char *utf8_buffer) noexcept;

        [[nodiscard]] static inline size_t
        convert_valid_utf16le_to_utf8(const char16_t *buf, size_t len, char *utf8_buffer) noexcept;

        [[nodiscard]] static inline size_t
        convert_valid_utf16be_to_utf8(const char16_t *buf, size_t len, char *utf8_buffer) noexcept;

        [[nodiscard]] static inline size_t
        convert_utf32_to_utf8(const char32_t *buf, size_t len, char *utf8_buffer) noexcept;

        [[nodiscard]] static inline UnicodeResult
        convert_utf32_to_utf8_with_errors(const char32_t *buf, size_t len, char *utf8_buffer) noexcept;

        [[nodiscard]] static inline size_t
        convert_valid_utf32_to_utf8(const char32_t *buf, size_t len, char *utf8_buffer) noexcept;

        [[nodiscard]] static inline size_t
        convert_utf32_to_utf16le(const char32_t *buf, size_t len, char16_t *utf16_buffer) noexcept;

        [[nodiscard]] static inline size_t
        convert_utf32_to_utf16be(const char32_t *buf, size_t len, char16_t *utf16_buffer) noexcept;

        [[nodiscard]] static inline UnicodeResult
        convert_utf32_to_utf16le_with_errors(const char32_t *buf, size_t len, char16_t *utf16_buffer) noexcept;

        [[nodiscard]] static inline UnicodeResult
        convert_utf32_to_utf16be_with_errors(const char32_t *buf, size_t len, char16_t *utf16_buffer) noexcept;

        [[nodiscard]] static inline size_t
        convert_valid_utf32_to_utf16le(const char32_t *buf, size_t len, char16_t *utf16_buffer) noexcept;

        [[nodiscard]] static inline size_t
        convert_valid_utf32_to_utf16be(const char32_t *buf, size_t len, char16_t *utf16_buffer) noexcept;

        [[nodiscard]] static inline size_t
        convert_utf16le_to_utf32(const char16_t *buf, size_t len, char32_t *utf32_buffer) noexcept;

        [[nodiscard]] static inline size_t
        convert_utf16be_to_utf32(const char16_t *buf, size_t len, char32_t *utf32_buffer) noexcept;

        [[nodiscard]] static inline UnicodeResult
        convert_utf16le_to_utf32_with_errors(const char16_t *buf, size_t len, char32_t *utf32_buffer) noexcept;

        [[nodiscard]] static inline UnicodeResult
        convert_utf16be_to_utf32_with_errors(const char16_t *buf, size_t len, char32_t *utf32_buffer) noexcept;

        [[nodiscard]] static inline size_t
        convert_valid_utf16le_to_utf32(const char16_t *buf, size_t len, char32_t *utf32_buffer) noexcept;

        [[nodiscard]] static inline size_t
        convert_valid_utf16be_to_utf32(const char16_t *buf, size_t len, char32_t *utf32_buffer) noexcept;

        [[nodiscard]] static inline void
        change_endianness_utf16(const char16_t *buf, size_t length, char16_t *output) noexcept;

        [[nodiscard]] static inline size_t count_utf16le(const char16_t *buf, size_t length) noexcept;

        [[nodiscard]] static inline size_t count_utf16be(const char16_t *buf, size_t length) noexcept;

        [[nodiscard]] static inline size_t count_utf8(const char *buf, size_t length) noexcept;

        [[nodiscard]] static inline size_t utf8_length_from_utf16le(const char16_t *input, size_t length) noexcept;

        [[nodiscard]] static inline size_t utf8_length_from_utf16be(const char16_t *input, size_t length) noexcept;

        [[nodiscard]] static inline size_t utf32_length_from_utf16le(const char16_t *input, size_t length) noexcept;

        [[nodiscard]] static inline size_t utf32_length_from_utf16be(const char16_t *input, size_t length) noexcept;

        [[nodiscard]] static inline size_t utf16_length_from_utf8(const char *input, size_t length) noexcept;

        [[nodiscard]] static inline size_t utf8_length_from_utf32(const char32_t *input, size_t length) noexcept;

        [[nodiscard]] static inline size_t utf16_length_from_utf32(const char32_t *input, size_t length) noexcept;

        [[nodiscard]] static inline size_t utf32_length_from_utf8(const char *input, size_t length) noexcept;
    };

    /// inlines

    [[nodiscard]] inline turbo::EncodingType Converter<scalar_engine>::auto_detect_encoding(const char *input, size_t length) noexcept {
        // If there is a BOM, then we trust it.
        auto bom_encoding = BOM::check_bom(input, length);
        if(bom_encoding != EncodingType::unspecified) { return bom_encoding; }
        // UTF8 is common, it includes ASCII, and is commonly represented
        // without a BOM, so if it fits, go with that. Note that it is still
        // possible to get it wrong, we are only 'guessing'. If some has UTF-16
        // data without a BOM, it could pass as UTF-8.
        //
        // An interesting twist might be to check for UTF-16 ASCII first (every
        // other byte is zero).
        if(validate_utf8(input, length)) { return EncodingType::UTF8; }
        // The next most common encoding that might appear without BOM is probably
        // UTF-16LE, so try that next.
        if((length % 2) == 0) {
            // important: we need to divide by two
            if(validate_utf16le(reinterpret_cast<const char16_t*>(input), length/2)) { return EncodingType::UTF16_LE; }
        }
        if((length % 4) == 0) {
            if(validate_utf32(reinterpret_cast<const char32_t*>(input), length/4)) { return EncodingType::UTF32_LE; }
        }
        return EncodingType::unspecified;
    }

    [[nodiscard]] inline int Converter<scalar_engine>::detect_encodings(const char *input, size_t length) noexcept {
        auto bom_encoding = turbo::BOM::check_bom(input, length);
        if (bom_encoding != EncodingType::unspecified) { return bom_encoding; }
        int out = 0;
        if (validate_utf8(input, length)) { out |= EncodingType::UTF8; }
        if ((length % 2) == 0) {
            if (validate_utf16le(reinterpret_cast<const char16_t *>(input),
                                 length / 2)) { out |= EncodingType::UTF16_LE; }
        }
        if ((length % 4) == 0) {
            if (validate_utf32(reinterpret_cast<const char32_t *>(input),
                               length / 4)) { out |= EncodingType::UTF32_LE; }
        }

        return out;
    }

    [[nodiscard]] inline bool Converter<scalar_engine>::validate_utf8(const char *buf, size_t len) noexcept {
        return utf8::validate(buf, len);
    }

    [[nodiscard]] inline UnicodeResult
    Converter<scalar_engine>::validate_utf8_with_errors(const char *buf, size_t len) noexcept {
        return utf8::validate_with_errors(buf, len);
    }

    [[nodiscard]] inline bool Converter<scalar_engine>::validate_ascii(const char *buf, size_t len) noexcept {
        return ascii::validate(buf, len);
    }

    [[nodiscard]] inline UnicodeResult
    Converter<scalar_engine>::validate_ascii_with_errors(const char *buf, size_t len) noexcept {
        return ascii::validate_with_errors(buf, len);
    }

    [[nodiscard]] inline bool Converter<scalar_engine>::validate_utf16le(const char16_t *buf, size_t len) noexcept {
        return utf16::validate<EndianNess::SYS_LITTLE_ENDIAN>(buf, len);
    }

    [[nodiscard]] inline bool Converter<scalar_engine>::validate_utf16be(const char16_t *buf, size_t len) noexcept {
        return utf16::validate<EndianNess::SYS_BIG_ENDIAN>(buf, len);
    }

    [[nodiscard]] inline UnicodeResult
    Converter<scalar_engine>::validate_utf16le_with_errors(const char16_t *buf, size_t len) noexcept {
        return utf16::validate_with_errors<EndianNess::SYS_LITTLE_ENDIAN>(buf, len);
    }

    [[nodiscard]] inline UnicodeResult
    Converter<scalar_engine>::validate_utf16be_with_errors(const char16_t *buf, size_t len) noexcept {
        return utf16::validate_with_errors<EndianNess::SYS_BIG_ENDIAN>(buf, len);
    }

    [[nodiscard]] inline bool Converter<scalar_engine>::validate_utf32(const char32_t *buf, size_t len) noexcept {
        return utf32::validate(buf, len);
    }

    [[nodiscard]] inline UnicodeResult
    Converter<scalar_engine>::validate_utf32_with_errors(const char32_t *buf, size_t len) noexcept {
        return utf32::validate_with_errors(buf, len);
    }

    [[nodiscard]] inline size_t
    Converter<scalar_engine>::convert_utf8_to_utf16le(const char *buf, size_t len, char16_t *utf16_output) noexcept {
        return utf8_to_utf16::convert<EndianNess::SYS_LITTLE_ENDIAN>(buf, len, utf16_output);
    }

    [[nodiscard]] inline size_t
    Converter<scalar_engine>::convert_utf8_to_utf16be(const char *buf, size_t len, char16_t *utf16_output) noexcept {
        return utf8_to_utf16::convert<EndianNess::SYS_BIG_ENDIAN>(buf, len, utf16_output);
    }

    [[nodiscard]] inline UnicodeResult
    Converter<scalar_engine>::convert_utf8_to_utf16le_with_errors(const char *buf, size_t len,
                                                                  char16_t *utf16_output) noexcept {
        return utf8_to_utf16::convert_with_errors<EndianNess::SYS_LITTLE_ENDIAN>(buf, len, utf16_output);
    }

    [[nodiscard]] inline UnicodeResult
    Converter<scalar_engine>::convert_utf8_to_utf16be_with_errors(const char *buf, size_t len,
                                                                  char16_t *utf16_output) noexcept {
        return utf8_to_utf16::convert_with_errors<EndianNess::SYS_BIG_ENDIAN>(buf, len, utf16_output);
    }

    [[nodiscard]] inline size_t
    Converter<scalar_engine>::convert_valid_utf8_to_utf16le(const char *buf, size_t len,
                                                            char16_t *utf16_buffer) noexcept {
        return utf8_to_utf16::convert_valid<EndianNess::SYS_LITTLE_ENDIAN>(buf, len, utf16_buffer);
    }

    [[nodiscard]] inline size_t
    Converter<scalar_engine>::convert_valid_utf8_to_utf16be(const char *buf, size_t len,
                                                            char16_t *utf16_buffer) noexcept {
        return utf8_to_utf16::convert_valid<EndianNess::SYS_BIG_ENDIAN>(buf, len, utf16_buffer);
    }

    [[nodiscard]] inline size_t
    Converter<scalar_engine>::convert_utf8_to_utf32(const char *buf, size_t len, char32_t *utf32_output) noexcept {
        return utf8_to_utf32::convert(buf, len, utf32_output);
    }

    [[nodiscard]] inline UnicodeResult Converter<scalar_engine>::convert_utf8_to_utf32_with_errors(const char *buf, size_t len,
                                                                                            char32_t *utf32_output) noexcept {
        return utf8_to_utf32::convert_with_errors(buf, len, utf32_output);
    }

    [[nodiscard]] inline size_t
    Converter<scalar_engine>::convert_valid_utf8_to_utf32(const char *buf, size_t len,
                                                          char32_t *utf32_buffer) noexcept {
        return utf8_to_utf32::convert_valid(buf, len, utf32_buffer);
    }

    [[nodiscard]] inline size_t
    Converter<scalar_engine>::convert_utf16le_to_utf8(const char16_t *buf, size_t len, char *utf8_buffer) noexcept {
        return utf16_to_utf8::convert<EndianNess::SYS_LITTLE_ENDIAN>(buf, len, utf8_buffer);
    }

    [[nodiscard]] inline size_t
    Converter<scalar_engine>::convert_utf16be_to_utf8(const char16_t *buf, size_t len, char *utf8_buffer) noexcept {
        return utf16_to_utf8::convert<EndianNess::SYS_BIG_ENDIAN>(buf, len, utf8_buffer);
    }

    [[nodiscard]] inline UnicodeResult
    Converter<scalar_engine>::convert_utf16le_to_utf8_with_errors(const char16_t *buf, size_t len,
                                                                  char *utf8_buffer) noexcept {
        return utf16_to_utf8::convert_with_errors<EndianNess::SYS_LITTLE_ENDIAN>(buf, len, utf8_buffer);
    }

    [[nodiscard]] inline UnicodeResult
    Converter<scalar_engine>::convert_utf16be_to_utf8_with_errors(const char16_t *buf, size_t len,
                                                                  char *utf8_buffer) noexcept {
        return utf16_to_utf8::convert_with_errors<EndianNess::SYS_BIG_ENDIAN>(buf, len, utf8_buffer);
    }

    [[nodiscard]] inline size_t
    Converter<scalar_engine>::convert_valid_utf16le_to_utf8(const char16_t *buf, size_t len,
                                                            char *utf8_buffer) noexcept {
        return utf16_to_utf8::convert_valid<EndianNess::SYS_LITTLE_ENDIAN>(buf, len, utf8_buffer);
    }

    [[nodiscard]] inline size_t
    Converter<scalar_engine>::convert_valid_utf16be_to_utf8(const char16_t *buf, size_t len,
                                                            char *utf8_buffer) noexcept {
        return utf16_to_utf8::convert_valid<EndianNess::SYS_BIG_ENDIAN>(buf, len, utf8_buffer);
    }

    [[nodiscard]] inline size_t
    Converter<scalar_engine>::convert_utf32_to_utf8(const char32_t *buf, size_t len, char *utf8_buffer) noexcept {
        return utf32_to_utf8::convert(buf, len, utf8_buffer);
    }

    [[nodiscard]] inline UnicodeResult
    Converter<scalar_engine>::convert_utf32_to_utf8_with_errors(const char32_t *buf, size_t len,
                                                                char *utf8_buffer) noexcept {
        return utf32_to_utf8::convert_with_errors(buf, len, utf8_buffer);
    }

    [[nodiscard]] inline size_t
    Converter<scalar_engine>::convert_valid_utf32_to_utf8(const char32_t *buf, size_t len, char *utf8_buffer) noexcept {
        return utf32_to_utf8::convert_valid(buf, len, utf8_buffer);
    }

    [[nodiscard]] inline size_t
    Converter<scalar_engine>::convert_utf32_to_utf16le(const char32_t *buf, size_t len,
                                                       char16_t *utf16_buffer) noexcept {
        return utf32_to_utf16::convert<EndianNess::SYS_LITTLE_ENDIAN>(buf, len, utf16_buffer);
    }

    [[nodiscard]] inline size_t
    Converter<scalar_engine>::convert_utf32_to_utf16be(const char32_t *buf, size_t len,
                                                       char16_t *utf16_buffer) noexcept {
        return utf32_to_utf16::convert<EndianNess::SYS_BIG_ENDIAN>(buf, len, utf16_buffer);
    }

    [[nodiscard]] inline UnicodeResult
    Converter<scalar_engine>::convert_utf32_to_utf16le_with_errors(const char32_t *buf, size_t len,
                                                                   char16_t *utf16_buffer) noexcept {
        return utf32_to_utf16::convert_with_errors<EndianNess::SYS_LITTLE_ENDIAN>(buf, len, utf16_buffer);
    }

    [[nodiscard]] inline UnicodeResult
    Converter<scalar_engine>::convert_utf32_to_utf16be_with_errors(const char32_t *buf, size_t len,
                                                                   char16_t *utf16_buffer) noexcept {
        return utf32_to_utf16::convert_with_errors<EndianNess::SYS_BIG_ENDIAN>(buf, len, utf16_buffer);
    }

    [[nodiscard]] inline size_t
    Converter<scalar_engine>::convert_valid_utf32_to_utf16le(const char32_t *buf, size_t len,
                                                             char16_t *utf16_buffer) noexcept {
        return utf32_to_utf16::convert_valid<EndianNess::SYS_LITTLE_ENDIAN>(buf, len, utf16_buffer);
    }

    [[nodiscard]] inline size_t
    Converter<scalar_engine>::convert_valid_utf32_to_utf16be(const char32_t *buf, size_t len,
                                                             char16_t *utf16_buffer) noexcept {
        return utf32_to_utf16::convert_valid<EndianNess::SYS_BIG_ENDIAN>(buf, len, utf16_buffer);
    }

    [[nodiscard]] inline size_t
    Converter<scalar_engine>::convert_utf16le_to_utf32(const char16_t *buf, size_t len,
                                                       char32_t *utf32_buffer) noexcept {
        return utf16_to_utf32::convert<EndianNess::SYS_LITTLE_ENDIAN>(buf, len, utf32_buffer);
    }

    [[nodiscard]] inline size_t
    Converter<scalar_engine>::convert_utf16be_to_utf32(const char16_t *buf, size_t len,
                                                       char32_t *utf32_buffer) noexcept {
        return utf16_to_utf32::convert<EndianNess::SYS_BIG_ENDIAN>(buf, len, utf32_buffer);
    }

    [[nodiscard]] inline UnicodeResult
    Converter<scalar_engine>::convert_utf16le_to_utf32_with_errors(const char16_t *buf, size_t len,
                                                                   char32_t *utf32_buffer) noexcept {
        return utf16_to_utf32::convert_with_errors<EndianNess::SYS_LITTLE_ENDIAN>(buf, len, utf32_buffer);
    }

    [[nodiscard]] inline UnicodeResult
    Converter<scalar_engine>::convert_utf16be_to_utf32_with_errors(const char16_t *buf, size_t len,
                                                                   char32_t *utf32_buffer) noexcept {
        return utf16_to_utf32::convert_with_errors<EndianNess::SYS_BIG_ENDIAN>(buf, len, utf32_buffer);
    }

    [[nodiscard]] inline size_t
    Converter<scalar_engine>::convert_valid_utf16le_to_utf32(const char16_t *buf, size_t len,
                                                             char32_t *utf32_buffer) noexcept {
        return utf16_to_utf32::convert_valid<EndianNess::SYS_LITTLE_ENDIAN>(buf, len, utf32_buffer);
    }

    [[nodiscard]] inline size_t
    Converter<scalar_engine>::convert_valid_utf16be_to_utf32(const char16_t *buf, size_t len,
                                                             char32_t *utf32_buffer) noexcept {
        return utf16_to_utf32::convert_valid<EndianNess::SYS_BIG_ENDIAN>(buf, len, utf32_buffer);
    }

    [[nodiscard]] inline void
    Converter<scalar_engine>::change_endianness_utf16(const char16_t *buf, size_t length, char16_t *output) noexcept {
        utf16::change_endianness_utf16(buf, length, output);
    }

    [[nodiscard]] inline size_t Converter<scalar_engine>::count_utf16le(const char16_t *buf, size_t length) noexcept {
        return utf16::count_code_points<EndianNess::SYS_LITTLE_ENDIAN>(buf, length);
    }

    [[nodiscard]] inline size_t Converter<scalar_engine>::count_utf16be(const char16_t *buf, size_t length) noexcept {
        return utf16::count_code_points<EndianNess::SYS_BIG_ENDIAN>(buf, length);
    }

    [[nodiscard]] inline size_t Converter<scalar_engine>::count_utf8(const char *buf, size_t length) noexcept {
        return utf8::count_code_points(buf, length);
    }

    [[nodiscard]] inline size_t
    Converter<scalar_engine>::utf8_length_from_utf16le(const char16_t *input, size_t length) noexcept {
        return utf16::utf8_length_from_utf16<EndianNess::SYS_LITTLE_ENDIAN>(input, length);
    }

    [[nodiscard]] inline size_t
    Converter<scalar_engine>::utf8_length_from_utf16be(const char16_t *input, size_t length) noexcept {
        return utf16::utf8_length_from_utf16<EndianNess::SYS_BIG_ENDIAN>(input, length);
    }


    [[nodiscard]] inline size_t
    Converter<scalar_engine>::utf32_length_from_utf16le(const char16_t *input, size_t length) noexcept {
        return utf16::utf32_length_from_utf16<EndianNess::SYS_LITTLE_ENDIAN>(input, length);
    }

    [[nodiscard]] inline size_t
    Converter<scalar_engine>::utf32_length_from_utf16be(const char16_t *input, size_t length) noexcept {
        return utf16::utf32_length_from_utf16<EndianNess::SYS_BIG_ENDIAN>(input, length);
    }

    [[nodiscard]] inline size_t
    Converter<scalar_engine>::utf16_length_from_utf8(const char *input, size_t length) noexcept {
        return utf8::utf16_length_from_utf8(input, length);
    }

    [[nodiscard]] inline size_t
    Converter<scalar_engine>::utf8_length_from_utf32(const char32_t *input, size_t length) noexcept {
        return utf32::utf8_length_from_utf32(input, length);
    }

    [[nodiscard]] inline size_t
    Converter<scalar_engine>::utf16_length_from_utf32(const char32_t *input, size_t length) noexcept {
        return utf32::utf16_length_from_utf32(input, length);
    }

    [[nodiscard]] inline size_t
    Converter<scalar_engine>::utf32_length_from_utf8(const char *input, size_t length) noexcept {
        return utf8::count_code_points(input, length);
    }

}  // namespace turbo::unicode
#endif  // TURBO_UNICODE_SCALAR_CONVERTER_H_
