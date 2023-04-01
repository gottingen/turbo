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

#ifndef TURBO_UNICODE_HASWELL_IMPLEMENTATION_H_
#define TURBO_UNICODE_HASWELL_IMPLEMENTATION_H_

#include "turbo/unicode/implementation.h"

// The constructor may be executed on any host, so we take care not to use TURBO_TARGET_REGION
namespace turbo {
namespace haswell {

using namespace turbo;

class Implementation final : public turbo::Implementation {
public:
  TURBO_FORCE_INLINE Implementation() : turbo::Implementation(
      "haswell",
      "Intel/AMD AVX2",
      internal::instruction_set::AVX2 | internal::instruction_set::PCLMULQDQ | internal::instruction_set::BMI1 | internal::instruction_set::BMI2
  ) {}
  TURBO_MUST_USE_RESULT int DetectEncodings(const char * input, size_t length) const noexcept final;
  TURBO_MUST_USE_RESULT bool ValidateUtf8(const char *buf, size_t len) const noexcept final;
  TURBO_MUST_USE_RESULT result ValidateUtf8WithErrors(const char *buf, size_t len) const noexcept final;
  TURBO_MUST_USE_RESULT bool ValidateAscii(const char *buf, size_t len) const noexcept final;
  TURBO_MUST_USE_RESULT result ValidateAsciiWithErrors(const char *buf, size_t len) const noexcept final;
  TURBO_MUST_USE_RESULT bool ValidateUtf16Le(const char16_t *buf, size_t len) const noexcept final;
  TURBO_MUST_USE_RESULT bool ValidateUtf16Be(const char16_t *buf, size_t len) const noexcept final;
  TURBO_MUST_USE_RESULT result ValidateUtf16LeWithErrors(const char16_t *buf, size_t len) const noexcept final;
  TURBO_MUST_USE_RESULT result ValidateUtf16BeWithErrors(const char16_t *buf, size_t len) const noexcept final;
  TURBO_MUST_USE_RESULT bool ValidateUtf32(const char32_t *buf, size_t len) const noexcept final;
  TURBO_MUST_USE_RESULT result ValidateUtf32WithErrors(const char32_t *buf, size_t len) const noexcept final;
  TURBO_MUST_USE_RESULT size_t ConvertUtf8ToUtf16Le(const char * buf, size_t len, char16_t* utf16_output) const noexcept final;
  TURBO_MUST_USE_RESULT size_t ConvertUtf8ToUtf16Be(const char * buf, size_t len, char16_t* utf16_output) const noexcept final;
  TURBO_MUST_USE_RESULT result ConvertUtf8ToUtf16LeWithErrors(const char * buf, size_t len, char16_t* utf16_output) const noexcept final;
  TURBO_MUST_USE_RESULT result ConvertUtf8ToUtf16BeWithErrors(const char * buf, size_t len, char16_t* utf16_output) const noexcept final;
  TURBO_MUST_USE_RESULT size_t ConvertValidUtf8ToUtf16Le(const char * buf, size_t len, char16_t* utf16_buffer) const noexcept final;
  TURBO_MUST_USE_RESULT size_t ConvertValidUtf8ToUtf16Be(const char * buf, size_t len, char16_t* utf16_buffer) const noexcept final;
  TURBO_MUST_USE_RESULT size_t ConvertUtf8ToUtf32(const char * buf, size_t len, char32_t* utf32_output) const noexcept final;
  TURBO_MUST_USE_RESULT result ConvertUtf8ToUtf32WithErrors(const char * buf, size_t len, char32_t* utf32_output) const noexcept final;
  TURBO_MUST_USE_RESULT size_t ConvertValidUtf8ToUtf32(const char * buf, size_t len, char32_t* utf32_buffer) const noexcept final;
  TURBO_MUST_USE_RESULT size_t ConvertUtf16LeToUtf8(const char16_t * buf, size_t len, char* utf8_buffer) const noexcept final;
  TURBO_MUST_USE_RESULT size_t ConvertUtf16BeToUtf8(const char16_t * buf, size_t len, char* utf8_buffer) const noexcept final;
  TURBO_MUST_USE_RESULT result ConvertUtf16LeToUtf8WithErrors(const char16_t * buf, size_t len, char* utf8_buffer) const noexcept final;
  TURBO_MUST_USE_RESULT result ConvertUtf16BeToUtf8WithErrors(const char16_t * buf, size_t len, char* utf8_buffer) const noexcept final;
  TURBO_MUST_USE_RESULT size_t ConvertValidUtf16LeToUtf8(const char16_t * buf, size_t len, char* utf8_buffer) const noexcept final;
  TURBO_MUST_USE_RESULT size_t ConvertValidUtf16BeToUtf8(const char16_t * buf, size_t len, char* utf8_buffer) const noexcept final;
  TURBO_MUST_USE_RESULT size_t ConvertUtf32ToUtf8(const char32_t * buf, size_t len, char* utf8_buffer) const noexcept final;
  TURBO_MUST_USE_RESULT result ConvertUtf32ToUtf8WithErrors(const char32_t * buf, size_t len, char* utf8_buffer) const noexcept final;
  TURBO_MUST_USE_RESULT size_t ConvertValidUtf32ToUtf8(const char32_t * buf, size_t len, char* utf8_buffer) const noexcept final;
  TURBO_MUST_USE_RESULT size_t ConvertUtf32ToUtf16Le(const char32_t * buf, size_t len, char16_t* utf16_buffer) const noexcept final;
  TURBO_MUST_USE_RESULT size_t ConvertUtf32ToUtf16Be(const char32_t * buf, size_t len, char16_t* utf16_buffer) const noexcept final;
  TURBO_MUST_USE_RESULT result ConvertUtf32ToUtf16leWithErrors(const char32_t * buf, size_t len, char16_t* utf16_buffer) const noexcept final;
  TURBO_MUST_USE_RESULT result ConvertUtf32ToUtf16BeWithErrors(const char32_t * buf, size_t len, char16_t* utf16_buffer) const noexcept final;
  TURBO_MUST_USE_RESULT size_t ConvertValidUtf32ToUtf16Le(const char32_t * buf, size_t len, char16_t* utf16_buffer) const noexcept final;
  TURBO_MUST_USE_RESULT size_t ConvertValidUtf32ToUtf16Be(const char32_t * buf, size_t len, char16_t* utf16_buffer) const noexcept final;
  TURBO_MUST_USE_RESULT size_t ConvertUtf16LeToUtf32(const char16_t * buf, size_t len, char32_t* utf32_buffer) const noexcept final;
  TURBO_MUST_USE_RESULT size_t ConvertUtf16BeToUtf32(const char16_t * buf, size_t len, char32_t* utf32_buffer) const noexcept final;
  TURBO_MUST_USE_RESULT result ConvertUtf16LeToUtf32WithErrors(const char16_t * buf, size_t len, char32_t* utf32_buffer) const noexcept final;
  TURBO_MUST_USE_RESULT result ConvertUtf16BeToUtf32WithErrors(const char16_t * buf, size_t len, char32_t* utf32_buffer) const noexcept final;
  TURBO_MUST_USE_RESULT size_t ConvertValidUtf16LeToUtf32(const char16_t * buf, size_t len, char32_t* utf32_buffer) const noexcept final;
  TURBO_MUST_USE_RESULT size_t ConvertValidUtf16BeToUtf32(const char16_t * buf, size_t len, char32_t* utf32_buffer) const noexcept final;
  void ChangeEndiannessUtf16(const char16_t * buf, size_t length, char16_t * output) const noexcept final;
  TURBO_MUST_USE_RESULT size_t CountUtf16Le(const char16_t * buf, size_t length) const noexcept;
  TURBO_MUST_USE_RESULT size_t CountUtf16Be(const char16_t * buf, size_t length) const noexcept;
  TURBO_MUST_USE_RESULT size_t CountUtf8(const char * buf, size_t length) const noexcept;
  TURBO_MUST_USE_RESULT size_t Utf8LengthFromUtf16Le(const char16_t * input, size_t length) const noexcept;
  TURBO_MUST_USE_RESULT size_t Utf8LengthFromUtf16be(const char16_t * input, size_t length) const noexcept;
  TURBO_MUST_USE_RESULT size_t Utf32LengthFromUtf16Le(const char16_t * input, size_t length) const noexcept;
  TURBO_MUST_USE_RESULT size_t Utf32LengthFromUtf16Be(const char16_t * input, size_t length) const noexcept;
  TURBO_MUST_USE_RESULT size_t Utf16LengthFromUtf8(const char * input, size_t length) const noexcept;
  TURBO_MUST_USE_RESULT size_t Utf8LengthFromUtf32(const char32_t * input, size_t length) const noexcept;
  TURBO_MUST_USE_RESULT size_t Utf16LengthFromUtf32(const char32_t * input, size_t length) const noexcept;
  TURBO_MUST_USE_RESULT size_t Utf32LengthFromUtf8(const char * input, size_t length) const noexcept;
};

} // namespace haswell
} // namespace turbo

#endif // TURBO_UNICODE_HASWELL_IMPLEMENTATION_H_
