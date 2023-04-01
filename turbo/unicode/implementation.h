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

#ifndef TURBO_UNICODE_IMPLEMENTATION_H_
#define TURBO_UNICODE_IMPLEMENTATION_H_
#include <string>
#include <atomic>
#include <vector>
#include <tuple>
#include "turbo/unicode/internal/config.h"
#include "turbo/unicode/internal/isadetection.h"


namespace turbo {

/**
 * Autodetect the encoding of the input, a single encoding is recommended.
 * E.g., the function might return turbo::EncodingType::UTF8,
 * turbo::EncodingType::UTF16_LE, turbo::EncodingType::UTF16_BE, or
 * turbo::EncodingType::UTF32_LE.
 *
 * @param input the string to analyze.
 * @param length the length of the string in bytes.
 * @return the detected encoding type
 */
TURBO_MUST_USE_RESULT turbo::EncodingType AutodetectEncoding(const char * input, size_t length) noexcept;
TURBO_FORCE_INLINE TURBO_MUST_USE_RESULT turbo::EncodingType AutodetectEncoding(const uint8_t * input, size_t length) noexcept {
  return AutodetectEncoding(reinterpret_cast<const char *>(input), length);
}

/**
 * Autodetect the possible encodings of the input in one pass.
 * E.g., if the input might be UTF-16LE or UTF-8, this function returns
 * the value (turbo::EncodingType::UTF8 | turbo::EncodingType::UTF16_LE).
 *
 * Overriden by each implementation.
 *
 * @param input the string to analyze.
 * @param length the length of the string in bytes.
 * @return the detected encoding type
 */
TURBO_MUST_USE_RESULT int DetectEncodings(const char * input, size_t length) noexcept;
TURBO_FORCE_INLINE TURBO_MUST_USE_RESULT int DetectEncodings(const uint8_t * input, size_t length) noexcept {
  return DetectEncodings(reinterpret_cast<const char *>(input), length);
}


/**
 * Validate the UTF-8 string. This function may be best when you expect
 * the input to be almost always valid. Otherwise, consider using
 * ValidateUtf8WithErrors.
 *
 * Overridden by each implementation.
 *
 * @param buf the UTF-8 string to validate.
 * @param len the length of the string in bytes.
 * @return true if and only if the string is valid UTF-8.
 */
TURBO_MUST_USE_RESULT bool ValidateUtf8(const char *buf, size_t len) noexcept;

/**
 * Validate the UTF-8 string and stop on error.
 *
 * Overridden by each implementation.
 *
 * @param buf the UTF-8 string to validate.
 * @param len the length of the string in bytes.
 * @return a result pair struct with an error code and either the position of the error if any or the number of words validated if successful.
 */
TURBO_MUST_USE_RESULT result ValidateUtf8WithErrors(const char *buf, size_t len) noexcept;

/**
 * Validate the ASCII string.
 *
 * Overridden by each implementation.
 *
 * @param buf the ASCII string to validate.
 * @param len the length of the string in bytes.
 * @return true if and only if the string is valid ASCII.
 */
TURBO_MUST_USE_RESULT bool ValidateAscii(const char *buf, size_t len) noexcept;

/**
 * Validate the ASCII string and stop on error. It might be faster than
 * ValidateUtf8 when an error is expected to occur early.
 *
 * Overridden by each implementation.
 *
 * @param buf the ASCII string to validate.
 * @param len the length of the string in bytes.
 * @return a result pair struct with an error code and either the position of the error if any or the number of words validated if successful.
 */
TURBO_MUST_USE_RESULT result ValidateAsciiWithErrors(const char *buf, size_t len) noexcept;

/**
 * Using native endianness; Validate the UTF-16 string.
 * This function may be best when you expect the input to be almost always valid.
 * Otherwise, consider using ValidateUtf16WithErrors.
 *
 * Overridden by each implementation.
 *
 * This function is not BOM-aware.
 *
 * @param buf the UTF-16 string to validate.
 * @param len the length of the string in number of 2-byte words (char16_t).
 * @return true if and only if the string is valid UTF-16.
 */
TURBO_MUST_USE_RESULT bool ValidateUtf16(const char16_t *buf, size_t len) noexcept;

/**
 * Validate the UTF-16LE string. This function may be best when you expect
 * the input to be almost always valid. Otherwise, consider using
 * ValidateUtf16LeWithErrors.
 *
 * Overridden by each implementation.
 *
 * This function is not BOM-aware.
 *
 * @param buf the UTF-16LE string to validate.
 * @param len the length of the string in number of 2-byte words (char16_t).
 * @return true if and only if the string is valid UTF-16LE.
 */
TURBO_MUST_USE_RESULT bool ValidateUtf16Le(const char16_t *buf, size_t len) noexcept;

/**
 * Validate the UTF-16BE string. This function may be best when you expect
 * the input to be almost always valid. Otherwise, consider using
 * ValidateUtf16BeWithErrors.
 *
 * Overridden by each implementation.
 *
 * This function is not BOM-aware.
 *
 * @param buf the UTF-16BE string to validate.
 * @param len the length of the string in number of 2-byte words (char16_t).
 * @return true if and only if the string is valid UTF-16BE.
 */
TURBO_MUST_USE_RESULT bool ValidateUtf16Be(const char16_t *buf, size_t len) noexcept;

/**
 * Using native endianness; Validate the UTF-16 string and stop on error.
 * It might be faster than ValidateUtf16 when an error is expected to occur early.
 *
 * Overridden by each implementation.
 *
 * This function is not BOM-aware.
 *
 * @param buf the UTF-16 string to validate.
 * @param len the length of the string in number of 2-byte words (char16_t).
 * @return a result pair struct with an error code and either the position of the error if any or the number of words validated if successful.
 */
TURBO_MUST_USE_RESULT result ValidateUtf16WithErrors(const char16_t *buf, size_t len) noexcept;

/**
 * Validate the UTF-16LE string and stop on error. It might be faster than
 * ValidateUtf16Le when an error is expected to occur early.
 *
 * Overridden by each implementation.
 *
 * This function is not BOM-aware.
 *
 * @param buf the UTF-16LE string to validate.
 * @param len the length of the string in number of 2-byte words (char16_t).
 * @return a result pair struct with an error code and either the position of the error if any or the number of words validated if successful.
 */
TURBO_MUST_USE_RESULT result ValidateUtf16LeWithErrors(const char16_t *buf, size_t len) noexcept;

/**
 * Validate the UTF-16BE string and stop on error. It might be faster than
 * ValidateUtf16Be when an error is expected to occur early.
 *
 * Overridden by each implementation.
 *
 * This function is not BOM-aware.
 *
 * @param buf the UTF-16BE string to validate.
 * @param len the length of the string in number of 2-byte words (char16_t).
 * @return a result pair struct with an error code and either the position of the error if any or the number of words validated if successful.
 */
TURBO_MUST_USE_RESULT result ValidateUtf16BeWithErrors(const char16_t *buf, size_t len) noexcept;

/**
 * Validate the UTF-32 string. This function may be best when you expect
 * the input to be almost always valid. Otherwise, consider using
 * ValidateUtf32WithErrors.
 *
 * Overridden by each implementation.
 *
 * This function is not BOM-aware.
 *
 * @param buf the UTF-32 string to validate.
 * @param len the length of the string in number of 4-byte words (char32_t).
 * @return true if and only if the string is valid UTF-32.
 */
TURBO_MUST_USE_RESULT bool ValidateUtf32(const char32_t *buf, size_t len) noexcept;

/**
 * Validate the UTF-32 string and stop on error. It might be faster than
 * ValidateUtf32 when an error is expected to occur early.
 *
 * Overridden by each implementation.
 *
 * This function is not BOM-aware.
 *
 * @param buf the UTF-32 string to validate.
 * @param len the length of the string in number of 4-byte words (char32_t).
 * @return a result pair struct with an error code and either the position of the error if any or the number of words validated if successful.
 */
TURBO_MUST_USE_RESULT result ValidateUtf32WithErrors(const char32_t *buf, size_t len) noexcept;

/**
 * Using native endianness; Convert possibly broken UTF-8 string into UTF-16 string.
 *
 * During the conversion also validation of the input string is done.
 * This function is suitable to work with inputs from untrusted sources.
 *
 * @param input         the UTF-8 string to convert
 * @param length        the length of the string in bytes
 * @param utf16_buffer  the pointer to buffer that can hold conversion result
 * @return the number of written char16_t; 0 if the input was not valid UTF-8 string
 */
TURBO_MUST_USE_RESULT size_t ConvertUtf8ToUtf16(const char * input, size_t length, char16_t* utf16_output) noexcept;

/**
 * Convert possibly broken UTF-8 string into UTF-16LE string.
 *
 * During the conversion also validation of the input string is done.
 * This function is suitable to work with inputs from untrusted sources.
 *
 * @param input         the UTF-8 string to convert
 * @param length        the length of the string in bytes
 * @param utf16_buffer  the pointer to buffer that can hold conversion result
 * @return the number of written char16_t; 0 if the input was not valid UTF-8 string
 */
TURBO_MUST_USE_RESULT size_t ConvertUtf8ToUtf16Le(const char * input, size_t length, char16_t* utf16_output) noexcept;

/**
 * Convert possibly broken UTF-8 string into UTF-16BE string.
 *
 * During the conversion also validation of the input string is done.
 * This function is suitable to work with inputs from untrusted sources.
 *
 * @param input         the UTF-8 string to convert
 * @param length        the length of the string in bytes
 * @param utf16_buffer  the pointer to buffer that can hold conversion result
 * @return the number of written char16_t; 0 if the input was not valid UTF-8 string
 */
TURBO_MUST_USE_RESULT size_t ConvertUtf8ToUtf16Be(const char * input, size_t length, char16_t* utf16_output) noexcept;

/**
 * Using native endianness; Convert possibly broken UTF-8 string into UTF-16
 * string and stop on error.
 *
 * During the conversion also validation of the input string is done.
 * This function is suitable to work with inputs from untrusted sources.
 *
 * @param input         the UTF-8 string to convert
 * @param length        the length of the string in bytes
 * @param utf16_buffer  the pointer to buffer that can hold conversion result
 * @return a result pair struct with an error code and either the position of the error if any or the number of char16_t written if successful.
 */
TURBO_MUST_USE_RESULT result ConvertUtf8ToUtf16WithErrors(const char * input, size_t length, char16_t* utf16_output) noexcept;

/**
 * Convert possibly broken UTF-8 string into UTF-16LE string and stop on error.
 *
 * During the conversion also validation of the input string is done.
 * This function is suitable to work with inputs from untrusted sources.
 *
 * @param input         the UTF-8 string to convert
 * @param length        the length of the string in bytes
 * @param utf16_buffer  the pointer to buffer that can hold conversion result
 * @return a result pair struct with an error code and either the position of the error if any or the number of char16_t written if successful.
 */
TURBO_MUST_USE_RESULT result ConvertUtf8ToUtf16LeWithErrors(const char * input, size_t length, char16_t* utf16_output) noexcept;

/**
 * Convert possibly broken UTF-8 string into UTF-16BE string and stop on error.
 *
 * During the conversion also validation of the input string is done.
 * This function is suitable to work with inputs from untrusted sources.
 *
 * @param input         the UTF-8 string to convert
 * @param length        the length of the string in bytes
 * @param utf16_buffer  the pointer to buffer that can hold conversion result
 * @return a result pair struct with an error code and either the position of the error if any or the number of char16_t written if successful.
 */
TURBO_MUST_USE_RESULT result ConvertUtf8ToUtf16BeWithErrors(const char * input, size_t length, char16_t* utf16_output) noexcept;

/**
 * Convert possibly broken UTF-8 string into UTF-32 string.
 *
 * During the conversion also validation of the input string is done.
 * This function is suitable to work with inputs from untrusted sources.
 *
 * @param input         the UTF-8 string to convert
 * @param length        the length of the string in bytes
 * @param utf32_buffer  the pointer to buffer that can hold conversion result
 * @return the number of written char32_t; 0 if the input was not valid UTF-8 string
 */
TURBO_MUST_USE_RESULT size_t ConvertUtf8ToUtf32(const char * input, size_t length, char32_t* utf32_output) noexcept;

/**
 * Convert possibly broken UTF-8 string into UTF-32 string and stop on error.
 *
 * During the conversion also validation of the input string is done.
 * This function is suitable to work with inputs from untrusted sources.
 *
 * @param input         the UTF-8 string to convert
 * @param length        the length of the string in bytes
 * @param utf32_buffer  the pointer to buffer that can hold conversion result
 * @return a result pair struct with an error code and either the position of the error if any or the number of char32_t written if successful.
 */
TURBO_MUST_USE_RESULT result ConvertUtf8ToUtf32WithErrors(const char * input, size_t length, char32_t* utf32_output) noexcept;

/**
 * Using native endianness; Convert valid UTF-8 string into UTF-16 string.
 *
 * This function assumes that the input string is valid UTF-8.
 *
 * @param input         the UTF-8 string to convert
 * @param length        the length of the string in bytes
 * @param utf16_buffer  the pointer to buffer that can hold conversion result
 * @return the number of written char16_t
 */
TURBO_MUST_USE_RESULT size_t ConvertValidUtf8ToUtf16(const char * input, size_t length, char16_t* utf16_buffer) noexcept;

/**
 * Convert valid UTF-8 string into UTF-16LE string.
 *
 * This function assumes that the input string is valid UTF-8.
 *
 * @param input         the UTF-8 string to convert
 * @param length        the length of the string in bytes
 * @param utf16_buffer  the pointer to buffer that can hold conversion result
 * @return the number of written char16_t
 */
TURBO_MUST_USE_RESULT size_t ConvertValidUtf8ToUtf16Le(const char * input, size_t length, char16_t* utf16_buffer) noexcept;

/**
 * Convert valid UTF-8 string into UTF-16BE string.
 *
 * This function assumes that the input string is valid UTF-8.
 *
 * @param input         the UTF-8 string to convert
 * @param length        the length of the string in bytes
 * @param utf16_buffer  the pointer to buffer that can hold conversion result
 * @return the number of written char16_t
 */
TURBO_MUST_USE_RESULT size_t ConvertValidUtf8ToUtf16Be(const char * input, size_t length, char16_t* utf16_buffer) noexcept;

/**
 * Convert valid UTF-8 string into UTF-32 string.
 *
 * This function assumes that the input string is valid UTF-8.
 *
 * @param input         the UTF-8 string to convert
 * @param length        the length of the string in bytes
 * @param utf32_buffer  the pointer to buffer that can hold conversion result
 * @return the number of written char32_t
 */
TURBO_MUST_USE_RESULT size_t ConvertValidUtf8ToUtf32(const char * input, size_t length, char32_t* utf32_buffer) noexcept;

/**
 * Compute the number of 2-byte words that this UTF-8 string would require in UTF-16LE format.
 *
 * This function does not validate the input.
 *
 * This function is not BOM-aware.
 *
 * @param input         the UTF-8 string to process
 * @param length        the length of the string in bytes
 * @return the number of char16_t words required to encode the UTF-8 string as UTF-16LE
 */
TURBO_MUST_USE_RESULT size_t Utf16LengthFromUtf8(const char * input, size_t length) noexcept;

/**
 * Compute the number of 4-byte words that this UTF-8 string would require in UTF-32 format.
 *
 * This function is equivalent to CountUtf8
 *
 * This function does not validate the input.
 *
 * This function is not BOM-aware.
 *
 * @param input         the UTF-8 string to process
 * @param length        the length of the string in bytes
 * @return the number of char32_t words required to encode the UTF-8 string as UTF-32
 */
TURBO_MUST_USE_RESULT size_t Utf32LengthFromUtf8(const char * input, size_t length) noexcept;

/**
 * Using native endianness; Convert possibly broken UTF-16 string into UTF-8 string.
 *
 * During the conversion also validation of the input string is done.
 * This function is suitable to work with inputs from untrusted sources.
 *
 * This function is not BOM-aware.
 *
 * @param input         the UTF-16 string to convert
 * @param length        the length of the string in 2-byte words (char16_t)
 * @param utf8_buffer   the pointer to buffer that can hold conversion result
 * @return number of written words; 0 if input is not a valid UTF-16LE string
 */
TURBO_MUST_USE_RESULT size_t ConvertUtf16ToUtf8(const char16_t * input, size_t length, char* utf8_buffer) noexcept;

/**
 * Convert possibly broken UTF-16LE string into UTF-8 string.
 *
 * During the conversion also validation of the input string is done.
 * This function is suitable to work with inputs from untrusted sources.
 *
 * This function is not BOM-aware.
 *
 * @param input         the UTF-16LE string to convert
 * @param length        the length of the string in 2-byte words (char16_t)
 * @param utf8_buffer   the pointer to buffer that can hold conversion result
 * @return number of written words; 0 if input is not a valid UTF-16LE string
 */
TURBO_MUST_USE_RESULT size_t ConvertUtf16LeToUtf8(const char16_t * input, size_t length, char* utf8_buffer) noexcept;

/**
 * Convert possibly broken UTF-16BE string into UTF-8 string.
 *
 * During the conversion also validation of the input string is done.
 * This function is suitable to work with inputs from untrusted sources.
 *
 * This function is not BOM-aware.
 *
 * @param input         the UTF-16BE string to convert
 * @param length        the length of the string in 2-byte words (char16_t)
 * @param utf8_buffer   the pointer to buffer that can hold conversion result
 * @return number of written words; 0 if input is not a valid UTF-16LE string
 */
TURBO_MUST_USE_RESULT size_t ConvertUtf16BeToUtf8(const char16_t * input, size_t length, char* utf8_buffer) noexcept;

/**
 * Using native endianness; Convert possibly broken UTF-16 string into UTF-8 string and stop on error.
 *
 * During the conversion also validation of the input string is done.
 * This function is suitable to work with inputs from untrusted sources.
 *
 * This function is not BOM-aware.
 *
 * @param input         the UTF-16 string to convert
 * @param length        the length of the string in 2-byte words (char16_t)
 * @param utf8_buffer   the pointer to buffer that can hold conversion result
 * @return a result pair struct with an error code and either the position of the error if any or the number of char written if successful.
 */
TURBO_MUST_USE_RESULT result ConvertUtf16ToUtf8WithErrors(const char16_t * input, size_t length, char* utf8_buffer) noexcept;

/**
 * Convert possibly broken UTF-16LE string into UTF-8 string and stop on error.
 *
 * During the conversion also validation of the input string is done.
 * This function is suitable to work with inputs from untrusted sources.
 *
 * This function is not BOM-aware.
 *
 * @param input         the UTF-16LE string to convert
 * @param length        the length of the string in 2-byte words (char16_t)
 * @param utf8_buffer   the pointer to buffer that can hold conversion result
 * @return a result pair struct with an error code and either the position of the error if any or the number of char written if successful.
 */
TURBO_MUST_USE_RESULT result ConvertUtf16LeToUtf8WithErrors(const char16_t * input, size_t length, char* utf8_buffer) noexcept;

/**
 * Convert possibly broken UTF-16BE string into UTF-8 string and stop on error.
 *
 * During the conversion also validation of the input string is done.
 * This function is suitable to work with inputs from untrusted sources.
 *
 * This function is not BOM-aware.
 *
 * @param input         the UTF-16BE string to convert
 * @param length        the length of the string in 2-byte words (char16_t)
 * @param utf8_buffer   the pointer to buffer that can hold conversion result
 * @return a result pair struct with an error code and either the position of the error if any or the number of char written if successful.
 */
TURBO_MUST_USE_RESULT result ConvertUtf16BeToUtf8WithErrors(const char16_t * input, size_t length, char* utf8_buffer) noexcept;

/**
 * Using native endianness; Convert valid UTF-16 string into UTF-8 string.
 *
 * This function assumes that the input string is valid UTF-16LE.
 *
 * This function is not BOM-aware.
 *
 * @param input         the UTF-16 string to convert
 * @param length        the length of the string in 2-byte words (char16_t)
 * @param utf8_buffer   the pointer to buffer that can hold the conversion result
 * @return number of written words; 0 if conversion is not possible
 */
TURBO_MUST_USE_RESULT size_t ConvertValidUtf16ToUtf8(const char16_t * input, size_t length, char* utf8_buffer) noexcept;

/**
 * Convert valid UTF-16LE string into UTF-8 string.
 *
 * This function assumes that the input string is valid UTF-16LE.
 *
 * This function is not BOM-aware.
 *
 * @param input         the UTF-16LE string to convert
 * @param length        the length of the string in 2-byte words (char16_t)
 * @param utf8_buffer   the pointer to buffer that can hold the conversion result
 * @return number of written words; 0 if conversion is not possible
 */
TURBO_MUST_USE_RESULT size_t ConvertValidUtf16LeToUtf8(const char16_t * input, size_t length, char* utf8_buffer) noexcept;

/**
 * Convert valid UTF-16BE string into UTF-8 string.
 *
 * This function assumes that the input string is valid UTF-16BE.
 *
 * This function is not BOM-aware.
 *
 * @param input         the UTF-16BE string to convert
 * @param length        the length of the string in 2-byte words (char16_t)
 * @param utf8_buffer   the pointer to buffer that can hold the conversion result
 * @return number of written words; 0 if conversion is not possible
 */
TURBO_MUST_USE_RESULT size_t ConvertValidUtf16BeToUtf8(const char16_t * input, size_t length, char* utf8_buffer) noexcept;

/**
 * Using native endianness; Convert possibly broken UTF-16 string into UTF-32 string.
 *
 * During the conversion also validation of the input string is done.
 * This function is suitable to work with inputs from untrusted sources.
 *
 * This function is not BOM-aware.
 *
 * @param input         the UTF-16 string to convert
 * @param length        the length of the string in 2-byte words (char16_t)
 * @param utf32_buffer   the pointer to buffer that can hold conversion result
 * @return number of written words; 0 if input is not a valid UTF-16LE string
 */
TURBO_MUST_USE_RESULT size_t ConvertUtf16ToUtf32(const char16_t * input, size_t length, char32_t* utf32_buffer) noexcept;

/**
 * Convert possibly broken UTF-16LE string into UTF-32 string.
 *
 * During the conversion also validation of the input string is done.
 * This function is suitable to work with inputs from untrusted sources.
 *
 * This function is not BOM-aware.
 *
 * @param input         the UTF-16LE string to convert
 * @param length        the length of the string in 2-byte words (char16_t)
 * @param utf32_buffer   the pointer to buffer that can hold conversion result
 * @return number of written words; 0 if input is not a valid UTF-16LE string
 */
TURBO_MUST_USE_RESULT size_t ConvertUtf16LeToUtf32(const char16_t * input, size_t length, char32_t* utf32_buffer) noexcept;

/**
 * Convert possibly broken UTF-16BE string into UTF-32 string.
 *
 * During the conversion also validation of the input string is done.
 * This function is suitable to work with inputs from untrusted sources.
 *
 * This function is not BOM-aware.
 *
 * @param input         the UTF-16BE string to convert
 * @param length        the length of the string in 2-byte words (char16_t)
 * @param utf32_buffer   the pointer to buffer that can hold conversion result
 * @return number of written words; 0 if input is not a valid UTF-16LE string
 */
TURBO_MUST_USE_RESULT size_t ConvertUtf16BeToUtf32(const char16_t * input, size_t length, char32_t* utf32_buffer) noexcept;

/**
 * Using native endianness; Convert possibly broken UTF-16 string into
 * UTF-32 string and stop on error.
 *
 * During the conversion also validation of the input string is done.
 * This function is suitable to work with inputs from untrusted sources.
 *
 * This function is not BOM-aware.
 *
 * @param input         the UTF-16 string to convert
 * @param length        the length of the string in 2-byte words (char16_t)
 * @param utf32_buffer   the pointer to buffer that can hold conversion result
 * @return a result pair struct with an error code and either the position of the error if any or the number of char32_t written if successful.
 */
TURBO_MUST_USE_RESULT result ConvertUtf16ToUtf32WithErrors(const char16_t * input, size_t length, char32_t* utf32_buffer) noexcept;

/**
 * Convert possibly broken UTF-16LE string into UTF-32 string and stop on error.
 *
 * During the conversion also validation of the input string is done.
 * This function is suitable to work with inputs from untrusted sources.
 *
 * This function is not BOM-aware.
 *
 * @param input         the UTF-16LE string to convert
 * @param length        the length of the string in 2-byte words (char16_t)
 * @param utf32_buffer   the pointer to buffer that can hold conversion result
 * @return a result pair struct with an error code and either the position of the error if any or the number of char32_t written if successful.
 */
TURBO_MUST_USE_RESULT result ConvertUtf16LeToUtf32WithErrors(const char16_t * input, size_t length, char32_t* utf32_buffer) noexcept;

/**
 * Convert possibly broken UTF-16BE string into UTF-32 string and stop on error.
 *
 * During the conversion also validation of the input string is done.
 * This function is suitable to work with inputs from untrusted sources.
 *
 * This function is not BOM-aware.
 *
 * @param input         the UTF-16BE string to convert
 * @param length        the length of the string in 2-byte words (char16_t)
 * @param utf32_buffer   the pointer to buffer that can hold conversion result
 * @return a result pair struct with an error code and either the position of the error if any or the number of char32_t written if successful.
 */
TURBO_MUST_USE_RESULT result ConvertUtf16BeToUtf32WithErrors(const char16_t * input, size_t length, char32_t* utf32_buffer) noexcept;

/**
 * Using native endianness; Convert valid UTF-16 string into UTF-32 string.
 *
 * This function assumes that the input string is valid UTF-16 (native endianness).
 *
 * This function is not BOM-aware.
 *
 * @param input         the UTF-16 string to convert
 * @param length        the length of the string in 2-byte words (char16_t)
 * @param utf32_buffer   the pointer to buffer that can hold the conversion result
 * @return number of written words; 0 if conversion is not possible
 */
TURBO_MUST_USE_RESULT size_t ConvertValidUtf16ToUtf32(const char16_t * input, size_t length, char32_t* utf32_buffer) noexcept;

/**
 * Convert valid UTF-16LE string into UTF-32 string.
 *
 * This function assumes that the input string is valid UTF-16LE.
 *
 * This function is not BOM-aware.
 *
 * @param input         the UTF-16LE string to convert
 * @param length        the length of the string in 2-byte words (char16_t)
 * @param utf32_buffer   the pointer to buffer that can hold the conversion result
 * @return number of written words; 0 if conversion is not possible
 */
TURBO_MUST_USE_RESULT size_t ConvertValidUtf16LeToUtf32(const char16_t * input, size_t length, char32_t* utf32_buffer) noexcept;

/**
 * Convert valid UTF-16BE string into UTF-32 string.
 *
 * This function assumes that the input string is valid UTF-16LE.
 *
 * This function is not BOM-aware.
 *
 * @param input         the UTF-16BE string to convert
 * @param length        the length of the string in 2-byte words (char16_t)
 * @param utf32_buffer   the pointer to buffer that can hold the conversion result
 * @return number of written words; 0 if conversion is not possible
 */
TURBO_MUST_USE_RESULT size_t ConvertValidUtf16BeToUtf32(const char16_t * input, size_t length, char32_t* utf32_buffer) noexcept;

/**
 * Using native endianness; Compute the number of bytes that this UTF-16
 * string would require in UTF-8 format.
 *
 * This function does not validate the input.
 *
 * @param input         the UTF-16 string to convert
 * @param length        the length of the string in 2-byte words (char16_t)
 * @return the number of bytes required to encode the UTF-16LE string as UTF-8
 */
TURBO_MUST_USE_RESULT size_t Utf8LengthFromUtf16(const char16_t * input, size_t length) noexcept;

/**
 * Compute the number of bytes that this UTF-16LE string would require in UTF-8 format.
 *
 * This function does not validate the input.
 *
 * @param input         the UTF-16LE string to convert
 * @param length        the length of the string in 2-byte words (char16_t)
 * @return the number of bytes required to encode the UTF-16LE string as UTF-8
 */
TURBO_MUST_USE_RESULT size_t Utf8LengthFromUtf16Le(const char16_t * input, size_t length) noexcept;

/**
 * Compute the number of bytes that this UTF-16BE string would require in UTF-8 format.
 *
 * This function does not validate the input.
 *
 * @param input         the UTF-16BE string to convert
 * @param length        the length of the string in 2-byte words (char16_t)
 * @return the number of bytes required to encode the UTF-16BE string as UTF-8
 */
TURBO_MUST_USE_RESULT size_t Utf8LengthFromUtf16be(const char16_t * input, size_t length) noexcept;

/**
 * Convert possibly broken UTF-32 string into UTF-8 string.
 *
 * During the conversion also validation of the input string is done.
 * This function is suitable to work with inputs from untrusted sources.
 *
 * This function is not BOM-aware.
 *
 * @param input         the UTF-32 string to convert
 * @param length        the length of the string in 4-byte words (char32_t)
 * @param utf8_buffer   the pointer to buffer that can hold conversion result
 * @return number of written words; 0 if input is not a valid UTF-32 string
 */
TURBO_MUST_USE_RESULT size_t ConvertUtf32ToUtf8(const char32_t * input, size_t length, char* utf8_buffer) noexcept;

/**
 * Convert possibly broken UTF-32 string into UTF-8 string and stop on error.
 *
 * During the conversion also validation of the input string is done.
 * This function is suitable to work with inputs from untrusted sources.
 *
 * This function is not BOM-aware.
 *
 * @param input         the UTF-32 string to convert
 * @param length        the length of the string in 4-byte words (char32_t)
 * @param utf8_buffer   the pointer to buffer that can hold conversion result
 * @return a result pair struct with an error code and either the position of the error if any or the number of char written if successful.
 */
TURBO_MUST_USE_RESULT result ConvertUtf32ToUtf8WithErrors(const char32_t * input, size_t length, char* utf8_buffer) noexcept;

/**
 * Convert valid UTF-32 string into UTF-8 string.
 *
 * This function assumes that the input string is valid UTF-32.
 *
 * This function is not BOM-aware.
 *
 * @param input         the UTF-32 string to convert
 * @param length        the length of the string in 4-byte words (char32_t)
 * @param utf8_buffer   the pointer to buffer that can hold the conversion result
 * @return number of written words; 0 if conversion is not possible
 */
TURBO_MUST_USE_RESULT size_t ConvertValidUtf32ToUtf8(const char32_t * input, size_t length, char* utf8_buffer) noexcept;

/**
 * Using native endianness; Convert possibly broken UTF-32 string into UTF-16 string.
 *
 * During the conversion also validation of the input string is done.
 * This function is suitable to work with inputs from untrusted sources.
 *
 * This function is not BOM-aware.
 *
 * @param input         the UTF-32 string to convert
 * @param length        the length of the string in 4-byte words (char32_t)
 * @param utf16_buffer   the pointer to buffer that can hold conversion result
 * @return number of written words; 0 if input is not a valid UTF-32 string
 */
TURBO_MUST_USE_RESULT size_t ConvertUtf32ToUtf16(const char32_t * input, size_t length, char16_t* utf16_buffer) noexcept;

/**
 * Convert possibly broken UTF-32 string into UTF-16LE string.
 *
 * During the conversion also validation of the input string is done.
 * This function is suitable to work with inputs from untrusted sources.
 *
 * This function is not BOM-aware.
 *
 * @param input         the UTF-32 string to convert
 * @param length        the length of the string in 4-byte words (char32_t)
 * @param utf16_buffer   the pointer to buffer that can hold conversion result
 * @return number of written words; 0 if input is not a valid UTF-32 string
 */
TURBO_MUST_USE_RESULT size_t ConvertUtf32ToUtf16Le(const char32_t * input, size_t length, char16_t* utf16_buffer) noexcept;

/**
 * Convert possibly broken UTF-32 string into UTF-16BE string.
 *
 * During the conversion also validation of the input string is done.
 * This function is suitable to work with inputs from untrusted sources.
 *
 * This function is not BOM-aware.
 *
 * @param input         the UTF-32 string to convert
 * @param length        the length of the string in 4-byte words (char32_t)
 * @param utf16_buffer   the pointer to buffer that can hold conversion result
 * @return number of written words; 0 if input is not a valid UTF-32 string
 */
TURBO_MUST_USE_RESULT size_t ConvertUtf32ToUtf16Be(const char32_t * input, size_t length, char16_t* utf16_buffer) noexcept;

/**
 * Using native endianness; Convert possibly broken UTF-32 string into UTF-16
 * string and stop on error.
 *
 * During the conversion also validation of the input string is done.
 * This function is suitable to work with inputs from untrusted sources.
 *
 * This function is not BOM-aware.
 *
 * @param input         the UTF-32 string to convert
 * @param length        the length of the string in 4-byte words (char32_t)
 * @param utf16_buffer   the pointer to buffer that can hold conversion result
 * @return a result pair struct with an error code and either the position of the error if any or the number of char16_t written if successful.
 */
TURBO_MUST_USE_RESULT result ConvertUtf32ToUtf16WithErrors(const char32_t * input, size_t length, char16_t* utf16_buffer) noexcept;

/**
 * Convert possibly broken UTF-32 string into UTF-16LE string and stop on error.
 *
 * During the conversion also validation of the input string is done.
 * This function is suitable to work with inputs from untrusted sources.
 *
 * This function is not BOM-aware.
 *
 * @param input         the UTF-32 string to convert
 * @param length        the length of the string in 4-byte words (char32_t)
 * @param utf16_buffer   the pointer to buffer that can hold conversion result
 * @return a result pair struct with an error code and either the position of the error if any or the number of char16_t written if successful.
 */
TURBO_MUST_USE_RESULT result ConvertUtf32ToUtf16leWithErrors(const char32_t * input, size_t length, char16_t* utf16_buffer) noexcept;

/**
 * Convert possibly broken UTF-32 string into UTF-16BE string and stop on error.
 *
 * During the conversion also validation of the input string is done.
 * This function is suitable to work with inputs from untrusted sources.
 *
 * This function is not BOM-aware.
 *
 * @param input         the UTF-32 string to convert
 * @param length        the length of the string in 4-byte words (char32_t)
 * @param utf16_buffer   the pointer to buffer that can hold conversion result
 * @return a result pair struct with an error code and either the position of the error if any or the number of char16_t written if successful.
 */
TURBO_MUST_USE_RESULT result ConvertUtf32ToUtf16BeWithErrors(const char32_t * input, size_t length, char16_t* utf16_buffer) noexcept;

/**
 * Using native endianness; Convert valid UTF-32 string into UTF-16 string.
 *
 * This function assumes that the input string is valid UTF-32.
 *
 * This function is not BOM-aware.
 *
 * @param input         the UTF-32 string to convert
 * @param length        the length of the string in 4-byte words (char32_t)
 * @param utf16_buffer   the pointer to buffer that can hold the conversion result
 * @return number of written words; 0 if conversion is not possible
 */
TURBO_MUST_USE_RESULT size_t ConvertValidUtf32ToUtf16(const char32_t * input, size_t length, char16_t* utf16_buffer) noexcept;

/**
 * Convert valid UTF-32 string into UTF-16LE string.
 *
 * This function assumes that the input string is valid UTF-32.
 *
 * This function is not BOM-aware.
 *
 * @param input         the UTF-32 string to convert
 * @param length        the length of the string in 4-byte words (char32_t)
 * @param utf16_buffer   the pointer to buffer that can hold the conversion result
 * @return number of written words; 0 if conversion is not possible
 */
TURBO_MUST_USE_RESULT size_t ConvertValidUtf32ToUtf16Le(const char32_t * input, size_t length, char16_t* utf16_buffer) noexcept;

/**
 * Convert valid UTF-32 string into UTF-16BE string.
 *
 * This function assumes that the input string is valid UTF-32.
 *
 * This function is not BOM-aware.
 *
 * @param input         the UTF-32 string to convert
 * @param length        the length of the string in 4-byte words (char32_t)
 * @param utf16_buffer   the pointer to buffer that can hold the conversion result
 * @return number of written words; 0 if conversion is not possible
 */
TURBO_MUST_USE_RESULT size_t ConvertValidUtf32ToUtf16Be(const char32_t * input, size_t length, char16_t* utf16_buffer) noexcept;

/**
 * Change the endianness of the input. Can be used to go from UTF-16LE to UTF-16BE or
 * from UTF-16BE to UTF-16LE.
 *
 * This function does not validate the input.
 *
 * This function is not BOM-aware.
 *
 * @param input         the UTF-16 string to process
 * @param length        the length of the string in 2-byte words (char16_t)
 * @param output        the pointer to buffer that can hold the conversion result
 */
void ChangeEndiannessUtf16(const char16_t * input, size_t length, char16_t * output) noexcept;

/**
 * Compute the number of bytes that this UTF-32 string would require in UTF-8 format.
 *
 * This function does not validate the input.
 *
 * @param input         the UTF-32 string to convert
 * @param length        the length of the string in 4-byte words (char32_t)
 * @return the number of bytes required to encode the UTF-32 string as UTF-8
 */
TURBO_MUST_USE_RESULT size_t Utf8LengthFromUtf32(const char32_t * input, size_t length) noexcept;

/**
 * Compute the number of two-byte words that this UTF-32 string would require in UTF-16 format.
 *
 * This function does not validate the input.
 *
 * @param input         the UTF-32 string to convert
 * @param length        the length of the string in 4-byte words (char32_t)
 * @return the number of bytes required to encode the UTF-32 string as UTF-16
 */
TURBO_MUST_USE_RESULT size_t Utf16LengthFromUtf32(const char32_t * input, size_t length) noexcept;

/**
 * Using native endianness; Compute the number of bytes that this UTF-16
 * string would require in UTF-32 format.
 *
 * This function is equivalent to CountUtf16.
 *
 * This function does not validate the input.
 *
 * This function is not BOM-aware.
 *
 * @param input         the UTF-16 string to convert
 * @param length        the length of the string in 2-byte words (char16_t)
 * @return the number of bytes required to encode the UTF-16LE string as UTF-32
 */
TURBO_MUST_USE_RESULT size_t Utf32LengthFromUtf16(const char16_t * input, size_t length) noexcept;

/**
 * Compute the number of bytes that this UTF-16LE string would require in UTF-32 format.
 *
 * This function is equivalent to CountUtf16Le.
 *
 * This function does not validate the input.
 *
 * This function is not BOM-aware.
 *
 * @param input         the UTF-16LE string to convert
 * @param length        the length of the string in 2-byte words (char16_t)
 * @return the number of bytes required to encode the UTF-16LE string as UTF-32
 */
TURBO_MUST_USE_RESULT size_t Utf32LengthFromUtf16Le(const char16_t * input, size_t length) noexcept;

/**
 * Compute the number of bytes that this UTF-16BE string would require in UTF-32 format.
 *
 * This function is equivalent to CountUtf16Be.
 *
 * This function does not validate the input.
 *
 * This function is not BOM-aware.
 *
 * @param input         the UTF-16BE string to convert
 * @param length        the length of the string in 2-byte words (char16_t)
 * @return the number of bytes required to encode the UTF-16BE string as UTF-32
 */
TURBO_MUST_USE_RESULT size_t Utf32LengthFromUtf16Be(const char16_t * input, size_t length) noexcept;

/**
 * Count the number of code points (characters) in the string assuming that
 * it is valid.
 *
 * This function assumes that the input string is valid UTF-16 (native endianness).
 *
 * This function is not BOM-aware.
 *
 * @param input         the UTF-16 string to process
 * @param length        the length of the string in 2-byte words (char16_t)
 * @return number of code points
 */
TURBO_MUST_USE_RESULT size_t CountUtf16(const char16_t * input, size_t length) noexcept;

/**
 * Count the number of code points (characters) in the string assuming that
 * it is valid.
 *
 * This function assumes that the input string is valid UTF-16LE.
 *
 * This function is not BOM-aware.
 *
 * @param input         the UTF-16LE string to process
 * @param length        the length of the string in 2-byte words (char16_t)
 * @return number of code points
 */
TURBO_MUST_USE_RESULT size_t CountUtf16Le(const char16_t * input, size_t length) noexcept;

/**
 * Count the number of code points (characters) in the string assuming that
 * it is valid.
 *
 * This function assumes that the input string is valid UTF-16BE.
 *
 * This function is not BOM-aware.
 *
 * @param input         the UTF-16BE string to process
 * @param length        the length of the string in 2-byte words (char16_t)
 * @return number of code points
 */
TURBO_MUST_USE_RESULT size_t CountUtf16Be(const char16_t * input, size_t length) noexcept;

/**
 * Count the number of code points (characters) in the string assuming that
 * it is valid.
 *
 * This function assumes that the input string is valid UTF-8.
 *
 * @param input         the UTF-8 string to process
 * @param length        the length of the string in bytes
 * @return number of code points
 */
TURBO_MUST_USE_RESULT size_t CountUtf8(const char * input, size_t length) noexcept;

/**
 * An implementation of unicode for a particular CPU architecture.
 *
 * Also used to maintain the currently active implementation. The active implementation is
 * automatically initialized on first use to the most advanced implementation supported by the host.
 */
class Implementation {
public:

