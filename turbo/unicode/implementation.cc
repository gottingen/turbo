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

#include "turbo/unicode/utf.h"
#include <initializer_list>
#include <climits>

// Useful for debugging purposes
namespace turbo {
namespace {

template <typename T>
std::string toBinaryString(T b) {
   std::string binary = "";
   T mask = T(1) << (sizeof(T) * CHAR_BIT - 1);
   while (mask > 0) {
    binary += ((b & mask) == 0) ? '0' : '1';
    mask >>= 1;
  }
  return binary;
}
}
}

// Implementations
// The best choice should always come first!
#include "turbo/unicode/arm64.h"
#include "turbo/unicode/icelake.h"
#include "turbo/unicode/haswell.h"
#include "turbo/unicode/westmere.h"
#include "turbo/unicode/ppc64.h"
#include "turbo/unicode/fallback.h" // have it always last.

namespace turbo {
bool Implementation::supported_by_runtime_system() const {
  uint32_t required_instruction_sets = this->RequiredInstructionSets();
  uint32_t supported_instruction_sets = internal::detect_supported_architectures();
  return ((supported_instruction_sets & required_instruction_sets) == required_instruction_sets);
}

TURBO_MUST_USE_RESULT EncodingType Implementation::AutodetectEncoding(const char * input, size_t length) const noexcept {
    // If there is a BOM, then we trust it.
    auto bom_encoding = turbo::BOM::check_bom(input, length);
    if(bom_encoding != EncodingType::unspecified) { return bom_encoding; }
    // UTF8 is common, it includes ASCII, and is commonly represented
    // without a BOM, so if it fits, go with that. Note that it is still
    // possible to get it wrong, we are only 'guessing'. If some has UTF-16
    // data without a BOM, it could pass as UTF-8.
    //
    // An interesting twist might be to check for UTF-16 ASCII first (every
    // other byte is zero).
    if(ValidateUtf8(input, length)) { return EncodingType::UTF8; }
    // The next most common encoding that might appear without BOM is probably
    // UTF-16LE, so try that next.
    if((length % 2) == 0) {
      // important: we need to divide by two
      if(ValidateUtf16Le(reinterpret_cast<const char16_t*>(input), length/2)) { return EncodingType::UTF16_LE; }
    }
    if((length % 4) == 0) {
      if(ValidateUtf32(reinterpret_cast<const char32_t*>(input), length/4)) { return EncodingType::UTF32_LE; }
    }
    return EncodingType::unspecified;
}

namespace internal {

// Static array of known implementations. We're hoping these get baked into the executable
// without requiring a static initializer.


#if TURBO_UNICODE_IMPLEMENTATION_ICELAKE
const icelake::implementation icelake_singleton{};
#endif
#if TURBO_UNICODE_IMPLEMENTATION_HASWELL
const haswell::implementation haswell_singleton{};
#endif
#if TURBO_UNICODE_IMPLEMENTATION_WESTMERE
const westmere::implementation westmere_singleton{};
#endif
#if TURBO_UNICODE_IMPLEMENTATION_ARM64
const arm64::Implementation arm64_singleton{};
#endif
#if TURBO_UNICODE_IMPLEMENTATION_PPC64
const ppc64::implementation ppc64_singleton{};
#endif
#if TURBO_UNICODE_IMPLEMENTATION_FALLBACK
const fallback::implementation fallback_singleton{};
#endif

/**
 * @private Detects best supported implementation on first use, and sets it
 */
class detect_best_supported_implementation_on_first_use final : public Implementation {
public:
  const std::string &Name() const noexcept final { return set_best()->Name(); }
  const std::string &Description() const noexcept final { return set_best()->Description(); }
  uint32_t RequiredInstructionSets() const noexcept final { return set_best()->RequiredInstructionSets(); }

  TURBO_MUST_USE_RESULT int DetectEncodings(const char * input, size_t length) const noexcept override {
    return set_best()->DetectEncodings(input, length);
  }

  TURBO_MUST_USE_RESULT bool ValidateUtf8(const char * buf, size_t len) const noexcept final override {
    return set_best()->ValidateUtf8(buf, len);
  }

  TURBO_MUST_USE_RESULT result ValidateUtf8WithErrors(const char * buf, size_t len) const noexcept final override {
    return set_best()->ValidateUtf8WithErrors(buf, len);
  }

  TURBO_MUST_USE_RESULT bool ValidateAscii(const char * buf, size_t len) const noexcept final override {
    return set_best()->ValidateAscii(buf, len);
  }

  TURBO_MUST_USE_RESULT result ValidateAsciiWithErrors(const char * buf, size_t len) const noexcept final override {
    return set_best()->ValidateAsciiWithErrors(buf, len);
  }

  TURBO_MUST_USE_RESULT bool ValidateUtf16Le(const char16_t * buf, size_t len) const noexcept final override {
    return set_best()->ValidateUtf16Le(buf, len);
  }

  TURBO_MUST_USE_RESULT bool ValidateUtf16Be(const char16_t * buf, size_t len) const noexcept final override {
    return set_best()->ValidateUtf16Be(buf, len);
  }

  TURBO_MUST_USE_RESULT result ValidateUtf16LeWithErrors(const char16_t * buf, size_t len) const noexcept final override {
    return set_best()->ValidateUtf16LeWithErrors(buf, len);
  }

  TURBO_MUST_USE_RESULT result ValidateUtf16BeWithErrors(const char16_t * buf, size_t len) const noexcept final override {
    return set_best()->ValidateUtf16BeWithErrors(buf, len);
  }

  TURBO_MUST_USE_RESULT bool ValidateUtf32(const char32_t * buf, size_t len) const noexcept final override {
    return set_best()->ValidateUtf32(buf, len);
  }

  TURBO_MUST_USE_RESULT result ValidateUtf32WithErrors(const char32_t * buf, size_t len) const noexcept final override {
    return set_best()->ValidateUtf32WithErrors(buf, len);
  }

  TURBO_MUST_USE_RESULT size_t ConvertUtf8ToUtf16Le(const char * buf, size_t len, char16_t* utf16_output) const noexcept final override {
    return set_best()->ConvertUtf8ToUtf16Le(buf, len, utf16_output);
  }

