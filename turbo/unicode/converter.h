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


namespace turbo {

    /**
     * @ingroup turbo_unicode_detector
     * @brief auto_detect_encoding auto detect encoding from input buffer, return EncodingType
     *        Example:
     *        @code
     *        auto encoding = auto_detect_encoding(input, length);
     *        // encoding is EncodingType::kUtf8 or EncodingType::kUtf16 or EncodingType::kUtf32
     *        @endcode
     *        If you want to use your own engine, you can use like this:
     *        @code
     *        auto encoding = auto_detect_encoding<YourEngine>(input, length);
     *        // encoding is EncodingType::kUtf8 or EncodingType::kUtf16 or EncodingType::kUtf32
     *        @endcode
     * @tparam Engine default is turbo::unicode::default_engine
     * @param input input buffer
     * @param length input buffer length
     * @return EncodingType that auto detected
     */
    template<typename Engine = turbo::unicode::default_engine, typename turbo::check_requires<turbo::unicode::is_unicode_engine<Engine>> = 0>
    [[nodiscard]] turbo::EncodingType auto_detect_encoding(const char *input, size_t length) noexcept;

    /**
     * @ingroup turbo_unicode_detector
     * @brief detect_encodings detect encodings from input buffer, return EncodingType
     *        Example:
     *        @code
     *        auto encoding = detect_encodings(input, length);
     *        // encoding is EncodingType::kUtf8 or EncodingType::kUtf16 or EncodingType::kUtf32
     *        @endcode
     *        If you want to use your own engine, you can use like this:
     *        @code
     *        auto encoding = detect_encodings<YourEngine>(input, length);
     *        // encoding is EncodingType::kUtf8 or EncodingType::kUtf16 or EncodingType::kUtf32
     *        @endcode
     * @tparam Engine default is turbo::unicode::default_engine
     * @param input input buffer
     * @param length input buffer length
     * @return EncodingType that auto detected
     */
    template<typename Engine = turbo::unicode::default_engine, typename turbo::check_requires<turbo::unicode::is_unicode_engine<Engine>> = 0>
    int detect_encodings(const char *input, size_t length) noexcept;

    /**
     * @ingroup turbo_unicode_validate
     * @brief validate_utf8 check whether input buffer is valid utf8. using this function is faster than validate_utf8_with_errors
     *        but it will not return error position.@see validate_utf8_with_errors.
     * @tparam Engine default is turbo::unicode::default_engine
     * @param buf buffer to check
     * @param len  buffer length
     * @return true if valid utf8, otherwise false
     */
    template<typename Engine = turbo::unicode::default_engine, typename turbo::check_requires<turbo::unicode::is_unicode_engine<Engine>> = 0>
    [[nodiscard]] bool validate_utf8(const char *buf, size_t len) noexcept;

    /**
     * @ingroup turbo_unicode_validate
     * @brief validate_utf8_with_errors check whether input buffer is valid utf8. using this function is slower than validate_utf8
     *        but it will return error position.
     * @tparam Engine default is turbo::unicode::default_engine
     * @param buf buffer to check
     * @param len  buffer length
     * @return result, if valid utf8, result is ok, otherwise result is error and error position is set.
     */
    template<typename Engine = turbo::unicode::default_engine, typename turbo::check_requires<turbo::unicode::is_unicode_engine<Engine>> = 0>
    [[nodiscard]] result validate_utf8_with_errors(const char *buf, size_t len) noexcept;

    /**
     * @ingroup turbo_unicode_validate
     * @brief validate_ascii check whether input buffer is valid ascii.
     *        using this function is faster than validate_ascii_with_errors.
     *        but it will not return error position.@see validate_ascii_with_errors.
     *        Example:
     *        @code
     *        auto is_ascii = validate_ascii(input, length);
     *        // is_ascii is true if input is valid ascii, otherwise false
     *        @endcode
     *        If you want to use your own engine, you can use like this:
     *        @code
     *        auto is_ascii = validate_ascii<YourEngine>(input, length);
     *        // is_ascii is true if input is valid ascii, otherwise false
     *        @endcode
     * @tparam Engine default is turbo::unicode::default_engine
     * @param buf buffer to check
     * @param len  buffer length
     * @return true if valid ascii, otherwise false
     */
    template<typename Engine = turbo::unicode::default_engine, typename turbo::check_requires<turbo::unicode::is_unicode_engine<Engine>> = 0>
    [[nodiscard]] bool validate_ascii(const char *buf, size_t len) noexcept;

    /**
     * @ingroup turbo_unicode_validate
     * @brief validate_ascii_with_errors check whether input buffer is valid ascii.
     *        using this function is slower than validate_ascii.
     *        but it will return error position.
     *        Example:
     *        @code
     *        auto result = validate_ascii_with_errors(input, length);
     *        // result is ok if input is valid ascii, otherwise result is error and error position is set.
     *        @endcode
     *        If you want to use your own engine, you can use like this:
     *        @code
     *        auto result = validate_ascii_with_errors<YourEngine>(input, length);
     *        // result is ok if input is valid ascii, otherwise result is error and error position is set.
     *        @endcode
     * @tparam Engine default is turbo::unicode::default_engine
     * @param buf buffer to check
     * @param len  buffer length
     * @return result, if valid ascii, result is ok, otherwise result is error and error position is set.
     */
    template<typename Engine = turbo::unicode::default_engine, typename turbo::check_requires<turbo::unicode::is_unicode_engine<Engine>> = 0>
    [[nodiscard]] result validate_ascii_with_errors(const char *buf, size_t len) noexcept;

    /**
     * @ingroup turbo_unicode_validate
     * @brief validate_utf16 check whether input buffer is valid utf16.
     *        using this function is faster than validate_utf16_with_errors.
     *        but it will not return error position.@see validate_utf16_with_errors.
     * @bote  this function will auto detect utf16 endian as host endian. input buffer must
     *        be host endian.
     *        Example:
     *        @code
     *        auto is_utf16 = validate_utf16(input, length);
     *        // is_utf16 is true if input is valid utf16, otherwise false
     *        @endcode
     *        If you want to use your own engine, you can use like this:
     *        @code
     *        auto is_utf16 = validate_utf16<YourEngine>(input, length);
     *        // is_utf16 is true if input is valid utf16, otherwise false
     *        @endcode
     * @tparam Engine default is turbo::unicode::default_engine
     * @param buf buffer to check
     * @param len  buffer length
     * @return true if valid utf16, otherwise false
     */
    template<typename Engine = turbo::unicode::default_engine, typename turbo::check_requires<turbo::unicode::is_unicode_engine<Engine>> = 0>
    [[nodiscard]] bool validate_utf16(const char16_t *buf, size_t len) noexcept;

    /**
     * @ingroup turbo_unicode_validate
     * @brief check whether input buffer is valid utf16 with little endian.
     *        @see also validate_utf16
     * @note guard that input buffer is little endian.
     * @tparam Engine default is turbo::unicode::default_engine
     * @param buf buffer to check
     * @param len  buffer length
     * @return true if valid utf16, otherwise false
     */
    template<typename Engine = turbo::unicode::default_engine, typename turbo::check_requires<turbo::unicode::is_unicode_engine<Engine>> = 0>
    [[nodiscard]] bool validate_utf16le(const char16_t *buf, size_t len) noexcept;

    /**
     * @ingroup turbo_unicode_validate
     * @brief check whether input buffer is valid utf16 with big endian.
     *        @see also validate_utf16
     * @note guard that input buffer is big endian.
     * @tparam Engine default is turbo::unicode::default_engine
     * @param buf buffer to check
     * @param len  buffer length
     * @return true if valid utf16, otherwise false
     */
    template<typename Engine = turbo::unicode::default_engine, typename turbo::check_requires<turbo::unicode::is_unicode_engine<Engine>> = 0>
    [[nodiscard]] bool validate_utf16be(const char16_t *buf, size_t len) noexcept;

    /**
     * @ingroup turbo_unicode_validate
     * @brief validate_utf16_with_errors check whether input buffer is valid utf16.
     *        using this function is slower than validate_utf16.
     *        but it will return error position.
     * @bote  this function will auto detect utf16 endian as host endian. input buffer must
     *        be host endian.
     *        Example:
     *        @code
     *        auto result = validate_utf16_with_errors(input, length);
     *        // result is ok if input is valid utf16, otherwise result is error and error position is set.
     *        @endcode
     *        If you want to use your own engine, you can use like this:
     *        @code
     *        auto result = validate_utf16_with_errors<YourEngine>(input, length);
     *        // result is ok if input is valid utf16, otherwise result is error and error position is set.
     *        @endcode
     * @tparam Engine default is turbo::unicode::default_engine
     * @param buf buffer to check
     * @param len  buffer length
     * @return result, if valid utf16, result is ok, the position is the len input , otherwise result is
     *         error and error position is set.
     */
    template<typename Engine = turbo::unicode::default_engine, typename turbo::check_requires<turbo::unicode::is_unicode_engine<Engine>> = 0>
    [[nodiscard]] result validate_utf16_with_errors(const char16_t *buf, size_t len) noexcept;

    /**
     * @ingroup turbo_unicode_validate
     * @brief check whether input buffer is valid utf16 with little endian.
     *        @see also validate_utf16_with_errors
     *        @see also validate_utf16le
     * @note guard that input buffer is little endian.
     * @tparam Engine default is turbo::unicode::default_engine
     * @param buf buffer to check
     * @param len  buffer length
     * @return result, if valid utf16, result is ok, the position is the len input , otherwise result is
     *         error and error position is set.
     */
    template<typename Engine = turbo::unicode::default_engine, typename turbo::check_requires<turbo::unicode::is_unicode_engine<Engine>> = 0>
    [[nodiscard]] result validate_utf16le_with_errors(const char16_t *buf, size_t len) noexcept;

    /**
     * @ingroup turbo_unicode_validate
     * @brief check whether input buffer is valid utf16 with big endian.
     *        @see also validate_utf16_with_errors
     *        @see also validate_utf16be
     * @note guard that input buffer is big endian.
     * @tparam Engine default is turbo::unicode::default_engine
     * @param buf buffer to check
     * @param len  buffer length
     * @return result, if valid utf16, result is ok, the position is the len input , otherwise result is
     *         error and error position is set.
     */
    template<typename Engine = turbo::unicode::default_engine, typename turbo::check_requires<turbo::unicode::is_unicode_engine<Engine>> = 0>
    [[nodiscard]] result validate_utf16be_with_errors(const char16_t *buf, size_t len) noexcept;