  /**
   * The name of this implementation.
   *
   *     const implementation *impl = turbo::active_implementation;
   *     cout << "unicode is optimized for " << impl->name() << "(" << impl->description() << ")" << endl;
   *
   * @return the name of the implementation, e.g. "haswell", "westmere", "arm64"
   */
  virtual const std::string &Name() const { return _name; }

  /**
   * The description of this implementation.
   *
   *     const implementation *impl = turbo::active_implementation;
   *     cout << "unicode is optimized for " << impl->name() << "(" << impl->description() << ")" << endl;
   *
   * @return the name of the implementation, e.g. "haswell", "westmere", "arm64"
   */
  virtual const std::string &Description() const { return _description; }

  /**
   * The instruction sets this implementation is compiled against
   * and the current CPU match. This function may poll the current CPU/system
   * and should therefore not be called too often if performance is a concern.
   *
   *
   * @return true if the implementation can be safely used on the current system (determined at runtime)
   */
  bool supported_by_runtime_system() const;

  /**
   * This function will try to detect the encoding
   * @param input the string to identify
   * @param length the length of the string in bytes.
   * @return the encoding type detected
   */
  virtual EncodingType AutodetectEncoding(const char * input, size_t length) const noexcept;

  /**
   * This function will try to detect the possible encodings in one pass
   * @param input the string to identify
   * @param length the length of the string in bytes.
   * @return the encoding type detected
   */
  virtual int DetectEncodings(const char * input, size_t length) const noexcept = 0;