  TURBO_MUST_USE_RESULT size_t ConvertUtf8ToUtf16Be(const char * buf, size_t len, char16_t* utf16_output) const noexcept final override {
    return set_best()->ConvertUtf8ToUtf16Be(buf, len, utf16_output);
  }

  TURBO_MUST_USE_RESULT result ConvertUtf8ToUtf16LeWithErrors(const char * buf, size_t len, char16_t* utf16_output) const noexcept final override {
    return set_best()->ConvertUtf8ToUtf16LeWithErrors(buf, len, utf16_output);
  }

  TURBO_MUST_USE_RESULT result ConvertUtf8ToUtf16BeWithErrors(const char * buf, size_t len, char16_t* utf16_output) const noexcept final override {
    return set_best()->ConvertUtf8ToUtf16BeWithErrors(buf, len, utf16_output);
  }

  TURBO_MUST_USE_RESULT size_t ConvertValidUtf8ToUtf16Le(const char * buf, size_t len, char16_t* utf16_output) const noexcept final override {
    return set_best()->ConvertValidUtf8ToUtf16Le(buf, len, utf16_output);
  }

  TURBO_MUST_USE_RESULT size_t ConvertValidUtf8ToUtf16Be(const char * buf, size_t len, char16_t* utf16_output) const noexcept final override {
    return set_best()->ConvertValidUtf8ToUtf16Be(buf, len, utf16_output);
  }

  TURBO_MUST_USE_RESULT size_t ConvertUtf8ToUtf32(const char * buf, size_t len, char32_t* utf32_output) const noexcept final override {
    return set_best()->ConvertUtf8ToUtf32(buf, len, utf32_output);
  }

  TURBO_MUST_USE_RESULT result ConvertUtf8ToUtf32WithErrors(const char * buf, size_t len, char32_t* utf32_output) const noexcept final override {
    return set_best()->ConvertUtf8ToUtf32WithErrors(buf, len, utf32_output);
  }

  TURBO_MUST_USE_RESULT size_t ConvertValidUtf8ToUtf32(const char * buf, size_t len, char32_t* utf32_output) const noexcept final override {
    return set_best()->ConvertValidUtf8ToUtf32(buf, len, utf32_output);
  }

  TURBO_MUST_USE_RESULT size_t ConvertUtf16LeToUtf8(const char16_t * buf, size_t len, char* utf8_output) const noexcept final override {
    return set_best()->ConvertUtf16LeToUtf8(buf, len, utf8_output);
  }

  TURBO_MUST_USE_RESULT size_t ConvertUtf16BeToUtf8(const char16_t * buf, size_t len, char* utf8_output) const noexcept final override {
    return set_best()->ConvertUtf16BeToUtf8(buf, len, utf8_output);
  }

  TURBO_MUST_USE_RESULT result ConvertUtf16LeToUtf8WithErrors(const char16_t * buf, size_t len, char* utf8_output) const noexcept final override {
    return set_best()->ConvertUtf16LeToUtf8WithErrors(buf, len, utf8_output);
  }

  TURBO_MUST_USE_RESULT result ConvertUtf16BeToUtf8WithErrors(const char16_t * buf, size_t len, char* utf8_output) const noexcept final override {
    return set_best()->ConvertUtf16BeToUtf8WithErrors(buf, len, utf8_output);
  }

  TURBO_MUST_USE_RESULT size_t ConvertValidUtf16LeToUtf8(const char16_t * buf, size_t len, char* utf8_output) const noexcept final override {
    return set_best()->ConvertValidUtf16LeToUtf8(buf, len, utf8_output);
  }

  TURBO_MUST_USE_RESULT size_t ConvertValidUtf16BeToUtf8(const char16_t * buf, size_t len, char* utf8_output) const noexcept final override {
    return set_best()->ConvertValidUtf16BeToUtf8(buf, len, utf8_output);
  }

  TURBO_MUST_USE_RESULT size_t ConvertUtf32ToUtf8(const char32_t * buf, size_t len, char* utf8_output) const noexcept final override {
    return set_best()->ConvertUtf32ToUtf8(buf, len, utf8_output);
  }

  TURBO_MUST_USE_RESULT result ConvertUtf32ToUtf8WithErrors(const char32_t * buf, size_t len, char* utf8_output) const noexcept final override {
    return set_best()->ConvertUtf32ToUtf8WithErrors(buf, len, utf8_output);
  }

  TURBO_MUST_USE_RESULT size_t ConvertValidUtf32ToUtf8(const char32_t * buf, size_t len, char* utf8_output) const noexcept final override {
    return set_best()->ConvertValidUtf32ToUtf8(buf, len, utf8_output);
  }

  TURBO_MUST_USE_RESULT size_t ConvertUtf32ToUtf16Le(const char32_t * buf, size_t len, char16_t* utf16_output) const noexcept final override {
    return set_best()->ConvertUtf32ToUtf16Le(buf, len, utf16_output);
  }

  TURBO_MUST_USE_RESULT size_t ConvertUtf32ToUtf16Be(const char32_t * buf, size_t len, char16_t* utf16_output) const noexcept final override {
    return set_best()->ConvertUtf32ToUtf16Be(buf, len, utf16_output);
  }

  TURBO_MUST_USE_RESULT result ConvertUtf32ToUtf16leWithErrors(const char32_t * buf, size_t len, char16_t* utf16_output) const noexcept final override {
    return set_best()->ConvertUtf32ToUtf16leWithErrors(buf, len, utf16_output);
  }

  TURBO_MUST_USE_RESULT result ConvertUtf32ToUtf16BeWithErrors(const char32_t * buf, size_t len, char16_t* utf16_output) const noexcept final override {
    return set_best()->ConvertUtf32ToUtf16BeWithErrors(buf, len, utf16_output);
  }

  TURBO_MUST_USE_RESULT size_t ConvertValidUtf32ToUtf16Le(const char32_t * buf, size_t len, char16_t* utf16_output) const noexcept final override {
    return set_best()->ConvertValidUtf32ToUtf16Le(buf, len, utf16_output);
  }

  TURBO_MUST_USE_RESULT size_t ConvertValidUtf32ToUtf16Be(const char32_t * buf, size_t len, char16_t* utf16_output) const noexcept final override {
    return set_best()->ConvertValidUtf32ToUtf16Be(buf, len, utf16_output);
  }

