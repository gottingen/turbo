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

#include "turbo/unicode/tables/utf8_to_utf16_tables.h"
#include "turbo/unicode/scalar/valid_utf8_to_utf16.h"
#include "turbo/unicode/scalar/utf8_to_utf16.h"
#include "turbo/unicode/scalar/valid_utf8_to_utf32.h"
#include "turbo/unicode/scalar/utf8_to_utf32.h"
#include "turbo/unicode/tables/utf16_to_utf8_tables.h"
#include "turbo/unicode/scalar/utf8.h"
#include "turbo/unicode/scalar/utf16.h"

#include "turbo/unicode/haswell/begin.h"
namespace turbo {
namespace TURBO_UNICODE_IMPLEMENTATION {
namespace {
#ifndef TURBO_UNICODE_HASWELL_H_
#error "haswell.h must be included"
#endif
using namespace simd;


TURBO_FORCE_INLINE bool is_ascii(const simd8x64<uint8_t>& input) {
  return input.reduce_or().is_ascii();
}

TURBO_MAYBE_UNUSED TURBO_FORCE_INLINE simd8<bool> must_be_continuation(const simd8<uint8_t> prev1, const simd8<uint8_t> prev2, const simd8<uint8_t> prev3) {
  simd8<uint8_t> is_second_byte = prev1.saturating_sub(0b11000000u-1); // Only 11______ will be > 0
  simd8<uint8_t> is_third_byte  = prev2.saturating_sub(0b11100000u-1); // Only 111_____ will be > 0
  simd8<uint8_t> is_fourth_byte = prev3.saturating_sub(0b11110000u-1); // Only 1111____ will be > 0
  // Caller requires a bool (all 1's). All values resulting from the subtraction will be <= 64, so signed comparison is fine.
  return simd8<int8_t>(is_second_byte | is_third_byte | is_fourth_byte) > int8_t(0);
}

TURBO_FORCE_INLINE simd8<bool> must_be_2_3_continuation(const simd8<uint8_t> prev2, const simd8<uint8_t> prev3) {
  simd8<uint8_t> is_third_byte  = prev2.saturating_sub(0b11100000u-1); // Only 111_____ will be > 0
  simd8<uint8_t> is_fourth_byte = prev3.saturating_sub(0b11110000u-1); // Only 1111____ will be > 0
  // Caller requires a bool (all 1's). All values resulting from the subtraction will be <= 64, so signed comparison is fine.
  return simd8<int8_t>(is_third_byte | is_fourth_byte) > int8_t(0);
}

#include "turbo/unicode/haswell/avx2_detect_encodings.cc"

#include "turbo/unicode/haswell/avx2_validate_utf16.cc"
#include "turbo/unicode/haswell/avx2_validate_utf32le.cc"

#include "turbo/unicode/haswell/avx2_convert_utf8_to_utf16.cc"
#include "turbo/unicode/haswell/avx2_convert_utf8_to_utf32.cc"

#include "turbo/unicode/haswell/avx2_convert_utf16_to_utf8.cc"
#include "turbo/unicode/haswell/avx2_convert_utf16_to_utf32.cc"

#include "turbo/unicode/haswell/avx2_convert_utf32_to_utf8.cc"
#include "turbo/unicode/haswell/avx2_convert_utf32_to_utf16.cc"
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

namespace turbo {
namespace TURBO_UNICODE_IMPLEMENTATION {

TURBO_MUST_USE_RESULT int Implementation::DetectEncodings(const char * input, size_t length) const noexcept {
  // If there is a BOM, then we trust it.
  auto bom_encoding = turbo::BOM::check_bom(input, length);
  if(bom_encoding != EncodingType::unspecified) { return bom_encoding; }
  if (length % 2 == 0) {
    return avx2_detect_encodings<utf8_validation::utf8_checker>(input, length);
  } else {
    if (Implementation::ValidateUtf8(input, length)) {
      return turbo::EncodingType::UTF8;
    } else {
      return turbo::EncodingType::unspecified;
    }
  }
}

TURBO_MUST_USE_RESULT bool Implementation::ValidateUtf8(const char *buf, size_t len) const noexcept {
  return haswell::utf8_validation::generic_validate_utf8(buf,len);
}

TURBO_MUST_USE_RESULT result Implementation::ValidateUtf8WithErrors(const char *buf, size_t len) const noexcept {
  return haswell::utf8_validation::generic_validate_utf8_with_errors(buf,len);
}

TURBO_MUST_USE_RESULT bool Implementation::ValidateAscii(const char *buf, size_t len) const noexcept {
  return haswell::utf8_validation::generic_validate_ascii(buf,len);
}

TURBO_MUST_USE_RESULT result Implementation::ValidateAsciiWithErrors(const char *buf, size_t len) const noexcept {
  return haswell::utf8_validation::generic_validate_ascii_with_errors(buf,len);
}

TURBO_MUST_USE_RESULT bool Implementation::ValidateUtf16Le(const char16_t *buf, size_t len) const noexcept {
  const char16_t* tail = avx2_validate_utf16<endianness::LITTLE>(buf, len);
  if (tail) {
    return scalar::utf16::validate<endianness::LITTLE>(tail, len - (tail - buf));
  } else {
    return false;
  }
}

TURBO_MUST_USE_RESULT bool Implementation::ValidateUtf16Be(const char16_t *buf, size_t len) const noexcept {
  const char16_t* tail = avx2_validate_utf16<endianness::BIG>(buf, len);
  if (tail) {
    return scalar::utf16::validate<endianness::BIG>(tail, len - (tail - buf));
  } else {
    return false;
  }
}

TURBO_MUST_USE_RESULT result Implementation::ValidateUtf16LeWithErrors(const char16_t *buf, size_t len) const noexcept {
  result res = avx2_validate_utf16_with_errors<endianness::LITTLE>(buf, len);
  if (res.count != len) {
    result scalar_res = scalar::utf16::validate_with_errors<endianness::LITTLE>(buf + res.count, len - res.count);
    return result(scalar_res.error, res.count + scalar_res.count);
  } else {
    return res;
  }
}

TURBO_MUST_USE_RESULT result Implementation::ValidateUtf16BeWithErrors(const char16_t *buf, size_t len) const noexcept {
  result res = avx2_validate_utf16_with_errors<endianness::BIG>(buf, len);
  if (res.count != len) {
    result scalar_res = scalar::utf16::validate_with_errors<endianness::BIG>(buf + res.count, len - res.count);
    return result(scalar_res.error, res.count + scalar_res.count);
  } else {
    return res;
  }
}

TURBO_MUST_USE_RESULT bool Implementation::ValidateUtf32(const char32_t *buf, size_t len) const noexcept {
  const char32_t* tail = avx2_validate_utf32le(buf, len);
  if (tail) {
    return scalar::utf32::validate(tail, len - (tail - buf));
  } else {
    return false;
  }
}

TURBO_MUST_USE_RESULT result Implementation::ValidateUtf32WithErrors(const char32_t *buf, size_t len) const noexcept {
  result res = avx2_validate_utf32le_with_errors(buf, len);
  if (res.count != len) {
    result scalar_res = scalar::utf32::validate_with_errors(buf + res.count, len - res.count);
    return result(scalar_res.error, res.count + scalar_res.count);
  } else {
    return res;
  }
}

TURBO_MUST_USE_RESULT size_t Implementation::ConvertUtf8ToUtf16Le(const char* buf, size_t len, char16_t* utf16_output) const noexcept {
  utf8_to_utf16::validating_transcoder converter;
  return converter.convert<endianness::LITTLE>(buf, len, utf16_output);
}

TURBO_MUST_USE_RESULT size_t Implementation::ConvertUtf8ToUtf16Be(const char* buf, size_t len, char16_t* utf16_output) const noexcept {
  utf8_to_utf16::validating_transcoder converter;
  return converter.convert<endianness::BIG>(buf, len, utf16_output);
}

TURBO_MUST_USE_RESULT result Implementation::ConvertUtf8ToUtf16LeWithErrors(const char* buf, size_t len, char16_t* utf16_output) const noexcept {
  utf8_to_utf16::validating_transcoder converter;
  return converter.convert_with_errors<endianness::LITTLE>(buf, len, utf16_output);
}

TURBO_MUST_USE_RESULT result Implementation::ConvertUtf8ToUtf16BeWithErrors(const char* buf, size_t len, char16_t* utf16_output) const noexcept {
  utf8_to_utf16::validating_transcoder converter;
  return converter.convert_with_errors<endianness::BIG>(buf, len, utf16_output);
}

TURBO_MUST_USE_RESULT size_t Implementation::ConvertValidUtf8ToUtf16Le(const char* input, size_t size,
    char16_t* utf16_output) const noexcept {
   return utf8_to_utf16::convert_valid<endianness::LITTLE>(input, size,  utf16_output);
}

TURBO_MUST_USE_RESULT size_t Implementation::ConvertValidUtf8ToUtf16Be(const char* input, size_t size,
    char16_t* utf16_output) const noexcept {
   return utf8_to_utf16::convert_valid<endianness::BIG>(input, size,  utf16_output);
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
  std::pair<const char16_t*, char*> ret = haswell::avx2_convert_utf16_to_utf8<endianness::LITTLE>(buf, len, utf8_output);
  if (ret.first == nullptr) { return 0; }
  size_t saved_bytes = ret.second - utf8_output;
  if (ret.first != buf + len) {
    const size_t scalar_saved_bytes = scalar::utf16_to_utf8::convert<endianness::LITTLE>(
                                        ret.first, len - (ret.first - buf), ret.second);
    if (scalar_saved_bytes == 0) { return 0; }
    saved_bytes += scalar_saved_bytes;
  }
  return saved_bytes;
}

TURBO_MUST_USE_RESULT size_t Implementation::ConvertUtf16BeToUtf8(const char16_t* buf, size_t len, char* utf8_output) const noexcept {
  std::pair<const char16_t*, char*> ret = haswell::avx2_convert_utf16_to_utf8<endianness::BIG>(buf, len, utf8_output);
  if (ret.first == nullptr) { return 0; }
  size_t saved_bytes = ret.second - utf8_output;
  if (ret.first != buf + len) {
    const size_t scalar_saved_bytes = scalar::utf16_to_utf8::convert<endianness::BIG>(
                                        ret.first, len - (ret.first - buf), ret.second);
    if (scalar_saved_bytes == 0) { return 0; }
    saved_bytes += scalar_saved_bytes;
  }
  return saved_bytes;
}

TURBO_MUST_USE_RESULT result Implementation::ConvertUtf16LeToUtf8WithErrors(const char16_t* buf, size_t len, char* utf8_output) const noexcept {
  // ret.first.count is always the position in the buffer, not the number of words written even if finished
  std::pair<result, char*> ret = haswell::avx2_convert_utf16_to_utf8_with_errors<endianness::LITTLE>(buf, len, utf8_output);
  if (ret.first.error) { return ret.first; }  // Can return directly since scalar fallback already found correct ret.first.count
  if (ret.first.count != len) { // All good so far, but not finished
    result scalar_res = scalar::utf16_to_utf8::convert_with_errors<endianness::LITTLE>(
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
  std::pair<result, char*> ret = haswell::avx2_convert_utf16_to_utf8_with_errors<endianness::BIG>(buf, len, utf8_output);
  if (ret.first.error) { return ret.first; }  // Can return directly since scalar fallback already found correct ret.first.count
  if (ret.first.count != len) { // All good so far, but not finished
    result scalar_res = scalar::utf16_to_utf8::convert_with_errors<endianness::BIG>(
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
  std::pair<const char32_t*, char*> ret = avx2_convert_utf32_to_utf8(buf, len, utf8_output);
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
  std::pair<result, char*> ret = haswell::avx2_convert_utf32_to_utf8_with_errors(buf, len, utf8_output);
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
  std::pair<const char16_t*, char32_t*> ret = haswell::avx2_convert_utf16_to_utf32<endianness::LITTLE>(buf, len, utf32_output);
  if (ret.first == nullptr) { return 0; }
  size_t saved_bytes = ret.second - utf32_output;
  if (ret.first != buf + len) {
    const size_t scalar_saved_bytes = scalar::utf16_to_utf32::convert<endianness::LITTLE>(
                                        ret.first, len - (ret.first - buf), ret.second);
    if (scalar_saved_bytes == 0) { return 0; }
    saved_bytes += scalar_saved_bytes;
  }
  return saved_bytes;
}

TURBO_MUST_USE_RESULT size_t Implementation::ConvertUtf16BeToUtf32(const char16_t* buf, size_t len, char32_t* utf32_output) const noexcept {
  std::pair<const char16_t*, char32_t*> ret = haswell::avx2_convert_utf16_to_utf32<endianness::BIG>(buf, len, utf32_output);
  if (ret.first == nullptr) { return 0; }
  size_t saved_bytes = ret.second - utf32_output;
  if (ret.first != buf + len) {
    const size_t scalar_saved_bytes = scalar::utf16_to_utf32::convert<endianness::BIG>(
                                        ret.first, len - (ret.first - buf), ret.second);
    if (scalar_saved_bytes == 0) { return 0; }
    saved_bytes += scalar_saved_bytes;
  }
  return saved_bytes;
}

TURBO_MUST_USE_RESULT result Implementation::ConvertUtf16LeToUtf32WithErrors(const char16_t* buf, size_t len, char32_t* utf32_output) const noexcept {
  // ret.first.count is always the position in the buffer, not the number of words written even if finished
  std::pair<result, char32_t*> ret = haswell::avx2_convert_utf16_to_utf32_with_errors<endianness::LITTLE>(buf, len, utf32_output);
  if (ret.first.error) { return ret.first; }  // Can return directly since scalar fallback already found correct ret.first.count
  if (ret.first.count != len) { // All good so far, but not finished
    result scalar_res = scalar::utf16_to_utf32::convert_with_errors<endianness::LITTLE>(
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
  std::pair<result, char32_t*> ret = haswell::avx2_convert_utf16_to_utf32_with_errors<endianness::BIG>(buf, len, utf32_output);
  if (ret.first.error) { return ret.first; }  // Can return directly since scalar fallback already found correct ret.first.count
  if (ret.first.count != len) { // All good so far, but not finished
    result scalar_res = scalar::utf16_to_utf32::convert_with_errors<endianness::BIG>(
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
  std::pair<const char32_t*, char16_t*> ret = avx2_convert_utf32_to_utf16<endianness::LITTLE>(buf, len, utf16_output);
  if (ret.first == nullptr) { return 0; }
  size_t saved_bytes = ret.second - utf16_output;
  if (ret.first != buf + len) {
    const size_t scalar_saved_bytes = scalar::utf32_to_utf16::convert<endianness::LITTLE>(
                                        ret.first, len - (ret.first - buf), ret.second);
    if (scalar_saved_bytes == 0) { return 0; }
    saved_bytes += scalar_saved_bytes;
  }
  return saved_bytes;
}

TURBO_MUST_USE_RESULT size_t Implementation::ConvertUtf32ToUtf16Be(const char32_t* buf, size_t len, char16_t* utf16_output) const noexcept {
  std::pair<const char32_t*, char16_t*> ret = avx2_convert_utf32_to_utf16<endianness::BIG>(buf, len, utf16_output);
  if (ret.first == nullptr) { return 0; }
  size_t saved_bytes = ret.second - utf16_output;
  if (ret.first != buf + len) {
    const size_t scalar_saved_bytes = scalar::utf32_to_utf16::convert<endianness::BIG>(
                                        ret.first, len - (ret.first - buf), ret.second);
    if (scalar_saved_bytes == 0) { return 0; }
    saved_bytes += scalar_saved_bytes;
  }
  return saved_bytes;
}

TURBO_MUST_USE_RESULT result Implementation::ConvertUtf32ToUtf16leWithErrors(const char32_t* buf, size_t len, char16_t* utf16_output) const noexcept {
  // ret.first.count is always the position in the buffer, not the number of words written even if finished
  std::pair<result, char16_t*> ret = haswell::avx2_convert_utf32_to_utf16_with_errors<endianness::LITTLE>(buf, len, utf16_output);
  if (ret.first.count != len) {
    result scalar_res = scalar::utf32_to_utf16::convert_with_errors<endianness::LITTLE>(
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
  std::pair<result, char16_t*> ret = haswell::avx2_convert_utf32_to_utf16_with_errors<endianness::BIG>(buf, len, utf16_output);
  if (ret.first.count != len) {
    result scalar_res = scalar::utf32_to_utf16::convert_with_errors<endianness::BIG>(
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
  return utf16::count_code_points<endianness::LITTLE>(input, length);
}

TURBO_MUST_USE_RESULT size_t Implementation::CountUtf16Be(const char16_t * input, size_t length) const noexcept {
  return utf16::count_code_points<endianness::BIG>(input, length);
}

TURBO_MUST_USE_RESULT size_t Implementation::CountUtf8(const char * input, size_t length) const noexcept {
  return utf8::count_code_points(input, length);
}

TURBO_MUST_USE_RESULT size_t Implementation::Utf8LengthFromUtf16Le(const char16_t * input, size_t length) const noexcept {
  return utf16::Utf8LengthFromUtf16<endianness::LITTLE>(input, length);
}

TURBO_MUST_USE_RESULT size_t Implementation::Utf8LengthFromUtf16be(const char16_t * input, size_t length) const noexcept {
  return utf16::Utf8LengthFromUtf16<endianness::BIG>(input, length);
}

TURBO_MUST_USE_RESULT size_t Implementation::Utf32LengthFromUtf16Le(const char16_t * input, size_t length) const noexcept {
  return utf16::Utf32LengthFromUtf16<endianness::LITTLE>(input, length);
}

TURBO_MUST_USE_RESULT size_t Implementation::Utf32LengthFromUtf16Be(const char16_t * input, size_t length) const noexcept {
  return utf16::Utf32LengthFromUtf16<endianness::BIG>(input, length);
}

TURBO_MUST_USE_RESULT size_t Implementation::Utf16LengthFromUtf8(const char * input, size_t length) const noexcept {
  return utf8::Utf16LengthFromUtf8(input, length);
}

TURBO_MUST_USE_RESULT size_t Implementation::Utf8LengthFromUtf32(const char32_t * input, size_t length) const noexcept {
  const __m256i v_00000000 = _mm256_setzero_si256();
  const __m256i v_ffffff80 = _mm256_set1_epi32((uint32_t)0xffffff80);
  const __m256i v_fffff800 = _mm256_set1_epi32((uint32_t)0xfffff800);
  const __m256i v_ffff0000 = _mm256_set1_epi32((uint32_t)0xffff0000);
  size_t pos = 0;
  size_t count = 0;
  for(;pos + 8 <= length; pos += 8) {
    __m256i in = _mm256_loadu_si256((__m256i*)const_cast<char32_t*>(input + pos));
    const __m256i ascii_bytes_bytemask = _mm256_cmpeq_epi32(_mm256_and_si256(in, v_ffffff80), v_00000000);
    const __m256i one_two_bytes_bytemask = _mm256_cmpeq_epi32(_mm256_and_si256(in, v_fffff800), v_00000000);
    const __m256i two_bytes_bytemask = _mm256_xor_si256(one_two_bytes_bytemask, ascii_bytes_bytemask);
    const __m256i one_two_three_bytes_bytemask = _mm256_cmpeq_epi32(_mm256_and_si256(in, v_ffff0000), v_00000000);
    const __m256i three_bytes_bytemask = _mm256_xor_si256(one_two_three_bytes_bytemask, one_two_bytes_bytemask);
    const uint32_t ascii_bytes_bitmask = static_cast<uint32_t>(_mm256_movemask_epi8(ascii_bytes_bytemask));
    const uint32_t two_bytes_bitmask = static_cast<uint32_t>(_mm256_movemask_epi8(two_bytes_bytemask));
    const uint32_t three_bytes_bitmask = static_cast<uint32_t>(_mm256_movemask_epi8(three_bytes_bytemask));

    size_t ascii_count = count_ones(ascii_bytes_bitmask) / 4;
    size_t two_bytes_count = count_ones(two_bytes_bitmask) / 4;
    size_t three_bytes_count = count_ones(three_bytes_bitmask) / 4;
    count += 32 - 3*ascii_count - 2*two_bytes_count - three_bytes_count;
  }
  return count + scalar::utf32::Utf8LengthFromUtf32(input + pos, length - pos);
}

TURBO_MUST_USE_RESULT size_t Implementation::Utf16LengthFromUtf32(const char32_t * input, size_t length) const noexcept {
  const __m256i v_00000000 = _mm256_setzero_si256();
  const __m256i v_ffff0000 = _mm256_set1_epi32((uint32_t)0xffff0000);
  size_t pos = 0;
  size_t count = 0;
  for(;pos + 8 <= length; pos += 8) {
    __m256i in = _mm256_loadu_si256((__m256i*)const_cast<char32_t*>(input + pos));
    const __m256i surrogate_bytemask = _mm256_cmpeq_epi32(_mm256_and_si256(in, v_ffff0000), v_00000000);
    const uint32_t surrogate_bitmask = static_cast<uint32_t>(_mm256_movemask_epi8(surrogate_bytemask));
    size_t surrogate_count = (32-count_ones(surrogate_bitmask))/4;
    count += 8 + surrogate_count;
  }
  return count + scalar::utf32::Utf16LengthFromUtf32(input + pos, length - pos);
}

TURBO_MUST_USE_RESULT size_t Implementation::Utf32LengthFromUtf8(const char * input, size_t length) const noexcept {
  return scalar::utf8::count_code_points(input, length);
}

} // namespace TURBO_UNICODE_IMPLEMENTATION
} // namespace turbo

#include "turbo/unicode/haswell/end.h"