  /**
   * @private For internal implementation use
   *
   * The instruction sets this implementation is compiled against.
   *
   * @return a mask of all required `internal::instruction_set::` values
   */
  virtual uint32_t RequiredInstructionSets() const { return _required_instruction_sets; }


  /**
   * Validate the UTF-8 string.
   *
   * Overridden by each implementation.
   *
   * @param buf the UTF-8 string to validate.
   * @param len the length of the string in bytes.
   * @return true if and only if the string is valid UTF-8.
   */
  TURBO_MUST_USE_RESULT virtual bool ValidateUtf8(const char *buf, size_t len) const noexcept = 0;

  /**
   * Validate the UTF-8 string and stop on errors.
   *
   * Overridden by each implementation.
   *
   * @param buf the UTF-8 string to validate.
   * @param len the length of the string in bytes.
   * @return a result pair struct with an error code and either the position of the error if any or the number of words validated if successful.
   */
  TURBO_MUST_USE_RESULT virtual result ValidateUtf8WithErrors(const char *buf, size_t len) const noexcept = 0;

  /**
   * Validate the ASCII string.
   *
   * Overridden by each implementation.
   *
   * @param buf the ASCII string to validate.
   * @param len the length of the string in bytes.
   * @return true if and only if the string is valid ASCII.
   */
  TURBO_MUST_USE_RESULT virtual bool ValidateAscii(const char *buf, size_t len) const noexcept = 0;