  TURBO_MUST_USE_RESULT size_t ConvertUtf16LeToUtf32(const char16_t * buf, size_t len, char32_t* utf32_output) const noexcept final override {
    return set_best()->ConvertUtf16LeToUtf32(buf, len, utf32_output);
  }

  TURBO_MUST_USE_RESULT size_t ConvertUtf16BeToUtf32(const char16_t * buf, size_t len, char32_t* utf32_output) const noexcept final override {
    return set_best()->ConvertUtf16BeToUtf32(buf, len, utf32_output);
  }

  TURBO_MUST_USE_RESULT result ConvertUtf16LeToUtf32WithErrors(const char16_t * buf, size_t len, char32_t* utf32_output) const noexcept final override {
    return set_best()->ConvertUtf16LeToUtf32WithErrors(buf, len, utf32_output);
  }

  TURBO_MUST_USE_RESULT result ConvertUtf16BeToUtf32WithErrors(const char16_t * buf, size_t len, char32_t* utf32_output) const noexcept final override {
    return set_best()->ConvertUtf16BeToUtf32WithErrors(buf, len, utf32_output);
  }

  TURBO_MUST_USE_RESULT size_t ConvertValidUtf16LeToUtf32(const char16_t * buf, size_t len, char32_t* utf32_output) const noexcept final override {
    return set_best()->ConvertValidUtf16LeToUtf32(buf, len, utf32_output);
  }

  TURBO_MUST_USE_RESULT size_t ConvertValidUtf16BeToUtf32(const char16_t * buf, size_t len, char32_t* utf32_output) const noexcept final override {
    return set_best()->ConvertValidUtf16BeToUtf32(buf, len, utf32_output);
  }

  void ChangeEndiannessUtf16(const char16_t * buf, size_t len, char16_t * output) const noexcept final override {
    set_best()->ChangeEndiannessUtf16(buf, len, output);
  }

  TURBO_MUST_USE_RESULT size_t CountUtf16Le(const char16_t * buf, size_t len) const noexcept final override {
    return set_best()->CountUtf16Le(buf, len);
  }

  TURBO_MUST_USE_RESULT size_t CountUtf16Be(const char16_t * buf, size_t len) const noexcept final override {
    return set_best()->CountUtf16Be(buf, len);
  }

  TURBO_MUST_USE_RESULT size_t CountUtf8(const char * buf, size_t len) const noexcept final override {
    return set_best()->CountUtf8(buf, len);
  }

  TURBO_MUST_USE_RESULT size_t Utf8LengthFromUtf16Le(const char16_t * buf, size_t len) const noexcept override {
    return set_best()->Utf8LengthFromUtf16Le(buf, len);
  }

  TURBO_MUST_USE_RESULT size_t Utf8LengthFromUtf16be(const char16_t * buf, size_t len) const noexcept override {
    return set_best()->Utf8LengthFromUtf16be(buf, len);
  }

  TURBO_MUST_USE_RESULT size_t Utf32LengthFromUtf16Le(const char16_t * buf, size_t len) const noexcept override {
    return set_best()->Utf32LengthFromUtf16Le(buf, len);
  }

  TURBO_MUST_USE_RESULT size_t Utf32LengthFromUtf16Be(const char16_t * buf, size_t len) const noexcept override {
    return set_best()->Utf32LengthFromUtf16Be(buf, len);
  }

  TURBO_MUST_USE_RESULT size_t Utf16LengthFromUtf8(const char * buf, size_t len) const noexcept override {
    return set_best()->Utf16LengthFromUtf8(buf, len);
  }

  TURBO_MUST_USE_RESULT size_t Utf8LengthFromUtf32(const char32_t * buf, size_t len) const noexcept override {
    return set_best()->Utf8LengthFromUtf32(buf, len);
  }

  TURBO_MUST_USE_RESULT size_t Utf16LengthFromUtf32(const char32_t * buf, size_t len) const noexcept override {
    return set_best()->Utf16LengthFromUtf32(buf, len);
  }

  TURBO_MUST_USE_RESULT size_t Utf32LengthFromUtf8(const char * buf, size_t len) const noexcept override {
    return set_best()->Utf32LengthFromUtf8(buf, len);
  }

  TURBO_FORCE_INLINE detect_best_supported_implementation_on_first_use() noexcept : Implementation("best_supported_detector", "Detects the best supported implementation and sets it", 0) {}

private:
  const Implementation *set_best() const noexcept;
};


const std::initializer_list<const Implementation *> available_implementation_pointers {
#if TURBO_UNICODE_IMPLEMENTATION_ICELAKE
  &icelake_singleton,
#endif
#if TURBO_UNICODE_IMPLEMENTATION_HASWELL
  &haswell_singleton,
#endif
#if TURBO_UNICODE_IMPLEMENTATION_WESTMERE
  &westmere_singleton,
#endif
#if TURBO_UNICODE_IMPLEMENTATION_ARM64
  &arm64_singleton,
#endif
#if TURBO_UNICODE_IMPLEMENTATION_PPC64
  &ppc64_singleton,
#endif
#if TURBO_UNICODE_IMPLEMENTATION_FALLBACK
  &fallback_singleton,
#endif
}; // available_implementation_pointers

// So we can return UNSUPPORTED_ARCHITECTURE from the parser when there is no support
class unsupported_implementation final : public Implementation {
public:
  TURBO_MUST_USE_RESULT int DetectEncodings(const char *, size_t) const noexcept override {
    return EncodingType::unspecified;
  }

  TURBO_MUST_USE_RESULT bool ValidateUtf8(const char *, size_t) const noexcept final override {
    return false; // Just refuse to validate. Given that we have a fallback implementation
    // it seems unlikely that unsupported_implementation will ever be used. If it is used,
    // then it will flag all strings as invalid. The alternative is to return an error_code
    // from which the user has to figure out whether the string is valid UTF-8... which seems
    // like a lot of work just to handle the very unlikely case that we have an unsupported
    // implementation. And, when it does happen (that we have an unsupported implementation),
    // what are the chances that the programmer has a fallback? Given that *we* provide the
    // fallback, it implies that the programmer would need a fallback for our fallback.
  }