    /**
     * @ingroup turbo_unicode_validate
     * @brief validate_utf32 check whether input buffer is valid utf32.
     *        using this function is faster than validate_utf32_with_errors.
     *        but it will not return error position.@see validate_utf32_with_errors.
     * @bote  this function will auto detect utf32 endian as host endian. input buffer must
     *        be host endian.
     *        Example:
     *        @code
     *        auto is_utf32 = validate_utf32(input, length);
     *        // is_utf32 is true if input is valid utf32, otherwise false
     *        @endcode
     *        If you want to use your own engine, you can use like this:
     *        @code
     *        auto is_utf32 = validate_utf32<YourEngine>(input, length);
     *        // is_utf32 is true if input is valid utf32, otherwise false
     *        @endcode
     * @tparam Engine default is turbo::unicode::default_engine
     * @param buf buffer to check
     * @param len  buffer length
     * @return true if valid utf32, otherwise false
     */
    template<typename Engine = turbo::unicode::default_engine, typename turbo::check_requires<turbo::unicode::is_unicode_engine<Engine>> = 0>
    [[nodiscard]] bool validate_utf32(const char32_t *buf, size_t len) noexcept;

    /**
     * @ingroup turbo_unicode_validate
     * @brief check whether input buffer is valid utf32 with little endian.
     *        @see also validate_utf32
     * @note guard that input buffer is little endian.
     * @tparam Engine default is turbo::unicode::default_engine
     * @param buf buffer to check
     * @param len  buffer length
     * @note guard that input buffer is little endian.
     * @return true if valid utf32, otherwise false
     */
    template<typename Engine = turbo::unicode::default_engine, typename turbo::check_requires<turbo::unicode::is_unicode_engine<Engine>> = 0>
    [[nodiscard]] result validate_utf32_with_errors(const char32_t *buf, size_t len) noexcept;

    /**
     * @ingroup turbo_unicode_converter
     * @brief convert utf8 to utf16 with auto detect endian.
     *       Example:
     *       @code
     *       char16_t utf16_buffer[1024];
     *       auto utf16_length = convert_utf8_to_utf16(input, length, utf16_buffer);
     *       // utf16_length is utf16 buffer length
     *       @endcode
     * @tparam Engine default is turbo::unicode::default_engine
     * @param input utf8 input buffer
     * @param length utf8 input buffer length
     * @param utf16_output utf16 output buffer
     * @return utf16 output buffer length
     */
    template<typename Engine = turbo::unicode::default_engine, typename turbo::check_requires<turbo::unicode::is_unicode_engine<Engine>> = 0>
    [[nodiscard]] size_t convert_utf8_to_utf16(const char *input, size_t length, char16_t *utf16_output) noexcept;

    /**
     * @ingroup turbo_unicode_converter
     * @brief convert utf8 to utf16 with little endian.
     *       Example:
     *       @code
     *       char16_t utf16_buffer[1024];
     *       auto utf16_length = convert_utf8_to_utf16_le(input, length, utf16_buffer);
     *       // utf16_length is utf16 buffer length
     *       @endcode
     * @tparam Engine default is turbo::unicode::default_engine
     * @param input utf8 input buffer
     * @param length utf8 input buffer length
     * @param utf16_output utf16 output buffer
     * @return utf16 output buffer length
     * @note guard that input buffer is little endian.
     */
    template<typename Engine = turbo::unicode::default_engine, typename turbo::check_requires<turbo::unicode::is_unicode_engine<Engine>> = 0>
    [[nodiscard]] size_t convert_utf8_to_utf16le(const char *input, size_t length, char16_t *utf16_output) noexcept;

    /**
     * @ingroup turbo_unicode_converter
     * @brief convert utf8 to utf16 with big endian.
     *       Example:
     *       @code
     *       char16_t utf16_buffer[1024];
     *       auto utf16_length = convert_utf8_to_utf16_be(input, length, utf16_buffer);
     *       // utf16_length is utf16 buffer length
     *       @endcode
     * @tparam Engine default is turbo::unicode::default_engine
     * @param input utf8 input buffer
     * @param length utf8 input buffer length
     * @param utf16_output utf16 output buffer
     * @return utf16 output buffer length
     * @note guard that input buffer is big endian.
     */
    template<typename Engine = turbo::unicode::default_engine, typename turbo::check_requires<turbo::unicode::is_unicode_engine<Engine>> = 0>
    [[nodiscard]] size_t convert_utf8_to_utf16be(const char *input, size_t length, char16_t *utf16_output) noexcept;

    /**
     * @ingroup turbo_unicode_converter
     * @brief convert utf8 to utf16 with auto detect endian.
     *       Example:
     *       @code
     *       char16_t utf16_buffer[1024];
     *       auto result = convert_utf8_to_utf16_with_errors(input, length, utf16_buffer);
     *       // result is ok if input is valid utf8, otherwise result is error and error position is set.
     *       @endcode
     * @tparam Engine default is turbo::unicode::default_engine
     * @param input utf8 input buffer
     * @param length utf8 input buffer length
     * @param utf16_output utf16 output buffer
     * @return result, if valid utf8, result is ok, otherwise result is error and error position is set.
     */
    template<typename Engine = turbo::unicode::default_engine, typename turbo::check_requires<turbo::unicode::is_unicode_engine<Engine>> = 0>
    [[nodiscard]] result convert_utf8_to_utf16_with_errors(const char *input, size_t length, char16_t *utf16_output) noexcept;

    /**
     * @ingroup turbo_unicode_converter
     * @brief convert utf8 to utf16 with little endian.
     *       Example:
     *       @code
     *       char16_t utf16_buffer[1024];
     *       auto result = convert_utf8_to_utf16_le_with_errors(input, length, utf16_buffer);
     *       // result is ok if input is valid utf8, otherwise result is error and error position is set.
     *       @endcode
     * @tparam Engine default is turbo::unicode::default_engine
     * @param input utf8 input buffer
     * @param length utf8 input buffer length
     * @param utf16_output utf16 output buffer
     * @return result, if valid utf8, result is ok, otherwise result is error and error position is set.
     * @note guard that input buffer is little endian.
     */
    template<typename Engine = turbo::unicode::default_engine, typename turbo::check_requires<turbo::unicode::is_unicode_engine<Engine>> = 0>
    [[nodiscard]] result
    convert_utf8_to_utf16le_with_errors(const char *input, size_t length, char16_t *utf16_output) noexcept;

    /**
     * @ingroup turbo_unicode_converter
     * @brief convert utf8 to utf16 with big endian.
     *       Example:
     *       @code
     *       char16_t utf16_buffer[1024];
     *       auto result = convert_utf8_to_utf16_be_with_errors(input, length, utf16_buffer);
     *       // result is ok if input is valid utf8, otherwise result is error and error position is set.
     *       @endcode
     * @tparam Engine default is turbo::unicode::default_engine
     * @param input utf8 input buffer
     * @param length utf8 input buffer length
     * @param utf16_output utf16 output buffer
     * @return result, if valid utf8, result is ok, otherwise result is error and error position is set.
     * @note guard that input buffer is big endian.
     */
    template<typename Engine = turbo::unicode::default_engine, typename turbo::check_requires<turbo::unicode::is_unicode_engine<Engine>> = 0>
    [[nodiscard]] result
    convert_utf8_to_utf16be_with_errors(const char *input, size_t length, char16_t *utf16_output) noexcept;

    /**
     * @ingroup turbo_unicode_converter
     * @brief convert utf8 to utf32.
     * @tparam Engine default is turbo::unicode::default_engine
     * @param input  utf8 input buffer
     * @param length  utf8 input buffer length
     * @param utf32_output utf32 output buffer
     * @return utf32 output buffer length
     */
    template<typename Engine = turbo::unicode::default_engine, typename turbo::check_requires<turbo::unicode::is_unicode_engine<Engine>> = 0>
    [[nodiscard]] size_t convert_utf8_to_utf32(const char *input, size_t length, char32_t *utf32_output) noexcept;

    /**
     * @ingroup turbo_unicode_converter
     * @brief convert utf8 to utf32 with little endian.
     * @tparam Engine default is turbo::unicode::default_engine
     * @param input  utf8 input buffer
     * @param length  utf8 input buffer length
     * @param utf32_output utf32 output buffer
     * @return utf32 output buffer length
     * @note guard that input buffer is little endian.
     */
    template<typename Engine = turbo::unicode::default_engine, typename turbo::check_requires<turbo::unicode::is_unicode_engine<Engine>> = 0>
    [[nodiscard]] result
    convert_utf8_to_utf32_with_errors(const char *input, size_t length, char32_t *utf32_output) noexcept;

    /**
     * @ingroup turbo_unicode_converter
     * @brief convert utf8 to utf16 with host endian. assume input buffer is valid utf8.
     *        using this function is faster than convert_valid_utf8_to_utf16_with_errors.
     * @note  this function will auto set utf16 endian as host endian. make sure that
     *        utf8 input buffer is valid utf8.
     * @tparam Engine default is turbo::unicode::default_engine
     * @param input  utf8 input buffer
     * @param length  utf8 input buffer length
     * @param utf32_output utf16 output buffer
     * @return utf16 output buffer length
     * @note guard that input buffer is big endian.
     */
    template<typename Engine = turbo::unicode::default_engine, typename turbo::check_requires<turbo::unicode::is_unicode_engine<Engine>> = 0>
    [[nodiscard]] size_t convert_valid_utf8_to_utf16(const char *input, size_t length, char16_t *utf16_buffer) noexcept;

    /**
     * @ingroup turbo_unicode_converter
     * @brief convert utf8 to utf16 with little endian. assume input buffer is valid utf8.
     *        using this function is faster than convert_valid_utf8_to_utf16_le_with_errors.
     * @note  this function will auto set utf16 endian as little endian. make sure that
     *        utf8 input buffer is valid utf8.
     * @tparam Engine default is turbo::unicode::default_engine
     * @param input  utf8 input buffer
     * @param length  utf8 input buffer length
     * @param utf32_output utf16 output buffer
     * @return utf16 output buffer length
     * @note guard that input buffer is little endian.
     */
    template<typename Engine = turbo::unicode::default_engine, typename turbo::check_requires<turbo::unicode::is_unicode_engine<Engine>> = 0>
    [[nodiscard]] size_t convert_valid_utf8_to_utf16le(const char *input, size_t length, char16_t *utf16_buffer) noexcept;

    /**
     * @ingroup turbo_unicode_converter
     * @brief convert utf8 to utf16 with big endian. assume input buffer is valid utf8.
     *        using this function is faster than convert_valid_utf8_to_utf16_be_with_errors.
     * @note  this function will auto set utf16 endian as big endian. make sure that
     *        utf8 input buffer is valid utf8.
     * @tparam Engine default is turbo::unicode::default_engine
     * @param input  utf8 input buffer
     * @param length  utf8 input buffer length
     * @param utf32_output utf16 output buffer
     * @return utf16 output buffer length
     * @note guard that input buffer is big endian.
     */
    template<typename Engine = turbo::unicode::default_engine, typename turbo::check_requires<turbo::unicode::is_unicode_engine<Engine>> = 0>
    [[nodiscard]] size_t convert_valid_utf8_to_utf16be(const char *input, size_t length, char16_t *utf16_buffer) noexcept;

