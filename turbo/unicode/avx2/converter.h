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

#ifndef TURBO_UNICODE_AVX2_CONVERTER_H_
#define TURBO_UNICODE_AVX2_CONVERTER_H_

#include "turbo/unicode/fwd.h"
#include "turbo/unicode/avx2/engine.h"
#include "turbo/unicode/encoding_types.h"
#include "turbo/unicode/error.h"
#include "turbo/unicode/simd/utf8.h"
#include "turbo/unicode/simd/utf8_validator.h"
#include "turbo/unicode/simd/utf8_to_utf16.h"
#include "turbo/unicode/simd/utf8_to_utf32.h"
#include "turbo/unicode/simd/valid_utf8_to_utf16.h"
#include "turbo/unicode/simd/valid_utf8_to_utf32.h"
#include "turbo/unicode/avx2/validate.h"
#include "turbo/unicode/avx2/avx2_utf16.h"
#include "turbo/unicode/avx2/avx2_utf32.h"
#include "turbo/unicode/scalar/validate.h"

namespace turbo::unicode {

    template<>
    struct Converter<avx2_engine> {
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

    [[nodiscard]] inline turbo::EncodingType
    Converter<avx2_engine>::auto_detect_encoding(const char *input, size_t length) noexcept {
        // If there is a BOM, then we trust it.
        auto bom_encoding = BOM::check_bom(input, length);
        if (bom_encoding != EncodingType::unspecified) { return bom_encoding; }
        // UTF8 is common, it includes ASCII, and is commonly represented
        // without a BOM, so if it fits, go with that. Note that it is still
        // possible to get it wrong, we are only 'guessing'. If some has UTF-16
        // data without a BOM, it could pass as UTF-8.
        //
        // An interesting twist might be to check for UTF-16 ASCII first (every
        // other byte is zero).
        if (validate_utf8(input, length)) { return EncodingType::UTF8; }
        // The next most common encoding that might appear without BOM is probably
        // UTF-16LE, so try that next.
        if ((length % 2) == 0) {
            // important: we need to divide by two
            if (validate_utf16le(reinterpret_cast<const char16_t *>(input),
                                 length / 2)) { return EncodingType::UTF16_LE; }
        }
        if ((length % 4) == 0) {
            if (validate_utf32(reinterpret_cast<const char32_t *>(input),
                               length / 4)) { return EncodingType::UTF32_LE; }
        }
        return EncodingType::unspecified;
    }