  TURBO_MUST_USE_RESULT result ValidateUtf8WithErrors(const char *, size_t) const noexcept final override {
    return result(error_code::OTHER, 0);
  }

  TURBO_MUST_USE_RESULT bool ValidateAscii(const char *, size_t) const noexcept final override {
    return false;
  }

  TURBO_MUST_USE_RESULT result ValidateAsciiWithErrors(const char *, size_t) const noexcept final override {
    return result(error_code::OTHER, 0);
  }

  TURBO_MUST_USE_RESULT bool ValidateUtf16Le(const char16_t*, size_t) const noexcept final override {
    return false;
  }

  TURBO_MUST_USE_RESULT bool ValidateUtf16Be(const char16_t*, size_t) const noexcept final override {
    return false;
  }

  TURBO_MUST_USE_RESULT result ValidateUtf16LeWithErrors(const char16_t*, size_t) const noexcept final override {
    return result(error_code::OTHER, 0);
  }

  TURBO_MUST_USE_RESULT result ValidateUtf16BeWithErrors(const char16_t*, size_t) const noexcept final override {
    return result(error_code::OTHER, 0);
  }

  TURBO_MUST_USE_RESULT bool ValidateUtf32(const char32_t*, size_t) const noexcept final override {
    return false;
  }

  TURBO_MUST_USE_RESULT result ValidateUtf32WithErrors(const char32_t*, size_t) const noexcept final override {
    return result(error_code::OTHER, 0);
  }

  TURBO_MUST_USE_RESULT size_t ConvertUtf8ToUtf16Le(const char*, size_t, char16_t*) const noexcept final override {
    return 0;
  }

  TURBO_MUST_USE_RESULT size_t ConvertUtf8ToUtf16Be(const char*, size_t, char16_t*) const noexcept final override {
    return 0;
  }

  TURBO_MUST_USE_RESULT result ConvertUtf8ToUtf16LeWithErrors(const char*, size_t, char16_t*) const noexcept final override {
    return result(error_code::OTHER, 0);
  }

  TURBO_MUST_USE_RESULT result ConvertUtf8ToUtf16BeWithErrors(const char*, size_t, char16_t*) const noexcept final override {
    return result(error_code::OTHER, 0);
  }

  TURBO_MUST_USE_RESULT size_t ConvertValidUtf8ToUtf16Le(const char*, size_t, char16_t*) const noexcept final override {
    return 0;
  }

  TURBO_MUST_USE_RESULT size_t ConvertValidUtf8ToUtf16Be(const char*, size_t, char16_t*) const noexcept final override {
    return 0;
  }

  TURBO_MUST_USE_RESULT size_t ConvertUtf8ToUtf32(const char*, size_t, char32_t*) const noexcept final override {
    return 0;
  }

  TURBO_MUST_USE_RESULT result ConvertUtf8ToUtf32WithErrors(const char*, size_t, char32_t*) const noexcept final override {
    return result(error_code::OTHER, 0);
  }

  TURBO_MUST_USE_RESULT size_t ConvertValidUtf8ToUtf32(const char*, size_t, char32_t*) const noexcept final override {
    return 0;
  }

  TURBO_MUST_USE_RESULT size_t ConvertUtf16LeToUtf8(const char16_t*, size_t, char*) const noexcept final override {
    return 0;
  }

  TURBO_MUST_USE_RESULT size_t ConvertUtf16BeToUtf8(const char16_t*, size_t, char*) const noexcept final override {
    return 0;
  }

  TURBO_MUST_USE_RESULT result ConvertUtf16LeToUtf8WithErrors(const char16_t*, size_t, char*) const noexcept final override {
    return result(error_code::OTHER, 0);
  }

  TURBO_MUST_USE_RESULT result ConvertUtf16BeToUtf8WithErrors(const char16_t*, size_t, char*) const noexcept final override {
    return result(error_code::OTHER, 0);
  }

  TURBO_MUST_USE_RESULT size_t ConvertValidUtf16LeToUtf8(const char16_t*, size_t, char*) const noexcept final override {
    return 0;
  }

  TURBO_MUST_USE_RESULT size_t ConvertValidUtf16BeToUtf8(const char16_t*, size_t, char*) const noexcept final override {
    return 0;
  }

  TURBO_MUST_USE_RESULT size_t ConvertUtf32ToUtf8(const char32_t*, size_t, char*) const noexcept final override {
    return 0;
  }

  TURBO_MUST_USE_RESULT result ConvertUtf32ToUtf8WithErrors(const char32_t*, size_t, char*) const noexcept final override {
    return result(error_code::OTHER, 0);
  }

  TURBO_MUST_USE_RESULT size_t ConvertValidUtf32ToUtf8(const char32_t*, size_t, char*) const noexcept final override {
    return 0;
  }

  TURBO_MUST_USE_RESULT size_t ConvertUtf32ToUtf16Le(const char32_t*, size_t, char16_t*) const noexcept final override {
    return 0;
  }

  TURBO_MUST_USE_RESULT size_t ConvertUtf32ToUtf16Be(const char32_t*, size_t, char16_t*) const noexcept final override {
    return 0;
  }

  TURBO_MUST_USE_RESULT result ConvertUtf32ToUtf16leWithErrors(const char32_t*, size_t, char16_t*) const noexcept final override {
    return result(error_code::OTHER, 0);
  }

  TURBO_MUST_USE_RESULT result ConvertUtf32ToUtf16BeWithErrors(const char32_t*, size_t, char16_t*) const noexcept final override {
    return result(error_code::OTHER, 0);
  }

  TURBO_MUST_USE_RESULT size_t ConvertValidUtf32ToUtf16Le(const char32_t*, size_t, char16_t*) const noexcept final override {
    return 0;
  }

  TURBO_MUST_USE_RESULT size_t ConvertValidUtf32ToUtf16Be(const char32_t*, size_t, char16_t*) const noexcept final override {
    return 0;
  }

  TURBO_MUST_USE_RESULT size_t ConvertUtf16LeToUtf32(const char16_t*, size_t, char32_t*) const noexcept final override {
    return 0;
  }

  TURBO_MUST_USE_RESULT size_t ConvertUtf16BeToUtf32(const char16_t*, size_t, char32_t*) const noexcept final override {
    return 0;
  }

