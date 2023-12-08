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


#ifndef TURBO_UNICODE_CONVERTER_H_
#define TURBO_UNICODE_CONVERTER_H_

#include "turbo/unicode/engine.h"
#include "turbo/unicode/scalar/converter.h"
#include "turbo/unicode/haswell/converter.h"

namespace turbo {

    template<typename Engine = turbo::unicode::default_engine, turbo::check_requires<turbo::unicode::is_unicode_engine<Engine>>>
    [[nodiscard]] turbo::EncodingType AutodetectEncoding(const char *input, size_t length) noexcept;

    template<typename Engine = turbo::unicode::default_engine, turbo::check_requires<turbo::unicode::is_unicode_engine<Engine>>>
    int DetectEncodings(const char *input, size_t length) noexcept;

    template<typename Engine = turbo::unicode::default_engine, turbo::check_requires<turbo::unicode::is_unicode_engine<Engine>>>
    [[nodiscard]] bool ValidateUtf8(const char *buf, size_t len) noexcept;

    template<typename Engine = turbo::unicode::default_engine, turbo::check_requires<turbo::unicode::is_unicode_engine<Engine>>>
    [[nodiscard]] result ValidateUtf8WithErrors(const char *buf, size_t len) noexcept;

    template<typename Engine = turbo::unicode::default_engine, turbo::check_requires<turbo::unicode::is_unicode_engine<Engine>>>
    [[nodiscard]] bool ValidateAscii(const char *buf, size_t len) noexcept;

    template<typename Engine = turbo::unicode::default_engine, turbo::check_requires<turbo::unicode::is_unicode_engine<Engine>>>
    [[nodiscard]] result ValidateAsciiWithErrors(const char *buf, size_t len) noexcept;

    template<typename Engine = turbo::unicode::default_engine, turbo::check_requires<turbo::unicode::is_unicode_engine<Engine>>>
    [[nodiscard]] bool ValidateUtf16(const char16_t *buf, size_t len) noexcept;

    template<typename Engine = turbo::unicode::default_engine, turbo::check_requires<turbo::unicode::is_unicode_engine<Engine>>>
    [[nodiscard]] bool ValidateUtf16Le(const char16_t *buf, size_t len) noexcept;

    template<typename Engine = turbo::unicode::default_engine, turbo::check_requires<turbo::unicode::is_unicode_engine<Engine>>>
    [[nodiscard]] bool ValidateUtf16Be(const char16_t *buf, size_t len) noexcept;

    template<typename Engine = turbo::unicode::default_engine, turbo::check_requires<turbo::unicode::is_unicode_engine<Engine>>>
    [[nodiscard]] result ValidateUtf16WithErrors(const char16_t *buf, size_t len) noexcept;

    template<typename Engine = turbo::unicode::default_engine, turbo::check_requires<turbo::unicode::is_unicode_engine<Engine>>>
    [[nodiscard]] result ValidateUtf16LeWithErrors(const char16_t *buf, size_t len) noexcept;

    template<typename Engine = turbo::unicode::default_engine, turbo::check_requires<turbo::unicode::is_unicode_engine<Engine>>>
    [[nodiscard]] result ValidateUtf16BeWithErrors(const char16_t *buf, size_t len) noexcept;

    template<typename Engine = turbo::unicode::default_engine, turbo::check_requires<turbo::unicode::is_unicode_engine<Engine>>>
    [[nodiscard]] bool ValidateUtf32(const char32_t *buf, size_t len) noexcept;

    template<typename Engine = turbo::unicode::default_engine, turbo::check_requires<turbo::unicode::is_unicode_engine<Engine>>>
    [[nodiscard]] result ValidateUtf32WithErrors(const char32_t *buf, size_t len) noexcept;

    template<typename Engine = turbo::unicode::default_engine, turbo::check_requires<turbo::unicode::is_unicode_engine<Engine>>>
    [[nodiscard]] size_t ConvertUtf8ToUtf16(const char *input, size_t length, char16_t *utf16_output) noexcept;

    template<typename Engine = turbo::unicode::default_engine, turbo::check_requires<turbo::unicode::is_unicode_engine<Engine>>>
    [[nodiscard]] size_t ConvertUtf8ToUtf16Le(const char *input, size_t length, char16_t *utf16_output) noexcept;

    template<typename Engine = turbo::unicode::default_engine, turbo::check_requires<turbo::unicode::is_unicode_engine<Engine>>>
    [[nodiscard]] size_t ConvertUtf8ToUtf16Be(const char *input, size_t length, char16_t *utf16_output) noexcept;

    template<typename Engine = turbo::unicode::default_engine, turbo::check_requires<turbo::unicode::is_unicode_engine<Engine>>>
    [[nodiscard]] result
    ConvertUtf8ToUtf16WithErrors(const char *input, size_t length, char16_t *utf16_output) noexcept;

    template<typename Engine = turbo::unicode::default_engine, turbo::check_requires<turbo::unicode::is_unicode_engine<Engine>>>
    [[nodiscard]] result
    ConvertUtf8ToUtf16LeWithErrors(const char *input, size_t length, char16_t *utf16_output) noexcept;

    template<typename Engine = turbo::unicode::default_engine, turbo::check_requires<turbo::unicode::is_unicode_engine<Engine>>>
    [[nodiscard]] result
    ConvertUtf8ToUtf16BeWithErrors(const char *input, size_t length, char16_t *utf16_output) noexcept;

    template<typename Engine = turbo::unicode::default_engine, turbo::check_requires<turbo::unicode::is_unicode_engine<Engine>>>
    [[nodiscard]] size_t ConvertUtf8ToUtf32(const char *input, size_t length, char32_t *utf32_output) noexcept;

    template<typename Engine = turbo::unicode::default_engine, turbo::check_requires<turbo::unicode::is_unicode_engine<Engine>>>
    [[nodiscard]] result
    ConvertUtf8ToUtf32WithErrors(const char *input, size_t length, char32_t *utf32_output) noexcept;

    template<typename Engine = turbo::unicode::default_engine, turbo::check_requires<turbo::unicode::is_unicode_engine<Engine>>>
    [[nodiscard]] size_t ConvertValidUtf8ToUtf16(const char *input, size_t length, char16_t *utf16_buffer) noexcept;