    [[nodiscard]] inline int Converter<avx2_engine>::detect_encodings(const char *input, size_t length) noexcept {
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

    [[nodiscard]] inline bool Converter<avx2_engine>::validate_utf8(const char *buf, size_t len) noexcept {
        return turbo::unicode::simd::utf8_validation::generic_validate_utf8<avx2_engine>(buf, len);
    }

    [[nodiscard]] inline UnicodeResult
    Converter<avx2_engine>::validate_utf8_with_errors(const char *buf, size_t len) noexcept {
        return turbo::unicode::simd::utf8_validation::generic_validate_utf8_with_errors<avx2_engine>(buf, len);
    }

    [[nodiscard]] inline bool Converter<avx2_engine>::validate_ascii(const char *buf, size_t len) noexcept {
        return turbo::unicode::simd::utf8_validation::generic_validate_ascii<avx2_engine>(buf, len);
    }

    [[nodiscard]] inline UnicodeResult
    Converter<avx2_engine>::validate_ascii_with_errors(const char *buf, size_t len) noexcept {
        return turbo::unicode::simd::utf8_validation::generic_validate_ascii_with_errors<avx2_engine>(buf, len);
    }

    [[nodiscard]] inline bool Converter<avx2_engine>::validate_utf16le(const char16_t *buf, size_t len) noexcept {
        const char16_t *tail = turbo::unicode::simd::avx2_validate_utf16<EndianNess::SYS_LITTLE_ENDIAN>(buf, len);
        if (tail != nullptr) {
            return turbo::unicode::utf16::validate<EndianNess::SYS_LITTLE_ENDIAN>(tail, len - (tail - buf));
        }
        return false;
    }

    [[nodiscard]] inline bool Converter<avx2_engine>::validate_utf16be(const char16_t *buf, size_t len) noexcept {
        const char16_t *tail = turbo::unicode::simd::avx2_validate_utf16<EndianNess::SYS_BIG_ENDIAN>(buf, len);
        if (tail != nullptr) {
            return turbo::unicode::utf16::validate<EndianNess::SYS_BIG_ENDIAN>(tail, len - (tail - buf));
        }
        return false;
    }

    [[nodiscard]] inline UnicodeResult
    Converter<avx2_engine>::validate_utf16le_with_errors(const char16_t *buf, size_t len) noexcept {
        UnicodeResult res = turbo::unicode::simd::avx2_validate_utf16_with_errors<EndianNess::SYS_LITTLE_ENDIAN>(buf, len);
        if (res.count != len) {
            UnicodeResult scalar_res = turbo::unicode::utf16::validate_with_errors<EndianNess::SYS_LITTLE_ENDIAN>(buf + res.count, len - res.count);
            return UnicodeResult(scalar_res.error, res.count + scalar_res.count);
        }
        return res;
    }

    [[nodiscard]] inline UnicodeResult
    Converter<avx2_engine>::validate_utf16be_with_errors(const char16_t *buf, size_t len) noexcept {
        UnicodeResult res = turbo::unicode::simd::avx2_validate_utf16_with_errors<EndianNess::SYS_BIG_ENDIAN>(buf, len);
        if (res.count != len) {
            UnicodeResult scalar_res = turbo::unicode::utf16::validate_with_errors<EndianNess::SYS_BIG_ENDIAN>(buf + res.count, len - res.count);
            return UnicodeResult(scalar_res.error, res.count + scalar_res.count);
        }
        return res;
    }

    [[nodiscard]] inline bool Converter<avx2_engine>::validate_utf32(const char32_t *buf, size_t len) noexcept {
        const char32_t *tail = turbo::unicode::simd::avx2_validate_utf32le(buf, len);
        if (tail) {
            return turbo::unicode::utf32::validate(tail, len - (tail - buf));
        }
        return false;

    }

    [[nodiscard]] inline UnicodeResult
    Converter<avx2_engine>::validate_utf32_with_errors(const char32_t *buf, size_t len) noexcept {
        UnicodeResult res = turbo::unicode::simd::avx2_validate_utf32le_with_errors(buf, len);
        if (res.count != len) {
            UnicodeResult scalar_res = turbo::unicode::utf32::validate_with_errors(buf + res.count, len - res.count);
            return UnicodeResult(scalar_res.error, res.count + scalar_res.count);
        }
        return res;
    }

    [[nodiscard]] inline size_t
    Converter<avx2_engine>::convert_utf8_to_utf16le(const char *buf, size_t len, char16_t *utf16_output) noexcept {
        turbo::unicode::simd::utf8_to_utf16::validating_transcoder<avx2_engine> converter;
        return converter.convert<EndianNess::SYS_LITTLE_ENDIAN>(buf, len, utf16_output);
    }

    [[nodiscard]] inline size_t
    Converter<avx2_engine>::convert_utf8_to_utf16be(const char *buf, size_t len, char16_t *utf16_output) noexcept {
        turbo::unicode::simd::utf8_to_utf16::validating_transcoder<avx2_engine> converter;
        return converter.convert<EndianNess::SYS_BIG_ENDIAN>(buf, len, utf16_output);
    }

    [[nodiscard]] inline UnicodeResult
    Converter<avx2_engine>::convert_utf8_to_utf16le_with_errors(const char *buf, size_t len, char16_t *utf16_output) noexcept {
        turbo::unicode::simd::utf8_to_utf16::validating_transcoder<avx2_engine> converter;
        return converter.convert_with_errors<EndianNess::SYS_LITTLE_ENDIAN>(buf, len, utf16_output);
    }

    [[nodiscard]] inline UnicodeResult
    Converter<avx2_engine>::convert_utf8_to_utf16be_with_errors(const char *buf, size_t len, char16_t *utf16_output) noexcept {
        turbo::unicode::simd::utf8_to_utf16::validating_transcoder<avx2_engine> converter;
        return converter.convert_with_errors<EndianNess::SYS_BIG_ENDIAN>(buf, len, utf16_output);
    }

    [[nodiscard]] inline size_t
    Converter<avx2_engine>::convert_valid_utf8_to_utf16le(const char *buf, size_t len, char16_t *utf16_buffer) noexcept {
        return turbo::unicode::simd::utf8_to_utf16::convert_valid<EndianNess::SYS_LITTLE_ENDIAN, avx2_engine>(buf, len,  utf16_buffer);
    }

    [[nodiscard]] inline size_t
    Converter<avx2_engine>::convert_valid_utf8_to_utf16be(const char *buf, size_t len, char16_t *utf16_buffer) noexcept {
        return turbo::unicode::simd::utf8_to_utf16::convert_valid<EndianNess::SYS_BIG_ENDIAN, avx2_engine>(buf, len,  utf16_buffer);
    }

    [[nodiscard]] inline size_t
    Converter<avx2_engine>::convert_utf8_to_utf32(const char *buf, size_t len, char32_t *utf32_output) noexcept {
        turbo::unicode::simd::utf8_to_utf32::validating_transcoder<avx2_engine> converter;
        return converter.convert(buf, len, utf32_output);
    }

    [[nodiscard]] inline UnicodeResult
    Converter<avx2_engine>::convert_utf8_to_utf32_with_errors(const char *buf, size_t len, char32_t *utf32_output) noexcept {
        turbo::unicode::simd::utf8_to_utf32::validating_transcoder<avx2_engine> converter;
        return converter.convert_with_errors(buf, len, utf32_output);
    }

    [[nodiscard]] inline size_t
    Converter<avx2_engine>::convert_valid_utf8_to_utf32(const char *buf, size_t len, char32_t *utf32_buffer) noexcept {
        return turbo::unicode::simd::utf8_to_utf32::convert_valid<avx2_engine>(buf, len,  utf32_buffer);
    }

    [[nodiscard]] inline size_t
    Converter<avx2_engine>::convert_utf16le_to_utf8(const char16_t *buf, size_t len, char *utf8_buffer) noexcept {
        std::pair<const char16_t*, char*> ret = turbo::unicode::simd::avx2_convert_utf16_to_utf8<EndianNess::SYS_LITTLE_ENDIAN>(buf, len, utf8_buffer);
        if (ret.first == nullptr) { return 0; }
        size_t saved_bytes = ret.second - utf8_buffer;
        if (ret.first != buf + len) {
            const size_t scalar_saved_bytes = turbo::unicode::utf16_to_utf8::convert<EndianNess::SYS_LITTLE_ENDIAN>(
                    ret.first, len - (ret.first - buf), ret.second);
            if (scalar_saved_bytes == 0) { return 0; }
            saved_bytes += scalar_saved_bytes;
        }
        return saved_bytes;
    }

    [[nodiscard]] inline size_t
    Converter<avx2_engine>::convert_utf16be_to_utf8(const char16_t *buf, size_t len, char *utf8_buffer) noexcept {
        std::pair<const char16_t*, char*> ret = turbo::unicode::simd::avx2_convert_utf16_to_utf8<EndianNess::SYS_BIG_ENDIAN>(buf, len, utf8_buffer);
        if (ret.first == nullptr) { return 0; }
        size_t saved_bytes = ret.second - utf8_buffer;
        if (ret.first != buf + len) {
            const size_t scalar_saved_bytes = turbo::unicode::utf16_to_utf8::convert<EndianNess::SYS_BIG_ENDIAN>(
                    ret.first, len - (ret.first - buf), ret.second);
            if (scalar_saved_bytes == 0) { return 0; }
            saved_bytes += scalar_saved_bytes;
        }
        return saved_bytes;
    }

    [[nodiscard]] inline UnicodeResult
    Converter<avx2_engine>::convert_utf16le_to_utf8_with_errors(const char16_t *buf, size_t len, char *utf8_buffer) noexcept {
        std::pair<UnicodeResult, char*> ret = turbo::unicode::simd::avx2_convert_utf16_to_utf8_with_errors<EndianNess::SYS_LITTLE_ENDIAN>(buf, len, utf8_buffer);
        if (is_unicode_error(ret.first)) {
            return ret.first;
        }

        if (ret.first.count != len) {
            UnicodeResult scalar_res = turbo::unicode::utf16_to_utf8::convert_with_errors<EndianNess::SYS_LITTLE_ENDIAN>(
                    buf + ret.first.count, len - ret.first.count, ret.second);
            if (is_unicode_error(scalar_res)) {
                scalar_res.count += ret.first.count;
                return scalar_res;
            } else {
                ret.second += scalar_res.count;
            }
        }
        ret.first.count = ret.second - utf8_buffer;   // Set count to the number of 8-bit code units written
        return ret.first;
    }

    [[nodiscard]] inline UnicodeResult
    Converter<avx2_engine>::convert_utf16be_to_utf8_with_errors(const char16_t *buf, size_t len, char *utf8_buffer) noexcept {
        std::pair<UnicodeResult, char*> ret = turbo::unicode::simd::avx2_convert_utf16_to_utf8_with_errors<EndianNess::SYS_BIG_ENDIAN>(buf, len, utf8_buffer);
        if (is_unicode_error(ret.first)) {
            return ret.first;
        }

        if (ret.first.count != len) {
            UnicodeResult scalar_res = turbo::unicode::utf16_to_utf8::convert_with_errors<EndianNess::SYS_BIG_ENDIAN>(
                    buf + ret.first.count, len - ret.first.count, ret.second);
            if (is_unicode_error(scalar_res)) {
                scalar_res.count += ret.first.count;
                return scalar_res;
            } else {
                ret.second += scalar_res.count;
            }
        }
        ret.first.count = ret.second - utf8_buffer;   // Set count to the number of 8-bit code units written
        return ret.first;
    }

    [[nodiscard]] inline size_t
    Converter<avx2_engine>::convert_valid_utf16le_to_utf8(const char16_t *buf, size_t len, char *utf8_buffer) noexcept {
        return convert_utf16le_to_utf8(buf, len, utf8_buffer);
    }

    [[nodiscard]] inline size_t
    Converter<avx2_engine>::convert_valid_utf16be_to_utf8(const char16_t *buf, size_t len, char *utf8_buffer) noexcept {
        return convert_utf16be_to_utf8(buf, len, utf8_buffer);
    }

    [[nodiscard]] inline size_t
    Converter<avx2_engine>::convert_utf32_to_utf8(const char32_t *buf, size_t len, char *utf8_buffer) noexcept {
        std::pair<const char32_t*, char*> ret = turbo::unicode::simd::avx2_convert_utf32_to_utf8(buf, len, utf8_buffer);
        if (ret.first == nullptr) { return 0; }
        size_t saved_bytes = ret.second - utf8_buffer;
        if (ret.first != buf + len) {
            const size_t scalar_saved_bytes = turbo::unicode::utf32_to_utf8::convert(
                    ret.first, len - (ret.first - buf), ret.second);
            if (scalar_saved_bytes == 0) { return 0; }
            saved_bytes += scalar_saved_bytes;
        }
        return saved_bytes;
    }

    [[nodiscard]] inline UnicodeResult
    Converter<avx2_engine>::convert_utf32_to_utf8_with_errors(const char32_t *buf, size_t len, char *utf8_buffer) noexcept {
        std::pair<UnicodeResult, char*> ret = turbo::unicode::simd::avx2_convert_utf32_to_utf8_with_errors(buf, len, utf8_buffer);
        if (ret.first.count != len) {
            UnicodeResult scalar_res = turbo::unicode::utf32_to_utf8::convert_with_errors(
                    buf + ret.first.count, len - ret.first.count, ret.second);
            if (is_unicode_error(scalar_res)) {
                scalar_res.count += ret.first.count;
                return scalar_res;
            } else {
                ret.second += scalar_res.count;
            }
        }
        ret.first.count = ret.second - utf8_buffer;   // Set count to the number of 8-bit code units written
        return ret.first;
    }

    [[nodiscard]] inline size_t
    Converter<avx2_engine>::convert_valid_utf32_to_utf8(const char32_t *buf, size_t len, char *utf8_buffer) noexcept {
        return convert_utf32_to_utf8(buf, len, utf8_buffer);
    }

    [[nodiscard]] inline size_t
    Converter<avx2_engine>::convert_utf32_to_utf16le(const char32_t *buf, size_t len, char16_t *utf16_buffer) noexcept {
        std::pair<const char32_t*, char16_t*> ret = turbo::unicode::simd::avx2_convert_utf32_to_utf16<EndianNess::SYS_LITTLE_ENDIAN>(buf, len, utf16_buffer);
        if (ret.first == nullptr) { return 0; }
        size_t saved_bytes = ret.second - utf16_buffer;
        if (ret.first != buf + len) {
            const size_t scalar_saved_bytes = turbo::unicode::utf32_to_utf16::convert<EndianNess::SYS_LITTLE_ENDIAN>(
                    ret.first, len - (ret.first - buf), ret.second);
            if (scalar_saved_bytes == 0) { return 0; }
            saved_bytes += scalar_saved_bytes;
        }
        return saved_bytes;
    }

    [[nodiscard]] inline size_t
    Converter<avx2_engine>::convert_utf32_to_utf16be(const char32_t *buf, size_t len, char16_t *utf16_buffer) noexcept {
        std::pair<const char32_t*, char16_t*> ret = turbo::unicode::simd::avx2_convert_utf32_to_utf16<EndianNess::SYS_BIG_ENDIAN>(buf, len, utf16_buffer);
        if (ret.first == nullptr) { return 0; }
        size_t saved_bytes = ret.second - utf16_buffer;
        if (ret.first != buf + len) {
            const size_t scalar_saved_bytes = turbo::unicode::utf32_to_utf16::convert<EndianNess::SYS_BIG_ENDIAN>(
                    ret.first, len - (ret.first - buf), ret.second);
            if (scalar_saved_bytes == 0) { return 0; }
            saved_bytes += scalar_saved_bytes;
        }
        return saved_bytes;
    }

    [[nodiscard]] inline UnicodeResult
    Converter<avx2_engine>::convert_utf32_to_utf16le_with_errors(const char32_t *buf, size_t len, char16_t *utf16_buffer) noexcept {
        std::pair<UnicodeResult, char16_t*> ret = turbo::unicode::simd::avx2_convert_utf32_to_utf16_with_errors<EndianNess::SYS_LITTLE_ENDIAN>(buf, len, utf16_buffer);
        if (ret.first.count != len) {
            UnicodeResult scalar_res = turbo::unicode::utf32_to_utf16::convert_with_errors<EndianNess::SYS_LITTLE_ENDIAN>(
                    buf + ret.first.count, len - ret.first.count, ret.second);
            if (is_unicode_error(scalar_res)) {
                scalar_res.count += ret.first.count;
                return scalar_res;
            } else {
                ret.second += scalar_res.count;
            }
        }
        ret.first.count = ret.second - utf16_buffer;   // Set count to the number of 8-bit code units written
        return ret.first;
    }

    [[nodiscard]] inline UnicodeResult
    Converter<avx2_engine>::convert_utf32_to_utf16be_with_errors(const char32_t *buf, size_t len, char16_t *utf16_buffer) noexcept {
        std::pair<UnicodeResult, char16_t*> ret = turbo::unicode::simd::avx2_convert_utf32_to_utf16_with_errors<EndianNess::SYS_BIG_ENDIAN>(buf, len, utf16_buffer);
        if (ret.first.count != len) {
            UnicodeResult scalar_res = turbo::unicode::utf32_to_utf16::convert_with_errors<EndianNess::SYS_BIG_ENDIAN>(
                    buf + ret.first.count, len - ret.first.count, ret.second);
            if (is_unicode_error(scalar_res)) {
                scalar_res.count += ret.first.count;
                return scalar_res;
            } else {
                ret.second += scalar_res.count;
            }
        }
        ret.first.count = ret.second - utf16_buffer;   // Set count to the number of 8-bit code units written
        return ret.first;
    }

    [[nodiscard]] inline size_t
    Converter<avx2_engine>::convert_valid_utf32_to_utf16le(const char32_t *buf, size_t len, char16_t *utf16_buffer) noexcept{
        return convert_utf32_to_utf16le(buf, len, utf16_buffer);
    }

    [[nodiscard]] inline size_t
    Converter<avx2_engine>::convert_valid_utf32_to_utf16be(const char32_t *buf, size_t len, char16_t *utf16_buffer) noexcept {
        return convert_utf32_to_utf16be(buf, len, utf16_buffer);
    }

    [[nodiscard]] inline size_t
    Converter<avx2_engine>::convert_utf16le_to_utf32(const char16_t *buf, size_t len, char32_t *utf32_buffer) noexcept {
        std::pair<const char16_t*, char32_t*> ret = turbo::unicode::simd::avx2_convert_utf16_to_utf32<EndianNess::SYS_LITTLE_ENDIAN>(buf, len, utf32_buffer);
        if (ret.first == nullptr) { return 0; }
        size_t saved_bytes = ret.second - utf32_buffer;
        if (ret.first != buf + len) {
            const size_t scalar_saved_bytes = turbo::unicode::utf16_to_utf32::convert<EndianNess::SYS_LITTLE_ENDIAN>(
                    ret.first, len - (ret.first - buf), ret.second);
            if (scalar_saved_bytes == 0) { return 0; }
            saved_bytes += scalar_saved_bytes;
        }
        return saved_bytes;
    }

    [[nodiscard]] inline size_t
    Converter<avx2_engine>::convert_utf16be_to_utf32(const char16_t *buf, size_t len, char32_t *utf32_buffer) noexcept {
        std::pair<const char16_t*, char32_t*> ret = turbo::unicode::simd::avx2_convert_utf16_to_utf32<EndianNess::SYS_BIG_ENDIAN>(buf, len, utf32_buffer);
        if (ret.first == nullptr) { return 0; }
        size_t saved_bytes = ret.second - utf32_buffer;
        if (ret.first != buf + len) {
            const size_t scalar_saved_bytes = turbo::unicode::utf16_to_utf32::convert<EndianNess::SYS_BIG_ENDIAN>(
                    ret.first, len - (ret.first - buf), ret.second);
            if (scalar_saved_bytes == 0) { return 0; }
            saved_bytes += scalar_saved_bytes;
        }
        return saved_bytes;
    }

    [[nodiscard]] inline UnicodeResult
    Converter<avx2_engine>::convert_utf16le_to_utf32_with_errors(const char16_t *buf, size_t len, char32_t *utf32_buffer) noexcept {
        std::pair<UnicodeResult, char32_t*> ret = turbo::unicode::simd::avx2_convert_utf16_to_utf32_with_errors<EndianNess::SYS_LITTLE_ENDIAN>(buf, len, utf32_buffer);
        if (is_unicode_error(ret.first)) { return ret.first; }  // Can return directly since scalar fallback already found correct ret.first.count
        if (ret.first.count != len) { // All good so far, but not finished
            UnicodeResult scalar_res = turbo::unicode::utf16_to_utf32::convert_with_errors<EndianNess::SYS_LITTLE_ENDIAN>(
                    buf + ret.first.count, len - ret.first.count, ret.second);
            if (is_unicode_error(scalar_res)) {
                scalar_res.count += ret.first.count;
                return scalar_res;
            } else {
                ret.second += scalar_res.count;
            }
        }
        ret.first.count = ret.second - utf32_buffer;   // Set count to the number of 8-bit code units written
        return ret.first;
    }

    [[nodiscard]] inline UnicodeResult
    Converter<avx2_engine>::convert_utf16be_to_utf32_with_errors(const char16_t *buf, size_t len, char32_t *utf32_buffer) noexcept {
        std::pair<UnicodeResult, char32_t*> ret = turbo::unicode::simd::avx2_convert_utf16_to_utf32_with_errors<EndianNess::SYS_BIG_ENDIAN>(buf, len, utf32_buffer);
        if (is_unicode_error(ret.first)) { return ret.first; }  // Can return directly since scalar fallback already found correct ret.first.count
        if (ret.first.count != len) { // All good so far, but not finished
            UnicodeResult scalar_res = turbo::unicode::utf16_to_utf32::convert_with_errors<EndianNess::SYS_BIG_ENDIAN>(
                    buf + ret.first.count, len - ret.first.count, ret.second);
            if (is_unicode_error(scalar_res)) {
                scalar_res.count += ret.first.count;
                return scalar_res;
            } else {
                ret.second += scalar_res.count;
            }
        }
        ret.first.count = ret.second - utf32_buffer;   // Set count to the number of 8-bit code units written
        return ret.first;
    }

    [[nodiscard]] inline size_t
    Converter<avx2_engine>::convert_valid_utf16le_to_utf32(const char16_t *buf, size_t len, char32_t *utf32_buffer) noexcept {
        return convert_utf16le_to_utf32(buf, len, utf32_buffer);
    }

    [[nodiscard]] inline size_t
    Converter<avx2_engine>::convert_valid_utf16be_to_utf32(const char16_t *buf, size_t len, char32_t *utf32_buffer) noexcept {
        return convert_utf16be_to_utf32(buf, len, utf32_buffer);
    }

    [[nodiscard]] inline void
    Converter<avx2_engine>::change_endianness_utf16(const char16_t *buf, size_t length, char16_t *output) noexcept {
        turbo::unicode::simd::avx2_change_endianness_utf16(buf, length, output);
    }

    [[nodiscard]] inline size_t Converter<avx2_engine>::count_utf16le(const char16_t *buf, size_t length) noexcept {
        return turbo::unicode::simd::count_code_points<EndianNess::SYS_LITTLE_ENDIAN>(buf, length);
    }

    [[nodiscard]] inline size_t Converter<avx2_engine>::count_utf16be(const char16_t *buf, size_t length) noexcept {
        return turbo::unicode::simd::count_code_points<EndianNess::SYS_BIG_ENDIAN>(buf, length);
    }

    [[nodiscard]] inline size_t Converter<avx2_engine>::count_utf8(const char *buf, size_t length) noexcept {
        return turbo::unicode::simd::utf8::count_code_points<avx2_engine>(buf, length);
    }

    [[nodiscard]] inline size_t Converter<avx2_engine>::utf8_length_from_utf16le(const char16_t *input, size_t length) noexcept {
        return turbo::unicode::simd::utf8_length_from_utf16<EndianNess::SYS_LITTLE_ENDIAN>(input, length);
    }

    [[nodiscard]] inline size_t Converter<avx2_engine>::utf8_length_from_utf16be(const char16_t *input, size_t length) noexcept {
        return turbo::unicode::simd::utf8_length_from_utf16<EndianNess::SYS_BIG_ENDIAN>(input, length);
    }

    [[nodiscard]] inline size_t Converter<avx2_engine>::utf32_length_from_utf16le(const char16_t *input, size_t length) noexcept {
        return  turbo::unicode::simd::utf32_length_from_utf16<EndianNess::SYS_LITTLE_ENDIAN>(input, length);
    }

    [[nodiscard]] inline size_t Converter<avx2_engine>::utf32_length_from_utf16be(const char16_t *input, size_t length) noexcept {
        return  turbo::unicode::simd::utf32_length_from_utf16<EndianNess::SYS_BIG_ENDIAN>(input, length);
    }

    [[nodiscard]] inline size_t Converter<avx2_engine>::utf16_length_from_utf8(const char *input, size_t length) noexcept {
        return turbo::unicode::simd::utf8::utf16_length_from_utf8<avx2_engine>(input, length);
    }

    [[nodiscard]] inline size_t Converter<avx2_engine>::utf8_length_from_utf32(const char32_t *input, size_t length) noexcept {
        const __m256i v_00000000 = _mm256_setzero_si256();
        const __m256i v_ffffff80 = _mm256_set1_epi32((uint32_t)0xffffff80);
        const __m256i v_fffff800 = _mm256_set1_epi32((uint32_t)0xfffff800);
        const __m256i v_ffff0000 = _mm256_set1_epi32((uint32_t)0xffff0000);
        size_t pos = 0;
        size_t count = 0;
        for(;pos + 8 <= length; pos += 8) {
            __m256i in = _mm256_loadu_si256((__m256i*)(input + pos));
            const __m256i ascii_bytes_bytemask = _mm256_cmpeq_epi32(_mm256_and_si256(in, v_ffffff80), v_00000000);
            const __m256i one_two_bytes_bytemask = _mm256_cmpeq_epi32(_mm256_and_si256(in, v_fffff800), v_00000000);
            const __m256i two_bytes_bytemask = _mm256_xor_si256(one_two_bytes_bytemask, ascii_bytes_bytemask);
            const __m256i one_two_three_bytes_bytemask = _mm256_cmpeq_epi32(_mm256_and_si256(in, v_ffff0000), v_00000000);
            const __m256i three_bytes_bytemask = _mm256_xor_si256(one_two_three_bytes_bytemask, one_two_bytes_bytemask);
            const uint32_t ascii_bytes_bitmask = static_cast<uint32_t>(_mm256_movemask_epi8(ascii_bytes_bytemask));
            const uint32_t two_bytes_bitmask = static_cast<uint32_t>(_mm256_movemask_epi8(two_bytes_bytemask));
            const uint32_t three_bytes_bitmask = static_cast<uint32_t>(_mm256_movemask_epi8(three_bytes_bytemask));

            size_t ascii_count = turbo::popcount(ascii_bytes_bitmask) / 4;
            size_t two_bytes_count = turbo::popcount(two_bytes_bitmask) / 4;
            size_t three_bytes_count = turbo::popcount(three_bytes_bitmask) / 4;
            count += 32 - 3*ascii_count - 2*two_bytes_count - three_bytes_count;
        }
        return count + turbo::unicode::utf32::utf8_length_from_utf32(input + pos, length - pos);
    }

    [[nodiscard]] inline size_t Converter<avx2_engine>::utf16_length_from_utf32(const char32_t *input, size_t length) noexcept {
        const __m256i v_00000000 = _mm256_setzero_si256();
        const __m256i v_ffff0000 = _mm256_set1_epi32((uint32_t)0xffff0000);
        size_t pos = 0;
        size_t count = 0;
        for(;pos + 8 <= length; pos += 8) {
            __m256i in = _mm256_loadu_si256((__m256i*)(input + pos));
            const __m256i surrogate_bytemask = _mm256_cmpeq_epi32(_mm256_and_si256(in, v_ffff0000), v_00000000);
            const uint32_t surrogate_bitmask = static_cast<uint32_t>(_mm256_movemask_epi8(surrogate_bytemask));
            size_t surrogate_count = (32-turbo::popcount(surrogate_bitmask))/4;
            count += 8 + surrogate_count;
        }
        return count + turbo::unicode::utf32::utf16_length_from_utf32(input + pos, length - pos);
    }

    [[nodiscard]] inline size_t Converter<avx2_engine>::utf32_length_from_utf8(const char *input, size_t length) noexcept {
        return turbo::unicode::simd::utf8::count_code_points<avx2_engine>(input, length);
    }
}  // namespace turbo::unicode
#endif  // TURBO_UNICODE_AVX2_CONVERTER_H_
