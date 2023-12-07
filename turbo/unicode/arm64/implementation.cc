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

#include "turbo/unicode/arm64/begin.h"
namespace turbo {
namespace TURBO_UNICODE_IMPLEMENTATION {
namespace {
#ifndef TURBO_UNICODE_ARM64_H_
#error "arm64.h must be included"
#endif
using namespace simd;

TURBO_FORCE_INLINE bool is_ascii(const simd8x64<uint8_t>& input) {
    simd8<uint8_t> bits = input.reduce_or();
    return bits.max_val() < 0b10000000u;
}

TURBO_MAYBE_UNUSED TURBO_FORCE_INLINE simd8<bool> must_be_continuation(const simd8<uint8_t> prev1, const simd8<uint8_t> prev2, const simd8<uint8_t> prev3) {
    simd8<bool> is_second_byte = prev1 >= uint8_t(0b11000000u);
    simd8<bool> is_third_byte  = prev2 >= uint8_t(0b11100000u);
    simd8<bool> is_fourth_byte = prev3 >= uint8_t(0b11110000u);
    // Use ^ instead of | for is_*_byte, because ^ is commutative, and the caller is using ^ as well.
    // This will work fine because we only have to report errors for cases with 0-1 lead bytes.
    // Multiple lead bytes implies 2 overlapping multibyte characters, and if that happens, there is
    // guaranteed to be at least *one* lead byte that is part of only 1 other multibyte character.
    // The error will be detected there.
    return is_second_byte ^ is_third_byte ^ is_fourth_byte;
}

TURBO_FORCE_INLINE simd8<bool> must_be_2_3_continuation(const simd8<uint8_t> prev2, const simd8<uint8_t> prev3) {
    simd8<bool> is_third_byte  = prev2 >= uint8_t(0b11100000u);
    simd8<bool> is_fourth_byte = prev3 >= uint8_t(0b11110000u);
    return is_third_byte ^ is_fourth_byte;
}

#include "turbo/unicode/arm64/arm_detect_encodings.cc"

#include "turbo/unicode/arm64/arm_validate_utf16.cc"
#include "turbo/unicode/arm64/arm_validate_utf32le.cc"

#include "turbo/unicode/arm64/arm_convert_utf8_to_utf16.cc"
#include "turbo/unicode/arm64/arm_convert_utf8_to_utf32.cc"

#include "turbo/unicode/arm64/arm_convert_utf16_to_utf8.cc"
#include "turbo/unicode/arm64/arm_convert_utf16_to_utf32.cc"

#include "turbo/unicode/arm64/arm_convert_utf32_to_utf8.cc"
#include "turbo/unicode/arm64/arm_convert_utf32_to_utf16.cc"
} // unnamed namespace
} // namespace TURBO_UNICODE_IMPLEMENTATION
} // namespace turbo
#include "turbo/unicode/generic/buf_block_reader.h"
#include "turbo/unicode/generic/utf8_lookup4_algorithm.h"
#include "turbo/unicode/generic/utf8_validator.h"
// transcoding from UTF-8 to UTF-16
#include "turbo/unicode/generic/valid_utf8_to_utf16.h"
#include "turbo/unicode/generic/utf8_to_utf16.h"
// transcoding from UTF-8 to UTF-32
#include "turbo/unicode/generic/valid_utf8_to_utf32.h"
#include "turbo/unicode/generic/utf8_to_utf32.h"
// other functions
#include "turbo/unicode/generic/utf8.h"
#include "turbo/unicode/generic/utf16.h"
//
// Implementation-specific overrides
//
namespace turbo {
namespace TURBO_UNICODE_IMPLEMENTATION {

TURBO_MUST_USE_RESULT int Implementation::DetectEncodings(const char * input, size_t length) const noexcept {
  // If there is a BOM, then we trust it.
  auto bom_encoding = turbo::BOM::check_bom(input, length);
  if(bom_encoding != EncodingType::unspecified) { return bom_encoding; }
  if (length % 2 == 0) {
    return arm_detect_encodings<utf8_validation::utf8_checker>(input, length);
  } else {
    if (Implementation::ValidateUtf8(input, length)) {
      return turbo::EncodingType::UTF8;
    } else {
      return turbo::EncodingType::unspecified;
    }
  }
}

TURBO_MUST_USE_RESULT bool Implementation::ValidateUtf8(const char *buf, size_t len) const noexcept {
  return arm64::utf8_validation::generic_validate_utf8(buf,len);
}

TURBO_MUST_USE_RESULT result Implementation::ValidateUtf8WithErrors(const char *buf, size_t len) const noexcept {
  return arm64::utf8_validation::generic_validate_utf8_with_errors(buf,len);
}

TURBO_MUST_USE_RESULT bool Implementation::ValidateAscii(const char *buf, size_t len) const noexcept {
  return arm64::utf8_validation::generic_validate_ascii(buf,len);
}

TURBO_MUST_USE_RESULT result Implementation::ValidateAsciiWithErrors(const char *buf, size_t len) const noexcept {
  return arm64::utf8_validation::generic_validate_ascii_with_errors(buf,len);
}

TURBO_MUST_USE_RESULT bool Implementation::ValidateUtf16Le(const char16_t *buf, size_t len) const noexcept {
  const char16_t* tail = arm_validate_utf16<EndianNess::SYS_LITTLE_ENDIAN>(buf, len);
  if (tail) {
    return scalar::utf16::validate<EndianNess::SYS_LITTLE_ENDIAN>(tail, len - (tail - buf));
  } else {
    return false;
  }
}

TURBO_MUST_USE_RESULT bool Implementation::ValidateUtf16Be(const char16_t *buf, size_t len) const noexcept {
  const char16_t* tail = arm_validate_utf16<EndianNess::SYS_BIG_ENDIAN>(buf, len);
  if (tail) {
    return scalar::utf16::validate<EndianNess::SYS_BIG_ENDIAN>(tail, len - (tail - buf));
  } else {
    return false;
  }
}

TURBO_MUST_USE_RESULT result Implementation::ValidateUtf16LeWithErrors(const char16_t *buf, size_t len) const noexcept {
  result res = arm_validate_utf16_with_errors<EndianNess::SYS_LITTLE_ENDIAN>(buf, len);
  if (res.count != len) {
    result scalar_res = scalar::utf16::validate_with_errors<EndianNess::SYS_LITTLE_ENDIAN>(buf + res.count, len - res.count);
    return result(scalar_res.error, res.count + scalar_res.count);
  } else {
    return res;
  }
}

TURBO_MUST_USE_RESULT result Implementation::ValidateUtf16BeWithErrors(const char16_t *buf, size_t len) const noexcept {
  result res = arm_validate_utf16_with_errors<EndianNess::SYS_BIG_ENDIAN>(buf, len);
  if (res.count != len) {
    result scalar_res = scalar::utf16::validate_with_errors<EndianNess::SYS_BIG_ENDIAN>(buf + res.count, len - res.count);
    return result(scalar_res.error, res.count + scalar_res.count);
  } else {
    return res;
  }
}

TURBO_MUST_USE_RESULT bool Implementation::ValidateUtf32(const char32_t *buf, size_t len) const noexcept {
  const char32_t* tail = arm_validate_utf32le(buf, len);
  if (tail) {
    return scalar::utf32::validate(tail, len - (tail - buf));
  } else {
    return false;
  }
}

TURBO_MUST_USE_RESULT result Implementation::ValidateUtf32WithErrors(const char32_t *buf, size_t len) const noexcept {
  result res = arm_validate_utf32le_with_errors(buf, len);
  if (res.count != len) {
    result scalar_res = scalar::utf32::validate_with_errors(buf + res.count, len - res.count);
    return result(scalar_res.error, res.count + scalar_res.count);
  } else {
    return res;
  }
}

TURBO_MUST_USE_RESULT size_t Implementation::ConvertUtf8ToUtf16Le(const char* buf, size_t len, char16_t* utf16_output) const noexcept {
  utf8_to_utf16::validating_transcoder converter;
  return converter.convert<EndianNess::SYS_LITTLE_ENDIAN>(buf, len, utf16_output);
}

TURBO_MUST_USE_RESULT size_t Implementation::ConvertUtf8ToUtf16Be(const char* buf, size_t len, char16_t* utf16_output) const noexcept {
  utf8_to_utf16::validating_transcoder converter;
  return converter.convert<EndianNess::SYS_BIG_ENDIAN>(buf, len, utf16_output);
}

TURBO_MUST_USE_RESULT result Implementation::ConvertUtf8ToUtf16LeWithErrors(const char* buf, size_t len, char16_t* utf16_output) const noexcept {
  utf8_to_utf16::validating_transcoder converter;
  return converter.convert_with_errors<EndianNess::SYS_LITTLE_ENDIAN>(buf, len, utf16_output);
}

TURBO_MUST_USE_RESULT result Implementation::ConvertUtf8ToUtf16BeWithErrors(const char* buf, size_t len, char16_t* utf16_output) const noexcept {
  utf8_to_utf16::validating_transcoder converter;
  return converter.convert_with_errors<EndianNess::SYS_BIG_ENDIAN>(buf, len, utf16_output);
}

TURBO_MUST_USE_RESULT size_t Implementation::ConvertValidUtf8ToUtf16Le(const char* input, size_t size,
    char16_t* utf16_output) const noexcept {
  return utf8_to_utf16::convert_valid<EndianNess::SYS_LITTLE_ENDIAN>(input, size,  utf16_output);
}

TURBO_MUST_USE_RESULT size_t Implementation::ConvertValidUtf8ToUtf16Be(const char* input, size_t size,
    char16_t* utf16_output) const noexcept {
  return utf8_to_utf16::convert_valid<EndianNess::SYS_BIG_ENDIAN>(input, size,  utf16_output);
}

TURBO_MUST_USE_RESULT size_t Implementation::ConvertUtf8ToUtf32(const char* buf, size_t len, char32_t* utf32_output) const noexcept {
  utf8_to_utf32::validating_transcoder converter;
  return converter.convert(buf, len, utf32_output);
}

TURBO_MUST_USE_RESULT result Implementation::ConvertUtf8ToUtf32WithErrors(const char* buf, size_t len, char32_t* utf32_output) const noexcept {
  utf8_to_utf32::validating_transcoder converter;
  return converter.convert_with_errors(buf, len, utf32_output);
}

TURBO_MUST_USE_RESULT size_t Implementation::ConvertValidUtf8ToUtf32(const char* input, size_t size,
    char32_t* utf32_output) const noexcept {
  return utf8_to_utf32::convert_valid(input, size,  utf32_output);
}

TURBO_MUST_USE_RESULT size_t Implementation::ConvertUtf16LeToUtf8(const char16_t* buf, size_t len, char* utf8_output) const noexcept {
  std::pair<const char16_t*, char*> ret = arm_convert_utf16_to_utf8<EndianNess::SYS_LITTLE_ENDIAN>(buf, len, utf8_output);
  if (ret.first == nullptr) { return 0; }
  size_t saved_bytes = ret.second - utf8_output;
  if (ret.first != buf + len) {
    const size_t scalar_saved_bytes = scalar::utf16_to_utf8::convert<EndianNess::SYS_LITTLE_ENDIAN>(
                                        ret.first, len - (ret.first - buf), ret.second);
    if (scalar_saved_bytes == 0) { return 0; }
    saved_bytes += scalar_saved_bytes;
  }
  return saved_bytes;
}

TURBO_MUST_USE_RESULT size_t Implementation::ConvertUtf16BeToUtf8(const char16_t* buf, size_t len, char* utf8_output) const noexcept {
  std::pair<const char16_t*, char*> ret = arm_convert_utf16_to_utf8<EndianNess::SYS_BIG_ENDIAN>(buf, len, utf8_output);
  if (ret.first == nullptr) { return 0; }
  size_t saved_bytes = ret.second - utf8_output;
  if (ret.first != buf + len) {
    const size_t scalar_saved_bytes = scalar::utf16_to_utf8::convert<EndianNess::SYS_BIG_ENDIAN>(
                                        ret.first, len - (ret.first - buf), ret.second);
    if (scalar_saved_bytes == 0) { return 0; }
    saved_bytes += scalar_saved_bytes;
  }
  return saved_bytes;
}

TURBO_MUST_USE_RESULT result Implementation::ConvertUtf16LeToUtf8WithErrors(const char16_t* buf, size_t len, char* utf8_output) const noexcept {
  // ret.first.count is always the position in the buffer, not the number of words written even if finished
  std::pair<result, char*> ret = arm_convert_utf16_to_utf8_with_errors<EndianNess::SYS_LITTLE_ENDIAN>(buf, len, utf8_output);
  if (ret.first.error) { return ret.first; }  // Can return directly since scalar fallback already found correct ret.first.count
  if (ret.first.count != len) { // All good so far, but not finished
    result scalar_res = scalar::utf16_to_utf8::convert_with_errors<EndianNess::SYS_LITTLE_ENDIAN>(
                                        buf + ret.first.count, len - ret.first.count, ret.second);
    if (scalar_res.error) {
      scalar_res.count += ret.first.count;
      return scalar_res;
    } else {
      ret.second += scalar_res.count;
    }
  }
  ret.first.count = ret.second - utf8_output;   // Set count to the number of 8-bit words written
  return ret.first;
}

TURBO_MUST_USE_RESULT result Implementation::ConvertUtf16BeToUtf8WithErrors(const char16_t* buf, size_t len, char* utf8_output) const noexcept {
  // ret.first.count is always the position in the buffer, not the number of words written even if finished
  std::pair<result, char*> ret = arm_convert_utf16_to_utf8_with_errors<EndianNess::SYS_BIG_ENDIAN>(buf, len, utf8_output);
  if (ret.first.error) { return ret.first; }  // Can return directly since scalar fallback already found correct ret.first.count
  if (ret.first.count != len) { // All good so far, but not finished
    result scalar_res = scalar::utf16_to_utf8::convert_with_errors<EndianNess::SYS_BIG_ENDIAN>(
                                        buf + ret.first.count, len - ret.first.count, ret.second);
    if (scalar_res.error) {
      scalar_res.count += ret.first.count;
      return scalar_res;
    } else {
      ret.second += scalar_res.count;
    }
  }
  ret.first.count = ret.second - utf8_output;   // Set count to the number of 8-bit words written
  return ret.first;
}

TURBO_MUST_USE_RESULT size_t Implementation::ConvertValidUtf16LeToUtf8(const char16_t* buf, size_t len, char* utf8_output) const noexcept {
  return ConvertUtf16LeToUtf8(buf, len, utf8_output);
}

TURBO_MUST_USE_RESULT size_t Implementation::ConvertValidUtf16BeToUtf8(const char16_t* buf, size_t len, char* utf8_output) const noexcept {
  return ConvertUtf16BeToUtf8(buf, len, utf8_output);
}

TURBO_MUST_USE_RESULT size_t Implementation::ConvertUtf32ToUtf8(const char32_t* buf, size_t len, char* utf8_output) const noexcept {
  std::pair<const char32_t*, char*> ret = arm_convert_utf32_to_utf8(buf, len, utf8_output);
  if (ret.first == nullptr) { return 0; }
  size_t saved_bytes = ret.second - utf8_output;
  if (ret.first != buf + len) {
    const size_t scalar_saved_bytes = scalar::utf32_to_utf8::convert(
                                        ret.first, len - (ret.first - buf), ret.second);
    if (scalar_saved_bytes == 0) { return 0; }
    saved_bytes += scalar_saved_bytes;
  }
  return saved_bytes;
}

TURBO_MUST_USE_RESULT result Implementation::ConvertUtf32ToUtf8WithErrors(const char32_t* buf, size_t len, char* utf8_output) const noexcept {
  // ret.first.count is always the position in the buffer, not the number of words written even if finished
  std::pair<result, char*> ret = arm_convert_utf32_to_utf8_with_errors(buf, len, utf8_output);
  if (ret.first.count != len) {
    result scalar_res = scalar::utf32_to_utf8::convert_with_errors(
                                        buf + ret.first.count, len - ret.first.count, ret.second);
    if (scalar_res.error) {
      scalar_res.count += ret.first.count;
      return scalar_res;
    } else {
      ret.second += scalar_res.count;
    }
  }
  ret.first.count = ret.second - utf8_output;   // Set count to the number of 8-bit words written
  return ret.first;
}

TURBO_MUST_USE_RESULT size_t Implementation::ConvertUtf16LeToUtf32(const char16_t* buf, size_t len, char32_t* utf32_output) const noexcept {
  std::pair<const char16_t*, char32_t*> ret = arm_convert_utf16_to_utf32<EndianNess::SYS_LITTLE_ENDIAN>(buf, len, utf32_output);
  if (ret.first == nullptr) { return 0; }
  size_t saved_bytes = ret.second - utf32_output;
  if (ret.first != buf + len) {
    const size_t scalar_saved_bytes = scalar::utf16_to_utf32::convert<EndianNess::SYS_LITTLE_ENDIAN>(
                                        ret.first, len - (ret.first - buf), ret.second);
    if (scalar_saved_bytes == 0) { return 0; }
    saved_bytes += scalar_saved_bytes;
  }
  return saved_bytes;
}

TURBO_MUST_USE_RESULT size_t Implementation::ConvertUtf16BeToUtf32(const char16_t* buf, size_t len, char32_t* utf32_output) const noexcept {
  std::pair<const char16_t*, char32_t*> ret = arm_convert_utf16_to_utf32<EndianNess::SYS_BIG_ENDIAN>(buf, len, utf32_output);
  if (ret.first == nullptr) { return 0; }
  size_t saved_bytes = ret.second - utf32_output;
  if (ret.first != buf + len) {
    const size_t scalar_saved_bytes = scalar::utf16_to_utf32::convert<EndianNess::SYS_BIG_ENDIAN>(
                                        ret.first, len - (ret.first - buf), ret.second);
    if (scalar_saved_bytes == 0) { return 0; }
    saved_bytes += scalar_saved_bytes;
  }
  return saved_bytes;
}

TURBO_MUST_USE_RESULT result Implementation::ConvertUtf16LeToUtf32WithErrors(const char16_t* buf, size_t len, char32_t* utf32_output) const noexcept {
  // ret.first.count is always the position in the buffer, not the number of words written even if finished
  std::pair<result, char32_t*> ret = arm_convert_utf16_to_utf32_with_errors<EndianNess::SYS_LITTLE_ENDIAN>(buf, len, utf32_output);
  if (ret.first.error) { return ret.first; }  // Can return directly since scalar fallback already found correct ret.first.count
  if (ret.first.count != len) { // All good so far, but not finished
    result scalar_res = scalar::utf16_to_utf32::convert_with_errors<EndianNess::SYS_LITTLE_ENDIAN>(
                                        buf + ret.first.count, len - ret.first.count, ret.second);
    if (scalar_res.error) {
      scalar_res.count += ret.first.count;
      return scalar_res;
    } else {
      ret.second += scalar_res.count;
    }
  }
  ret.first.count = ret.second - utf32_output;   // Set count to the number of 8-bit words written
  return ret.first;
}

TURBO_MUST_USE_RESULT result Implementation::ConvertUtf16BeToUtf32WithErrors(const char16_t* buf, size_t len, char32_t* utf32_output) const noexcept {
  // ret.first.count is always the position in the buffer, not the number of words written even if finished
  std::pair<result, char32_t*> ret = arm_convert_utf16_to_utf32_with_errors<EndianNess::SYS_BIG_ENDIAN>(buf, len, utf32_output);
  if (ret.first.error) { return ret.first; }  // Can return directly since scalar fallback already found correct ret.first.count
  if (ret.first.count != len) { // All good so far, but not finished
    result scalar_res = scalar::utf16_to_utf32::convert_with_errors<EndianNess::SYS_BIG_ENDIAN>(
                                        buf + ret.first.count, len - ret.first.count, ret.second);
    if (scalar_res.error) {
      scalar_res.count += ret.first.count;
      return scalar_res;
    } else {
      ret.second += scalar_res.count;
    }
  }
  ret.first.count = ret.second - utf32_output;   // Set count to the number of 8-bit words written
  return ret.first;
}

TURBO_MUST_USE_RESULT size_t Implementation::ConvertValidUtf32ToUtf8(const char32_t* buf, size_t len, char* utf8_output) const noexcept {
  return ConvertUtf32ToUtf8(buf, len, utf8_output);
}

TURBO_MUST_USE_RESULT size_t Implementation::ConvertUtf32ToUtf16Le(const char32_t* buf, size_t len, char16_t* utf16_output) const noexcept {
  std::pair<const char32_t*, char16_t*> ret = arm_convert_utf32_to_utf16<EndianNess::SYS_LITTLE_ENDIAN>(buf, len, utf16_output);
  if (ret.first == nullptr) { return 0; }
  size_t saved_bytes = ret.second - utf16_output;
  if (ret.first != buf + len) {
    const size_t scalar_saved_bytes = scalar::utf32_to_utf16::convert<EndianNess::SYS_LITTLE_ENDIAN>(
                                        ret.first, len - (ret.first - buf), ret.second);
    if (scalar_saved_bytes == 0) { return 0; }
    saved_bytes += scalar_saved_bytes;
  }
  return saved_bytes;
}

TURBO_MUST_USE_RESULT size_t Implementation::ConvertUtf32ToUtf16Be(const char32_t* buf, size_t len, char16_t* utf16_output) const noexcept {
  std::pair<const char32_t*, char16_t*> ret = arm_convert_utf32_to_utf16<EndianNess::SYS_BIG_ENDIAN>(buf, len, utf16_output);
  if (ret.first == nullptr) { return 0; }
  size_t saved_bytes = ret.second - utf16_output;
  if (ret.first != buf + len) {
    const size_t scalar_saved_bytes = scalar::utf32_to_utf16::convert<EndianNess::SYS_BIG_ENDIAN>(
                                        ret.first, len - (ret.first - buf), ret.second);
    if (scalar_saved_bytes == 0) { return 0; }
    saved_bytes += scalar_saved_bytes;
  }
  return saved_bytes;
}

TURBO_MUST_USE_RESULT result Implementation::ConvertUtf32ToUtf16leWithErrors(const char32_t* buf, size_t len, char16_t* utf16_output) const noexcept {
  // ret.first.count is always the position in the buffer, not the number of words written even if finished
  std::pair<result, char16_t*> ret = arm_convert_utf32_to_utf16_with_errors<EndianNess::SYS_LITTLE_ENDIAN>(buf, len, utf16_output);
  if (ret.first.count != len) {
    result scalar_res = scalar::utf32_to_utf16::convert_with_errors<EndianNess::SYS_LITTLE_ENDIAN>(
                                        buf + ret.first.count, len - ret.first.count, ret.second);
    if (scalar_res.error) {
      scalar_res.count += ret.first.count;
      return scalar_res;
    } else {
      ret.second += scalar_res.count;
    }
  }
  ret.first.count = ret.second - utf16_output;   // Set count to the number of 8-bit words written
  return ret.first;
}

TURBO_MUST_USE_RESULT result Implementation::ConvertUtf32ToUtf16BeWithErrors(const char32_t* buf, size_t len, char16_t* utf16_output) const noexcept {
  // ret.first.count is always the position in the buffer, not the number of words written even if finished
  std::pair<result, char16_t*> ret = arm_convert_utf32_to_utf16_with_errors<EndianNess::SYS_BIG_ENDIAN>(buf, len, utf16_output);
  if (ret.first.count != len) {
    result scalar_res = scalar::utf32_to_utf16::convert_with_errors<EndianNess::SYS_BIG_ENDIAN>(
                                        buf + ret.first.count, len - ret.first.count, ret.second);
    if (scalar_res.error) {
      scalar_res.count += ret.first.count;
      return scalar_res;
    } else {
      ret.second += scalar_res.count;
    }
  }
  ret.first.count = ret.second - utf16_output;   // Set count to the number of 8-bit words written
  return ret.first;
}

TURBO_MUST_USE_RESULT size_t Implementation::ConvertValidUtf32ToUtf16Le(const char32_t* buf, size_t len, char16_t* utf16_output) const noexcept {
  return ConvertUtf32ToUtf16Le(buf, len, utf16_output);
}

TURBO_MUST_USE_RESULT size_t Implementation::ConvertValidUtf32ToUtf16Be(const char32_t* buf, size_t len, char16_t* utf16_output) const noexcept {
  return ConvertUtf32ToUtf16Be(buf, len, utf16_output);
}

TURBO_MUST_USE_RESULT size_t Implementation::ConvertValidUtf16LeToUtf32(const char16_t* buf, size_t len, char32_t* utf32_output) const noexcept {
  return ConvertUtf16LeToUtf32(buf, len, utf32_output);
}

TURBO_MUST_USE_RESULT size_t Implementation::ConvertValidUtf16BeToUtf32(const char16_t* buf, size_t len, char32_t* utf32_output) const noexcept {
  return ConvertUtf16BeToUtf32(buf, len, utf32_output);
}

void Implementation::ChangeEndiannessUtf16(const char16_t * input, size_t length, char16_t * output) const noexcept {
  utf16::ChangeEndiannessUtf16(input, length, output);
}

TURBO_MUST_USE_RESULT size_t Implementation::CountUtf16Le(const char16_t * input, size_t length) const noexcept {
  return utf16::count_code_points<EndianNess::SYS_LITTLE_ENDIAN>(input, length);
}

TURBO_MUST_USE_RESULT size_t Implementation::CountUtf16Be(const char16_t * input, size_t length) const noexcept {
  return utf16::count_code_points<EndianNess::SYS_BIG_ENDIAN>(input, length);
}

TURBO_MUST_USE_RESULT size_t Implementation::CountUtf8(const char * input, size_t length) const noexcept {
  return utf8::count_code_points(input, length);
}

TURBO_MUST_USE_RESULT size_t Implementation::Utf8LengthFromUtf16Le(const char16_t * input, size_t length) const noexcept {
  return utf16::Utf8LengthFromUtf16<EndianNess::SYS_LITTLE_ENDIAN>(input, length);
}

TURBO_MUST_USE_RESULT size_t Implementation::Utf8LengthFromUtf16be(const char16_t * input, size_t length) const noexcept {
  return utf16::Utf8LengthFromUtf16<EndianNess::SYS_BIG_ENDIAN>(input, length);
}

TURBO_MUST_USE_RESULT size_t Implementation::Utf32LengthFromUtf16Le(const char16_t * input, size_t length) const noexcept {
  return utf16::Utf32LengthFromUtf16<EndianNess::SYS_LITTLE_ENDIAN>(input, length);
}

TURBO_MUST_USE_RESULT size_t Implementation::Utf32LengthFromUtf16Be(const char16_t * input, size_t length) const noexcept {
  return utf16::Utf32LengthFromUtf16<EndianNess::SYS_BIG_ENDIAN>(input, length);
}

TURBO_MUST_USE_RESULT size_t Implementation::Utf16LengthFromUtf8(const char * input, size_t length) const noexcept {
  return utf8::Utf16LengthFromUtf8(input, length);
}

TURBO_MUST_USE_RESULT size_t Implementation::Utf8LengthFromUtf32(const char32_t * input, size_t length) const noexcept {
  const uint32x4_t v_7f = vmovq_n_u32((uint32_t)0x7f);
  const uint32x4_t v_7ff = vmovq_n_u32((uint32_t)0x7ff);
  const uint32x4_t v_ffff = vmovq_n_u32((uint32_t)0xffff);
  const uint32x4_t v_1 = vmovq_n_u32((uint32_t)0x1);
  size_t pos = 0;
  size_t count = 0;
  for(;pos + 4 <= length; pos += 4) {
    uint32x4_t in = vld1q_u32(reinterpret_cast<const uint32_t *>(input + pos));
    const uint32x4_t ascii_bytes_bytemask = vcleq_u32(in, v_7f);
    const uint32x4_t one_two_bytes_bytemask = vcleq_u32(in, v_7ff);
    const uint32x4_t two_bytes_bytemask = veorq_u32(one_two_bytes_bytemask, ascii_bytes_bytemask);
    const uint32x4_t three_bytes_bytemask = veorq_u32(vcleq_u32(in, v_ffff), one_two_bytes_bytemask);

    const uint16x8_t reduced_ascii_bytes_bytemask = vreinterpretq_u16_u32(vandq_u32(ascii_bytes_bytemask, v_1));
    const uint16x8_t reduced_two_bytes_bytemask = vreinterpretq_u16_u32(vandq_u32(two_bytes_bytemask, v_1));
    const uint16x8_t reduced_three_bytes_bytemask = vreinterpretq_u16_u32(vandq_u32(three_bytes_bytemask, v_1));

    const uint16x8_t compressed_bytemask0 = vpaddq_u16(reduced_ascii_bytes_bytemask, reduced_two_bytes_bytemask);
    const uint16x8_t compressed_bytemask1 = vpaddq_u16(reduced_three_bytes_bytemask, reduced_three_bytes_bytemask);

    size_t ascii_count = count_ones(vgetq_lane_u64(vreinterpretq_u64_u16(compressed_bytemask0), 0));
    size_t two_bytes_count = count_ones(vgetq_lane_u64(vreinterpretq_u64_u16(compressed_bytemask0), 1));
    size_t three_bytes_count = count_ones(vgetq_lane_u64(vreinterpretq_u64_u16(compressed_bytemask1), 0));

    count += 16 - 3*ascii_count - 2*two_bytes_count - three_bytes_count;
  }
  return count + scalar::utf32::Utf8LengthFromUtf32(input + pos, length - pos);
}

TURBO_MUST_USE_RESULT size_t Implementation::Utf16LengthFromUtf32(const char32_t * input, size_t length) const noexcept {
  const uint32x4_t v_ffff = vmovq_n_u32((uint32_t)0xffff);
  const uint32x4_t v_1 = vmovq_n_u32((uint32_t)0x1);
  size_t pos = 0;
  size_t count = 0;
  for(;pos + 4 <= length; pos += 4) {
    uint32x4_t in = vld1q_u32(reinterpret_cast<const uint32_t *>(input + pos));
    const uint32x4_t surrogate_bytemask = vcgtq_u32(in, v_ffff);
    const uint16x8_t reduced_bytemask = vreinterpretq_u16_u32(vandq_u32(surrogate_bytemask, v_1));
    const uint16x8_t compressed_bytemask = vpaddq_u16(reduced_bytemask, reduced_bytemask);
    size_t surrogate_count = count_ones(vgetq_lane_u64(vreinterpretq_u64_u16(compressed_bytemask), 0));
    count += 4 + surrogate_count;
  }
  return count + scalar::utf32::Utf16LengthFromUtf32(input + pos, length - pos);
}

TURBO_MUST_USE_RESULT size_t Implementation::Utf32LengthFromUtf8(const char * input, size_t length) const noexcept {
  return utf8::Utf32LengthFromUtf8(input, length);
}

} // namespace TURBO_UNICODE_IMPLEMENTATION
} // namespace turbo

#include "turbo/unicode/arm64/end.h"