    template<typename Engine = turbo::unicode::default_engine, turbo::check_requires<turbo::unicode::is_unicode_engine<Engine>>>
    [[nodiscard]] size_t ConvertValidUtf8ToUtf16Le(const char *input, size_t length, char16_t *utf16_buffer) noexcept;

    template<typename Engine = turbo::unicode::default_engine, turbo::check_requires<turbo::unicode::is_unicode_engine<Engine>>>
    [[nodiscard]] size_t ConvertValidUtf8ToUtf16Be(const char *input, size_t length, char16_t *utf16_buffer) noexcept;

    template<typename Engine = turbo::unicode::default_engine, turbo::check_requires<turbo::unicode::is_unicode_engine<Engine>>>
    [[nodiscard]] size_t ConvertValidUtf8ToUtf32(const char *input, size_t length, char32_t *utf32_buffer) noexcept;

    template<typename Engine = turbo::unicode::default_engine, turbo::check_requires<turbo::unicode::is_unicode_engine<Engine>>>
    [[nodiscard]] size_t Utf16LengthFromUtf8(const char *input, size_t length) noexcept;

    template<typename Engine = turbo::unicode::default_engine, turbo::check_requires<turbo::unicode::is_unicode_engine<Engine>>>
    [[nodiscard]] size_t Utf32LengthFromUtf8(const char *input, size_t length) noexcept;

    template<typename Engine = turbo::unicode::default_engine, turbo::check_requires<turbo::unicode::is_unicode_engine<Engine>>>
    [[nodiscard]] size_t ConvertUtf16ToUtf8(const char16_t *input, size_t length, char *utf8_buffer) noexcept;

    template<typename Engine = turbo::unicode::default_engine, turbo::check_requires<turbo::unicode::is_unicode_engine<Engine>>>
    [[nodiscard]] size_t ConvertUtf16LeToUtf8(const char16_t *input, size_t length, char *utf8_buffer) noexcept;

    template<typename Engine = turbo::unicode::default_engine, turbo::check_requires<turbo::unicode::is_unicode_engine<Engine>>>
    [[nodiscard]] size_t ConvertUtf16BeToUtf8(const char16_t *input, size_t length, char *utf8_buffer) noexcept;

    template<typename Engine = turbo::unicode::default_engine, turbo::check_requires<turbo::unicode::is_unicode_engine<Engine>>>
    [[nodiscard]] result ConvertUtf16ToUtf8WithErrors(const char16_t *input, size_t length, char *utf8_buffer) noexcept;

    template<typename Engine = turbo::unicode::default_engine, turbo::check_requires<turbo::unicode::is_unicode_engine<Engine>>>
    [[nodiscard]] result
    ConvertUtf16LeToUtf8WithErrors(const char16_t *input, size_t length, char *utf8_buffer) noexcept;

    template<typename Engine = turbo::unicode::default_engine, turbo::check_requires<turbo::unicode::is_unicode_engine<Engine>>>
    [[nodiscard]] result
    ConvertUtf16BeToUtf8WithErrors(const char16_t *input, size_t length, char *utf8_buffer) noexcept;

    template<typename Engine = turbo::unicode::default_engine, turbo::check_requires<turbo::unicode::is_unicode_engine<Engine>>>
    [[nodiscard]] size_t ConvertValidUtf16ToUtf8(const char16_t *input, size_t length, char *utf8_buffer) noexcept;

    template<typename Engine = turbo::unicode::default_engine, turbo::check_requires<turbo::unicode::is_unicode_engine<Engine>>>
    [[nodiscard]] size_t ConvertValidUtf16LeToUtf8(const char16_t *input, size_t length, char *utf8_buffer) noexcept;

    template<typename Engine = turbo::unicode::default_engine, turbo::check_requires<turbo::unicode::is_unicode_engine<Engine>>>
    [[nodiscard]] size_t ConvertValidUtf16BeToUtf8(const char16_t *input, size_t length, char *utf8_buffer) noexcept;

    template<typename Engine = turbo::unicode::default_engine, turbo::check_requires<turbo::unicode::is_unicode_engine<Engine>>>
    [[nodiscard]] size_t ConvertUtf16ToUtf32(const char16_t *input, size_t length, char32_t *utf32_buffer) noexcept;

    template<typename Engine = turbo::unicode::default_engine, turbo::check_requires<turbo::unicode::is_unicode_engine<Engine>>>
    [[nodiscard]] size_t ConvertUtf16LeToUtf32(const char16_t *input, size_t length, char32_t *utf32_buffer) noexcept;

    template<typename Engine = turbo::unicode::default_engine, turbo::check_requires<turbo::unicode::is_unicode_engine<Engine>>>
    [[nodiscard]] size_t ConvertUtf16BeToUtf32(const char16_t *input, size_t length, char32_t *utf32_buffer) noexcept;

    template<typename Engine = turbo::unicode::default_engine, turbo::check_requires<turbo::unicode::is_unicode_engine<Engine>>>
    [[nodiscard]] result
    ConvertUtf16ToUtf32WithErrors(const char16_t *input, size_t length, char32_t *utf32_buffer) noexcept;

    template<typename Engine = turbo::unicode::default_engine, turbo::check_requires<turbo::unicode::is_unicode_engine<Engine>>>
    [[nodiscard]] result
    ConvertUtf16LeToUtf32WithErrors(const char16_t *input, size_t length, char32_t *utf32_buffer) noexcept;

    template<typename Engine = turbo::unicode::default_engine, turbo::check_requires<turbo::unicode::is_unicode_engine<Engine>>>
    [[nodiscard]] result
    ConvertUtf16BeToUtf32WithErrors(const char16_t *input, size_t length, char32_t *utf32_buffer) noexcept;

    template<typename Engine = turbo::unicode::default_engine, turbo::check_requires<turbo::unicode::is_unicode_engine<Engine>>>
    [[nodiscard]] size_t
    ConvertValidUtf16ToUtf32(const char16_t *input, size_t length, char32_t *utf32_buffer) noexcept;

