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

        [[nodiscard]] static inline int detect_encodings(const char *input, size_t length) noexcept;

        [[nodiscard]] static inline bool ValidateUtf8(const char *buf, size_t len) noexcept;

        [[nodiscard]] static inline result ValidateUtf8WithErrors(const char *buf, size_t len) noexcept;

        [[nodiscard]] static inline bool ValidateAscii(const char *buf, size_t len) noexcept;

        [[nodiscard]] static inline result ValidateAsciiWithErrors(const char *buf, size_t len) noexcept;

        [[nodiscard]] static inline bool ValidateUtf16Le(const char16_t *buf, size_t len) noexcept;

        [[nodiscard]] static inline bool ValidateUtf16Be(const char16_t *buf, size_t len) noexcept;

        [[nodiscard]] static inline result ValidateUtf16LeWithErrors(const char16_t *buf, size_t len) noexcept;

        [[nodiscard]] static inline result ValidateUtf16BeWithErrors(const char16_t *buf, size_t len) noexcept;

        [[nodiscard]] static inline bool ValidateUtf32(const char32_t *buf, size_t len) noexcept;

        [[nodiscard]] static inline result ValidateUtf32WithErrors(const char32_t *buf, size_t len) noexcept;

        [[nodiscard]] static inline size_t
        ConvertUtf8ToUtf16Le(const char *buf, size_t len, char16_t *utf16_output) noexcept;

        [[nodiscard]] static inline size_t
        ConvertUtf8ToUtf16Be(const char *buf, size_t len, char16_t *utf16_output) noexcept;

        [[nodiscard]] static inline result
        ConvertUtf8ToUtf16LeWithErrors(const char *buf, size_t len, char16_t *utf16_output) noexcept;

        [[nodiscard]] static inline result
        ConvertUtf8ToUtf16BeWithErrors(const char *buf, size_t len, char16_t *utf16_output) noexcept;

        [[nodiscard]] static inline size_t
        ConvertValidUtf8ToUtf16Le(const char *buf, size_t len, char16_t *utf16_buffer) noexcept;

        [[nodiscard]] static inline size_t
        ConvertValidUtf8ToUtf16Be(const char *buf, size_t len, char16_t *utf16_buffer) noexcept;

        [[nodiscard]] static inline size_t
        ConvertUtf8ToUtf32(const char *buf, size_t len, char32_t *utf32_output) noexcept;

        [[nodiscard]] static inline result
        ConvertUtf8ToUtf32WithErrors(const char *buf, size_t len, char32_t *utf32_output) noexcept;

        [[nodiscard]] static inline size_t
        ConvertValidUtf8ToUtf32(const char *buf, size_t len, char32_t *utf32_buffer) noexcept;

        [[nodiscard]] static inline size_t
        ConvertUtf16LeToUtf8(const char16_t *buf, size_t len, char *utf8_buffer) noexcept;

        [[nodiscard]] static inline size_t
        ConvertUtf16BeToUtf8(const char16_t *buf, size_t len, char *utf8_buffer) noexcept;

        [[nodiscard]] static inline result
        ConvertUtf16LeToUtf8WithErrors(const char16_t *buf, size_t len, char *utf8_buffer) noexcept;

        [[nodiscard]] static inline result
        ConvertUtf16BeToUtf8WithErrors(const char16_t *buf, size_t len, char *utf8_buffer) noexcept;

        [[nodiscard]] static inline size_t
        ConvertValidUtf16LeToUtf8(const char16_t *buf, size_t len, char *utf8_buffer) noexcept;

        [[nodiscard]] static inline size_t
        ConvertValidUtf16BeToUtf8(const char16_t *buf, size_t len, char *utf8_buffer) noexcept;

        [[nodiscard]] static inline size_t
        ConvertUtf32ToUtf8(const char32_t *buf, size_t len, char *utf8_buffer) noexcept;

        [[nodiscard]] static inline result
        ConvertUtf32ToUtf8WithErrors(const char32_t *buf, size_t len, char *utf8_buffer) noexcept;

        [[nodiscard]] static inline size_t
        ConvertValidUtf32ToUtf8(const char32_t *buf, size_t len, char *utf8_buffer) noexcept;

        [[nodiscard]] static inline size_t
        ConvertUtf32ToUtf16Le(const char32_t *buf, size_t len, char16_t *utf16_buffer) noexcept;

        [[nodiscard]] static inline size_t
        ConvertUtf32ToUtf16Be(const char32_t *buf, size_t len, char16_t *utf16_buffer) noexcept;

        [[nodiscard]] static inline result
        ConvertUtf32ToUtf16leWithErrors(const char32_t *buf, size_t len, char16_t *utf16_buffer) noexcept;

        [[nodiscard]] static inline result
        ConvertUtf32ToUtf16BeWithErrors(const char32_t *buf, size_t len, char16_t *utf16_buffer) noexcept;

        [[nodiscard]] static inline size_t
        ConvertValidUtf32ToUtf16Le(const char32_t *buf, size_t len, char16_t *utf16_buffer) noexcept;

        [[nodiscard]] static inline size_t
        ConvertValidUtf32ToUtf16Be(const char32_t *buf, size_t len, char16_t *utf16_buffer) noexcept;

        [[nodiscard]] static inline size_t
        ConvertUtf16LeToUtf32(const char16_t *buf, size_t len, char32_t *utf32_buffer) noexcept;

        [[nodiscard]] static inline size_t
        ConvertUtf16BeToUtf32(const char16_t *buf, size_t len, char32_t *utf32_buffer) noexcept;

        [[nodiscard]] static inline result
        ConvertUtf16LeToUtf32WithErrors(const char16_t *buf, size_t len, char32_t *utf32_buffer) noexcept;

        [[nodiscard]] static inline result
        ConvertUtf16BeToUtf32WithErrors(const char16_t *buf, size_t len, char32_t *utf32_buffer) noexcept;

        [[nodiscard]] static inline size_t
        ConvertValidUtf16LeToUtf32(const char16_t *buf, size_t len, char32_t *utf32_buffer) noexcept;

        [[nodiscard]] static inline size_t
        ConvertValidUtf16BeToUtf32(const char16_t *buf, size_t len, char32_t *utf32_buffer) noexcept;

        [[nodiscard]] static inline void
        ChangeEndiannessUtf16(const char16_t *buf, size_t length, char16_t *output) noexcept;

        [[nodiscard]] static inline size_t CountUtf16Le(const char16_t *buf, size_t length) noexcept;

        [[nodiscard]] static inline size_t CountUtf16Be(const char16_t *buf, size_t length) noexcept;

        [[nodiscard]] static inline size_t CountUtf8(const char *buf, size_t length) noexcept;

        [[nodiscard]] static inline size_t Utf8LengthFromUtf16Le(const char16_t *input, size_t length) noexcept;

        [[nodiscard]] static inline size_t Utf8LengthFromUtf16be(const char16_t *input, size_t length) noexcept;

        [[nodiscard]] static inline size_t Utf32LengthFromUtf16Le(const char16_t *input, size_t length) noexcept;

        [[nodiscard]] static inline size_t Utf32LengthFromUtf16Be(const char16_t *input, size_t length) noexcept;

        [[nodiscard]] static inline size_t Utf16LengthFromUtf8(const char *input, size_t length) noexcept;

        [[nodiscard]] static inline size_t Utf8LengthFromUtf32(const char32_t *input, size_t length) noexcept;

        [[nodiscard]] static inline size_t Utf16LengthFromUtf32(const char32_t *input, size_t length) noexcept;

        [[nodiscard]] static inline size_t Utf32LengthFromUtf8(const char *input, size_t length) noexcept;
    };

    /// inlines
    [[nodiscard]] inline int Converter<scalar_engine>::detect_encodings(const char *input, size_t length) noexcept {
        auto bom_encoding = turbo::BOM::check_bom(input, length);
        if (bom_encoding != EncodingType::unspecified) { return bom_encoding; }
        int out = 0;
        if (ValidateUtf8(input, length)) { out |= EncodingType::UTF8; }
        if ((length % 2) == 0) {
            if (ValidateUtf16Le(reinterpret_cast<const char16_t *>(input),
                                length / 2)) { out |= EncodingType::UTF16_LE; }
        }
        if ((length % 4) == 0) {
            if (ValidateUtf32(reinterpret_cast<const char32_t *>(input), length / 4)) { out |= EncodingType::UTF32_LE; }
        }

        return out;
    }

    [[nodiscard]] inline bool Converter<scalar_engine>::ValidateUtf8(const char *buf, size_t len) noexcept {
        return utf8::validate(buf, len);
    }

    [[nodiscard]] inline result Converter<scalar_engine>::ValidateUtf8WithErrors(const char *buf, size_t len) noexcept {
        return utf8::validate_with_errors(buf, len);
    }

    [[nodiscard]] inline bool Converter<scalar_engine>::ValidateAscii(const char *buf, size_t len) noexcept {
        return ascii::validate(buf, len);
    }

    [[nodiscard]] inline result
    Converter<scalar_engine>::ValidateAsciiWithErrors(const char *buf, size_t len) noexcept {
        return ascii::validate_with_errors(buf, len);
    }

    [[nodiscard]] inline bool Converter<scalar_engine>::ValidateUtf16Le(const char16_t *buf, size_t len) noexcept {
        return utf16::validate<EndianNess::SYS_LITTLE_ENDIAN>(buf, len);
    }

    [[nodiscard]] inline bool Converter<scalar_engine>::ValidateUtf16Be(const char16_t *buf, size_t len) noexcept {
        return utf16::validate<EndianNess::SYS_BIG_ENDIAN>(buf, len);
    }

    [[nodiscard]] inline result
    Converter<scalar_engine>::ValidateUtf16LeWithErrors(const char16_t *buf, size_t len) noexcept {
        return utf16::validate_with_errors<EndianNess::SYS_LITTLE_ENDIAN>(buf, len);
    }

    [[nodiscard]] inline result
    Converter<scalar_engine>::ValidateUtf16BeWithErrors(const char16_t *buf, size_t len) noexcept {
        return utf16::validate_with_errors<EndianNess::SYS_BIG_ENDIAN>(buf, len);
    }

    [[nodiscard]] inline bool Converter<scalar_engine>::ValidateUtf32(const char32_t *buf, size_t len) noexcept {
        return utf32::validate(buf, len);
    }

    [[nodiscard]] inline result
    Converter<scalar_engine>::ValidateUtf32WithErrors(const char32_t *buf, size_t len) noexcept {
        return utf32::validate_with_errors(buf, len);
    }

    [[nodiscard]] inline size_t
    Converter<scalar_engine>::ConvertUtf8ToUtf16Le(const char *buf, size_t len, char16_t *utf16_output) noexcept {
        return utf8_to_utf16::convert<EndianNess::SYS_LITTLE_ENDIAN>(buf, len, utf16_output);
    }

    [[nodiscard]] inline size_t
    Converter<scalar_engine>::ConvertUtf8ToUtf16Be(const char *buf, size_t len, char16_t *utf16_output) noexcept {
        return utf8_to_utf16::convert<EndianNess::SYS_BIG_ENDIAN>(buf, len, utf16_output);
    }

    [[nodiscard]] inline result Converter<scalar_engine>::ConvertUtf8ToUtf16LeWithErrors(const char *buf, size_t len,
                                                                                         char16_t *utf16_output) noexcept {
        return utf8_to_utf16::convert_with_errors<EndianNess::SYS_LITTLE_ENDIAN>(buf, len, utf16_output);
    }

    [[nodiscard]] inline result Converter<scalar_engine>::ConvertUtf8ToUtf16BeWithErrors(const char *buf, size_t len,
                                                                                         char16_t *utf16_output) noexcept {
        return utf8_to_utf16::convert_with_errors<EndianNess::SYS_BIG_ENDIAN>(buf, len, utf16_output);
    }

    [[nodiscard]] inline size_t
    Converter<scalar_engine>::ConvertValidUtf8ToUtf16Le(const char *buf, size_t len, char16_t *utf16_buffer) noexcept {
        return utf8_to_utf16::convert_valid<EndianNess::SYS_LITTLE_ENDIAN>(buf, len, utf16_buffer);
    }

    [[nodiscard]] inline size_t
    Converter<scalar_engine>::ConvertValidUtf8ToUtf16Be(const char *buf, size_t len, char16_t *utf16_buffer) noexcept {
        return utf8_to_utf16::convert_valid<EndianNess::SYS_BIG_ENDIAN>(buf, len, utf16_buffer);
    }

    [[nodiscard]] inline size_t
    Converter<scalar_engine>::ConvertUtf8ToUtf32(const char *buf, size_t len, char32_t *utf32_output) noexcept {
        return utf8_to_utf32::convert(buf, len, utf32_output);
    }

    [[nodiscard]] inline result Converter<scalar_engine>::ConvertUtf8ToUtf32WithErrors(const char *buf, size_t len,
                                                                                       char32_t *utf32_output) noexcept {
        return utf8_to_utf32::convert_with_errors(buf, len, utf32_output);
    }

    [[nodiscard]] inline size_t
    Converter<scalar_engine>::ConvertValidUtf8ToUtf32(const char *buf, size_t len, char32_t *utf32_buffer) noexcept {
        return utf8_to_utf32::convert_valid(buf, len, utf32_buffer);
    }

    [[nodiscard]] inline size_t
    Converter<scalar_engine>::ConvertUtf16LeToUtf8(const char16_t *buf, size_t len, char *utf8_buffer) noexcept {
        return utf16_to_utf8::convert<EndianNess::SYS_LITTLE_ENDIAN>(buf, len, utf8_buffer);
    }

    [[nodiscard]] inline size_t
    Converter<scalar_engine>::ConvertUtf16BeToUtf8(const char16_t *buf, size_t len, char *utf8_buffer) noexcept {
        return utf16_to_utf8::convert<EndianNess::SYS_BIG_ENDIAN>(buf, len, utf8_buffer);
    }

    [[nodiscard]] inline result
    Converter<scalar_engine>::ConvertUtf16LeToUtf8WithErrors(const char16_t *buf, size_t len,
                                                             char *utf8_buffer) noexcept {
        return utf16_to_utf8::convert_with_errors<EndianNess::SYS_LITTLE_ENDIAN>(buf, len, utf8_buffer);
    }

    [[nodiscard]] inline result
    Converter<scalar_engine>::ConvertUtf16BeToUtf8WithErrors(const char16_t *buf, size_t len,
                                                             char *utf8_buffer) noexcept {
        return utf16_to_utf8::convert_with_errors<EndianNess::SYS_BIG_ENDIAN>(buf, len, utf8_buffer);
    }

    [[nodiscard]] inline size_t
    Converter<scalar_engine>::ConvertValidUtf16LeToUtf8(const char16_t *buf, size_t len, char *utf8_buffer) noexcept {
        return utf16_to_utf8::convert_valid<EndianNess::SYS_LITTLE_ENDIAN>(buf, len, utf8_buffer);
    }

    [[nodiscard]] inline size_t
    Converter<scalar_engine>::ConvertValidUtf16BeToUtf8(const char16_t *buf, size_t len, char *utf8_buffer) noexcept {
        return utf16_to_utf8::convert_valid<EndianNess::SYS_BIG_ENDIAN>(buf, len, utf8_buffer);
    }

    [[nodiscard]] inline size_t
    Converter<scalar_engine>::ConvertUtf32ToUtf8(const char32_t *buf, size_t len, char *utf8_buffer) noexcept {
        return utf32_to_utf8::convert(buf, len, utf8_buffer);
    }

    [[nodiscard]] inline result Converter<scalar_engine>::ConvertUtf32ToUtf8WithErrors(const char32_t *buf, size_t len,
                                                                                       char *utf8_buffer) noexcept {
        return utf32_to_utf8::convert_with_errors(buf, len, utf8_buffer);
    }

    [[nodiscard]] inline size_t
    Converter<scalar_engine>::ConvertValidUtf32ToUtf8(const char32_t *buf, size_t len, char *utf8_buffer) noexcept {
        return utf32_to_utf8::convert_valid(buf, len, utf8_buffer);
    }

    [[nodiscard]] inline size_t
    Converter<scalar_engine>::ConvertUtf32ToUtf16Le(const char32_t *buf, size_t len, char16_t *utf16_buffer) noexcept {
        return utf32_to_utf16::convert<EndianNess::SYS_LITTLE_ENDIAN>(buf, len, utf16_buffer);
    }

    [[nodiscard]] inline size_t
    Converter<scalar_engine>::ConvertUtf32ToUtf16Be(const char32_t *buf, size_t len, char16_t *utf16_buffer) noexcept {
        return utf32_to_utf16::convert<EndianNess::SYS_BIG_ENDIAN>(buf, len, utf16_buffer);
    }

    [[nodiscard]] inline result
    Converter<scalar_engine>::ConvertUtf32ToUtf16leWithErrors(const char32_t *buf, size_t len,
                                                              char16_t *utf16_buffer) noexcept {
        return utf32_to_utf16::convert_with_errors<EndianNess::SYS_LITTLE_ENDIAN>(buf, len, utf16_buffer);
    }

    [[nodiscard]] inline result
    Converter<scalar_engine>::ConvertUtf32ToUtf16BeWithErrors(const char32_t *buf, size_t len,
                                                              char16_t *utf16_buffer) noexcept {
        return utf32_to_utf16::convert_with_errors<EndianNess::SYS_BIG_ENDIAN>(buf, len, utf16_buffer);
    }

    [[nodiscard]] inline size_t Converter<scalar_engine>::ConvertValidUtf32ToUtf16Le(const char32_t *buf, size_t len,
                                                                                     char16_t *utf16_buffer) noexcept {
        return utf32_to_utf16::convert_valid<EndianNess::SYS_LITTLE_ENDIAN>(buf, len, utf16_buffer);
    }

    [[nodiscard]] inline size_t Converter<scalar_engine>::ConvertValidUtf32ToUtf16Be(const char32_t *buf, size_t len,
                                                                                     char16_t *utf16_buffer) noexcept {
        return utf32_to_utf16::convert_valid<EndianNess::SYS_BIG_ENDIAN>(buf, len, utf16_buffer);
    }

    [[nodiscard]] inline size_t
    Converter<scalar_engine>::ConvertUtf16LeToUtf32(const char16_t *buf, size_t len, char32_t *utf32_buffer) noexcept {
        return utf16_to_utf32::convert<EndianNess::SYS_LITTLE_ENDIAN>(buf, len, utf32_buffer);
    }

    [[nodiscard]] inline size_t
    Converter<scalar_engine>::ConvertUtf16BeToUtf32(const char16_t *buf, size_t len, char32_t *utf32_buffer) noexcept {
        return utf16_to_utf32::convert<EndianNess::SYS_BIG_ENDIAN>(buf, len, utf32_buffer);
    }

    [[nodiscard]] inline result
    Converter<scalar_engine>::ConvertUtf16LeToUtf32WithErrors(const char16_t *buf, size_t len,
                                                              char32_t *utf32_buffer) noexcept {
        return utf16_to_utf32::convert_with_errors<EndianNess::SYS_LITTLE_ENDIAN>(buf, len, utf32_buffer);
    }

    [[nodiscard]] inline result
    Converter<scalar_engine>::ConvertUtf16BeToUtf32WithErrors(const char16_t *buf, size_t len,
                                                              char32_t *utf32_buffer) noexcept {
        return utf16_to_utf32::convert_with_errors<EndianNess::SYS_BIG_ENDIAN>(buf, len, utf32_buffer);
    }

    [[nodiscard]] inline size_t Converter<scalar_engine>::ConvertValidUtf16LeToUtf32(const char16_t *buf, size_t len,
                                                                                     char32_t *utf32_buffer) noexcept {
        return utf16_to_utf32::convert_valid<EndianNess::SYS_LITTLE_ENDIAN>(buf, len, utf32_buffer);
    }

    [[nodiscard]] inline size_t Converter<scalar_engine>::ConvertValidUtf16BeToUtf32(const char16_t *buf, size_t len,
                                                                                     char32_t *utf32_buffer) noexcept {
        return utf16_to_utf32::convert_valid<EndianNess::SYS_BIG_ENDIAN>(buf, len, utf32_buffer);
    }

    [[nodiscard]] inline void
    Converter<scalar_engine>::ChangeEndiannessUtf16(const char16_t *buf, size_t length, char16_t *output) noexcept {
        utf16::ChangeEndiannessUtf16(buf, length, output);
    }

    [[nodiscard]] inline size_t Converter<scalar_engine>::CountUtf16Le(const char16_t *buf, size_t length) noexcept {
        return utf16::count_code_points<EndianNess::SYS_LITTLE_ENDIAN>(buf, length);
    }

    [[nodiscard]] inline size_t Converter<scalar_engine>::CountUtf16Be(const char16_t *buf, size_t length) noexcept {
        return utf16::count_code_points<EndianNess::SYS_BIG_ENDIAN>(buf, length);
    }

    [[nodiscard]] inline size_t Converter<scalar_engine>::CountUtf8(const char *buf, size_t length) noexcept {
        return utf8::count_code_points(buf, length);
    }

    [[nodiscard]] inline size_t
    Converter<scalar_engine>::Utf8LengthFromUtf16Le(const char16_t *input, size_t length) noexcept {
        return utf16::Utf8LengthFromUtf16<EndianNess::SYS_LITTLE_ENDIAN>(input, length);
    }

    [[nodiscard]] inline size_t
    Converter<scalar_engine>::Utf8LengthFromUtf16be(const char16_t *input, size_t length) noexcept {
        return utf16::Utf8LengthFromUtf16<EndianNess::SYS_BIG_ENDIAN>(input, length);
    }


    [[nodiscard]] inline size_t
    Converter<scalar_engine>::Utf32LengthFromUtf16Le(const char16_t *input, size_t length) noexcept {
        return utf16::Utf32LengthFromUtf16<EndianNess::SYS_LITTLE_ENDIAN>(input, length);
    }

    [[nodiscard]] inline size_t
    Converter<scalar_engine>::Utf32LengthFromUtf16Be(const char16_t *input, size_t length) noexcept {
        return utf16::Utf32LengthFromUtf16<EndianNess::SYS_BIG_ENDIAN>(input, length);
    }

    [[nodiscard]] inline size_t
    Converter<scalar_engine>::Utf16LengthFromUtf8(const char *input, size_t length) noexcept {
        return utf8::Utf16LengthFromUtf8(input, length);
    }

    [[nodiscard]] inline size_t
    Converter<scalar_engine>::Utf8LengthFromUtf32(const char32_t *input, size_t length) noexcept {
        return utf32::Utf8LengthFromUtf32(input, length);
    }

    [[nodiscard]] inline size_t
    Converter<scalar_engine>::Utf16LengthFromUtf32(const char32_t *input, size_t length) noexcept {
        return utf32::Utf16LengthFromUtf32(input, length);
    }

    [[nodiscard]] inline size_t
    Converter<scalar_engine>::Utf32LengthFromUtf8(const char *input, size_t length) noexcept {
        return utf8::count_code_points(input, length);
    }

}  // namespace turbo::unicode
#endif  // TURBO_UNICODE_SCALAR_CONVERTER_H_