    /**
     * @ingroup turbo_unicode_converter
     * @brief convert utf8 to utf32. assume input buffer is valid utf8.
     *        using this function is faster than convert_valid_utf8_to_utf32_with_errors.
     * @note  this function will auto set utf32 endian as host endian. make sure that
     *        utf8 input buffer is valid utf8.
     * @tparam Engine default is turbo::unicode::default_engine
     * @param input  utf8 input buffer
     * @param length  utf8 input buffer length
     * @param utf32_output utf32 output buffer
     * @return utf32 output buffer length
     * @note guard that input buffer is big endian.
     */
    template<typename Engine = turbo::unicode::default_engine, typename turbo::check_requires<turbo::unicode::is_unicode_engine<Engine>> = 0>
    [[nodiscard]] size_t convert_valid_utf8_to_utf32(const char *input, size_t length, char32_t *utf32_buffer) noexcept;

    /**
     * @ingroup turbo_unicode_converter
     * @brief  get utf16 length from utf8 input buffer.
     *        Example:
     *        @code
     *        auto utf16_length = utf16_length_from_utf8(input, length);
     *        // utf16_length is utf16 buffer length
     *        @endcode
     * @tparam Engine default is turbo::unicode::default_engine
     * @param input utf8 input buffer
     * @param length utf8 input buffer length
     * @return utf16 output buffer length
     */
    template<typename Engine = turbo::unicode::default_engine, typename turbo::check_requires<turbo::unicode::is_unicode_engine<Engine>> = 0>
    [[nodiscard]] size_t utf16_length_from_utf8(const char *input, size_t length) noexcept;

    /**
     * @ingroup turbo_unicode_converter
     * @brief get utf32 length from utf8 input buffer.
     *       Example:
     *       @code
     *       auto utf32_length = utf32_length_from_utf8(input, length);
     *       // utf32_length is utf32 buffer length
     *       @endcode
     * @tparam Engine default is turbo::unicode::default_engine
     * @param input utf8 input buffer
     * @param length utf8 input buffer length
     * @return utf32 output buffer length
     */
    template<typename Engine = turbo::unicode::default_engine, typename turbo::check_requires<turbo::unicode::is_unicode_engine<Engine>> = 0>
    [[nodiscard]] size_t utf32_length_from_utf8(const char *input, size_t length) noexcept;

    /**
     * @ingroup turbo_unicode_converter
     * @brief convert utf16 to utf8 with auto detect endian.
     *       Example:
     *       @code
     *       char utf8_buffer[1024];
     *       auto utf8_length = convert_utf16_to_utf8(input, length, utf8_buffer);
     *       // utf8_length is utf8 buffer length
     *       @endcode
     * @tparam Engine default is turbo::unicode::default_engine
     * @param input utf16 input buffer
     * @param length utf16 input buffer length
     * @param utf8_output utf8 output buffer
     * @return utf8 output buffer length
     */
    template<typename Engine = turbo::unicode::default_engine, typename turbo::check_requires<turbo::unicode::is_unicode_engine<Engine>> = 0>
    [[nodiscard]] size_t convert_utf16_to_utf8(const char16_t *input, size_t length, char *utf8_buffer) noexcept;

    /**
     * @ingroup turbo_unicode_converter
     * @brief convert utf16 to utf8 with little endian.
     *       Example:
     *       @code
     *       char utf8_buffer[1024];
     *       auto utf8_length = convert_utf16_to_utf8_le(input, length, utf8_buffer);
     *       // utf8_length is utf8 buffer length
     *       @endcode
     * @tparam Engine default is turbo::unicode::default_engine
     * @param input utf16 input buffer
     * @param length utf16 input buffer length
     * @param utf8_output utf8 output buffer
     * @return utf8 output buffer length
     * @note guard that input buffer is little endian.
     */
    template<typename Engine = turbo::unicode::default_engine, typename turbo::check_requires<turbo::unicode::is_unicode_engine<Engine>> = 0>
    [[nodiscard]] size_t convert_utf16le_to_utf8(const char16_t *input, size_t length, char *utf8_buffer) noexcept;

    /**
     * @ingroup turbo_unicode_converter
     * @brief convert utf16 to utf8 with big endian.
     *       Example:
     *       @code
     *       char utf8_buffer[1024];
     *       auto utf8_length = convert_utf16_to_utf8_be(input, length, utf8_buffer);
     *       // utf8_length is utf8 buffer length
     *       @endcode
     * @tparam Engine default is turbo::unicode::default_engine
     * @param input utf16 input buffer
     * @param length utf16 input buffer length
     * @param utf8_output utf8 output buffer
     * @return utf8 output buffer length
     * @note guard that input buffer is big endian.
     */
    template<typename Engine = turbo::unicode::default_engine, typename turbo::check_requires<turbo::unicode::is_unicode_engine<Engine>> = 0>
    [[nodiscard]] size_t convert_utf16be_to_utf8(const char16_t *input, size_t length, char *utf8_buffer) noexcept;

    /**
     * @ingroup turbo_unicode_converter
     * @brief convert utf16 to utf8 with auto detect endian.
     *       Example:
     *       @code
     *       char utf8_buffer[1024];
     *       auto result = convert_utf16_to_utf8_with_errors(input, length, utf8_buffer);
     *       // result is ok if input is valid utf16, otherwise result is error and error position is set.
     *       @endcode
     * @tparam Engine default is turbo::unicode::default_engine
     * @param input utf16 input buffer
     * @param length utf16 input buffer length
     * @param utf8_output utf8 output buffer
     * @return result, if valid utf16, result is ok, otherwise result is error and error position is set.
     * @note guard that input buffer is big endian.
     */
    template<typename Engine = turbo::unicode::default_engine, typename turbo::check_requires<turbo::unicode::is_unicode_engine<Engine>> = 0>
    [[nodiscard]] result convert_utf16_to_utf8_with_errors(const char16_t *input, size_t length, char *utf8_buffer) noexcept;

    /**
     * @ingroup turbo_unicode_converter
     * @brief convert utf16 to utf8 with little endian.
     *       Example:
     *       @code
     *       char utf8_buffer[1024];
     *       auto result = convert_utf16_to_utf8_le_with_errors(input, length, utf8_buffer);
     *       // result is ok if input is valid utf16, otherwise result is error and error position is set.
     *       @endcode
     * @tparam Engine default is turbo::unicode::default_engine
     * @param input utf16 input buffer
     * @param length utf16 input buffer length
     * @param utf8_output utf8 output buffer
     * @return result, if valid utf16, result is ok, otherwise result is error and error position is set.
     * @note guard that input buffer is little endian.
     */
    template<typename Engine = turbo::unicode::default_engine, typename turbo::check_requires<turbo::unicode::is_unicode_engine<Engine>> = 0>
    [[nodiscard]] result
    convert_utf16le_to_utf8_with_errors(const char16_t *input, size_t length, char *utf8_buffer) noexcept;

    /**
     * @ingroup turbo_unicode_converter
     * @brief convert utf16 to utf8 with big endian.
     *       Example:
     *       @code
     *       char utf8_buffer[1024];
     *       auto result = convert_utf16_to_utf8_be_with_errors(input, length, utf8_buffer);
     *       // result is ok if input is valid utf16, otherwise result is error and error position is set.
     *       @endcode
     * @tparam Engine default is turbo::unicode::default_engine
     * @param input utf16 input buffer
     * @param length utf16 input buffer length
     * @param utf8_output utf8 output buffer
     * @return result, if valid utf16, result is ok, otherwise result is error and error position is set.
     * @note guard that input buffer is big endian.
     */
    template<typename Engine = turbo::unicode::default_engine, typename turbo::check_requires<turbo::unicode::is_unicode_engine<Engine>> = 0>
    [[nodiscard]] result
    convert_utf16be_to_utf8_with_errors(const char16_t *input, size_t length, char *utf8_buffer) noexcept;

    /**
     * @ingroup turbo_unicode_converter
     * @brief convert utf16 to utf32 with auto detect endian.
     *       Example:
     *       @code
     *       char32_t utf32_buffer[1024];
     *       auto utf32_length = convert_utf16_to_utf32(input, length, utf32_buffer);
     *       // utf32_length is utf32 buffer length
     *       @endcode
     * @note guard that input buffer is host endian. input buffer must be valid utf16.
     * @tparam Engine default is turbo::unicode::default_engine
     * @param input utf16 input buffer
     * @param length utf16 input buffer length
     * @param utf8_buffer utf8 output buffer
     * @return utf8 output buffer length
     */
    template<typename Engine = turbo::unicode::default_engine, typename turbo::check_requires<turbo::unicode::is_unicode_engine<Engine>> = 0>
    [[nodiscard]] size_t convert_valid_utf16_to_utf8(const char16_t *input, size_t length, char *utf8_buffer) noexcept;

    /**
     * @ingroup turbo_unicode_converter
     * @brief convert utf16 with little endian to utf8, and assume input buffer is valid utf16.
     *       using this function is faster than convert_valid_utf16_to_utf8_le_with_errors.
     * @note guard that input buffer is little endian. input buffer must be valid utf16.
     * @tparam Engine default is turbo::unicode::default_engine
     * @param input utf16 input buffer
     * @param length utf16 input buffer length
     * @param utf8_buffer utf8 output buffer
     * @return utf8 output buffer length
     */
    template<typename Engine = turbo::unicode::default_engine, typename turbo::check_requires<turbo::unicode::is_unicode_engine<Engine>> = 0>
    [[nodiscard]] size_t convert_valid_utf16le_to_utf8(const char16_t *input, size_t length, char *utf8_buffer) noexcept;

    /**
     * @ingroup turbo_unicode_converter
     * @brief convert utf16 with big endian to utf8, and assume input buffer is valid utf16.
     *       using this function is faster than convert_valid_utf16_to_utf8_be_with_errors.
     * @note guard that input buffer is big endian. input buffer must be valid utf16.
     * @tparam Engine default is turbo::unicode::default_engine
     * @param input utf16 input buffer
     * @param length utf16 input buffer length
     * @param utf8_buffer utf8 output buffer
     * @return utf8 output buffer length
     */
    template<typename Engine = turbo::unicode::default_engine, typename turbo::check_requires<turbo::unicode::is_unicode_engine<Engine>> = 0>
    [[nodiscard]] size_t convert_valid_utf16be_to_utf8(const char16_t *input, size_t length, char *utf8_buffer) noexcept;

    /**
     * @ingroup turbo_unicode_converter
     * @brief convert utf16 to utf32 with auto detect endian.
     *       Example:
     *       @code
     *       char32_t utf32_buffer[1024];
     *       auto result = convert_utf16_to_utf32_with_errors(input, length, utf32_buffer);
     *       // result is ok if input is valid utf16, otherwise result is error and error position is set.
     *       @endcode
     * @note guard that input buffer is host endian. input buffer must be valid utf16.
     * @tparam Engine default is turbo::unicode::default_engine
     * @param input utf16 input buffer
     * @param length utf16 input buffer length
     * @param utf8_buffer utf8 output buffer
     * @return result, if valid utf16, result is ok, otherwise result is error and error position is set.
     */
    template<typename Engine = turbo::unicode::default_engine, typename turbo::check_requires<turbo::unicode::is_unicode_engine<Engine>> = 0>
    [[nodiscard]] size_t convert_utf16_to_utf32(const char16_t *input, size_t length, char32_t *utf32_buffer) noexcept;