    template<typename Engine = turbo::unicode::default_engine, turbo::check_requires<turbo::unicode::is_unicode_engine<Engine>>>
    [[nodiscard]] size_t
    ConvertValidUtf16LeToUtf32(const char16_t *input, size_t length, char32_t *utf32_buffer) noexcept;

    template<typename Engine = turbo::unicode::default_engine, turbo::check_requires<turbo::unicode::is_unicode_engine<Engine>>>
    [[nodiscard]] size_t
    ConvertValidUtf16BeToUtf32(const char16_t *input, size_t length, char32_t *utf32_buffer) noexcept;

    template<typename Engine = turbo::unicode::default_engine, turbo::check_requires<turbo::unicode::is_unicode_engine<Engine>>>
    [[nodiscard]] size_t Utf8LengthFromUtf16(const char16_t *input, size_t length) noexcept;

    template<typename Engine = turbo::unicode::default_engine, turbo::check_requires<turbo::unicode::is_unicode_engine<Engine>>>
    [[nodiscard]] size_t Utf8LengthFromUtf16Le(const char16_t *input, size_t length) noexcept;

    template<typename Engine = turbo::unicode::default_engine, turbo::check_requires<turbo::unicode::is_unicode_engine<Engine>>>
    [[nodiscard]] size_t Utf8LengthFromUtf16be(const char16_t *input, size_t length) noexcept;

    template<typename Engine = turbo::unicode::default_engine, turbo::check_requires<turbo::unicode::is_unicode_engine<Engine>>>
    [[nodiscard]] size_t ConvertUtf32ToUtf8(const char32_t *input, size_t length, char *utf8_buffer) noexcept;

    template<typename Engine = turbo::unicode::default_engine, turbo::check_requires<turbo::unicode::is_unicode_engine<Engine>>>
    [[nodiscard]] result ConvertUtf32ToUtf8WithErrors(const char32_t *input, size_t length, char *utf8_buffer) noexcept;

    template<typename Engine = turbo::unicode::default_engine, turbo::check_requires<turbo::unicode::is_unicode_engine<Engine>>>
    [[nodiscard]] size_t ConvertValidUtf32ToUtf8(const char32_t *input, size_t length, char *utf8_buffer) noexcept;

    template<typename Engine = turbo::unicode::default_engine, turbo::check_requires<turbo::unicode::is_unicode_engine<Engine>>>
    [[nodiscard]] size_t ConvertUtf32ToUtf16(const char32_t *input, size_t length, char16_t *utf16_buffer) noexcept;

    template<typename Engine = turbo::unicode::default_engine, turbo::check_requires<turbo::unicode::is_unicode_engine<Engine>>>
    [[nodiscard]] size_t ConvertUtf32ToUtf16Le(const char32_t *input, size_t length, char16_t *utf16_buffer) noexcept;

    template<typename Engine = turbo::unicode::default_engine, turbo::check_requires<turbo::unicode::is_unicode_engine<Engine>>>
    [[nodiscard]] size_t ConvertUtf32ToUtf16Be(const char32_t *input, size_t length, char16_t *utf16_buffer) noexcept;

    template<typename Engine = turbo::unicode::default_engine, turbo::check_requires<turbo::unicode::is_unicode_engine<Engine>>>
    [[nodiscard]] result
    ConvertUtf32ToUtf16WithErrors(const char32_t *input, size_t length, char16_t *utf16_buffer) noexcept;

    template<typename Engine = turbo::unicode::default_engine, turbo::check_requires<turbo::unicode::is_unicode_engine<Engine>>>
    [[nodiscard]] result
    ConvertUtf32ToUtf16leWithErrors(const char32_t *input, size_t length, char16_t *utf16_buffer) noexcept;

    template<typename Engine = turbo::unicode::default_engine, turbo::check_requires<turbo::unicode::is_unicode_engine<Engine>>>
    [[nodiscard]] result
    ConvertUtf32ToUtf16BeWithErrors(const char32_t *input, size_t length, char16_t *utf16_buffer) noexcept;

    template<typename Engine = turbo::unicode::default_engine, turbo::check_requires<turbo::unicode::is_unicode_engine<Engine>>>
    [[nodiscard]] size_t
    ConvertValidUtf32ToUtf16(const char32_t *input, size_t length, char16_t *utf16_buffer) noexcept;

    template<typename Engine = turbo::unicode::default_engine, turbo::check_requires<turbo::unicode::is_unicode_engine<Engine>>>
    [[nodiscard]] size_t
    ConvertValidUtf32ToUtf16Le(const char32_t *input, size_t length, char16_t *utf16_buffer) noexcept;

    template<typename Engine = turbo::unicode::default_engine, turbo::check_requires<turbo::unicode::is_unicode_engine<Engine>>>
    [[nodiscard]] size_t
    ConvertValidUtf32ToUtf16Be(const char32_t *input, size_t length, char16_t *utf16_buffer) noexcept;

    template<typename Engine = turbo::unicode::default_engine, turbo::check_requires<turbo::unicode::is_unicode_engine<Engine>>>
    void ChangeEndiannessUtf16(const char16_t *input, size_t length, char16_t *output) noexcept;

    template<typename Engine = turbo::unicode::default_engine, turbo::check_requires<turbo::unicode::is_unicode_engine<Engine>>>
    [[nodiscard]] size_t Utf8LengthFromUtf32(const char32_t * input, size_t length) noexcept;

    template<typename Engine = turbo::unicode::default_engine, turbo::check_requires<turbo::unicode::is_unicode_engine<Engine>>>
    [[nodiscard]] size_t Utf16LengthFromUtf32(const char32_t * input, size_t length) noexcept;

    template<typename Engine = turbo::unicode::default_engine, turbo::check_requires<turbo::unicode::is_unicode_engine<Engine>>>
    [[nodiscard]] size_t Utf32LengthFromUtf16(const char16_t * input, size_t length) noexcept;

    template<typename Engine = turbo::unicode::default_engine, turbo::check_requires<turbo::unicode::is_unicode_engine<Engine>>>
    [[nodiscard]] size_t Utf32LengthFromUtf16Le(const char16_t * input, size_t length) noexcept;