  /**
   * Validate the ASCII string and stop on error.
   *
   * Overridden by each implementation.
   *
   * @param buf the ASCII string to validate.
   * @param len the length of the string in bytes.
   * @return a result pair struct with an error code and either the position of the error if any or the number of words validated if successful.
   */
  TURBO_MUST_USE_RESULT virtual result ValidateAsciiWithErrors(const char *buf, size_t len) const noexcept = 0;

  /**
   * Validate the UTF-16LE string.This function may be best when you expect
   * the input to be almost always valid. Otherwise, consider using
   * ValidateUtf16LeWithErrors.
   *
   * Overridden by each implementation.
   *
   * This function is not BOM-aware.
   *
   * @param buf the UTF-16LE string to validate.
   * @param len the length of the string in number of 2-byte words (char16_t).
   * @return true if and only if the string is valid UTF-16LE.
   */
  TURBO_MUST_USE_RESULT virtual bool ValidateUtf16Le(const char16_t *buf, size_t len) const noexcept = 0;

  /**
   * Validate the UTF-16BE string. This function may be best when you expect
   * the input to be almost always valid. Otherwise, consider using
   * ValidateUtf16BeWithErrors.
   *
   * Overridden by each implementation.
   *
   * This function is not BOM-aware.
   *
   * @param buf the UTF-16BE string to validate.
   * @param len the length of the string in number of 2-byte words (char16_t).
   * @return true if and only if the string is valid UTF-16BE.
   */
  TURBO_MUST_USE_RESULT virtual bool ValidateUtf16Be(const char16_t *buf, size_t len) const noexcept = 0;