    /**
     * @ingroup turbo_unicode_converter
     * @brief convert utf16 with little endian to utf32, and assume input buffer is valid utf16.
     *       using this function is faster than convert_valid_utf16_to_utf32_le_with_errors.
     * @note guard that input buffer is little endian. input buffer must be valid utf16.
     * @tparam Engine default is turbo::unicode::default_engine
     * @param input utf16 input buffer
     * @param length utf16 input buffer length
     * @param utf32_buffer utf32 output buffer
     * @return utf32 output buffer length
     */
    template<typename Engine = turbo::unicode::default_engine, typename turbo::check_requires<turbo::unicode::is_unicode_engine<Engine>> = 0>
    [[nodiscard]] size_t convert_utf16le_to_utf32(const char16_t *input, size_t length, char32_t *utf32_buffer) noexcept;

    /**
     * @ingroup turbo_unicode_converter
     * @brief convert utf16 with big endian to utf32, and assume input buffer is valid utf16.
     *       using this function is faster than convert_valid_utf16_to_utf32_be_with_errors.
     * @note guard that input buffer is big endian. input buffer must be valid utf16.
     * @tparam Engine default is turbo::unicode::default_engine
     * @param input utf16 input buffer
     * @param length utf16 input buffer length
     * @param utf32_buffer utf32 output buffer
     * @return utf32 output buffer length
     */
    template<typename Engine = turbo::unicode::default_engine, typename turbo::check_requires<turbo::unicode::is_unicode_engine<Engine>> = 0>
    [[nodiscard]] size_t convert_utf16be_to_utf32(const char16_t *input, size_t length, char32_t *utf32_buffer) noexcept;

    /**
     * @ingroup turbo_unicode_converter
     * @brief convert utf16 to utf32 with auto detect endian.
     *       Example:
     *       @code
     *       char32_t utf32_buffer[1024];
     *       auto result = convert_utf16_to_utf32_with_errors(input, length, utf32_buffer);
     *       // result is ok if input is valid utf16, otherwise result is error and error position is set.
     *       @endcode
     * @note guard that input buffer is host endian. input buffer must be valid utf16.
     * @tparam Engine default is turbo::unicode::default_engine
     * @param input utf16 input buffer
     * @param length utf16 input buffer length
     * @param utf8_buffer utf8 output buffer
     * @return result, if valid utf16, result is ok, otherwise result is error and error position is set.
     */
    template<typename Engine = turbo::unicode::default_engine, typename turbo::check_requires<turbo::unicode::is_unicode_engine<Engine>> = 0>
    [[nodiscard]] result
    convert_utf16_to_utf32_with_errors(const char16_t *input, size_t length, char32_t *utf32_buffer) noexcept;

    /**
     * @ingroup turbo_unicode_converter
     * @brief convert utf16 with little endian to utf32, and assume input buffer is valid utf16.
     *       using this function is faster than convert_valid_utf16_to_utf32_le_with_errors.
     * @note guard that input buffer is little endian. input buffer must be valid utf16.
     * @tparam Engine default is turbo::unicode::default_engine
     * @param input utf16 input buffer
     * @param length utf16 input buffer length
     * @param utf32_buffer utf32 output buffer
     * @return utf32 output buffer length
     */
    template<typename Engine = turbo::unicode::default_engine, typename turbo::check_requires<turbo::unicode::is_unicode_engine<Engine>> = 0>
    [[nodiscard]] result
    convert_utf16le_to_utf32_with_errors(const char16_t *input, size_t length, char32_t *utf32_buffer) noexcept;

    /**
     * @ingroup turbo_unicode_converter
     * @brief convert utf16 with big endian to utf32, and assume input buffer is valid utf16.
     *       using this function is faster than convert_valid_utf16_to_utf32_be_with_errors.
     * @note guard that input buffer is big endian. input buffer must be valid utf16.
     * @tparam Engine default is turbo::unicode::default_engine
     * @param input utf16 input buffer
     * @param length utf16 input buffer length
     * @param utf32_buffer utf32 output buffer
     * @return utf32 output buffer length
     */
    template<typename Engine = turbo::unicode::default_engine, typename turbo::check_requires<turbo::unicode::is_unicode_engine<Engine>> = 0>
    [[nodiscard]] result
    convert_utf16be_to_utf32_with_errors(const char16_t *input, size_t length, char32_t *utf32_buffer) noexcept;

    /**
     * @ingroup turbo_unicode_converter
     * @brief convert utf32 to utf16 with auto detect endian.
     *       Example:
     *       @code
     *       char16_t utf16_buffer[1024];
     *       auto utf16_length = convert_utf32_to_utf16(input, length, utf16_buffer);
     *       // utf16_length is utf16 buffer length
     *       @endcode
     * @note guard that input buffer is host endian. input buffer must be valid utf32.
     * @tparam Engine default is turbo::unicode::default_engine
     * @param input utf32 input buffer
     * @param length utf32 input buffer length
     * @param utf16_buffer utf16 output buffer
     * @return utf16 output buffer length
     */
    template<typename Engine = turbo::unicode::default_engine, typename turbo::check_requires<turbo::unicode::is_unicode_engine<Engine>> = 0>
    [[nodiscard]] size_t
    convert_valid_utf16_to_utf32(const char16_t *input, size_t length, char32_t *utf32_buffer) noexcept;

    /**
     * @ingroup turbo_unicode_converter
     * @brief convert utf32 with little endian to utf16, and assume input buffer is valid utf32.
     *       using this function is faster than convert_valid_utf32_to_utf16_le_with_errors.
     * @note guard that input buffer is little endian. input buffer must be valid utf32.
     * @tparam Engine default is turbo::unicode::default_engine
     * @param input utf32 input buffer
     * @param length utf32 input buffer length
     * @param utf16_buffer utf16 output buffer
     * @return utf16 output buffer length
     */
    template<typename Engine = turbo::unicode::default_engine, typename turbo::check_requires<turbo::unicode::is_unicode_engine<Engine>> = 0>
    [[nodiscard]] size_t
    convert_valid_utf16le_to_utf32(const char16_t *input, size_t length, char32_t *utf32_buffer) noexcept;

    /**
     * @ingroup turbo_unicode_converter
     * @brief convert utf32 with big endian to utf16, and assume input buffer is valid utf32.
     *       using this function is faster than convert_valid_utf32_to_utf16_be_with_errors.
     * @note guard that input buffer is big endian. input buffer must be valid utf32.
     * @tparam Engine default is turbo::unicode::default_engine
     * @param input utf32 input buffer
     * @param length utf32 input buffer length
     * @param utf16_buffer utf16 output buffer
     * @return utf16 output buffer length
     */
    template<typename Engine = turbo::unicode::default_engine, typename turbo::check_requires<turbo::unicode::is_unicode_engine<Engine>> = 0>
    [[nodiscard]] size_t
    convert_valid_utf16be_to_utf32(const char16_t *input, size_t length, char32_t *utf32_buffer) noexcept;

    /**
     * @ingroup turbo_unicode_converter
     * @brief get utf8 length from utf16 input buffer.
     * @tparam Engine default is turbo::unicode::default_engine
     * @param input utf16 input buffer
     * @param length utf16 input buffer length
     * @return utf8 output buffer length
     */
    template<typename Engine = turbo::unicode::default_engine, typename turbo::check_requires<turbo::unicode::is_unicode_engine<Engine>> = 0>
    [[nodiscard]] size_t utf8_length_from_utf16(const char16_t *input, size_t length) noexcept;

    /**
     * @ingroup turbo_unicode_converter
     * @brief get utf8 length from utf16 little endian input buffer.
     * @tparam Engine default is turbo::unicode::default_engine
     * @param input utf16 input buffer
     * @param length utf16 input buffer length
     * @return utf8 output buffer length
     * @note guard that input buffer is little endian.
     */
    template<typename Engine = turbo::unicode::default_engine, typename turbo::check_requires<turbo::unicode::is_unicode_engine<Engine>> = 0>
    [[nodiscard]] size_t utf8_length_from_utf16le(const char16_t *input, size_t length) noexcept;

    /**
     * @ingroup turbo_unicode_converter
     * @brief get utf8 length from utf16 big endian input buffer.
     * @tparam Engine default is turbo::unicode::default_engine
     * @param input utf16 input buffer
     * @param length utf16 input buffer length
     * @return utf8 output buffer length
     * @note guard that input buffer is big endian.
     */
    template<typename Engine = turbo::unicode::default_engine, typename turbo::check_requires<turbo::unicode::is_unicode_engine<Engine>> = 0>
    [[nodiscard]] size_t utf8_length_from_utf16be(const char16_t *input, size_t length) noexcept;

    /**
     * @ingroup turbo_unicode_converter
     * @Brief convert utf32 to utf8.
     *      Example:
     *      @code
     *      char utf8_buffer[1024];
     *      auto utf8_length = convert_utf32_to_utf8(input, length, utf8_buffer);
     *      // utf8_length is utf8 buffer length
     *      @endcode
     * @tparam Engine default is turbo::unicode::default_engine
     * @param input utf32 input buffer
     * @param length utf32 input buffer length
     * @param utf8_buffer utf8 output buffer
     * @return utf8 output buffer length
     */
    template<typename Engine = turbo::unicode::default_engine, typename turbo::check_requires<turbo::unicode::is_unicode_engine<Engine>> = 0>
    [[nodiscard]] size_t convert_utf32_to_utf8(const char32_t *input, size_t length, char *utf8_buffer) noexcept;

    /**
     * @ingroup turbo_unicode_converter
     * @Brief convert utf32 to utf8 with little endian.
     *      Example:
     *      @code
     *      char utf8_buffer[1024];
     *      auto utf8_length = convert_utf32_to_utf8_le(input, length, utf8_buffer);
     *      // utf8_length is utf8 buffer length
     *      @endcode
     * @tparam Engine default is turbo::unicode::default_engine
     * @param input utf32 input buffer
     * @param length utf32 input buffer length
     * @param utf8_buffer utf8 output buffer
     * @return utf8 output buffer length
     */
    template<typename Engine = turbo::unicode::default_engine, typename turbo::check_requires<turbo::unicode::is_unicode_engine<Engine>> = 0>
    [[nodiscard]] result convert_utf32_to_utf8_with_errors(const char32_t *input, size_t length, char *utf8_buffer) noexcept;