    template<typename Engine = turbo::unicode::default_engine, turbo::check_requires<turbo::unicode::is_unicode_engine<Engine>>>
    [[nodiscard]] size_t Utf32LengthFromUtf16Be(const char16_t * input, size_t length) noexcept;

    template<typename Engine = turbo::unicode::default_engine, turbo::check_requires<turbo::unicode::is_unicode_engine<Engine>>>
    [[nodiscard]] size_t CountUtf16(const char16_t * input, size_t length) noexcept;

    template<typename Engine = turbo::unicode::default_engine, turbo::check_requires<turbo::unicode::is_unicode_engine<Engine>>>
    [[nodiscard]] size_t CountUtf16Le(const char16_t * input, size_t length) noexcept;

    template<typename Engine = turbo::unicode::default_engine, turbo::check_requires<turbo::unicode::is_unicode_engine<Engine>>>
    [[nodiscard]] size_t CountUtf16Be(const char16_t * input, size_t length) noexcept;

    template<typename Engine = turbo::unicode::default_engine, turbo::check_requires<turbo::unicode::is_unicode_engine<Engine>>>
    [[nodiscard]] size_t CountUtf8(const char * input, size_t length) noexcept;

    /// Implementation
    /*template <typename Engine, turbo::check_requires<turbo::unicode::is_unicode_engine<Engine>>>
    inline turbo::EncodingType AutodetectEncoding(const char * input, size_t length) noexcept {
        turbo::unicode::Converter<Engine>::
    }*/

    template<typename Engine, turbo::check_requires<turbo::unicode::is_unicode_engine<Engine>>>
    inline int DetectEncodings(const char *input, size_t length) noexcept {
        return turbo::unicode::Converter<Engine>::detect_encodings(input, length);
    }

    template<typename Engine, turbo::check_requires<turbo::unicode::is_unicode_engine<Engine>>>
    [[nodiscard]] inline bool ValidateUtf8(const char *buf, size_t len) noexcept {
        return turbo::unicode::Converter<Engine>::ValidateUtf8(buf, len);
    }

    template<typename Engine, turbo::check_requires<turbo::unicode::is_unicode_engine<Engine>>>
    [[nodiscard]] inline result ValidateUtf8WithErrors(const char *buf, size_t len) noexcept {
        return turbo::unicode::Converter<Engine>::ValidateUtf8WithErrors(buf, len);
    }

    template<typename Engine, turbo::check_requires<turbo::unicode::is_unicode_engine<Engine>>>
    [[nodiscard]] bool ValidateAscii(const char *buf, size_t len) noexcept {
        return turbo::unicode::Converter<Engine>::ValidateAscii(buf, len);
    }

    template<typename Engine, turbo::check_requires<turbo::unicode::is_unicode_engine<Engine>>>
    [[nodiscard]] inline result ValidateAsciiWithErrors(const char *buf, size_t len) noexcept {
        return turbo::unicode::Converter<Engine>::ValidateAsciiWithErrors(buf, len);
    }

    template<typename Engine, turbo::check_requires<turbo::unicode::is_unicode_engine<Engine>>>
    [[nodiscard]] inline bool ValidateUtf16(const char16_t *buf, size_t len) noexcept {
        return turbo::unicode::Converter<Engine>::ValidateUtf16(buf, len);
    }

    template<typename Engine, turbo::check_requires<turbo::unicode::is_unicode_engine<Engine>>>
    [[nodiscard]] inline bool ValidateUtf16Le(const char16_t *buf, size_t len) noexcept {
        return turbo::unicode::Converter<Engine>::ValidateUtf16Le(buf, len);
    }

    template<typename Engine, turbo::check_requires<turbo::unicode::is_unicode_engine<Engine>>>
    [[nodiscard]] inline bool ValidateUtf16Be(const char16_t *buf, size_t len) noexcept {
        return turbo::unicode::Converter<Engine>::ValidateUtf16Be(buf, len);
    }

    template<typename Engine, turbo::check_requires<turbo::unicode::is_unicode_engine<Engine>>>
    [[nodiscard]] inline result ValidateUtf16WithErrors(const char16_t *buf, size_t len) noexcept {
        return turbo::unicode::Converter<Engine>::ValidateUtf16WithErrors(buf, len);
    }

    template<typename Engine, turbo::check_requires<turbo::unicode::is_unicode_engine<Engine>>>
    [[nodiscard]] inline result ValidateUtf16LeWithErrors(const char16_t *buf, size_t len) noexcept {
        return turbo::unicode::Converter<Engine>::ValidateUtf16LeWithErrors(buf, len);
    }

    template<typename Engine, turbo::check_requires<turbo::unicode::is_unicode_engine<Engine>>>
    [[nodiscard]] inline result ValidateUtf16BeWithErrors(const char16_t *buf, size_t len) noexcept {
        return turbo::unicode::Converter<Engine>::ValidateUtf16BeWithErrors(buf, len);
    }

    template<typename Engine, turbo::check_requires<turbo::unicode::is_unicode_engine<Engine>>>
    [[nodiscard]] inline bool ValidateUtf32(const char32_t *buf, size_t len) noexcept {
        return turbo::unicode::Converter<Engine>::ValidateUtf32(buf, len);
    }

    template<typename Engine, turbo::check_requires<turbo::unicode::is_unicode_engine<Engine>>>
    [[nodiscard]] inline result ValidateUtf32WithErrors(const char32_t *buf, size_t len) noexcept {
        return turbo::unicode::Converter<Engine>::ValidateUtf32WithErrors(buf, len);
    }

    template<typename Engine, turbo::check_requires<turbo::unicode::is_unicode_engine<Engine>>>
    [[nodiscard]] inline size_t ConvertUtf8ToUtf16(const char *input, size_t length, char16_t *utf16_output) noexcept {
        if constexpr (kIsLittleEndian) {
            return turbo::unicode::Converter<Engine>::ConvertUtf8ToUtf16Le(input, length, utf16_output);
        } else {
            return turbo::unicode::Converter<Engine>::ConvertUtf8ToUtf16Be(input, length, utf16_output);
        }
    }