  /**
   * Validate the UTF-16LE string and stop on error.  It might be faster than
 * ValidateUtf16Le when an error is expected to occur early.
   *
   * Overridden by each implementation.
   *
   * This function is not BOM-aware.
   *
   * @param buf the UTF-16LE string to validate.
   * @param len the length of the string in number of 2-byte words (char16_t).
   * @return a result pair struct with an error code and either the position of the error if any or the number of words validated if successful.
   */
  TURBO_MUST_USE_RESULT virtual result ValidateUtf16LeWithErrors(const char16_t *buf, size_t len) const noexcept = 0;

  /**
   * Validate the UTF-16BE string and stop on error. It might be faster than
   * ValidateUtf16Be when an error is expected to occur early.
   *
   * Overridden by each implementation.
   *
   * This function is not BOM-aware.
   *
   * @param buf the UTF-16BE string to validate.
   * @param len the length of the string in number of 2-byte words (char16_t).
   * @return a result pair struct with an error code and either the position of the error if any or the number of words validated if successful.
   */
  TURBO_MUST_USE_RESULT virtual result ValidateUtf16BeWithErrors(const char16_t *buf, size_t len) const noexcept = 0;

  /**
   * Validate the UTF-32 string.
   *
   * Overridden by each implementation.
   *
   * This function is not BOM-aware.
   *
   * @param buf the UTF-32 string to validate.
   * @param len the length of the string in number of 4-byte words (char32_t).
   * @return true if and only if the string is valid UTF-32.
   */
  TURBO_MUST_USE_RESULT virtual bool ValidateUtf32(const char32_t *buf, size_t len) const noexcept = 0;