    /**
     * @ingroup turbo_unicode_converter
     * @Brief convert utf32 to utf8 with host endian.
     *      Example:
     *      @code
     *      char utf8_buffer[1024];
     *      auto utf8_length = convert_utf32_to_utf8_le(input, length, utf8_buffer);
     *      // utf8_length is utf8 buffer length
     *      @endcode
     * @tparam Engine default is turbo::unicode::default_engine
     * @param input utf32 input buffer
     * @param length utf32 input buffer length
     * @param utf8_buffer utf8 output buffer
     * @return utf8 output buffer length
     * @note guard that input buffer is host endian.
     */
    template<typename Engine = turbo::unicode::default_engine, typename turbo::check_requires<turbo::unicode::is_unicode_engine<Engine>> = 0>
    [[nodiscard]] size_t convert_valid_utf32_to_utf8(const char32_t *input, size_t length, char *utf8_buffer) noexcept;

    /**
     * @ingroup turbo_unicode_converter
     * @Brief convert utf32 to utf16 host endian.
     *     Example:
     *     @code
     *     char16_t utf16_buffer[1024];
     *     auto utf16_length = convert_utf32_to_utf16(input, length, utf16_buffer);
     *     // utf16_length is utf16 buffer length
     *     @endcode
     * @tparam Engine default is turbo::unicode::default_engine
     * @param input utf32 input buffer
     * @param length utf32 input buffer length
     * @param utf16_buffer utf16 output buffer
     * @return utf16 output buffer length
     * @note guard that input buffer is host endian.
     */
    template<typename Engine = turbo::unicode::default_engine, typename turbo::check_requires<turbo::unicode::is_unicode_engine<Engine>> = 0>
    [[nodiscard]] size_t convert_utf32_to_utf16(const char32_t *input, size_t length, char16_t *utf16_buffer) noexcept;

    /**
     * @ingroup turbo_unicode_converter
     * @Brief convert utf32 to utf16 little endian.
     *     Example:
     *     @code
     *     char16_t utf16_buffer[1024];
     *     auto utf16_length = convert_utf32_to_utf16_le(input, length, utf16_buffer);
     *     // utf16_length is utf16 buffer length
     *     @endcode
     * @tparam Engine default is turbo::unicode::default_engine
     * @param input utf32 input buffer
     * @param length utf32 input buffer length
     * @param utf16_buffer utf16 output buffer
     * @return utf16 output buffer length
     * @note guard that input buffer is little endian.
     */
    template<typename Engine = turbo::unicode::default_engine, typename turbo::check_requires<turbo::unicode::is_unicode_engine<Engine>> = 0>
    [[nodiscard]] size_t convert_utf32_to_utf16le(const char32_t *input, size_t length, char16_t *utf16_buffer) noexcept;

    /**
     * @ingroup turbo_unicode_converter
     * @Brief convert utf32 to utf16 big endian.
     *     Example:
     *     @code
     *     char16_t utf16_buffer[1024];
     *     auto utf16_length = convert_utf32_to_utf16_be(input, length, utf16_buffer);
     *     // utf16_length is utf16 buffer length
     *     @endcode
     * @tparam Engine default is turbo::unicode::default_engine
     * @param input utf32 input buffer
     * @param length utf32 input buffer length
     * @param utf16_buffer utf16 output buffer
     * @return utf16 output buffer length
     * @note guard that input buffer is big endian.
     */
    template<typename Engine = turbo::unicode::default_engine, typename turbo::check_requires<turbo::unicode::is_unicode_engine<Engine>> = 0>
    [[nodiscard]] size_t convert_utf32_to_utf16be(const char32_t *input, size_t length, char16_t *utf16_buffer) noexcept;

    /**
     * @ingroup turbo_unicode_converter
     * @Brief convert utf32 to utf16 with auto detect endian.
     *     Example:
     *     @code
     *     char16_t utf16_buffer[1024];
     *     auto result = convert_utf32_to_utf16_with_errors(input, length, utf16_buffer);
     *     // result is ok if input is valid utf32, otherwise result is error and error position is set.
     *     @endcode
     * @note guard that input buffer is host endian. input buffer must be valid utf32.
     * @tparam Engine default is turbo::unicode::default_engine
     * @param input utf32 input buffer
     * @param length utf32 input buffer length
     * @param utf16_buffer utf16 output buffer
     * @return result, if valid utf32, result is ok, otherwise result is error and error position is set.
     */
    template<typename Engine = turbo::unicode::default_engine, typename turbo::check_requires<turbo::unicode::is_unicode_engine<Engine>> = 0>
    [[nodiscard]] result
    convert_utf32_to_utf16_with_errors(const char32_t *input, size_t length, char16_t *utf16_buffer) noexcept;

    /**
     * @ingroup turbo_unicode_converter
     * @Brief convert utf32 to utf16 with little endian.
     *     Example:
     *     @code
     *     char16_t utf16_buffer[1024];
     *     auto result = convert_utf32_to_utf16_le_with_errors(input, length, utf16_buffer);
     *     // result is ok if input is valid utf32, otherwise result is error and error position is set.
     *     @endcode
     * @note guard that input buffer is little endian. input buffer must be valid utf32.
     * @tparam Engine default is turbo::unicode::default_engine
     * @param input utf32 input buffer
     * @param length utf32 input buffer length
     * @param utf16_buffer utf16 output buffer
     * @return result, if valid utf32, result is ok, otherwise result is error and error position is set.
     */
    template<typename Engine = turbo::unicode::default_engine, typename turbo::check_requires<turbo::unicode::is_unicode_engine<Engine>> = 0>
    [[nodiscard]] result
    convert_utf32_to_utf16le_with_errors(const char32_t *input, size_t length, char16_t *utf16_buffer) noexcept;

    /**
     * @ingroup turbo_unicode_converter
     * @Brief convert utf32 to utf16 with big endian.
     *     Example:
     *     @code
     *     char16_t utf16_buffer[1024];
     *     auto result = convert_utf32_to_utf16_be_with_errors(input, length, utf16_buffer);
     *     // result is ok if input is valid utf32, otherwise result is error and error position is set.
     *     @endcode
     * @note guard that input buffer is big endian. input buffer must be valid utf32.
     * @tparam Engine default is turbo::unicode::default_engine
     * @param input utf32 input buffer
     * @param length utf32 input buffer length
     * @param utf16_buffer utf16 output buffer
     * @return result, if valid utf32, result is ok, otherwise result is error and error position is set.
     */
    template<typename Engine = turbo::unicode::default_engine, typename turbo::check_requires<turbo::unicode::is_unicode_engine<Engine>> = 0>
    [[nodiscard]] result
    convert_utf32_to_utf16be_with_errors(const char32_t *input, size_t length, char16_t *utf16_buffer) noexcept;

    /**
     * @ingroup turbo_unicode_converter
     * @Brief convert utf32 to utf16 with auto detect endian.
     *     Example:
     *     @code
     *     char16_t utf16_buffer[1024];
     *     auto result = convert_utf32_to_utf16_with_errors(input, length, utf16_buffer);
     *     // result is ok if input is valid utf32, otherwise result is error and error position is set.
     *     @endcode
     * @note guard that input buffer is host endian. input buffer must be valid utf32.
     * @tparam Engine default is turbo::unicode::default_engine
     * @param input utf32 input buffer
     * @param length utf32 input buffer length
     * @param utf16_buffer utf16 output buffer
     * @return result, if valid utf32, result is ok, otherwise result is error and error position is set.
     */
    template<typename Engine = turbo::unicode::default_engine, typename turbo::check_requires<turbo::unicode::is_unicode_engine<Engine>> = 0>
    [[nodiscard]] size_t
    convert_valid_utf32_to_utf16(const char32_t *input, size_t length, char16_t *utf16_buffer) noexcept;

    /**
     * @ingroup turbo_unicode_converter
     * @Brief convert utf32 to utf16 with little endian.
     *     Example:
     *     @code
     *     char16_t utf16_buffer[1024];
     *     auto result = convert_utf32_to_utf16_le_with_errors(input, length, utf16_buffer);
     *     // result is ok if input is valid utf32, otherwise result is error and error position is set.
     *     @endcode
     * @note guard that input buffer is little endian. input buffer must be valid utf32.
     * @tparam Engine default is turbo::unicode::default_engine
     * @param input utf32 input buffer
     * @param length utf32 input buffer length
     * @param utf16_buffer utf16 output buffer
     * @return result, if valid utf32, result is ok, otherwise result is error and error position is set.
     */
    template<typename Engine = turbo::unicode::default_engine, typename turbo::check_requires<turbo::unicode::is_unicode_engine<Engine>> = 0>
    [[nodiscard]] size_t
    convert_valid_utf32_to_utf16le(const char32_t *input, size_t length, char16_t *utf16_buffer) noexcept;

    /**
     * @ingroup turbo_unicode_converter
     * @Brief convert utf32 to utf16 with big endian.
     *     Example:
     *     @code
     *     char16_t utf16_buffer[1024];
     *     auto result = convert_utf32_to_utf16_be_with_errors(input, length, utf16_buffer);
     *     // result is ok if input is valid utf32, otherwise result is error and error position is set.
     *     @endcode
     * @note guard that input buffer is big endian. input buffer must be valid utf32.
     * @tparam Engine default is turbo::unicode::default_engine
     * @param input utf32 input buffer
     * @param length utf32 input buffer length
     * @param utf16_buffer utf16 output buffer
     * @return result, if valid utf32, result is ok, otherwise result is error and error position is set.
     */
    template<typename Engine = turbo::unicode::default_engine, typename turbo::check_requires<turbo::unicode::is_unicode_engine<Engine>> = 0>
    [[nodiscard]] size_t
    convert_valid_utf32_to_utf16be(const char32_t *input, size_t length, char16_t *utf16_buffer) noexcept;

    /**
     * @ingroup turbo_unicode_converter
     * @brief convert utf16 endian ro reverse endian.
     * @tparam Engine default is turbo::unicode::default_engine
     * @param input utf16 input buffer
     * @param length utf16 input buffer length
     * @param output utf16 output buffer
     * @note guard that input buffer is host endian.
     */
    template<typename Engine = turbo::unicode::default_engine, typename turbo::check_requires<turbo::unicode::is_unicode_engine<Engine>> = 0>
    void change_endianness_utf16(const char16_t *input, size_t length, char16_t *output) noexcept;

    /**
     * @ingroup turbo_unicode_converter
     * @brief get utf8 length from utf32 input buffer.
     * @tparam Engine default is turbo::unicode::default_engine
     * @param input utf32 input buffer
     * @param length utf32 input buffer length
     * @return utf8 output buffer length
     */
    template<typename Engine = turbo::unicode::default_engine, typename turbo::check_requires<turbo::unicode::is_unicode_engine<Engine>> = 0>
    [[nodiscard]] size_t utf8_length_from_utf32(const char32_t *input, size_t length) noexcept;