    template<typename Engine, turbo::check_requires<turbo::unicode::is_unicode_engine<Engine>>>
    [[nodiscard]] inline size_t
    ConvertUtf8ToUtf16Le(const char *input, size_t length, char16_t *utf16_output) noexcept {
        return turbo::unicode::Converter<Engine>::ConvertUtf8ToUtf16Le(input, length, utf16_output);
    }

    template<typename Engine, turbo::check_requires<turbo::unicode::is_unicode_engine<Engine>>>
    [[nodiscard]] inline size_t
    ConvertUtf8ToUtf16Be(const char *input, size_t length, char16_t *utf16_output) noexcept {
        return turbo::unicode::Converter<Engine>::ConvertUtf8ToUtf16Be(input, length, utf16_output);
    }

    template<typename Engine, turbo::check_requires<turbo::unicode::is_unicode_engine<Engine>>>
    [[nodiscard]] inline result
    ConvertUtf8ToUtf16WithErrors(const char *input, size_t length, char16_t *utf16_output) noexcept {
        if constexpr (kIsLittleEndian) {
            return turbo::unicode::Converter<Engine>::ConvertUtf8ToUtf16LeWithErrors(input, length, utf16_output);
        } else {
            return turbo::unicode::Converter<Engine>::ConvertUtf8ToUtf16BeWithErrors(input, length, utf16_output);
        }
    }

    template<typename Engine, turbo::check_requires<turbo::unicode::is_unicode_engine<Engine>>>
    [[nodiscard]] inline result
    ConvertUtf8ToUtf16LeWithErrors(const char *input, size_t length, char16_t *utf16_output) noexcept {
        return turbo::unicode::Converter<Engine>::ConvertUtf8ToUtf16LeWithErrors(input, length, utf16_output);
    }

    template<typename Engine, turbo::check_requires<turbo::unicode::is_unicode_engine<Engine>>>
    [[nodiscard]] inline result
    ConvertUtf8ToUtf16BeWithErrors(const char *input, size_t length, char16_t *utf16_output) noexcept {
        return turbo::unicode::Converter<Engine>::ConvertUtf8ToUtf16BeWithErrors(input, length, utf16_output);
    }

    template<typename Engine, turbo::check_requires<turbo::unicode::is_unicode_engine<Engine>>>
    [[nodiscard]] inline size_t ConvertUtf8ToUtf32(const char *input, size_t length, char32_t *utf32_output) noexcept {
        if constexpr (kIsLittleEndian) {
            return turbo::unicode::Converter<Engine>::ConvertUtf8ToUtf32Le(input, length, utf32_output);
        } else {
            return turbo::unicode::Converter<Engine>::ConvertUtf8ToUtf32Be(input, length, utf32_output);
        }
    }

    template<typename Engine, turbo::check_requires<turbo::unicode::is_unicode_engine<Engine>>>
    [[nodiscard]] inline result
    ConvertUtf8ToUtf32WithErrors(const char *input, size_t length, char32_t *utf32_output) noexcept {
        if constexpr (kIsLittleEndian) {
            return turbo::unicode::Converter<Engine>::ConvertUtf8ToUtf32LeWithErrors(input, length, utf32_output);
        } else {
            return turbo::unicode::Converter<Engine>::ConvertUtf8ToUtf32BeWithErrors(input, length, utf32_output);
        }
    }

    template<typename Engine, turbo::check_requires<turbo::unicode::is_unicode_engine<Engine>>>
    [[nodiscard]] size_t ConvertValidUtf8ToUtf16(const char *input, size_t length, char16_t *utf16_buffer) noexcept {
        if constexpr (kIsLittleEndian) {
            return turbo::unicode::Converter<Engine>::ConvertValidUtf8ToUtf16Le(input, length, utf16_buffer);
        } else {
            return turbo::unicode::Converter<Engine>::ConvertValidUtf8ToUtf16Be(input, length, utf16_buffer);
        }
    }

    template<typename Engine, turbo::check_requires<turbo::unicode::is_unicode_engine<Engine>>>
    [[nodiscard]] inline size_t
    ConvertValidUtf8ToUtf16Le(const char *input, size_t length, char16_t *utf16_buffer) noexcept {
        return turbo::unicode::Converter<Engine>::ConvertValidUtf8ToUtf16Le(input, length, utf16_buffer);
    }

    template<typename Engine, turbo::check_requires<turbo::unicode::is_unicode_engine<Engine>>>
    [[nodiscard]] inline size_t
    ConvertValidUtf8ToUtf16Be(const char *input, size_t length, char16_t *utf16_buffer) noexcept {
        return turbo::unicode::Converter<Engine>::ConvertValidUtf8ToUtf16Be(input, length, utf16_buffer);
    }

    template<typename Engine, turbo::check_requires<turbo::unicode::is_unicode_engine<Engine>>>
    [[nodiscard]] inline size_t
    ConvertValidUtf8ToUtf32(const char *input, size_t length, char32_t *utf32_buffer) noexcept {
        if constexpr (kIsLittleEndian) {
            return turbo::unicode::Converter<Engine>::ConvertValidUtf8ToUtf32Le(input, length, utf32_buffer);
        } else {
            return turbo::unicode::Converter<Engine>::ConvertValidUtf8ToUtf32Be(input, length, utf32_buffer);
        }
    }

    template<typename Engine, turbo::check_requires<turbo::unicode::is_unicode_engine<Engine>>>
    [[nodiscard]] inline size_t Utf16LengthFromUtf8(const char *input, size_t length) noexcept {
        return turbo::unicode::Converter<Engine>::Utf16LengthFromUtf8(input, length);
    }

    template<typename Engine, turbo::check_requires<turbo::unicode::is_unicode_engine<Engine>>>
    [[nodiscard]] inline size_t Utf32LengthFromUtf8(const char *input, size_t length) noexcept {
        return turbo::unicode::Converter<Engine>::Utf32LengthFromUtf8(input, length);
    }