  /**
   * Validate the UTF-32 string and stop on error.
   *
   * Overridden by each implementation.
   *
   * This function is not BOM-aware.
   *
   * @param buf the UTF-32 string to validate.
   * @param len the length of the string in number of 4-byte words (char32_t).
   * @return a result pair struct with an error code and either the position of the error if any or the number of words validated if successful.
   */
  TURBO_MUST_USE_RESULT virtual result ValidateUtf32WithErrors(const char32_t *buf, size_t len) const noexcept = 0;

  /**
   * Convert possibly broken UTF-8 string into UTF-16LE string.
   *
   * During the conversion also validation of the input string is done.
   * This function is suitable to work with inputs from untrusted sources.
   *
   * @param input         the UTF-8 string to convert
   * @param length        the length of the string in bytes
   * @param utf16_buffer  the pointer to buffer that can hold conversion result
   * @return the number of written char16_t; 0 if the input was not valid UTF-8 string
   */
  TURBO_MUST_USE_RESULT virtual size_t ConvertUtf8ToUtf16Le(const char * input, size_t length, char16_t* utf16_output) const noexcept = 0;

  /**
   * Convert possibly broken UTF-8 string into UTF-16BE string.
   *
   * During the conversion also validation of the input string is done.
   * This function is suitable to work with inputs from untrusted sources.
   *
   * @param input         the UTF-8 string to convert
   * @param length        the length of the string in bytes
   * @param utf16_buffer  the pointer to buffer that can hold conversion result
   * @return the number of written char16_t; 0 if the input was not valid UTF-8 string
   */
  TURBO_MUST_USE_RESULT virtual size_t ConvertUtf8ToUtf16Be(const char * input, size_t length, char16_t* utf16_output) const noexcept = 0;

  /**
   * Convert possibly broken UTF-8 string into UTF-16LE string and stop on error.
   *
   * During the conversion also validation of the input string is done.
   * This function is suitable to work with inputs from untrusted sources.
   *
   * @param input         the UTF-8 string to convert
   * @param length        the length of the string in bytes
   * @param utf16_buffer  the pointer to buffer that can hold conversion result
   * @return a result pair struct with an error code and either the position of the error if any or the number of words validated if successful.
   */
  TURBO_MUST_USE_RESULT virtual result ConvertUtf8ToUtf16LeWithErrors(const char * input, size_t length, char16_t* utf16_output) const noexcept = 0;

  /**
   * Convert possibly broken UTF-8 string into UTF-16BE string and stop on error.
   *
   * During the conversion also validation of the input string is done.
   * This function is suitable to work with inputs from untrusted sources.
   *
   * @param input         the UTF-8 string to convert
   * @param length        the length of the string in bytes
   * @param utf16_buffer  the pointer to buffer that can hold conversion result
   * @return a result pair struct with an error code and either the position of the error if any or the number of words validated if successful.
   */
  TURBO_MUST_USE_RESULT virtual result ConvertUtf8ToUtf16BeWithErrors(const char * input, size_t length, char16_t* utf16_output) const noexcept = 0;

  /**
   * Convert possibly broken UTF-8 string into UTF-32 string.
   *
   * During the conversion also validation of the input string is done.
   * This function is suitable to work with inputs from untrusted sources.
   *
   * @param input         the UTF-8 string to convert
   * @param length        the length of the string in bytes
   * @param utf32_buffer  the pointer to buffer that can hold conversion result
   * @return the number of written char16_t; 0 if the input was not valid UTF-8 string
   */
  TURBO_MUST_USE_RESULT virtual size_t ConvertUtf8ToUtf32(const char * input, size_t length, char32_t* utf32_output) const noexcept = 0;

  /**
   * Convert possibly broken UTF-8 string into UTF-32 string and stop on error.
   *
   * During the conversion also validation of the input string is done.
   * This function is suitable to work with inputs from untrusted sources.
   *
   * @param input         the UTF-8 string to convert
   * @param length        the length of the string in bytes
   * @param utf32_buffer  the pointer to buffer that can hold conversion result
   * @return a result pair struct with an error code and either the position of the error if any or the number of char32_t written if successful.
   */
  TURBO_MUST_USE_RESULT virtual result ConvertUtf8ToUtf32WithErrors(const char * input, size_t length, char32_t* utf32_output) const noexcept = 0;

  /**
   * Convert valid UTF-8 string into UTF-16LE string.
   *
   * This function assumes that the input string is valid UTF-8.
   *
   * @param input         the UTF-8 string to convert
   * @param length        the length of the string in bytes
   * @param utf16_buffer  the pointer to buffer that can hold conversion result
   * @return the number of written char16_t
   */
  TURBO_MUST_USE_RESULT virtual size_t ConvertValidUtf8ToUtf16Le(const char * input, size_t length, char16_t* utf16_buffer) const noexcept = 0;

/**
   * Convert valid UTF-8 string into UTF-16BE string.
   *
   * This function assumes that the input string is valid UTF-8.
   *
   * @param input         the UTF-8 string to convert
   * @param length        the length of the string in bytes
   * @param utf16_buffer  the pointer to buffer that can hold conversion result
   * @return the number of written char16_t
   */
  TURBO_MUST_USE_RESULT virtual size_t ConvertValidUtf8ToUtf16Be(const char * input, size_t length, char16_t* utf16_buffer) const noexcept = 0;

  /**
   * Convert valid UTF-8 string into UTF-32 string.
   *
   * This function assumes that the input string is valid UTF-8.
   *
   * @param input         the UTF-8 string to convert
   * @param length        the length of the string in bytes
   * @param utf16_buffer  the pointer to buffer that can hold conversion result
   * @return the number of written char32_t
   */
  TURBO_MUST_USE_RESULT virtual size_t ConvertValidUtf8ToUtf32(const char * input, size_t length, char32_t* utf32_buffer) const noexcept = 0;

  /**
   * Compute the number of 2-byte words that this UTF-8 string would require in UTF-16LE format.
   *
   * This function does not validate the input.
   *
   * @param input         the UTF-8 string to process
   * @param length        the length of the string in bytes
   * @return the number of char16_t words required to encode the UTF-8 string as UTF-16LE
   */
  TURBO_MUST_USE_RESULT virtual size_t Utf16LengthFromUtf8(const char * input, size_t length) const noexcept = 0;

   /**
   * Compute the number of 4-byte words that this UTF-8 string would require in UTF-32 format.
   *
   * This function is equivalent to CountUtf8.
   *
   * This function does not validate the input.
   *
   * @param input         the UTF-8 string to process
   * @param length        the length of the string in bytes
   * @return the number of char32_t words required to encode the UTF-8 string as UTF-32
   */
  TURBO_MUST_USE_RESULT virtual size_t Utf32LengthFromUtf8(const char * input, size_t length) const noexcept = 0;

  /**
   * Convert possibly broken UTF-16LE string into UTF-8 string.
   *
   * During the conversion also validation of the input string is done.
   * This function is suitable to work with inputs from untrusted sources.
   *
   * This function is not BOM-aware.
   *
   * @param input         the UTF-16LE string to convert
   * @param length        the length of the string in 2-byte words (char16_t)
   * @param utf8_buffer   the pointer to buffer that can hold conversion result
   * @return number of written words; 0 if input is not a valid UTF-16LE string
   */
  TURBO_MUST_USE_RESULT virtual size_t ConvertUtf16LeToUtf8(const char16_t * input, size_t length, char* utf8_buffer) const noexcept = 0;

  /**
   * Convert possibly broken UTF-16BE string into UTF-8 string.
   *
   * During the conversion also validation of the input string is done.
   * This function is suitable to work with inputs from untrusted sources.
   *
   * This function is not BOM-aware.
   *
   * @param input         the UTF-16BE string to convert
   * @param length        the length of the string in 2-byte words (char16_t)
   * @param utf8_buffer   the pointer to buffer that can hold conversion result
   * @return number of written words; 0 if input is not a valid UTF-16BE string
   */
  TURBO_MUST_USE_RESULT virtual size_t ConvertUtf16BeToUtf8(const char16_t * input, size_t length, char* utf8_buffer) const noexcept = 0;

  /**
   * Convert possibly broken UTF-16LE string into UTF-8 string and stop on error.
   *
   * During the conversion also validation of the input string is done.
   * This function is suitable to work with inputs from untrusted sources.
   *
   * This function is not BOM-aware.
   *
   * @param input         the UTF-16LE string to convert
   * @param length        the length of the string in 2-byte words (char16_t)
   * @param utf8_buffer   the pointer to buffer that can hold conversion result
   * @return a result pair struct with an error code and either the position of the error if any or the number of char written if successful.
   */
  TURBO_MUST_USE_RESULT virtual result ConvertUtf16LeToUtf8WithErrors(const char16_t * input, size_t length, char* utf8_buffer) const noexcept = 0;

  /**
   * Convert possibly broken UTF-16BE string into UTF-8 string and stop on error.
   *
   * During the conversion also validation of the input string is done.
   * This function is suitable to work with inputs from untrusted sources.
   *
   * This function is not BOM-aware.
   *
   * @param input         the UTF-16BE string to convert
   * @param length        the length of the string in 2-byte words (char16_t)
   * @param utf8_buffer   the pointer to buffer that can hold conversion result
   * @return a result pair struct with an error code and either the position of the error if any or the number of char written if successful.
   */
  TURBO_MUST_USE_RESULT virtual result ConvertUtf16BeToUtf8WithErrors(const char16_t * input, size_t length, char* utf8_buffer) const noexcept = 0;