    /**
     * @ingroup turbo_unicode_converter
     * @brief get utf16 length from utf32 input buffer.
     * @tparam Engine default is turbo::unicode::default_engine
     * @param input utf32 input buffer
     * @param length utf32 input buffer length
     * @return utf16 output buffer length
     * @note guard that input buffer is host endian.
     */
    template<typename Engine = turbo::unicode::default_engine, typename turbo::check_requires<turbo::unicode::is_unicode_engine<Engine>> = 0>
    [[nodiscard]] size_t utf16_length_from_utf32(const char32_t *input, size_t length) noexcept;

    /**
     * @ingroup turbo_unicode_converter
     * @brief get utf16 length from utf32 host endian input buffer.
     * @tparam Engine default is turbo::unicode::default_engine
     * @param input utf32 input buffer
     * @param length utf32 input buffer length
     * @return utf16 output buffer length
     * @note guard that input buffer is little endian.
     */
    template<typename Engine = turbo::unicode::default_engine, typename turbo::check_requires<turbo::unicode::is_unicode_engine<Engine>> = 0>
    [[nodiscard]] size_t utf32_length_from_utf16(const char16_t *input, size_t length) noexcept;

    /**
     * @ingroup turbo_unicode_converter
     * @brief get utf16 length from utf32 little endian input buffer.
     * @tparam Engine default is turbo::unicode::default_engine
     * @param input utf32 input buffer
     * @param length utf32 input buffer length
     * @return utf16 output buffer length
     * @note guard that input buffer is little endian.
     */
    template<typename Engine = turbo::unicode::default_engine, typename turbo::check_requires<turbo::unicode::is_unicode_engine<Engine>> = 0>
    [[nodiscard]] size_t utf32_length_from_utf16le(const char16_t *input, size_t length) noexcept;

    /**
     * @ingroup turbo_unicode_converter
     * @brief get utf16 length from utf32 big endian input buffer.
     * @tparam Engine default is turbo::unicode::default_engine
     * @param input utf32 input buffer
     * @param length utf32 input buffer length
     * @return utf16 output buffer length
     * @note guard that input buffer is big endian.
     */
    template<typename Engine = turbo::unicode::default_engine, typename turbo::check_requires<turbo::unicode::is_unicode_engine<Engine>> = 0>
    [[nodiscard]] size_t utf32_length_from_utf16be(const char16_t *input, size_t length) noexcept;

    /**
     * @ingroup turbo_unicode_converter
     * @brief count utf16 code points.
     * @tparam Engine default is turbo::unicode::default_engine
     * @param input utf16 input buffer
     * @param length utf16 input buffer length
     * @return utf16 code points count
     * @note guard that input buffer is host endian.
     */
    template<typename Engine = turbo::unicode::default_engine, typename turbo::check_requires<turbo::unicode::is_unicode_engine<Engine>> = 0>
    [[nodiscard]] size_t count_utf16(const char16_t *input, size_t length) noexcept;

    /**
     * @ingroup turbo_unicode_converter
     * @brief count utf16 code points from utf16 little endian input buffer.
     * @tparam Engine default is turbo::unicode::default_engine
     * @param input utf16 input buffer
     * @param length utf16 input buffer length
     * @return utf16 code points count
     * @note guard that input buffer is little endian.
     */
    template<typename Engine = turbo::unicode::default_engine, typename turbo::check_requires<turbo::unicode::is_unicode_engine<Engine>> = 0>
    [[nodiscard]] size_t count_utf16le(const char16_t *input, size_t length) noexcept;

    /**
     * @ingroup turbo_unicode_converter
     * @brief count utf16 code points from utf16 big endian input buffer.
     * @tparam Engine default is turbo::unicode::default_engine
     * @param input utf16 input buffer
     * @param length utf16 input buffer length
     * @return utf16 code points count
     * @note guard that input buffer is big endian.
     */
    template<typename Engine = turbo::unicode::default_engine, typename turbo::check_requires<turbo::unicode::is_unicode_engine<Engine>> = 0>
    [[nodiscard]] size_t count_utf16be(const char16_t *input, size_t length) noexcept;

    /**
     * @ingroup turbo_unicode_converter
     * @brief count utf8 code points.
     * @tparam Engine default is turbo::unicode::default_engine
     * @param input utf8 input buffer
     * @param length utf8 input buffer length
     * @return utf8 code points count
     */
    template<typename Engine = turbo::unicode::default_engine, typename turbo::check_requires<turbo::unicode::is_unicode_engine<Engine>> = 0>
    [[nodiscard]] size_t count_utf8(const char *input, size_t length) noexcept;


    /// Implementation
    template <typename Engine, turbo::check_requires<turbo::unicode::is_unicode_engine<Engine>>>
    inline turbo::EncodingType auto_detect_encoding(const char * input, size_t length) noexcept {
        return turbo::unicode::Converter<Engine>::auto_detect_encoding(input, length);
    }

    template<typename Engine, turbo::check_requires<turbo::unicode::is_unicode_engine<Engine>>>
    inline int detect_encodings(const char *input, size_t length) noexcept {
        return turbo::unicode::Converter<Engine>::detect_encodings(input, length);
    }

    template<typename Engine, turbo::check_requires<turbo::unicode::is_unicode_engine<Engine>>>
    [[nodiscard]] inline bool validate_utf8(const char *buf, size_t len) noexcept {
        return turbo::unicode::Converter<Engine>::validate_utf8(buf, len);
    }

    template<typename Engine, turbo::check_requires<turbo::unicode::is_unicode_engine<Engine>>>
    [[nodiscard]] inline result validate_utf8_with_errors(const char *buf, size_t len) noexcept {
        return turbo::unicode::Converter<Engine>::validate_utf8_with_errors(buf, len);
    }

    template<typename Engine, turbo::check_requires<turbo::unicode::is_unicode_engine<Engine>>>
    [[nodiscard]] bool validate_ascii(const char *buf, size_t len) noexcept {
        return turbo::unicode::Converter<Engine>::validate_ascii(buf, len);
    }

    template<typename Engine, turbo::check_requires<turbo::unicode::is_unicode_engine<Engine>>>
    [[nodiscard]] inline result validate_ascii_with_errors(const char *buf, size_t len) noexcept {
        return turbo::unicode::Converter<Engine>::validate_ascii_with_errors(buf, len);
    }

    template<typename Engine, turbo::check_requires<turbo::unicode::is_unicode_engine<Engine>>>
    [[nodiscard]] inline bool validate_utf16(const char16_t *buf, size_t len) noexcept {
        return turbo::unicode::Converter<Engine>::validate_utf16(buf, len);
    }

    template<typename Engine, turbo::check_requires<turbo::unicode::is_unicode_engine<Engine>>>
    [[nodiscard]] inline bool validate_utf16le(const char16_t *buf, size_t len) noexcept {
        return turbo::unicode::Converter<Engine>::validate_utf16le(buf, len);
    }

    template<typename Engine, turbo::check_requires<turbo::unicode::is_unicode_engine<Engine>>>
    [[nodiscard]] inline bool validate_utf16be(const char16_t *buf, size_t len) noexcept {
        return turbo::unicode::Converter<Engine>::validate_utf16be(buf, len);
    }

    template<typename Engine, turbo::check_requires<turbo::unicode::is_unicode_engine<Engine>>>
    [[nodiscard]] inline result validate_utf16_with_errors(const char16_t *buf, size_t len) noexcept {
        return turbo::unicode::Converter<Engine>::validate_utf16_with_errors(buf, len);
    }

    template<typename Engine, turbo::check_requires<turbo::unicode::is_unicode_engine<Engine>>>
    [[nodiscard]] inline result validate_utf16le_with_errors(const char16_t *buf, size_t len) noexcept {
        return turbo::unicode::Converter<Engine>::validate_utf16le_with_errors(buf, len);
    }

    template<typename Engine, turbo::check_requires<turbo::unicode::is_unicode_engine<Engine>>>
    [[nodiscard]] inline result validate_utf16be_with_errors(const char16_t *buf, size_t len) noexcept {
        return turbo::unicode::Converter<Engine>::validate_utf16be_with_errors(buf, len);
    }

    template<typename Engine, turbo::check_requires<turbo::unicode::is_unicode_engine<Engine>>>
    [[nodiscard]] inline bool validate_utf32(const char32_t *buf, size_t len) noexcept {
        return turbo::unicode::Converter<Engine>::validate_utf32(buf, len);
    }

    template<typename Engine, turbo::check_requires<turbo::unicode::is_unicode_engine<Engine>>>
    [[nodiscard]] inline result validate_utf32_with_errors(const char32_t *buf, size_t len) noexcept {
        return turbo::unicode::Converter<Engine>::validate_utf32_with_errors(buf, len);
    }

    template<typename Engine, turbo::check_requires<turbo::unicode::is_unicode_engine<Engine>>>
    [[nodiscard]] inline size_t convert_utf8_to_utf16(const char *input, size_t length, char16_t *utf16_output) noexcept {
        if constexpr (kIsLittleEndian) {
            return turbo::unicode::Converter<Engine>::convert_utf8_to_utf16le(input, length, utf16_output);
        } else {
            return turbo::unicode::Converter<Engine>::convert_utf8_to_utf16be(input, length, utf16_output);
        }
    }

    template<typename Engine, turbo::check_requires<turbo::unicode::is_unicode_engine<Engine>>>
    [[nodiscard]] inline size_t
    convert_utf8_to_utf16le(const char *input, size_t length, char16_t *utf16_output) noexcept {
        return turbo::unicode::Converter<Engine>::convert_utf8_to_utf16le(input, length, utf16_output);
    }

    template<typename Engine, turbo::check_requires<turbo::unicode::is_unicode_engine<Engine>>>
    [[nodiscard]] inline size_t
    convert_utf8_to_utf16be(const char *input, size_t length, char16_t *utf16_output) noexcept {
        return turbo::unicode::Converter<Engine>::convert_utf8_to_utf16be(input, length, utf16_output);
    }

    template<typename Engine, turbo::check_requires<turbo::unicode::is_unicode_engine<Engine>>>
    [[nodiscard]] inline result
    convert_utf8_to_utf16_with_errors(const char *input, size_t length, char16_t *utf16_output) noexcept {
        if constexpr (kIsLittleEndian) {
            return turbo::unicode::Converter<Engine>::convert_utf8_to_utf16le_with_errors(input, length, utf16_output);
        } else {
            return turbo::unicode::Converter<Engine>::convert_utf8_to_utf16be_with_errors(input, length, utf16_output);
        }
    }

    template<typename Engine, turbo::check_requires<turbo::unicode::is_unicode_engine<Engine>>>
    [[nodiscard]] inline result
    convert_utf8_to_utf16le_with_errors(const char *input, size_t length, char16_t *utf16_output) noexcept {
        return turbo::unicode::Converter<Engine>::convert_utf8_to_utf16le_with_errors(input, length, utf16_output);
    }