    template<typename Engine, turbo::check_requires<turbo::unicode::is_unicode_engine<Engine>>>
    [[nodiscard]] inline size_t ConvertUtf16ToUtf8(const char16_t *input, size_t length, char *utf8_buffer) noexcept {
        if constexpr (kIsLittleEndian) {
            return turbo::unicode::Converter<Engine>::ConvertUtf16LeToUtf8(input, length, utf8_buffer);
        } else {
            return turbo::unicode::Converter<Engine>::ConvertUtf16BeToUtf8(input, length, utf8_buffer);
        }
    }

    template<typename Engine, turbo::check_requires<turbo::unicode::is_unicode_engine<Engine>>>
    [[nodiscard]] inline size_t ConvertUtf16LeToUtf8(const char16_t *input, size_t length, char *utf8_buffer) noexcept {
        return turbo::unicode::Converter<Engine>::ConvertUtf16LeToUtf8(input, length, utf8_buffer);
    }

    template<typename Engine, turbo::check_requires<turbo::unicode::is_unicode_engine<Engine>>>
    [[nodiscard]] inline size_t ConvertUtf16BeToUtf8(const char16_t *input, size_t length, char *utf8_buffer) noexcept {
        return turbo::unicode::Converter<Engine>::ConvertUtf16BeToUtf8(input, length, utf8_buffer);
    }

    template<typename Engine, turbo::check_requires<turbo::unicode::is_unicode_engine<Engine>>>
    [[nodiscard]] inline result
    ConvertUtf16ToUtf8WithErrors(const char16_t *input, size_t length, char *utf8_buffer) noexcept {
        if constexpr (kIsLittleEndian) {
            return turbo::unicode::Converter<Engine>::ConvertUtf16LeToUtf8WithErrors(input, length, utf8_buffer);
        } else {
            return turbo::unicode::Converter<Engine>::ConvertUtf16BeToUtf8WithErrors(input, length, utf8_buffer);
        }
    }

    template<typename Engine, turbo::check_requires<turbo::unicode::is_unicode_engine<Engine>>>
    [[nodiscard]] inline result
    ConvertUtf16LeToUtf8WithErrors(const char16_t *input, size_t length, char *utf8_buffer) noexcept {
        return turbo::unicode::Converter<Engine>::ConvertUtf16LeToUtf8WithErrors(input, length, utf8_buffer);
    }

    template<typename Engine, turbo::check_requires<turbo::unicode::is_unicode_engine<Engine>>>
    [[nodiscard]] inline result
    ConvertUtf16BeToUtf8WithErrors(const char16_t *input, size_t length, char *utf8_buffer) noexcept {
        return turbo::unicode::Converter<Engine>::ConvertUtf16BeToUtf8WithErrors(input, length, utf8_buffer);
    }

    template<typename Engine, turbo::check_requires<turbo::unicode::is_unicode_engine<Engine>>>
    [[nodiscard]] inline size_t
    ConvertValidUtf16ToUtf8(const char16_t *input, size_t length, char *utf8_buffer) noexcept {
        if constexpr (kIsLittleEndian) {
            return turbo::unicode::Converter<Engine>::ConvertValidUtf16LeToUtf8(input, length, utf8_buffer);
        } else {
            return turbo::unicode::Converter<Engine>::ConvertValidUtf16BeToUtf8(input, length, utf8_buffer);
        }
    }

    template<typename Engine, turbo::check_requires<turbo::unicode::is_unicode_engine<Engine>>>
    [[nodiscard]] inline size_t
    ConvertValidUtf16LeToUtf8(const char16_t *input, size_t length, char *utf8_buffer) noexcept {
        return turbo::unicode::Converter<Engine>::ConvertValidUtf16LeToUtf8(input, length, utf8_buffer);
    }

    template<typename Engine, turbo::check_requires<turbo::unicode::is_unicode_engine<Engine>>>
    [[nodiscard]] inline size_t
    ConvertValidUtf16BeToUtf8(const char16_t *input, size_t length, char *utf8_buffer) noexcept {
        return turbo::unicode::Converter<Engine>::ConvertValidUtf16BeToUtf8(input, length, utf8_buffer);
    }

    template<typename Engine, turbo::check_requires<turbo::unicode::is_unicode_engine<Engine>>>
    [[nodiscard]] inline size_t
    ConvertUtf16ToUtf32(const char16_t *input, size_t length, char32_t *utf32_buffer) noexcept {
        if constexpr (kIsLittleEndian) {
            return turbo::unicode::Converter<Engine>::ConvertUtf16LeToUtf32(input, length, utf32_buffer);
        } else {
            return turbo::unicode::Converter<Engine>::ConvertUtf16BeToUtf32(input, length, utf32_buffer);
        }
    }

    template<typename Engine, turbo::check_requires<turbo::unicode::is_unicode_engine<Engine>>>
    [[nodiscard]] inline size_t
    ConvertUtf16LeToUtf32(const char16_t *input, size_t length, char32_t *utf32_buffer) noexcept {
        return turbo::unicode::Converter<Engine>::ConvertUtf16LeToUtf32(input, length, utf32_buffer);
    }

    template<typename Engine, turbo::check_requires<turbo::unicode::is_unicode_engine<Engine>>>
    [[nodiscard]] inline size_t
    ConvertUtf16BeToUtf32(const char16_t *input, size_t length, char32_t *utf32_buffer) noexcept {
        return turbo::unicode::Converter<Engine>::ConvertUtf16BeToUtf32(input, length, utf32_buffer);
    }

    template<typename Engine, turbo::check_requires<turbo::unicode::is_unicode_engine<Engine>>>
    [[nodiscard]] inline result
    ConvertUtf16ToUtf32WithErrors(const char16_t *input, size_t length, char32_t *utf32_buffer) noexcept {
        if constexpr (kIsLittleEndian) {
            return turbo::unicode::Converter<Engine>::ConvertUtf16LeToUtf32WithErrors(input, length, utf32_buffer);
        } else {
            return turbo::unicode::Converter<Engine>::ConvertUtf16BeToUtf32WithErrors(input, length, utf32_buffer);
        }
    }