  TURBO_MUST_USE_RESULT result ConvertUtf16LeToUtf32WithErrors(const char16_t*, size_t, char32_t*) const noexcept final override {
    return result(error_code::OTHER, 0);
  }

  TURBO_MUST_USE_RESULT result ConvertUtf16BeToUtf32WithErrors(const char16_t*, size_t, char32_t*) const noexcept final override {
    return result(error_code::OTHER, 0);
  }

  TURBO_MUST_USE_RESULT size_t ConvertValidUtf16LeToUtf32(const char16_t*, size_t, char32_t*) const noexcept final override {
    return 0;
  }

  TURBO_MUST_USE_RESULT size_t ConvertValidUtf16BeToUtf32(const char16_t*, size_t, char32_t*) const noexcept final override {
    return 0;
  }

  void ChangeEndiannessUtf16(const char16_t *, size_t, char16_t *) const noexcept final override {

  }

  TURBO_MUST_USE_RESULT size_t CountUtf16Le(const char16_t *, size_t) const noexcept final override {
    return 0;
  }

  TURBO_MUST_USE_RESULT size_t CountUtf16Be(const char16_t *, size_t) const noexcept final override {
    return 0;
  }

  TURBO_MUST_USE_RESULT size_t CountUtf8(const char *, size_t) const noexcept final override {
    return 0;
  }

  TURBO_MUST_USE_RESULT size_t Utf8LengthFromUtf16Le(const char16_t *, size_t) const noexcept override {
    return 0;
  }

  TURBO_MUST_USE_RESULT size_t Utf8LengthFromUtf16be(const char16_t *, size_t) const noexcept override {
    return 0;
  }

  TURBO_MUST_USE_RESULT size_t Utf32LengthFromUtf16Le(const char16_t *, size_t) const noexcept override {
    return 0;
  }

  TURBO_MUST_USE_RESULT size_t Utf32LengthFromUtf16Be(const char16_t *, size_t) const noexcept override {
    return 0;
  }

  TURBO_MUST_USE_RESULT size_t Utf16LengthFromUtf8(const char *, size_t) const noexcept override {
    return 0;
  }

  TURBO_MUST_USE_RESULT size_t Utf8LengthFromUtf32(const char32_t *, size_t) const noexcept override {
    return 0;
  }

  TURBO_MUST_USE_RESULT size_t Utf16LengthFromUtf32(const char32_t *, size_t) const noexcept override {
    return 0;
  }

  TURBO_MUST_USE_RESULT size_t Utf32LengthFromUtf8(const char *, size_t) const noexcept override {
    return 0;
  }

