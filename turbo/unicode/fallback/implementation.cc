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

#include "turbo/unicode/fallback/begin.h"

#include "turbo/unicode/scalar/valid_utf8_to_utf16.h"
#include "turbo/unicode/scalar/utf8_to_utf16.h"

#include "turbo/unicode/scalar/valid_utf8_to_utf32.h"
#include "turbo/unicode/scalar/utf8_to_utf32.h"

#include "turbo/unicode/scalar/valid_utf16_to_utf8.h"
#include "turbo/unicode/scalar/utf16_to_utf8.h"

#include "turbo/unicode/scalar/valid_utf16_to_utf32.h"
#include "turbo/unicode/scalar/utf16_to_utf32.h"

#include "turbo/unicode/scalar/valid_utf32_to_utf8.h"
#include "turbo/unicode/scalar/utf32_to_utf8.h"

#include "turbo/unicode/scalar/valid_utf32_to_utf16.h"
#include "turbo/unicode/scalar/utf32_to_utf16.h"

#include "turbo/unicode/scalar/ascii.h"
#include "turbo/unicode/scalar/utf8.h"
#include "turbo/unicode/scalar/utf16.h"

namespace turbo {
namespace TURBO_UNICODE_IMPLEMENTATION {

TURBO_MUST_USE_RESULT int Implementation::DetectEncodings(const char * input, size_t length) const noexcept {
  // If there is a BOM, then we trust it.
  auto bom_encoding = turbo::BOM::check_bom(input, length);
  if(bom_encoding != EncodingType::unspecified) { return bom_encoding; }
  int out = 0;
  if(ValidateUtf8(input, length)) { out |= EncodingType::UTF8; }
  if((length % 2) == 0) {
    if(ValidateUtf16Le(reinterpret_cast<const char16_t*>(input), length/2)) { out |= EncodingType::UTF16_LE; }
  }
  if((length % 4) == 0) {
    if(ValidateUtf32(reinterpret_cast<const char32_t*>(input), length/4)) { out |= EncodingType::UTF32_LE; }
  }

  return out;
}

TURBO_MUST_USE_RESULT bool Implementation::ValidateUtf8(const char *buf, size_t len) const noexcept {
    return scalar::utf8::validate(buf, len);
}

TURBO_MUST_USE_RESULT result Implementation::ValidateUtf8WithErrors(const char *buf, size_t len) const noexcept {
    return scalar::utf8::validate_with_errors(buf, len);
}

TURBO_MUST_USE_RESULT bool Implementation::ValidateAscii(const char *buf, size_t len) const noexcept {
    return scalar::ascii::validate(buf, len);
}

TURBO_MUST_USE_RESULT result Implementation::ValidateAsciiWithErrors(const char *buf, size_t len) const noexcept {
    return scalar::ascii::validate_with_errors(buf, len);
}

TURBO_MUST_USE_RESULT bool Implementation::ValidateUtf16Le(const char16_t *buf, size_t len) const noexcept {
    return scalar::utf16::validate<EndianNess::SYS_LITTLE_ENDIAN>(buf, len);
}

TURBO_MUST_USE_RESULT bool Implementation::ValidateUtf16Be(const char16_t *buf, size_t len) const noexcept {
    return scalar::utf16::validate<EndianNess::SYS_BIG_ENDIAN>(buf, len);
}

TURBO_MUST_USE_RESULT result Implementation::ValidateUtf16LeWithErrors(const char16_t *buf, size_t len) const noexcept {
    return scalar::utf16::validate_with_errors<EndianNess::SYS_LITTLE_ENDIAN>(buf, len);
}

TURBO_MUST_USE_RESULT result Implementation::ValidateUtf16BeWithErrors(const char16_t *buf, size_t len) const noexcept {
    return scalar::utf16::validate_with_errors<EndianNess::SYS_BIG_ENDIAN>(buf, len);
}

TURBO_MUST_USE_RESULT bool Implementation::ValidateUtf32(const char32_t *buf, size_t len) const noexcept {
    return scalar::utf32::validate(buf, len);
}

TURBO_MUST_USE_RESULT result Implementation::ValidateUtf32WithErrors(const char32_t *buf, size_t len) const noexcept {
    return scalar::utf32::validate_with_errors(buf, len);
}

TURBO_MUST_USE_RESULT size_t Implementation::ConvertUtf8ToUtf16Le(const char* buf, size_t len, char16_t* utf16_output) const noexcept {
   return scalar::utf8_to_utf16::convert<EndianNess::SYS_LITTLE_ENDIAN>(buf, len, utf16_output);
}

TURBO_MUST_USE_RESULT size_t Implementation::ConvertUtf8ToUtf16Be(const char* buf, size_t len, char16_t* utf16_output) const noexcept {
   return scalar::utf8_to_utf16::convert<EndianNess::SYS_BIG_ENDIAN>(buf, len, utf16_output);
}

TURBO_MUST_USE_RESULT result Implementation::ConvertUtf8ToUtf16LeWithErrors(const char* buf, size_t len, char16_t* utf16_output) const noexcept {
   return scalar::utf8_to_utf16::convert_with_errors<EndianNess::SYS_LITTLE_ENDIAN>(buf, len, utf16_output);
}

TURBO_MUST_USE_RESULT result Implementation::ConvertUtf8ToUtf16BeWithErrors(const char* buf, size_t len, char16_t* utf16_output) const noexcept {
   return scalar::utf8_to_utf16::convert_with_errors<EndianNess::SYS_BIG_ENDIAN>(buf, len, utf16_output);
}

TURBO_MUST_USE_RESULT size_t Implementation::ConvertValidUtf8ToUtf16Le(const char* buf, size_t len, char16_t* utf16_output) const noexcept {
   return scalar::utf8_to_utf16::convert_valid<EndianNess::SYS_LITTLE_ENDIAN>(buf, len, utf16_output);
}

TURBO_MUST_USE_RESULT size_t Implementation::ConvertValidUtf8ToUtf16Be(const char* buf, size_t len, char16_t* utf16_output) const noexcept {
   return scalar::utf8_to_utf16::convert_valid<EndianNess::SYS_BIG_ENDIAN>(buf, len, utf16_output);
}

TURBO_MUST_USE_RESULT size_t Implementation::ConvertUtf8ToUtf32(const char* buf, size_t len, char32_t* utf32_output) const noexcept {
   return scalar::utf8_to_utf32::convert(buf, len, utf32_output);
}

TURBO_MUST_USE_RESULT result Implementation::ConvertUtf8ToUtf32WithErrors(const char* buf, size_t len, char32_t* utf32_output) const noexcept {
   return scalar::utf8_to_utf32::convert_with_errors(buf, len, utf32_output);
}

TURBO_MUST_USE_RESULT size_t Implementation::ConvertValidUtf8ToUtf32(const char* input, size_t size,
    char32_t* utf32_output) const noexcept {
  return scalar::utf8_to_utf32::convert_valid(input, size,  utf32_output);
}

TURBO_MUST_USE_RESULT size_t Implementation::ConvertUtf16LeToUtf8(const char16_t* buf, size_t len, char* utf8_output) const noexcept {
  return scalar::utf16_to_utf8::convert<EndianNess::SYS_LITTLE_ENDIAN>(buf, len, utf8_output);
}

TURBO_MUST_USE_RESULT size_t Implementation::ConvertUtf16BeToUtf8(const char16_t* buf, size_t len, char* utf8_output) const noexcept {
  return scalar::utf16_to_utf8::convert<EndianNess::SYS_BIG_ENDIAN>(buf, len, utf8_output);
}

TURBO_MUST_USE_RESULT result Implementation::ConvertUtf16LeToUtf8WithErrors(const char16_t* buf, size_t len, char* utf8_output) const noexcept {
  return scalar::utf16_to_utf8::convert_with_errors<EndianNess::SYS_LITTLE_ENDIAN>(buf, len, utf8_output);
}

TURBO_MUST_USE_RESULT result Implementation::ConvertUtf16BeToUtf8WithErrors(const char16_t* buf, size_t len, char* utf8_output) const noexcept {
  return scalar::utf16_to_utf8::convert_with_errors<EndianNess::SYS_BIG_ENDIAN>(buf, len, utf8_output);
}

TURBO_MUST_USE_RESULT size_t Implementation::ConvertValidUtf16LeToUtf8(const char16_t* buf, size_t len, char* utf8_output) const noexcept {
  return scalar::utf16_to_utf8::convert_valid<EndianNess::SYS_LITTLE_ENDIAN>(buf, len, utf8_output);
}

TURBO_MUST_USE_RESULT size_t Implementation::ConvertValidUtf16BeToUtf8(const char16_t* buf, size_t len, char* utf8_output) const noexcept {
  return scalar::utf16_to_utf8::convert_valid<EndianNess::SYS_BIG_ENDIAN>(buf, len, utf8_output);
}

TURBO_MUST_USE_RESULT size_t Implementation::ConvertUtf32ToUtf8(const char32_t* buf, size_t len, char* utf8_output) const noexcept {
  return scalar::utf32_to_utf8::convert(buf, len, utf8_output);
}

TURBO_MUST_USE_RESULT result Implementation::ConvertUtf32ToUtf8WithErrors(const char32_t* buf, size_t len, char* utf8_output) const noexcept {
  return scalar::utf32_to_utf8::convert_with_errors(buf, len, utf8_output);
}

TURBO_MUST_USE_RESULT size_t Implementation::ConvertValidUtf32ToUtf8(const char32_t* buf, size_t len, char* utf8_output) const noexcept {
  return scalar::utf32_to_utf8::convert_valid(buf, len, utf8_output);
}

TURBO_MUST_USE_RESULT size_t Implementation::ConvertUtf32ToUtf16Le(const char32_t* buf, size_t len, char16_t* utf16_output) const noexcept {
  return scalar::utf32_to_utf16::convert<EndianNess::SYS_LITTLE_ENDIAN>(buf, len, utf16_output);
}

TURBO_MUST_USE_RESULT size_t Implementation::ConvertUtf32ToUtf16Be(const char32_t* buf, size_t len, char16_t* utf16_output) const noexcept {
  return scalar::utf32_to_utf16::convert<EndianNess::SYS_BIG_ENDIAN>(buf, len, utf16_output);
}

TURBO_MUST_USE_RESULT result Implementation::ConvertUtf32ToUtf16leWithErrors(const char32_t* buf, size_t len, char16_t* utf16_output) const noexcept {
  return scalar::utf32_to_utf16::convert_with_errors<EndianNess::SYS_LITTLE_ENDIAN>(buf, len, utf16_output);
}

TURBO_MUST_USE_RESULT result Implementation::ConvertUtf32ToUtf16BeWithErrors(const char32_t* buf, size_t len, char16_t* utf16_output) const noexcept {
  return scalar::utf32_to_utf16::convert_with_errors<EndianNess::SYS_BIG_ENDIAN>(buf, len, utf16_output);
}

TURBO_MUST_USE_RESULT size_t Implementation::ConvertValidUtf32ToUtf16Le(const char32_t* buf, size_t len, char16_t* utf16_output) const noexcept {
  return scalar::utf32_to_utf16::convert_valid<EndianNess::SYS_LITTLE_ENDIAN>(buf, len, utf16_output);
}

TURBO_MUST_USE_RESULT size_t Implementation::ConvertValidUtf32ToUtf16Be(const char32_t* buf, size_t len, char16_t* utf16_output) const noexcept {
  return scalar::utf32_to_utf16::convert_valid<EndianNess::SYS_BIG_ENDIAN>(buf, len, utf16_output);
}

TURBO_MUST_USE_RESULT size_t Implementation::ConvertUtf16LeToUtf32(const char16_t* buf, size_t len, char32_t* utf32_output) const noexcept {
  return scalar::utf16_to_utf32::convert<EndianNess::SYS_LITTLE_ENDIAN>(buf, len, utf32_output);
}

TURBO_MUST_USE_RESULT size_t Implementation::ConvertUtf16BeToUtf32(const char16_t* buf, size_t len, char32_t* utf32_output) const noexcept {
  return scalar::utf16_to_utf32::convert<EndianNess::SYS_BIG_ENDIAN>(buf, len, utf32_output);
}

TURBO_MUST_USE_RESULT result Implementation::ConvertUtf16LeToUtf32WithErrors(const char16_t* buf, size_t len, char32_t* utf32_output) const noexcept {
  return scalar::utf16_to_utf32::convert_with_errors<EndianNess::SYS_LITTLE_ENDIAN>(buf, len, utf32_output);
}

TURBO_MUST_USE_RESULT result Implementation::ConvertUtf16BeToUtf32WithErrors(const char16_t* buf, size_t len, char32_t* utf32_output) const noexcept {
  return scalar::utf16_to_utf32::convert_with_errors<EndianNess::SYS_BIG_ENDIAN>(buf, len, utf32_output);
}

TURBO_MUST_USE_RESULT size_t Implementation::ConvertValidUtf16LeToUtf32(const char16_t* buf, size_t len, char32_t* utf32_output) const noexcept {
  return scalar::utf16_to_utf32::convert_valid<EndianNess::SYS_LITTLE_ENDIAN>(buf, len, utf32_output);
}

TURBO_MUST_USE_RESULT size_t Implementation::ConvertValidUtf16BeToUtf32(const char16_t* buf, size_t len, char32_t* utf32_output) const noexcept {
  return scalar::utf16_to_utf32::convert_valid<EndianNess::SYS_BIG_ENDIAN>(buf, len, utf32_output);
}

void Implementation::ChangeEndiannessUtf16(const char16_t * input, size_t length, char16_t * output) const noexcept {
  scalar::utf16::ChangeEndiannessUtf16(input, length, output);
}

TURBO_MUST_USE_RESULT size_t Implementation::CountUtf16Le(const char16_t * input, size_t length) const noexcept {
  return scalar::utf16::count_code_points<EndianNess::SYS_LITTLE_ENDIAN>(input, length);
}

TURBO_MUST_USE_RESULT size_t Implementation::CountUtf16Be(const char16_t * input, size_t length) const noexcept {
  return scalar::utf16::count_code_points<EndianNess::SYS_BIG_ENDIAN>(input, length);
}

TURBO_MUST_USE_RESULT size_t Implementation::CountUtf8(const char * input, size_t length) const noexcept {
  return scalar::utf8::count_code_points(input, length);
}

TURBO_MUST_USE_RESULT size_t Implementation::Utf8LengthFromUtf16Le(const char16_t * input, size_t length) const noexcept {
  return scalar::utf16::Utf8LengthFromUtf16<EndianNess::SYS_LITTLE_ENDIAN>(input, length);
}

TURBO_MUST_USE_RESULT size_t Implementation::Utf8LengthFromUtf16be(const char16_t * input, size_t length) const noexcept {
  return scalar::utf16::Utf8LengthFromUtf16<EndianNess::SYS_BIG_ENDIAN>(input, length);
}

TURBO_MUST_USE_RESULT size_t Implementation::Utf32LengthFromUtf16Le(const char16_t * input, size_t length) const noexcept {
  return scalar::utf16::Utf32LengthFromUtf16<EndianNess::SYS_LITTLE_ENDIAN>(input, length);
}

TURBO_MUST_USE_RESULT size_t Implementation::Utf32LengthFromUtf16Be(const char16_t * input, size_t length) const noexcept {
  return scalar::utf16::Utf32LengthFromUtf16<EndianNess::SYS_BIG_ENDIAN>(input, length);
}

TURBO_MUST_USE_RESULT size_t Implementation::Utf16LengthFromUtf8(const char * input, size_t length) const noexcept {
  return scalar::utf8::Utf16LengthFromUtf8(input, length);
}

TURBO_MUST_USE_RESULT size_t Implementation::Utf8LengthFromUtf32(const char32_t * input, size_t length) const noexcept {
  return scalar::utf32::Utf8LengthFromUtf32(input, length);
}

TURBO_MUST_USE_RESULT size_t Implementation::Utf16LengthFromUtf32(const char32_t * input, size_t length) const noexcept {
  return scalar::utf32::Utf16LengthFromUtf32(input, length);
}

TURBO_MUST_USE_RESULT size_t Implementation::Utf32LengthFromUtf8(const char * input, size_t length) const noexcept {
  return scalar::utf8::count_code_points(input, length);
}

} // namespace TURBO_UNICODE_IMPLEMENTATION
} // namespace turbo

#include "turbo/unicode/fallback/end.h"