    template<typename Engine, turbo::check_requires<turbo::unicode::is_unicode_engine<Engine>>>
    [[nodiscard]] inline result
    ConvertUtf16LeToUtf32WithErrors(const char16_t *input, size_t length, char32_t *utf32_buffer) noexcept {
        return turbo::unicode::Converter<Engine>::ConvertUtf16LeToUtf32WithErrors(input, length, utf32_buffer);
    }

    template<typename Engine, turbo::check_requires<turbo::unicode::is_unicode_engine<Engine>>>
    [[nodiscard]] inline result
    ConvertUtf16BeToUtf32WithErrors(const char16_t *input, size_t length, char32_t *utf32_buffer) noexcept {
        return turbo::unicode::Converter<Engine>::ConvertUtf16BeToUtf32WithErrors(input, length, utf32_buffer);
    }

    template<typename Engine, turbo::check_requires<turbo::unicode::is_unicode_engine<Engine>>>
    [[nodiscard]] inline size_t
    ConvertValidUtf16ToUtf32(const char16_t *input, size_t length, char32_t *utf32_buffer) noexcept {
        if constexpr (kIsLittleEndian) {
            return turbo::unicode::Converter<Engine>::ConvertValidUtf16LeToUtf32(input, length, utf32_buffer);
        } else {
            return turbo::unicode::Converter<Engine>::ConvertValidUtf16BeToUtf32(input, length, utf32_buffer);
        }
    }

    template<typename Engine, turbo::check_requires<turbo::unicode::is_unicode_engine<Engine>>>
    [[nodiscard]] inline size_t
    ConvertValidUtf16LeToUtf32(const char16_t *input, size_t length, char32_t *utf32_buffer) noexcept {
        return turbo::unicode::Converter<Engine>::ConvertValidUtf16LeToUtf32(input, length, utf32_buffer);
    }

    template<typename Engine, turbo::check_requires<turbo::unicode::is_unicode_engine<Engine>>>
    [[nodiscard]] inline size_t
    ConvertValidUtf16BeToUtf32(const char16_t *input, size_t length, char32_t *utf32_buffer) noexcept {
        return turbo::unicode::Converter<Engine>::ConvertValidUtf16BeToUtf32(input, length, utf32_buffer);
    }

    template<typename Engine, turbo::check_requires<turbo::unicode::is_unicode_engine<Engine>>>
    [[nodiscard]] inline size_t Utf8LengthFromUtf16(const char16_t *input, size_t length) noexcept {
        if constexpr (kIsLittleEndian) {
            return turbo::unicode::Converter<Engine>::Utf8LengthFromUtf16Le(input, length);
        } else {
            return turbo::unicode::Converter<Engine>::Utf8LengthFromUtf16Be(input, length);
        }
    }

    template<typename Engine, turbo::check_requires<turbo::unicode::is_unicode_engine<Engine>>>
    [[nodiscard]] inline size_t Utf8LengthFromUtf16Le(const char16_t *input, size_t length) noexcept {
        return turbo::unicode::Converter<Engine>::Utf8LengthFromUtf16Le(input, length);
    }

    template<typename Engine, turbo::check_requires<turbo::unicode::is_unicode_engine<Engine>>>
    [[nodiscard]] inline size_t Utf8LengthFromUtf16be(const char16_t *input, size_t length) noexcept {
        return turbo::unicode::Converter<Engine>::Utf8LengthFromUtf16Be(input, length);
    }

    template<typename Engine, turbo::check_requires<turbo::unicode::is_unicode_engine<Engine>>>
    [[nodiscard]] inline size_t ConvertUtf32ToUtf8(const char32_t *input, size_t length, char *utf8_buffer) noexcept {
        return turbo::unicode::Converter<Engine>::ConvertUtf32ToUtf8(input, length, utf8_buffer);
    }

    template<typename Engine, turbo::check_requires<turbo::unicode::is_unicode_engine<Engine>>>
    [[nodiscard]] inline result
    ConvertUtf32ToUtf8WithErrors(const char32_t *input, size_t length, char *utf8_buffer) noexcept {
        return turbo::unicode::Converter<Engine>::ConvertUtf32ToUtf8WithErrors(input, length, utf8_buffer);
    }

    template<typename Engine, turbo::check_requires<turbo::unicode::is_unicode_engine<Engine>>>
    [[nodiscard]] inline size_t
    ConvertValidUtf32ToUtf8(const char32_t *input, size_t length, char *utf8_buffer) noexcept {
        return turbo::unicode::Converter<Engine>::ConvertValidUtf32ToUtf8(input, length, utf8_buffer);
    }

    template<typename Engine, turbo::check_requires<turbo::unicode::is_unicode_engine<Engine>>>
    [[nodiscard]] inline size_t
    ConvertUtf32ToUtf16(const char32_t *input, size_t length, char16_t *utf16_buffer) noexcept {
        if constexpr (kIsLittleEndian) {
            return turbo::unicode::Converter<Engine>::ConvertUtf32ToUtf16Le(input, length, utf16_buffer);
        } else {
            return turbo::unicode::Converter<Engine>::ConvertUtf32ToUtf16Be(input, length, utf16_buffer);
        }
    }

    template<typename Engine, turbo::check_requires<turbo::unicode::is_unicode_engine<Engine>>>
    [[nodiscard]] inline size_t
    ConvertUtf32ToUtf16Le(const char32_t *input, size_t length, char16_t *utf16_buffer) noexcept {
        return turbo::unicode::Converter<Engine>::ConvertUtf32ToUtf16Le(input, length, utf16_buffer);
    }

    template<typename Engine, turbo::check_requires<turbo::unicode::is_unicode_engine<Engine>>>
    [[nodiscard]] inline size_t
    ConvertUtf32ToUtf16Be(const char32_t *input, size_t length, char16_t *utf16_buffer) noexcept {
        return turbo::unicode::Converter<Engine>::ConvertUtf32ToUtf16Be(input, length, utf16_buffer);
    }

    template<typename Engine, turbo::check_requires<turbo::unicode::is_unicode_engine<Engine>>>
    [[nodiscard]] inline result
    ConvertUtf32ToUtf16WithErrors(const char32_t *input, size_t length, char16_t *utf16_buffer) noexcept {
        if constexpr (kIsLittleEndian) {
            return turbo::unicode::Converter<Engine>::ConvertUtf32ToUtf16LeWithErrors(input, length, utf16_buffer);
        } else {
            return turbo::unicode::Converter<Engine>::ConvertUtf32ToUtf16BeWithErrors(input, length, utf16_buffer);
        }
    }