  /**
   * Convert valid UTF-16LE string into UTF-8 string.
   *
   * This function assumes that the input string is valid UTF-16LE.
   *
   * This function is not BOM-aware.
   *
   * @param input         the UTF-16LE string to convert
   * @param length        the length of the string in 2-byte words (char16_t)
   * @param utf8_buffer   the pointer to buffer that can hold the conversion result
   * @return number of written words; 0 if conversion is not possible
   */
  TURBO_MUST_USE_RESULT virtual size_t ConvertValidUtf16LeToUtf8(const char16_t * input, size_t length, char* utf8_buffer) const noexcept = 0;

  /**
   * Convert valid UTF-16BE string into UTF-8 string.
   *
   * This function assumes that the input string is valid UTF-16BE.
   *
   * This function is not BOM-aware.
   *
   * @param input         the UTF-16BE string to convert
   * @param length        the length of the string in 2-byte words (char16_t)
   * @param utf8_buffer   the pointer to buffer that can hold the conversion result
   * @return number of written words; 0 if conversion is not possible
   */
  TURBO_MUST_USE_RESULT virtual size_t ConvertValidUtf16BeToUtf8(const char16_t * input, size_t length, char* utf8_buffer) const noexcept = 0;

  /**
   * Convert possibly broken UTF-16LE string into UTF-32 string.
   *
   * During the conversion also validation of the input string is done.
   * This function is suitable to work with inputs from untrusted sources.
   *
   * This function is not BOM-aware.
   *
   * @param input         the UTF-16LE string to convert
   * @param length        the length of the string in 2-byte words (char16_t)
   * @param utf32_buffer   the pointer to buffer that can hold conversion result
   * @return number of written words; 0 if input is not a valid UTF-16LE string
   */
  TURBO_MUST_USE_RESULT virtual size_t ConvertUtf16LeToUtf32(const char16_t * input, size_t length, char32_t* utf32_buffer) const noexcept = 0;

  /**
   * Convert possibly broken UTF-16BE string into UTF-32 string.
   *
   * During the conversion also validation of the input string is done.
   * This function is suitable to work with inputs from untrusted sources.
   *
   * This function is not BOM-aware.
   *
   * @param input         the UTF-16BE string to convert
   * @param length        the length of the string in 2-byte words (char16_t)
   * @param utf32_buffer   the pointer to buffer that can hold conversion result
   * @return number of written words; 0 if input is not a valid UTF-16BE string
   */
  TURBO_MUST_USE_RESULT virtual size_t ConvertUtf16BeToUtf32(const char16_t * input, size_t length, char32_t* utf32_buffer) const noexcept = 0;

  /**
   * Convert possibly broken UTF-16LE string into UTF-32 string and stop on error.
   *
   * During the conversion also validation of the input string is done.
   * This function is suitable to work with inputs from untrusted sources.
   *
   * This function is not BOM-aware.
   *
   * @param input         the UTF-16LE string to convert
   * @param length        the length of the string in 2-byte words (char16_t)
   * @param utf32_buffer   the pointer to buffer that can hold conversion result
   * @return a result pair struct with an error code and either the position of the error if any or the number of char32_t written if successful.
   */
  TURBO_MUST_USE_RESULT virtual result ConvertUtf16LeToUtf32WithErrors(const char16_t * input, size_t length, char32_t* utf32_buffer) const noexcept = 0;

  /**
   * Convert possibly broken UTF-16BE string into UTF-32 string and stop on error.
   *
   * During the conversion also validation of the input string is done.
   * This function is suitable to work with inputs from untrusted sources.
   *
   * This function is not BOM-aware.
   *
   * @param input         the UTF-16BE string to convert
   * @param length        the length of the string in 2-byte words (char16_t)
   * @param utf32_buffer   the pointer to buffer that can hold conversion result
   * @return a result pair struct with an error code and either the position of the error if any or the number of char32_t written if successful.
   */
  TURBO_MUST_USE_RESULT virtual result ConvertUtf16BeToUtf32WithErrors(const char16_t * input, size_t length, char32_t* utf32_buffer) const noexcept = 0;

  /**
   * Convert valid UTF-16LE string into UTF-32 string.
   *
   * This function assumes that the input string is valid UTF-16LE.
   *
   * This function is not BOM-aware.
   *
   * @param input         the UTF-16LE string to convert
   * @param length        the length of the string in 2-byte words (char16_t)
   * @param utf32_buffer   the pointer to buffer that can hold the conversion result
   * @return number of written words; 0 if conversion is not possible
   */
  TURBO_MUST_USE_RESULT virtual size_t ConvertValidUtf16LeToUtf32(const char16_t * input, size_t length, char32_t* utf32_buffer) const noexcept = 0;

  /**
   * Convert valid UTF-16LE string into UTF-32BE string.
   *
   * This function assumes that the input string is valid UTF-16BE.
   *
   * This function is not BOM-aware.
   *
   * @param input         the UTF-16BE string to convert
   * @param length        the length of the string in 2-byte words (char16_t)
   * @param utf32_buffer   the pointer to buffer that can hold the conversion result
   * @return number of written words; 0 if conversion is not possible
   */
  TURBO_MUST_USE_RESULT virtual size_t ConvertValidUtf16BeToUtf32(const char16_t * input, size_t length, char32_t* utf32_buffer) const noexcept = 0;

  /**
   * Compute the number of bytes that this UTF-16LE string would require in UTF-8 format.
   *
   * This function does not validate the input.
   *
   * This function is not BOM-aware.
   *
   * @param input         the UTF-16LE string to convert
   * @param length        the length of the string in 2-byte words (char16_t)
   * @return the number of bytes required to encode the UTF-16LE string as UTF-8
   */
  TURBO_MUST_USE_RESULT virtual size_t Utf8LengthFromUtf16Le(const char16_t * input, size_t length) const noexcept = 0;

  /**
   * Compute the number of bytes that this UTF-16BE string would require in UTF-8 format.
   *
   * This function does not validate the input.
   *
   * This function is not BOM-aware.
   *
   * @param input         the UTF-16BE string to convert
   * @param length        the length of the string in 2-byte words (char16_t)
   * @return the number of bytes required to encode the UTF-16BE string as UTF-8
   */
  TURBO_MUST_USE_RESULT virtual size_t Utf8LengthFromUtf16be(const char16_t * input, size_t length) const noexcept = 0;

  /**
   * Convert possibly broken UTF-32 string into UTF-8 string.
   *
   * During the conversion also validation of the input string is done.
   * This function is suitable to work with inputs from untrusted sources.
   *
   * This function is not BOM-aware.
   *
   * @param input         the UTF-32 string to convert
   * @param length        the length of the string in 4-byte words (char32_t)
   * @param utf8_buffer   the pointer to buffer that can hold conversion result
   * @return number of written words; 0 if input is not a valid UTF-32 string
   */
  TURBO_MUST_USE_RESULT virtual size_t ConvertUtf32ToUtf8(const char32_t * input, size_t length, char* utf8_buffer) const noexcept = 0;

  /**
   * Convert possibly broken UTF-32 string into UTF-8 string and stop on error.
   *
   * During the conversion also validation of the input string is done.
   * This function is suitable to work with inputs from untrusted sources.
   *
   * This function is not BOM-aware.
   *
   * @param input         the UTF-32 string to convert
   * @param length        the length of the string in 4-byte words (char32_t)
   * @param utf8_buffer   the pointer to buffer that can hold conversion result
   * @return a result pair struct with an error code and either the position of the error if any or the number of char written if successful.
   */
  TURBO_MUST_USE_RESULT virtual result ConvertUtf32ToUtf8WithErrors(const char32_t * input, size_t length, char* utf8_buffer) const noexcept = 0;

  /**
   * Convert valid UTF-32 string into UTF-8 string.
   *
   * This function assumes that the input string is valid UTF-32.
   *
   * This function is not BOM-aware.
   *
   * @param input         the UTF-32 string to convert
   * @param length        the length of the string in 4-byte words (char32_t)
   * @param utf8_buffer   the pointer to buffer that can hold the conversion result
   * @return number of written words; 0 if conversion is not possible
   */
  TURBO_MUST_USE_RESULT virtual size_t ConvertValidUtf32ToUtf8(const char32_t * input, size_t length, char* utf8_buffer) const noexcept = 0;

  /**
   * Convert possibly broken UTF-32 string into UTF-16LE string.
   *
   * During the conversion also validation of the input string is done.
   * This function is suitable to work with inputs from untrusted sources.
   *
   * This function is not BOM-aware.
   *
   * @param input         the UTF-32 string to convert
   * @param length        the length of the string in 4-byte words (char32_t)
   * @param utf16_buffer   the pointer to buffer that can hold conversion result
   * @return number of written words; 0 if input is not a valid UTF-32 string
   */
  TURBO_MUST_USE_RESULT virtual size_t ConvertUtf32ToUtf16Le(const char32_t * input, size_t length, char16_t* utf16_buffer) const noexcept = 0;

  /**
   * Convert possibly broken UTF-32 string into UTF-16BE string.
   *
   * During the conversion also validation of the input string is done.
   * This function is suitable to work with inputs from untrusted sources.
   *
   * This function is not BOM-aware.
   *
   * @param input         the UTF-32 string to convert
   * @param length        the length of the string in 4-byte words (char32_t)
   * @param utf16_buffer   the pointer to buffer that can hold conversion result
   * @return number of written words; 0 if input is not a valid UTF-32 string
   */
  TURBO_MUST_USE_RESULT virtual size_t ConvertUtf32ToUtf16Be(const char32_t * input, size_t length, char16_t* utf16_buffer) const noexcept = 0;