    template<typename Engine, turbo::check_requires<turbo::unicode::is_unicode_engine<Engine>>>
    [[nodiscard]] inline result
    convert_utf8_to_utf16be_with_errors(const char *input, size_t length, char16_t *utf16_output) noexcept {
        return turbo::unicode::Converter<Engine>::convert_utf8_to_utf16be_with_errors(input, length, utf16_output);
    }

    template<typename Engine, turbo::check_requires<turbo::unicode::is_unicode_engine<Engine>>>
    [[nodiscard]] inline size_t convert_utf8_to_utf32(const char *input, size_t length, char32_t *utf32_output) noexcept {
        if constexpr (kIsLittleEndian) {
            return turbo::unicode::Converter<Engine>::ConvertUtf8ToUtf32Le(input, length, utf32_output);
        } else {
            return turbo::unicode::Converter<Engine>::ConvertUtf8ToUtf32Be(input, length, utf32_output);
        }
    }

    template<typename Engine, turbo::check_requires<turbo::unicode::is_unicode_engine<Engine>>>
    [[nodiscard]] inline result
    convert_utf8_to_utf32_with_errors(const char *input, size_t length, char32_t *utf32_output) noexcept {
        if constexpr (kIsLittleEndian) {
            return turbo::unicode::Converter<Engine>::ConvertUtf8ToUtf32LeWithErrors(input, length, utf32_output);
        } else {
            return turbo::unicode::Converter<Engine>::ConvertUtf8ToUtf32BeWithErrors(input, length, utf32_output);
        }
    }

    template<typename Engine, turbo::check_requires<turbo::unicode::is_unicode_engine<Engine>>>
    [[nodiscard]] size_t ConvertValidUtf8ToUtf16(const char *input, size_t length, char16_t *utf16_buffer) noexcept {
        if constexpr (kIsLittleEndian) {
            return turbo::unicode::Converter<Engine>::convert_valid_utf8_to_utf16le(input, length, utf16_buffer);
        } else {
            return turbo::unicode::Converter<Engine>::convert_valid_utf8_to_utf16be(input, length, utf16_buffer);
        }
    }

    template<typename Engine, turbo::check_requires<turbo::unicode::is_unicode_engine<Engine>>>
    [[nodiscard]] inline size_t
    convert_valid_utf8_to_utf16le(const char *input, size_t length, char16_t *utf16_buffer) noexcept {
        return turbo::unicode::Converter<Engine>::convert_valid_utf8_to_utf16le(input, length, utf16_buffer);
    }

    template<typename Engine, turbo::check_requires<turbo::unicode::is_unicode_engine<Engine>>>
    [[nodiscard]] inline size_t
    convert_valid_utf8_to_utf16be(const char *input, size_t length, char16_t *utf16_buffer) noexcept {
        return turbo::unicode::Converter<Engine>::convert_valid_utf8_to_utf16be(input, length, utf16_buffer);
    }

    template<typename Engine, turbo::check_requires<turbo::unicode::is_unicode_engine<Engine>>>
    [[nodiscard]] inline size_t
    convert_valid_utf8_to_utf32(const char *input, size_t length, char32_t *utf32_buffer) noexcept {
        if constexpr (kIsLittleEndian) {
            return turbo::unicode::Converter<Engine>::ConvertValidUtf8ToUtf32Le(input, length, utf32_buffer);
        } else {
            return turbo::unicode::Converter<Engine>::ConvertValidUtf8ToUtf32Be(input, length, utf32_buffer);
        }
    }

    template<typename Engine, turbo::check_requires<turbo::unicode::is_unicode_engine<Engine>>>
    [[nodiscard]] inline size_t utf16_length_from_utf8(const char *input, size_t length) noexcept {
        return turbo::unicode::Converter<Engine>::utf16_length_from_utf8(input, length);
    }

    template<typename Engine, turbo::check_requires<turbo::unicode::is_unicode_engine<Engine>>>
    [[nodiscard]] inline size_t utf32_length_from_utf8(const char *input, size_t length) noexcept {
        return turbo::unicode::Converter<Engine>::utf32_length_from_utf8(input, length);
    }

    template<typename Engine, turbo::check_requires<turbo::unicode::is_unicode_engine<Engine>>>
    [[nodiscard]] inline size_t convert_utf16_to_utf8(const char16_t *input, size_t length, char *utf8_buffer) noexcept {
        if constexpr (kIsLittleEndian) {
            return turbo::unicode::Converter<Engine>::convert_utf16le_to_utf8(input, length, utf8_buffer);
        } else {
            return turbo::unicode::Converter<Engine>::convert_utf16be_to_utf8(input, length, utf8_buffer);
        }
    }

    template<typename Engine, turbo::check_requires<turbo::unicode::is_unicode_engine<Engine>>>
    [[nodiscard]] inline size_t convert_utf16le_to_utf8(const char16_t *input, size_t length, char *utf8_buffer) noexcept {
        return turbo::unicode::Converter<Engine>::convert_utf16le_to_utf8(input, length, utf8_buffer);
    }

    template<typename Engine, turbo::check_requires<turbo::unicode::is_unicode_engine<Engine>>>
    [[nodiscard]] inline size_t convert_utf16be_to_utf8(const char16_t *input, size_t length, char *utf8_buffer) noexcept {
        return turbo::unicode::Converter<Engine>::convert_utf16be_to_utf8(input, length, utf8_buffer);
    }

    template<typename Engine, turbo::check_requires<turbo::unicode::is_unicode_engine<Engine>>>
    [[nodiscard]] inline result
    convert_utf16_to_utf8_with_errors(const char16_t *input, size_t length, char *utf8_buffer) noexcept {
        if constexpr (kIsLittleEndian) {
            return turbo::unicode::Converter<Engine>::convert_utf16le_to_utf8_with_errors(input, length, utf8_buffer);
        } else {
            return turbo::unicode::Converter<Engine>::convert_utf16be_to_utf8_with_errors(input, length, utf8_buffer);
        }
    }

    template<typename Engine, turbo::check_requires<turbo::unicode::is_unicode_engine<Engine>>>
    [[nodiscard]] inline result
    convert_utf16le_to_utf8_with_errors(const char16_t *input, size_t length, char *utf8_buffer) noexcept {
        return turbo::unicode::Converter<Engine>::convert_utf16le_to_utf8_with_errors(input, length, utf8_buffer);
    }

    template<typename Engine, turbo::check_requires<turbo::unicode::is_unicode_engine<Engine>>>
    [[nodiscard]] inline result
    convert_utf16be_to_utf8_with_errors(const char16_t *input, size_t length, char *utf8_buffer) noexcept {
        return turbo::unicode::Converter<Engine>::convert_utf16be_to_utf8_with_errors(input, length, utf8_buffer);
    }

    template<typename Engine, turbo::check_requires<turbo::unicode::is_unicode_engine<Engine>>>
    [[nodiscard]] inline size_t
    convert_valid_utf16_to_utf8(const char16_t *input, size_t length, char *utf8_buffer) noexcept {
        if constexpr (kIsLittleEndian) {
            return turbo::unicode::Converter<Engine>::convert_valid_utf16le_to_utf8(input, length, utf8_buffer);
        } else {
            return turbo::unicode::Converter<Engine>::convert_valid_utf16be_to_utf8(input, length, utf8_buffer);
        }
    }

    template<typename Engine, turbo::check_requires<turbo::unicode::is_unicode_engine<Engine>>>
    [[nodiscard]] inline size_t
    convert_valid_utf16le_to_utf8(const char16_t *input, size_t length, char *utf8_buffer) noexcept {
        return turbo::unicode::Converter<Engine>::convert_valid_utf16le_to_utf8(input, length, utf8_buffer);
    }

    template<typename Engine, turbo::check_requires<turbo::unicode::is_unicode_engine<Engine>>>
    [[nodiscard]] inline size_t
    convert_valid_utf16be_to_utf8(const char16_t *input, size_t length, char *utf8_buffer) noexcept {
        return turbo::unicode::Converter<Engine>::convert_valid_utf16be_to_utf8(input, length, utf8_buffer);
    }

    template<typename Engine, turbo::check_requires<turbo::unicode::is_unicode_engine<Engine>>>
    [[nodiscard]] inline size_t
    convert_utf16_to_utf32(const char16_t *input, size_t length, char32_t *utf32_buffer) noexcept {
        if constexpr (kIsLittleEndian) {
            return turbo::unicode::Converter<Engine>::convert_utf16le_to_utf32(input, length, utf32_buffer);
        } else {
            return turbo::unicode::Converter<Engine>::convert_utf16be_to_utf32(input, length, utf32_buffer);
        }
    }

    template<typename Engine, turbo::check_requires<turbo::unicode::is_unicode_engine<Engine>>>
    [[nodiscard]] inline size_t
    convert_utf16le_to_utf32(const char16_t *input, size_t length, char32_t *utf32_buffer) noexcept {
        return turbo::unicode::Converter<Engine>::convert_utf16le_to_utf32(input, length, utf32_buffer);
    }

    template<typename Engine, turbo::check_requires<turbo::unicode::is_unicode_engine<Engine>>>
    [[nodiscard]] inline size_t
    convert_utf16be_to_utf32(const char16_t *input, size_t length, char32_t *utf32_buffer) noexcept {
        return turbo::unicode::Converter<Engine>::convert_utf16be_to_utf32(input, length, utf32_buffer);
    }

    template<typename Engine, turbo::check_requires<turbo::unicode::is_unicode_engine<Engine>>>
    [[nodiscard]] inline result
    convert_utf16_to_utf32_with_errors(const char16_t *input, size_t length, char32_t *utf32_buffer) noexcept {
        if constexpr (kIsLittleEndian) {
            return turbo::unicode::Converter<Engine>::convert_utf16le_to_utf32_with_errors(input, length, utf32_buffer);
        } else {
            return turbo::unicode::Converter<Engine>::convert_utf16be_to_utf32_with_errors(input, length, utf32_buffer);
        }
    }

    template<typename Engine, turbo::check_requires<turbo::unicode::is_unicode_engine<Engine>>>
    [[nodiscard]] inline result
    convert_utf16le_to_utf32_with_errors(const char16_t *input, size_t length, char32_t *utf32_buffer) noexcept {
        return turbo::unicode::Converter<Engine>::convert_utf16le_to_utf32_with_errors(input, length, utf32_buffer);
    }

    template<typename Engine, turbo::check_requires<turbo::unicode::is_unicode_engine<Engine>>>
    [[nodiscard]] inline result
    convert_utf16be_to_utf32_with_errors(const char16_t *input, size_t length, char32_t *utf32_buffer) noexcept {
        return turbo::unicode::Converter<Engine>::convert_utf16be_to_utf32_with_errors(input, length, utf32_buffer);
    }