    template<typename Engine, turbo::check_requires<turbo::unicode::is_unicode_engine<Engine>>>
    [[nodiscard]] inline result
    ConvertUtf32ToUtf16leWithErrors(const char32_t *input, size_t length, char16_t *utf16_buffer) noexcept {
        return turbo::unicode::Converter<Engine>::ConvertUtf32ToUtf16LeWithErrors(input, length, utf16_buffer);
    }

    template<typename Engine, turbo::check_requires<turbo::unicode::is_unicode_engine<Engine>>>
    [[nodiscard]] result
    ConvertUtf32ToUtf16BeWithErrors(const char32_t *input, size_t length, char16_t *utf16_buffer) noexcept {
        return turbo::unicode::Converter<Engine>::ConvertUtf32ToUtf16BeWithErrors(input, length, utf16_buffer);
    }

    template<typename Engine, turbo::check_requires<turbo::unicode::is_unicode_engine<Engine>>>
    [[nodiscard]] inline size_t
    ConvertValidUtf32ToUtf16(const char32_t *input, size_t length, char16_t *utf16_buffer) noexcept {
        if constexpr (kIsLittleEndian) {
            return turbo::unicode::Converter<Engine>::ConvertValidUtf32ToUtf16Le(input, length, utf16_buffer);
        } else {
            return turbo::unicode::Converter<Engine>::ConvertValidUtf32ToUtf16Be(input, length, utf16_buffer);
        }
    }

    template<typename Engine, turbo::check_requires<turbo::unicode::is_unicode_engine<Engine>>>
    [[nodiscard]] inline size_t
    ConvertValidUtf32ToUtf16Le(const char32_t *input, size_t length, char16_t *utf16_buffer) noexcept {
        return turbo::unicode::Converter<Engine>::ConvertValidUtf32ToUtf16Le(input, length, utf16_buffer);
    }

    template<typename Engine, turbo::check_requires<turbo::unicode::is_unicode_engine<Engine>>>
    [[nodiscard]] inline size_t
    ConvertValidUtf32ToUtf16Be(const char32_t *input, size_t length, char16_t *utf16_buffer) noexcept {
        return turbo::unicode::Converter<Engine>::ConvertValidUtf32ToUtf16Be(input, length, utf16_buffer);
    }

    template<typename Engine, turbo::check_requires<turbo::unicode::is_unicode_engine<Engine>>>
    inline void ChangeEndiannessUtf16(const char16_t *input, size_t length, char16_t *output) noexcept {
        turbo::unicode::Converter<Engine>::ChangeEndiannessUtf16(input, length, output);
    }

    template<typename Engine, turbo::check_requires<turbo::unicode::is_unicode_engine<Engine>>>
    [[nodiscard]] inline size_t Utf8LengthFromUtf32(const char32_t * input, size_t length) noexcept {
        return turbo::unicode::Converter<Engine>::Utf8LengthFromUtf32(input, length);
    }

    template<typename Engine, turbo::check_requires<turbo::unicode::is_unicode_engine<Engine>>>
    [[nodiscard]] inline size_t Utf16LengthFromUtf32(const char32_t * input, size_t length) noexcept {
        return turbo::unicode::Converter<Engine>::Utf16LengthFromUtf32(input, length);
    }

    template<typename Engine, turbo::check_requires<turbo::unicode::is_unicode_engine<Engine>>>
    [[nodiscard]] inline size_t Utf32LengthFromUtf16(const char16_t * input, size_t length) noexcept {
        if constexpr (kIsLittleEndian) {
            return turbo::unicode::Converter<Engine>::Utf32LengthFromUtf16Le(input, length);
        } else {
            return turbo::unicode::Converter<Engine>::Utf32LengthFromUtf16Be(input, length);
        }
    }

    template<typename Engine, turbo::check_requires<turbo::unicode::is_unicode_engine<Engine>>>
    [[nodiscard]] inline size_t Utf32LengthFromUtf16Le(const char16_t * input, size_t length) noexcept {
        return turbo::unicode::Converter<Engine>::Utf32LengthFromUtf16Le(input, length);
    }

    template<typename Engine, turbo::check_requires<turbo::unicode::is_unicode_engine<Engine>>>
    [[nodiscard]] inline size_t Utf32LengthFromUtf16Be(const char16_t * input, size_t length) noexcept {
        return turbo::unicode::Converter<Engine>::Utf32LengthFromUtf16Be(input, length);
    }

    template<typename Engine, turbo::check_requires<turbo::unicode::is_unicode_engine<Engine>>>
    [[nodiscard]] inline size_t CountUtf16(const char16_t * input, size_t length) noexcept {
        if constexpr (kIsLittleEndian) {
            return turbo::unicode::Converter<Engine>::CountUtf16Le(input, length);
        } else {
            return turbo::unicode::Converter<Engine>::CountUtf16Be(input, length);
        }
    }

    template<typename Engine, turbo::check_requires<turbo::unicode::is_unicode_engine<Engine>>>
    [[nodiscard]] inline size_t CountUtf16Le(const char16_t * input, size_t length) noexcept {
        return turbo::unicode::Converter<Engine>::CountUtf16Le(input, length);
    }

    template<typename Engine, turbo::check_requires<turbo::unicode::is_unicode_engine<Engine>>>
    [[nodiscard]] inline size_t CountUtf16Be(const char16_t * input, size_t length) noexcept {
        return turbo::unicode::Converter<Engine>::CountUtf16Be(input, length);
    }

    template<typename Engine, turbo::check_requires<turbo::unicode::is_unicode_engine<Engine>>>
    [[nodiscard]] inline size_t CountUtf8(const char * input, size_t length) noexcept {
        return turbo::unicode::Converter<Engine>::CountUtf8(input, length);
    }
}  // namespace turbo

#endif  // TURBO_UNICODE_CONVERTER_H_