  /**
   * Convert possibly broken UTF-32 string into UTF-16LE string and stop on error.
   *
   * During the conversion also validation of the input string is done.
   * This function is suitable to work with inputs from untrusted sources.
   *
   * This function is not BOM-aware.
   *
   * @param input         the UTF-32 string to convert
   * @param length        the length of the string in 4-byte words (char32_t)
   * @param utf16_buffer   the pointer to buffer that can hold conversion result
   * @return a result pair struct with an error code and either the position of the error if any or the number of char16_t written if successful.
   */
  TURBO_MUST_USE_RESULT virtual result ConvertUtf32ToUtf16leWithErrors(const char32_t * input, size_t length, char16_t* utf16_buffer) const noexcept = 0;

  /**
   * Convert possibly broken UTF-32 string into UTF-16BE string and stop on error.
   *
   * During the conversion also validation of the input string is done.
   * This function is suitable to work with inputs from untrusted sources.
   *
   * This function is not BOM-aware.
   *
   * @param input         the UTF-32 string to convert
   * @param length        the length of the string in 4-byte words (char32_t)
   * @param utf16_buffer   the pointer to buffer that can hold conversion result
   * @return a result pair struct with an error code and either the position of the error if any or the number of char16_t written if successful.
   */
  TURBO_MUST_USE_RESULT virtual result ConvertUtf32ToUtf16BeWithErrors(const char32_t * input, size_t length, char16_t* utf16_buffer) const noexcept = 0;

  /**
   * Convert valid UTF-32 string into UTF-16LE string.
   *
   * This function assumes that the input string is valid UTF-32.
   *
   * This function is not BOM-aware.
   *
   * @param input         the UTF-32 string to convert
   * @param length        the length of the string in 4-byte words (char32_t)
   * @param utf16_buffer   the pointer to buffer that can hold the conversion result
   * @return number of written words; 0 if conversion is not possible
   */
  TURBO_MUST_USE_RESULT virtual size_t ConvertValidUtf32ToUtf16Le(const char32_t * input, size_t length, char16_t* utf16_buffer) const noexcept = 0;

  /**
   * Convert valid UTF-32 string into UTF-16BE string.
   *
   * This function assumes that the input string is valid UTF-32.
   *
   * This function is not BOM-aware.
   *
   * @param input         the UTF-32 string to convert
   * @param length        the length of the string in 4-byte words (char32_t)
   * @param utf16_buffer   the pointer to buffer that can hold the conversion result
   * @return number of written words; 0 if conversion is not possible
   */
  TURBO_MUST_USE_RESULT virtual size_t ConvertValidUtf32ToUtf16Be(const char32_t * input, size_t length, char16_t* utf16_buffer) const noexcept = 0;

  /**
   * Change the endianness of the input. Can be used to go from UTF-16LE to UTF-16BE or
   * from UTF-16BE to UTF-16LE.
   *
   * This function does not validate the input.
   *
   * This function is not BOM-aware.
   *
   * @param input         the UTF-16 string to process
   * @param length        the length of the string in 2-byte words (char16_t)
   * @param output        the pointer to buffer that can hold the conversion result
   */
  virtual void ChangeEndiannessUtf16(const char16_t * input, size_t length, char16_t * output) const noexcept = 0;

  /**
   * Compute the number of bytes that this UTF-32 string would require in UTF-8 format.
   *
   * This function does not validate the input.
   *
   * @param input         the UTF-32 string to convert
   * @param length        the length of the string in 4-byte words (char32_t)
   * @return the number of bytes required to encode the UTF-32 string as UTF-8
   */
  TURBO_MUST_USE_RESULT virtual size_t Utf8LengthFromUtf32(const char32_t * input, size_t length) const noexcept = 0;

  /**
   * Compute the number of two-byte words that this UTF-32 string would require in UTF-16 format.
   *
   * This function does not validate the input.
   *
   * @param input         the UTF-32 string to convert
   * @param length        the length of the string in 4-byte words (char32_t)
   * @return the number of bytes required to encode the UTF-32 string as UTF-16
   */
  TURBO_MUST_USE_RESULT virtual size_t Utf16LengthFromUtf32(const char32_t * input, size_t length) const noexcept = 0;

  /*
   * Compute the number of bytes that this UTF-16LE string would require in UTF-32 format.
   *
   * This function is equivalent to CountUtf16Le.
   *
   * This function does not validate the input.
   *
   * This function is not BOM-aware.
   *
   * @param input         the UTF-16LE string to convert
   * @param length        the length of the string in 2-byte words (char16_t)
   * @return the number of bytes required to encode the UTF-16LE string as UTF-32
   */
  TURBO_MUST_USE_RESULT virtual size_t Utf32LengthFromUtf16Le(const char16_t * input, size_t length) const noexcept = 0;

  /*
   * Compute the number of bytes that this UTF-16BE string would require in UTF-32 format.
   *
   * This function is equivalent to CountUtf16Be.
   *
   * This function does not validate the input.
   *
   * This function is not BOM-aware.
   *
   * @param input         the UTF-16BE string to convert
   * @param length        the length of the string in 2-byte words (char16_t)
   * @return the number of bytes required to encode the UTF-16BE string as UTF-32
   */
  TURBO_MUST_USE_RESULT virtual size_t Utf32LengthFromUtf16Be(const char16_t * input, size_t length) const noexcept = 0;

  /**
   * Count the number of code points (characters) in the string assuming that
   * it is valid.
   *
   * This function assumes that the input string is valid UTF-16LE.
   *
   * This function is not BOM-aware.
   *
   * @param input         the UTF-16LE string to process
   * @param length        the length of the string in 2-byte words (char16_t)
   * @return number of code points
   */
  TURBO_MUST_USE_RESULT virtual size_t CountUtf16Le(const char16_t * input, size_t length) const noexcept = 0;

  /**
   * Count the number of code points (characters) in the string assuming that
   * it is valid.
   *
   * This function assumes that the input string is valid UTF-16BE.
   *
   * This function is not BOM-aware.
   *
   * @param input         the UTF-16BE string to process
   * @param length        the length of the string in 2-byte words (char16_t)
   * @return number of code points
   */
  TURBO_MUST_USE_RESULT virtual size_t CountUtf16Be(const char16_t * input, size_t length) const noexcept = 0;


  /**
   * Count the number of code points (characters) in the string assuming that
   * it is valid.
   *
   * This function assumes that the input string is valid UTF-8.
   *
   * @param input         the UTF-8 string to process
   * @param length        the length of the string in bytes
   * @return number of code points
   */
  TURBO_MUST_USE_RESULT virtual size_t CountUtf8(const char * input, size_t length) const noexcept = 0;



protected:
  /** @private Construct an implementation with the given name and description. For subclasses. */
  TURBO_FORCE_INLINE Implementation(
    std::string name,
    std::string description,
    uint32_t required_instruction_sets
  ) :
    _name(name),
    _description(description),
    _required_instruction_sets(required_instruction_sets)
  {
  }
  virtual ~Implementation()=default;

private:
  /**
   * The name of this implementation.
   */
  const std::string _name;

  /**
   * The description of this implementation.
   */
  const std::string _description;

  /**
   * Instruction sets required for this implementation.
   */
  const uint32_t _required_instruction_sets;
};

/** @private */
namespace internal {

/**
 * The list of available implementations compiled into unicode.
 */
class available_implementation_list {
public:
  /** Get the list of available implementations compiled into unicode */
  TURBO_FORCE_INLINE available_implementation_list() {}
  /** Number of implementations */
  size_t size() const noexcept;
  /** STL const begin() iterator */
  const Implementation * const *begin() const noexcept;
  /** STL const end() iterator */
  const Implementation * const *end() const noexcept;

  /**
   * Get the implementation with the given name.
   *
   * Case sensitive.
   *
   *     const implementation *impl = turbo::available_implementations["westmere"];
   *     if (!impl) { exit(1); }
   *     if (!imp->supported_by_runtime_system()) { exit(1); }
   *     turbo::active_implementation = impl;
   *
   * @param name the implementation to find, e.g. "westmere", "haswell", "arm64"
   * @return the implementation, or nullptr if the parse failed.
   */
  const Implementation * operator[](const std::string &name) const noexcept {
    for (const Implementation * impl : *this) {
      if (impl->Name() == name) { return impl; }
    }
    return nullptr;
  }

  /**
   * Detect the most advanced implementation supported by the current host.
   *
   * This is used to initialize the implementation on startup.
   *
   *     const implementation *impl = turbo::available_implementation::detect_best_supported();
   *     turbo::active_implementation = impl;
   *
   * @return the most advanced supported implementation for the current host, or an
   *         implementation that returns UNSUPPORTED_ARCHITECTURE if there is no supported
   *         implementation. Will never return nullptr.
   */
  const Implementation *detect_best_supported() const noexcept;
};

template<typename T>
class atomic_ptr {
public:
  atomic_ptr(T *_ptr) : ptr{_ptr} {}

  operator const T*() const { return ptr.load(); }
  const T& operator*() const { return *ptr; }
  const T* operator->() const { return ptr.load(); }

  operator T*() { return ptr.load(); }
  T& operator*() { return *ptr; }
  T* operator->() { return ptr.load(); }
  atomic_ptr& operator=(T *_ptr) { ptr = _ptr; return *this; }


private:
  std::atomic<T*> ptr;
};

class detect_best_supported_implementation_on_first_use;

} // namespace internal

/**
 * The list of available implementations compiled into unicode.
 */
extern TURBO_DLL const internal::available_implementation_list& get_available_implementations();

/**
  * The active implementation.
  *
  * Automatically initialized on first use to the most advanced implementation supported by this hardware.
  */
extern TURBO_DLL internal::atomic_ptr<const Implementation>& get_active_implementation();


} // namespace turbo

#endif // TURBO_UNICODE_IMPLEMENTATION_H_