  unsupported_implementation() : Implementation("unsupported", "Unsupported CPU (no detected SIMD instructions)", 0) {}
};

const unsupported_implementation unsupported_singleton{};

size_t available_implementation_list::size() const noexcept {
  return internal::available_implementation_pointers.size();
}
const Implementation * const *available_implementation_list::begin() const noexcept {
  return internal::available_implementation_pointers.begin();
}
const Implementation * const *available_implementation_list::end() const noexcept {
  return internal::available_implementation_pointers.end();
}
const Implementation *available_implementation_list::detect_best_supported() const noexcept {
  // They are prelisted in priority order, so we just go down the list
  uint32_t supported_instruction_sets = internal::detect_supported_architectures();
  for (const Implementation *impl : internal::available_implementation_pointers) {
    uint32_t required_instruction_sets = impl->RequiredInstructionSets();
    if ((supported_instruction_sets & required_instruction_sets) == required_instruction_sets) { return impl; }
  }
  return &unsupported_singleton; // this should never happen?
}

const Implementation *detect_best_supported_implementation_on_first_use::set_best() const noexcept {
  char *force_implementation_name = getenv("TURBO_UNICODE_FORCE_IMPLEMENTATION");
  if (force_implementation_name) {
    auto force_implementation = get_available_implementations()[force_implementation_name];
    if (force_implementation) {
      return get_active_implementation() = force_implementation;
    } else {
      // Note: abort() and stderr usage within the library is forbidden.
      return get_active_implementation() = &unsupported_singleton;
    }
  }
  return get_active_implementation() = get_available_implementations().detect_best_supported();
}

} // namespace internal



/**
 * The list of available implementations compiled into unicode.
 */
TURBO_DLL const internal::available_implementation_list& get_available_implementations() {
  static const internal::available_implementation_list available_implementations{};
  return available_implementations;
}

/**
  * The active implementation.
  */
TURBO_DLL internal::atomic_ptr<const Implementation>& get_active_implementation() {
    static const internal::detect_best_supported_implementation_on_first_use detect_best_supported_implementation_on_first_use_singleton;
    static internal::atomic_ptr<const Implementation> active_implementation{&detect_best_supported_implementation_on_first_use_singleton};
    return active_implementation;
}

TURBO_MUST_USE_RESULT bool ValidateUtf8(const char *buf, size_t len) noexcept {
  return get_active_implementation()->ValidateUtf8(buf, len);
}
TURBO_MUST_USE_RESULT result ValidateUtf8WithErrors(const char *buf, size_t len) noexcept {
  return get_active_implementation()->ValidateUtf8WithErrors(buf, len);
}
TURBO_MUST_USE_RESULT bool ValidateAscii(const char *buf, size_t len) noexcept {
  return get_active_implementation()->ValidateAscii(buf, len);
}
TURBO_MUST_USE_RESULT result ValidateAsciiWithErrors(const char *buf, size_t len) noexcept {
  return get_active_implementation()->ValidateAsciiWithErrors(buf, len);
}
TURBO_MUST_USE_RESULT size_t ConvertUtf8ToUtf16(const char * input, size_t length, char16_t* utf16_output) noexcept {
  #if TURBO_IS_BIG_ENDIAN
  return ConvertUtf8ToUtf16Be(input, length, utf16_output);
  #else
  return ConvertUtf8ToUtf16Le(input, length, utf16_output);
  #endif
}
TURBO_MUST_USE_RESULT size_t ConvertUtf8ToUtf16Le(const char * input, size_t length, char16_t* utf16_output) noexcept {
  return get_active_implementation()->ConvertUtf8ToUtf16Le(input, length, utf16_output);
}
TURBO_MUST_USE_RESULT size_t ConvertUtf8ToUtf16Be(const char * input, size_t length, char16_t* utf16_output) noexcept {
  return get_active_implementation()->ConvertUtf8ToUtf16Be(input, length, utf16_output);
}
TURBO_MUST_USE_RESULT result ConvertUtf8ToUtf16WithErrors(const char * input, size_t length, char16_t* utf16_output) noexcept {
  #if TURBO_IS_BIG_ENDIAN
  return ConvertUtf8ToUtf16BeWithErrors(input, length, utf16_output);
  #else
  return ConvertUtf8ToUtf16LeWithErrors(input, length, utf16_output);
  #endif
}
TURBO_MUST_USE_RESULT result ConvertUtf8ToUtf16LeWithErrors(const char * input, size_t length, char16_t* utf16_output) noexcept {
  return get_active_implementation()->ConvertUtf8ToUtf16LeWithErrors(input, length, utf16_output);
}
TURBO_MUST_USE_RESULT result ConvertUtf8ToUtf16BeWithErrors(const char * input, size_t length, char16_t* utf16_output) noexcept {
  return get_active_implementation()->ConvertUtf8ToUtf16BeWithErrors(input, length, utf16_output);
}
TURBO_MUST_USE_RESULT size_t ConvertUtf8ToUtf32(const char * input, size_t length, char32_t* utf32_output) noexcept {
  return get_active_implementation()->ConvertUtf8ToUtf32(input, length, utf32_output);
}
TURBO_MUST_USE_RESULT result ConvertUtf8ToUtf32WithErrors(const char * input, size_t length, char32_t* utf32_output) noexcept {
  return get_active_implementation()->ConvertUtf8ToUtf32WithErrors(input, length, utf32_output);
}
TURBO_MUST_USE_RESULT bool ValidateUtf16(const char16_t * buf, size_t len) noexcept {
  #if TURBO_IS_BIG_ENDIAN
  return ValidateUtf16Be(buf, len);
  #else
  return ValidateUtf16Le(buf, len);
  #endif
}
TURBO_MUST_USE_RESULT bool ValidateUtf16Le(const char16_t * buf, size_t len) noexcept {
  return get_active_implementation()->ValidateUtf16Le(buf, len);
}
TURBO_MUST_USE_RESULT bool ValidateUtf16Be(const char16_t * buf, size_t len) noexcept {
  return get_active_implementation()->ValidateUtf16Be(buf, len);
}
TURBO_MUST_USE_RESULT result ValidateUtf16WithErrors(const char16_t * buf, size_t len) noexcept {
  #if TURBO_IS_BIG_ENDIAN
  return ValidateUtf16BeWithErrors(buf, len);
  #else
  return ValidateUtf16LeWithErrors(buf, len);
  #endif
}
TURBO_MUST_USE_RESULT result ValidateUtf16LeWithErrors(const char16_t * buf, size_t len) noexcept {
  return get_active_implementation()->ValidateUtf16LeWithErrors(buf, len);
}
TURBO_MUST_USE_RESULT result ValidateUtf16BeWithErrors(const char16_t * buf, size_t len) noexcept {
  return get_active_implementation()->ValidateUtf16BeWithErrors(buf, len);
}
TURBO_MUST_USE_RESULT bool ValidateUtf32(const char32_t * buf, size_t len) noexcept {
  return get_active_implementation()->ValidateUtf32(buf, len);
}
TURBO_MUST_USE_RESULT result ValidateUtf32WithErrors(const char32_t * buf, size_t len) noexcept {
  return get_active_implementation()->ValidateUtf32WithErrors(buf, len);
}
TURBO_MUST_USE_RESULT size_t ConvertValidUtf8ToUtf16(const char * input, size_t length, char16_t* utf16_buffer) noexcept {
  #if TURBO_IS_BIG_ENDIAN
  return ConvertValidUtf8ToUtf16Be(input, length, utf16_buffer);
  #else
  return ConvertValidUtf8ToUtf16Le(input, length, utf16_buffer);
  #endif
}
TURBO_MUST_USE_RESULT size_t ConvertValidUtf8ToUtf16Le(const char * input, size_t length, char16_t* utf16_buffer) noexcept {
  return get_active_implementation()->ConvertValidUtf8ToUtf16Le(input, length, utf16_buffer);
}
TURBO_MUST_USE_RESULT size_t ConvertValidUtf8ToUtf16Be(const char * input, size_t length, char16_t* utf16_buffer) noexcept {
  return get_active_implementation()->ConvertValidUtf8ToUtf16Be(input, length, utf16_buffer);
}
TURBO_MUST_USE_RESULT size_t ConvertValidUtf8ToUtf32(const char * input, size_t length, char32_t* utf32_buffer) noexcept {
  return get_active_implementation()->ConvertValidUtf8ToUtf32(input, length, utf32_buffer);
}
TURBO_MUST_USE_RESULT size_t ConvertUtf16ToUtf8(const char16_t * buf, size_t len, char* utf8_buffer) noexcept {
  #if TURBO_IS_BIG_ENDIAN
  return ConvertUtf16BeToUtf8(buf, len, utf8_buffer);
  #else
  return ConvertUtf16LeToUtf8(buf, len, utf8_buffer);
  #endif
}
TURBO_MUST_USE_RESULT size_t ConvertUtf16LeToUtf8(const char16_t * buf, size_t len, char* utf8_buffer) noexcept {
  return get_active_implementation()->ConvertUtf16LeToUtf8(buf, len, utf8_buffer);
}
TURBO_MUST_USE_RESULT size_t ConvertUtf16BeToUtf8(const char16_t * buf, size_t len, char* utf8_buffer) noexcept {
  return get_active_implementation()->ConvertUtf16BeToUtf8(buf, len, utf8_buffer);
}
TURBO_MUST_USE_RESULT result ConvertUtf16ToUtf8WithErrors(const char16_t * buf, size_t len, char* utf8_buffer) noexcept {
  #if TURBO_IS_BIG_ENDIAN
  return ConvertUtf16BeToUtf8WithErrors(buf, len, utf8_buffer);
  #else
  return ConvertUtf16LeToUtf8WithErrors(buf, len, utf8_buffer);
  #endif
}
TURBO_MUST_USE_RESULT result ConvertUtf16LeToUtf8WithErrors(const char16_t * buf, size_t len, char* utf8_buffer) noexcept {
  return get_active_implementation()->ConvertUtf16LeToUtf8WithErrors(buf, len, utf8_buffer);
}
TURBO_MUST_USE_RESULT result ConvertUtf16BeToUtf8WithErrors(const char16_t * buf, size_t len, char* utf8_buffer) noexcept {
  return get_active_implementation()->ConvertUtf16BeToUtf8WithErrors(buf, len, utf8_buffer);
}
TURBO_MUST_USE_RESULT size_t ConvertValidUtf16ToUtf8(const char16_t * buf, size_t len, char* utf8_buffer) noexcept {
  #if BIG_ENDIAN
  return ConvertValidUtf16BeToUtf8(buf, len, utf8_buffer);
  #else
  return ConvertValidUtf16LeToUtf8(buf, len, utf8_buffer);
  #endif
}
TURBO_MUST_USE_RESULT size_t ConvertValidUtf16LeToUtf8(const char16_t * buf, size_t len, char* utf8_buffer) noexcept {
  return get_active_implementation()->ConvertValidUtf16LeToUtf8(buf, len, utf8_buffer);
}
TURBO_MUST_USE_RESULT size_t ConvertValidUtf16BeToUtf8(const char16_t * buf, size_t len, char* utf8_buffer) noexcept {
  return get_active_implementation()->ConvertValidUtf16BeToUtf8(buf, len, utf8_buffer);
}
TURBO_MUST_USE_RESULT size_t ConvertUtf32ToUtf8(const char32_t * buf, size_t len, char* utf8_buffer) noexcept {
  return get_active_implementation()->ConvertUtf32ToUtf8(buf, len, utf8_buffer);
}
TURBO_MUST_USE_RESULT result ConvertUtf32ToUtf8WithErrors(const char32_t * buf, size_t len, char* utf8_buffer) noexcept {
  return get_active_implementation()->ConvertUtf32ToUtf8WithErrors(buf, len, utf8_buffer);
}
TURBO_MUST_USE_RESULT size_t ConvertValidUtf32ToUtf8(const char32_t * buf, size_t len, char* utf8_buffer) noexcept {
  return get_active_implementation()->ConvertValidUtf32ToUtf8(buf, len, utf8_buffer);
}
TURBO_MUST_USE_RESULT size_t ConvertUtf32ToUtf16(const char32_t * buf, size_t len, char16_t* utf16_buffer) noexcept {
  #if TURBO_IS_BIG_ENDIAN
  return ConvertUtf32ToUtf16Be(buf, len, utf16_buffer);
  #else
  return ConvertUtf32ToUtf16Le(buf, len, utf16_buffer);
  #endif
}
TURBO_MUST_USE_RESULT size_t ConvertUtf32ToUtf16Le(const char32_t * buf, size_t len, char16_t* utf16_buffer) noexcept {
  return get_active_implementation()->ConvertUtf32ToUtf16Le(buf, len, utf16_buffer);
}
TURBO_MUST_USE_RESULT size_t ConvertUtf32ToUtf16Be(const char32_t * buf, size_t len, char16_t* utf16_buffer) noexcept {
  return get_active_implementation()->ConvertUtf32ToUtf16Be(buf, len, utf16_buffer);
}
TURBO_MUST_USE_RESULT result ConvertUtf32ToUtf16WithErrors(const char32_t * buf, size_t len, char16_t* utf16_buffer) noexcept {
  #if TURBO_IS_BIG_ENDIAN
  return ConvertUtf32ToUtf16BeWithErrors(buf, len, utf16_buffer);
  #else
  return ConvertUtf32ToUtf16leWithErrors(buf, len, utf16_buffer);
  #endif
}
TURBO_MUST_USE_RESULT result ConvertUtf32ToUtf16leWithErrors(const char32_t * buf, size_t len, char16_t* utf16_buffer) noexcept {
  return get_active_implementation()->ConvertUtf32ToUtf16leWithErrors(buf, len, utf16_buffer);
}
TURBO_MUST_USE_RESULT result ConvertUtf32ToUtf16BeWithErrors(const char32_t * buf, size_t len, char16_t* utf16_buffer) noexcept {
  return get_active_implementation()->ConvertUtf32ToUtf16BeWithErrors(buf, len, utf16_buffer);
}
TURBO_MUST_USE_RESULT size_t ConvertValidUtf32ToUtf16(const char32_t * buf, size_t len, char16_t* utf16_buffer) noexcept {
  #if TURBO_IS_BIG_ENDIAN
  return ConvertValidUtf32ToUtf16Be(buf, len, utf16_buffer);
  #else
  return ConvertValidUtf32ToUtf16Le(buf, len, utf16_buffer);
  #endif
}
TURBO_MUST_USE_RESULT size_t ConvertValidUtf32ToUtf16Le(const char32_t * buf, size_t len, char16_t* utf16_buffer) noexcept {
  return get_active_implementation()->ConvertValidUtf32ToUtf16Le(buf, len, utf16_buffer);
}
TURBO_MUST_USE_RESULT size_t ConvertValidUtf32ToUtf16Be(const char32_t * buf, size_t len, char16_t* utf16_buffer) noexcept {
  return get_active_implementation()->ConvertValidUtf32ToUtf16Be(buf, len, utf16_buffer);
}
TURBO_MUST_USE_RESULT size_t ConvertUtf16ToUtf32(const char16_t * buf, size_t len, char32_t* utf32_buffer) noexcept {
  #if TURBO_IS_BIG_ENDIAN
  return ConvertUtf16BeToUtf32(buf, len, utf32_buffer);
  #else
  return ConvertUtf16LeToUtf32(buf, len, utf32_buffer);
  #endif
}
TURBO_MUST_USE_RESULT size_t ConvertUtf16LeToUtf32(const char16_t * buf, size_t len, char32_t* utf32_buffer) noexcept {
  return get_active_implementation()->ConvertUtf16LeToUtf32(buf, len, utf32_buffer);
}
TURBO_MUST_USE_RESULT size_t ConvertUtf16BeToUtf32(const char16_t * buf, size_t len, char32_t* utf32_buffer) noexcept {
  return get_active_implementation()->ConvertUtf16BeToUtf32(buf, len, utf32_buffer);
}
TURBO_MUST_USE_RESULT result ConvertUtf16ToUtf32WithErrors(const char16_t * buf, size_t len, char32_t* utf32_buffer) noexcept {
  #if TURBO_IS_BIG_ENDIAN
  return ConvertUtf16BeToUtf32WithErrors(buf, len, utf32_buffer);
  #else
  return ConvertUtf16LeToUtf32WithErrors(buf, len, utf32_buffer);
  #endif
}
TURBO_MUST_USE_RESULT result ConvertUtf16LeToUtf32WithErrors(const char16_t * buf, size_t len, char32_t* utf32_buffer) noexcept {
  return get_active_implementation()->ConvertUtf16LeToUtf32WithErrors(buf, len, utf32_buffer);
}
TURBO_MUST_USE_RESULT result ConvertUtf16BeToUtf32WithErrors(const char16_t * buf, size_t len, char32_t* utf32_buffer) noexcept {
  return get_active_implementation()->ConvertUtf16BeToUtf32WithErrors(buf, len, utf32_buffer);
}
TURBO_MUST_USE_RESULT size_t ConvertValidUtf16ToUtf32(const char16_t * buf, size_t len, char32_t* utf32_buffer) noexcept {
  #if TURBO_IS_BIG_ENDIAN
  return ConvertValidUtf16BeToUtf32(buf, len, utf32_buffer);
  #else
  return ConvertValidUtf16LeToUtf32(buf, len, utf32_buffer);
  #endif
}
TURBO_MUST_USE_RESULT size_t ConvertValidUtf16LeToUtf32(const char16_t * buf, size_t len, char32_t* utf32_buffer) noexcept {
  return get_active_implementation()->ConvertValidUtf16LeToUtf32(buf, len, utf32_buffer);
}
TURBO_MUST_USE_RESULT size_t ConvertValidUtf16BeToUtf32(const char16_t * buf, size_t len, char32_t* utf32_buffer) noexcept {
  return get_active_implementation()->ConvertValidUtf16BeToUtf32(buf, len, utf32_buffer);
}
void ChangeEndiannessUtf16(const char16_t * input, size_t length, char16_t * output) noexcept {
  get_active_implementation()->ChangeEndiannessUtf16(input, length, output);
}
TURBO_MUST_USE_RESULT size_t CountUtf16(const char16_t * input, size_t length) noexcept {
  #if TURBO_IS_BIG_ENDIAN
  return CountUtf16Be(input, length);
  #else
  return CountUtf16Le(input, length);
  #endif
}
TURBO_MUST_USE_RESULT size_t CountUtf16Le(const char16_t * input, size_t length) noexcept {
  return get_active_implementation()->CountUtf16Le(input, length);
}
TURBO_MUST_USE_RESULT size_t CountUtf16Be(const char16_t * input, size_t length) noexcept {
  return get_active_implementation()->CountUtf16Be(input, length);
}
TURBO_MUST_USE_RESULT size_t CountUtf8(const char * input, size_t length) noexcept {
  return get_active_implementation()->CountUtf8(input, length);
}
TURBO_MUST_USE_RESULT size_t Utf8LengthFromUtf16(const char16_t * input, size_t length) noexcept {
  #if TURBO_IS_BIG_ENDIAN
  return Utf8LengthFromUtf16be(input, length);
  #else
  return Utf8LengthFromUtf16Le(input, length);
  #endif
}
TURBO_MUST_USE_RESULT size_t Utf8LengthFromUtf16Le(const char16_t * input, size_t length) noexcept {
  return get_active_implementation()->Utf8LengthFromUtf16Le(input, length);
}
TURBO_MUST_USE_RESULT size_t Utf8LengthFromUtf16be(const char16_t * input, size_t length) noexcept {
  return get_active_implementation()->Utf8LengthFromUtf16be(input, length);
}
TURBO_MUST_USE_RESULT size_t Utf32LengthFromUtf16(const char16_t * input, size_t length) noexcept {
  #if TURBO_IS_BIG_ENDIAN
  return Utf32LengthFromUtf16Be(input, length);
  #else
  return Utf32LengthFromUtf16Le(input, length);
  #endif
}
TURBO_MUST_USE_RESULT size_t Utf32LengthFromUtf16Le(const char16_t * input, size_t length) noexcept {
  return get_active_implementation()->Utf32LengthFromUtf16Le(input, length);
}
TURBO_MUST_USE_RESULT size_t Utf32LengthFromUtf16Be(const char16_t * input, size_t length) noexcept {
  return get_active_implementation()->Utf32LengthFromUtf16Be(input, length);
}
TURBO_MUST_USE_RESULT size_t Utf16LengthFromUtf8(const char * input, size_t length) noexcept {
  return get_active_implementation()->Utf16LengthFromUtf8(input, length);
}
TURBO_MUST_USE_RESULT size_t Utf8LengthFromUtf32(const char32_t * input, size_t length) noexcept {
  return get_active_implementation()->Utf8LengthFromUtf32(input, length);
}
TURBO_MUST_USE_RESULT size_t Utf16LengthFromUtf32(const char32_t * input, size_t length) noexcept {
  return get_active_implementation()->Utf16LengthFromUtf32(input, length);
}
TURBO_MUST_USE_RESULT size_t Utf32LengthFromUtf8(const char * input, size_t length) noexcept {
  return get_active_implementation()->Utf32LengthFromUtf8(input, length);
}
TURBO_MUST_USE_RESULT turbo::EncodingType AutodetectEncoding(const char * buf, size_t length) noexcept {
  return get_active_implementation()->AutodetectEncoding(buf, length);
}
TURBO_MUST_USE_RESULT int DetectEncodings(const char * buf, size_t length) noexcept {
  return get_active_implementation()->DetectEncodings(buf, length);
}

const Implementation * builtin_implementation() {
  static const Implementation * builtin_impl = get_available_implementations()[TURBO_STRINGIFY(TURBO_UNICODE_BUILTIN_IMPLEMENTATION)];
  return builtin_impl;
}


} // namespace turbo