    template<typename Engine, turbo::check_requires<turbo::unicode::is_unicode_engine<Engine>>>
    [[nodiscard]] inline size_t
    convert_valid_utf16_to_utf32(const char16_t *input, size_t length, char32_t *utf32_buffer) noexcept {
        if constexpr (kIsLittleEndian) {
            return turbo::unicode::Converter<Engine>::convert_valid_utf16le_to_utf32(input, length, utf32_buffer);
        } else {
            return turbo::unicode::Converter<Engine>::convert_valid_utf16be_to_utf32(input, length, utf32_buffer);
        }
    }

    template<typename Engine, turbo::check_requires<turbo::unicode::is_unicode_engine<Engine>>>
    [[nodiscard]] inline size_t
    convert_valid_utf16le_to_utf32(const char16_t *input, size_t length, char32_t *utf32_buffer) noexcept {
        return turbo::unicode::Converter<Engine>::convert_valid_utf16le_to_utf32(input, length, utf32_buffer);
    }

    template<typename Engine, turbo::check_requires<turbo::unicode::is_unicode_engine<Engine>>>
    [[nodiscard]] inline size_t
    convert_valid_utf16be_to_utf32(const char16_t *input, size_t length, char32_t *utf32_buffer) noexcept {
        return turbo::unicode::Converter<Engine>::convert_valid_utf16be_to_utf32(input, length, utf32_buffer);
    }

    template<typename Engine, turbo::check_requires<turbo::unicode::is_unicode_engine<Engine>>>
    [[nodiscard]] inline size_t utf8_length_from_utf16(const char16_t *input, size_t length) noexcept {
        if constexpr (kIsLittleEndian) {
            return turbo::unicode::Converter<Engine>::utf8_length_from_utf16le(input, length);
        } else {
            return turbo::unicode::Converter<Engine>::Utf8LengthFromUtf16Be(input, length);
        }
    }

    template<typename Engine, turbo::check_requires<turbo::unicode::is_unicode_engine<Engine>>>
    [[nodiscard]] inline size_t utf8_length_from_utf16le(const char16_t *input, size_t length) noexcept {
        return turbo::unicode::Converter<Engine>::utf8_length_from_utf16le(input, length);
    }

    template<typename Engine, turbo::check_requires<turbo::unicode::is_unicode_engine<Engine>>>
    [[nodiscard]] inline size_t utf8_length_from_utf16be(const char16_t *input, size_t length) noexcept {
        return turbo::unicode::Converter<Engine>::Utf8LengthFromUtf16Be(input, length);
    }

    template<typename Engine, turbo::check_requires<turbo::unicode::is_unicode_engine<Engine>>>
    [[nodiscard]] inline size_t convert_utf32_to_utf8(const char32_t *input, size_t length, char *utf8_buffer) noexcept {
        return turbo::unicode::Converter<Engine>::convert_utf32_to_utf8(input, length, utf8_buffer);
    }

    template<typename Engine, turbo::check_requires<turbo::unicode::is_unicode_engine<Engine>>>
    [[nodiscard]] inline result
    convert_utf32_to_utf8_with_errors(const char32_t *input, size_t length, char *utf8_buffer) noexcept {
        return turbo::unicode::Converter<Engine>::convert_utf32_to_utf8_with_errors(input, length, utf8_buffer);
    }

    template<typename Engine, turbo::check_requires<turbo::unicode::is_unicode_engine<Engine>>>
    [[nodiscard]] inline size_t
    convert_valid_utf32_to_utf8(const char32_t *input, size_t length, char *utf8_buffer) noexcept {
        return turbo::unicode::Converter<Engine>::convert_valid_utf32_to_utf8(input, length, utf8_buffer);
    }

    template<typename Engine, turbo::check_requires<turbo::unicode::is_unicode_engine<Engine>>>
    [[nodiscard]] inline size_t
    convert_utf32_to_utf16(const char32_t *input, size_t length, char16_t *utf16_buffer) noexcept {
        if constexpr (kIsLittleEndian) {
            return turbo::unicode::Converter<Engine>::convert_utf32_to_utf16le(input, length, utf16_buffer);
        } else {
            return turbo::unicode::Converter<Engine>::convert_utf32_to_utf16be(input, length, utf16_buffer);
        }
    }

    template<typename Engine, turbo::check_requires<turbo::unicode::is_unicode_engine<Engine>>>
    [[nodiscard]] inline size_t
    convert_utf32_to_utf16le(const char32_t *input, size_t length, char16_t *utf16_buffer) noexcept {
        return turbo::unicode::Converter<Engine>::convert_utf32_to_utf16le(input, length, utf16_buffer);
    }

    template<typename Engine, turbo::check_requires<turbo::unicode::is_unicode_engine<Engine>>>
    [[nodiscard]] inline size_t
    convert_utf32_to_utf16be(const char32_t *input, size_t length, char16_t *utf16_buffer) noexcept {
        return turbo::unicode::Converter<Engine>::convert_utf32_to_utf16be(input, length, utf16_buffer);
    }

    template<typename Engine, turbo::check_requires<turbo::unicode::is_unicode_engine<Engine>>>
    [[nodiscard]] inline result
    convert_utf32_to_utf16_with_errors(const char32_t *input, size_t length, char16_t *utf16_buffer) noexcept {
        if constexpr (kIsLittleEndian) {
            return turbo::unicode::Converter<Engine>::ConvertUtf32ToUtf16LeWithErrors(input, length, utf16_buffer);
        } else {
            return turbo::unicode::Converter<Engine>::convert_utf32_to_utf16be_with_errors(input, length, utf16_buffer);
        }
    }

    template<typename Engine, turbo::check_requires<turbo::unicode::is_unicode_engine<Engine>>>
    [[nodiscard]] inline result
    convert_utf32_to_utf16le_with_errors(const char32_t *input, size_t length, char16_t *utf16_buffer) noexcept {
        return turbo::unicode::Converter<Engine>::ConvertUtf32ToUtf16LeWithErrors(input, length, utf16_buffer);
    }

    template<typename Engine, turbo::check_requires<turbo::unicode::is_unicode_engine<Engine>>>
    [[nodiscard]] result
    convert_utf32_to_utf16be_with_errors(const char32_t *input, size_t length, char16_t *utf16_buffer) noexcept {
        return turbo::unicode::Converter<Engine>::convert_utf32_to_utf16be_with_errors(input, length, utf16_buffer);
    }

    template<typename Engine, turbo::check_requires<turbo::unicode::is_unicode_engine<Engine>>>
    [[nodiscard]] inline size_t
    convert_valid_utf32_to_utf16(const char32_t *input, size_t length, char16_t *utf16_buffer) noexcept {
        if constexpr (kIsLittleEndian) {
            return turbo::unicode::Converter<Engine>::convert_valid_utf32_to_utf16le(input, length, utf16_buffer);
        } else {
            return turbo::unicode::Converter<Engine>::convert_valid_utf32_to_utf16be(input, length, utf16_buffer);
        }
    }

    template<typename Engine, turbo::check_requires<turbo::unicode::is_unicode_engine<Engine>>>
    [[nodiscard]] inline size_t
    convert_valid_utf32_to_utf16le(const char32_t *input, size_t length, char16_t *utf16_buffer) noexcept {
        return turbo::unicode::Converter<Engine>::convert_valid_utf32_to_utf16le(input, length, utf16_buffer);
    }

    template<typename Engine, turbo::check_requires<turbo::unicode::is_unicode_engine<Engine>>>
    [[nodiscard]] inline size_t
    convert_valid_utf32_to_utf16be(const char32_t *input, size_t length, char16_t *utf16_buffer) noexcept {
        return turbo::unicode::Converter<Engine>::convert_valid_utf32_to_utf16be(input, length, utf16_buffer);
    }

    template<typename Engine, turbo::check_requires<turbo::unicode::is_unicode_engine<Engine>>>
    inline void change_endianness_utf16(const char16_t *input, size_t length, char16_t *output) noexcept {
        turbo::unicode::Converter<Engine>::change_endianness_utf16(input, length, output);
    }

    template<typename Engine, turbo::check_requires<turbo::unicode::is_unicode_engine<Engine>>>
    [[nodiscard]] inline size_t utf8_length_from_utf32(const char32_t *input, size_t length) noexcept {
        return turbo::unicode::Converter<Engine>::utf8_length_from_utf32(input, length);
    }

    template<typename Engine, turbo::check_requires<turbo::unicode::is_unicode_engine<Engine>>>
    [[nodiscard]] inline size_t utf16_length_from_utf32(const char32_t *input, size_t length) noexcept {
        return turbo::unicode::Converter<Engine>::utf16_length_from_utf32(input, length);
    }

    template<typename Engine, turbo::check_requires<turbo::unicode::is_unicode_engine<Engine>>>
    [[nodiscard]] inline size_t utf32_length_from_utf16(const char16_t *input, size_t length) noexcept {
        if constexpr (kIsLittleEndian) {
            return turbo::unicode::Converter<Engine>::utf32_length_from_utf16le(input, length);
        } else {
            return turbo::unicode::Converter<Engine>::utf32_length_from_utf16be(input, length);
        }
    }

    template<typename Engine, turbo::check_requires<turbo::unicode::is_unicode_engine<Engine>>>
    [[nodiscard]] inline size_t utf32_length_from_utf16le(const char16_t *input, size_t length) noexcept {
        return turbo::unicode::Converter<Engine>::utf32_length_from_utf16le(input, length);
    }

    template<typename Engine, turbo::check_requires<turbo::unicode::is_unicode_engine<Engine>>>
    [[nodiscard]] inline size_t utf32_length_from_utf16be(const char16_t *input, size_t length) noexcept {
        return turbo::unicode::Converter<Engine>::utf32_length_from_utf16be(input, length);
    }

    template<typename Engine, turbo::check_requires<turbo::unicode::is_unicode_engine<Engine>>>
    [[nodiscard]] inline size_t count_utf16(const char16_t *input, size_t length) noexcept {
        if constexpr (kIsLittleEndian) {
            return turbo::unicode::Converter<Engine>::count_utf16le(input, length);
        } else {
            return turbo::unicode::Converter<Engine>::count_utf16be(input, length);
        }
    }

    template<typename Engine, turbo::check_requires<turbo::unicode::is_unicode_engine<Engine>>>
    [[nodiscard]] inline size_t count_utf16le(const char16_t *input, size_t length) noexcept {
        return turbo::unicode::Converter<Engine>::count_utf16le(input, length);
    }

    template<typename Engine, turbo::check_requires<turbo::unicode::is_unicode_engine<Engine>>>
    [[nodiscard]] inline size_t count_utf16be(const char16_t *input, size_t length) noexcept {
        return turbo::unicode::Converter<Engine>::count_utf16be(input, length);
    }

    template<typename Engine, turbo::check_requires<turbo::unicode::is_unicode_engine<Engine>>>
    [[nodiscard]] inline size_t count_utf8(const char *input, size_t length) noexcept {
        return turbo::unicode::Converter<Engine>::count_utf8(input, length);
    }
}  // namespace turbo

#endif  // TURBO_UNICODE_CONVERTER_H_
