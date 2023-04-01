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

#include "turbo/unicode/icelake/intrinsics.h"

#include "turbo/unicode/scalar/valid_utf16_to_utf8.h"
#include "turbo/unicode/scalar/utf16_to_utf8.h"
#include "turbo/unicode/scalar/valid_utf8_to_utf16.h"
#include "turbo/unicode/scalar/utf8_to_utf16.h"
#include "turbo/unicode/scalar/utf8.h"
#include "turbo/unicode/scalar/utf16.h"

#include "turbo/unicode/icelake/begin.h"
namespace turbo {
namespace TURBO_UNICODE_IMPLEMENTATION {
namespace {
#ifndef TURBO_UNICODE_ICELAKE_H_
#error "icelake.h must be included"
#endif
#include "turbo/unicode/icelake/icelake_utf8_common.inl.cc"
#include "turbo/unicode/icelake/icelake_macros.inl.cc"
#include "turbo/unicode/icelake/icelake_from_valid_utf8.inl.cc"
#include "turbo/unicode/icelake/icelake_utf8_validation.inl.cc"
#include "turbo/unicode/icelake/icelake_from_utf8.inl.cc"
#include "turbo/unicode/icelake/icelake_convert_utf16_to_utf32.inl.cc"
#include "turbo/unicode/icelake/icelake_convert_utf32_to_utf8.inl.cc"
#include "turbo/unicode/icelake/icelake_convert_utf32_to_utf16.inl.cc"
#include "turbo/unicode/icelake/icelake_ascii_validation.inl.cc"
#include "turbo/unicode/icelake/icelake_utf32_validation.inl.cc"
#include "turbo/unicode/icelake/icelake_convert_utf16_to_utf8.inl.cc"

} // namespace
} // namespace TURBO_UNICODE_IMPLEMENTATION
} // namespace turbo

namespace turbo {
namespace TURBO_UNICODE_IMPLEMENTATION {


TURBO_MUST_USE_RESULT int
Implementation::DetectEncodings(const char *input,
                                 size_t length) const noexcept {
  // If there is a BOM, then we trust it.
  auto bom_encoding = turbo::BOM::check_bom(input, length);
  if(bom_encoding != EncodingType::unspecified) { return bom_encoding; }
  if (length % 2 == 0) {
    const char *buf = input;

    const char *start = buf;
    const char *end = input + length;

    bool is_utf8 = true;
    bool is_utf16 = true;
    bool is_utf32 = true;

    int out = 0;

    avx512_utf8_checker checker{};
    __m512i currentmax = _mm512_setzero_si512();
    while (buf + 64 <= end) {
      __m512i in = _mm512_loadu_si512((__m512i *)const_cast<char*>(buf));
      __m512i diff = _mm512_sub_epi16(in, _mm512_set1_epi16(uint16_t(0xD800)));
      __mmask32 surrogates =
          _mm512_cmplt_epu16_mask(diff, _mm512_set1_epi16(uint16_t(0x0800)));
      if (surrogates) {
        is_utf8 = false;

        // Can still be either UTF-16LE or UTF-32 depending on the positions
        // of the surrogates To be valid UTF-32, a surrogate cannot be in the
        // two most significant bytes of any 32-bit word. On the other hand, to
        // be valid UTF-16LE, at least one surrogate must be in the two most
        // significant bytes of a 32-bit word since they always come in pairs in
        // UTF-16LE. Note that we always proceed in multiple of 4 before this
        // point so there is no offset in 32-bit words.

        if ((surrogates & 0xaaaaaaaa) != 0) {
          is_utf32 = false;
          __mmask32 highsurrogates = _mm512_cmplt_epu16_mask(
              diff, _mm512_set1_epi16(uint16_t(0x0400)));
          __mmask32 lowsurrogates = surrogates ^ highsurrogates;
          // high must be followed by low
          if ((highsurrogates << 1) != lowsurrogates) {
            return turbo::EncodingType::unspecified;
          }

          bool ends_with_high = ((highsurrogates & 0x80000000) != 0);
          if (ends_with_high) {
            buf +=
                31 *
                sizeof(char16_t); // advance only by 31 words so that we start
                                  // with the high surrogate on the next round.
          } else {
            buf += 32 * sizeof(char16_t);
          }
          is_utf16 = ValidateUtf16Le(reinterpret_cast<const char16_t *>(buf),
                                      (end - buf) / sizeof(char16_t));
          if (!is_utf16) {
            return turbo::EncodingType::unspecified;

          } else {
            return turbo::EncodingType::UTF16_LE;
          }

        } else {
          is_utf16 = false;
          // Check for UTF-32
          if (length % 4 == 0) {
            const char32_t *input32 = reinterpret_cast<const char32_t *>(buf);
            const char32_t *end32 =
                reinterpret_cast<const char32_t *>(start) + length / 4;
            if (ValidateUtf32(input32, end32 - input32)) {
              return turbo::EncodingType::UTF32_LE;
            }
          }
          return turbo::EncodingType::unspecified;
        }
        break;
      }
      // If no surrogate, validate under other encodings as well

      // UTF-32 validation
      currentmax = _mm512_max_epu32(in, currentmax);

      // UTF-8 validation
      checker.check_next_input(in);

      buf += 64;
    }

    // Check which encodings are possible

    if (is_utf8) {
      size_t current_length = static_cast<size_t>(buf - start);
      if (current_length != length) {
        const __m512i utf8 = _mm512_maskz_loadu_epi8(
            (1ULL << (length - current_length)) - 1, (const __m512i *)buf);
        checker.check_next_input(utf8);
      }
      checker.check_eof();
      if (!checker.errors()) {
        out |= turbo::EncodingType::UTF8;
      }
    }

    if (is_utf16 && scalar::utf16::validate<endianness::LITTLE>(
                        reinterpret_cast<const char16_t *>(buf),
                        (length - (buf - start)) / 2)) {
      out |= turbo::EncodingType::UTF16_LE;
    }

    if (is_utf32 && (length % 4 == 0)) {
      currentmax = _mm512_max_epu32(
          _mm512_maskz_loadu_epi8(
              (1ULL << (length - static_cast<size_t>(buf - start))) - 1,
              (const __m512i *)buf),
          currentmax);
      __mmask16 outside_range = _mm512_cmp_epu32_mask(currentmax, _mm512_set1_epi32(0x10ffff),
                                _MM_CMPINT_GT);
      if (outside_range == 0) {
        out |= turbo::EncodingType::UTF32_LE;
      }
    }

    return out;
  } else if (Implementation::ValidateUtf8(input, length)) {
    return turbo::EncodingType::UTF8;
  } else {
    return turbo::EncodingType::unspecified;
  }
}

TURBO_MUST_USE_RESULT bool Implementation::ValidateUtf8(const char *buf, size_t len) const noexcept {
    avx512_utf8_checker checker{};
    const char* ptr = buf;
    const char* end = ptr + len;
    for (; ptr + 64 <= end; ptr += 64) {
        const __m512i utf8 = _mm512_loadu_si512((const __m512i*)ptr);
        checker.check_next_input(utf8);
    }
    {
       const __m512i utf8 = _mm512_maskz_loadu_epi8((1ULL<<(end - ptr))-1, (const __m512i*)ptr);
       checker.check_next_input(utf8);
    }
    checker.check_eof();
    return ! checker.errors();
}

TURBO_MUST_USE_RESULT result Implementation::ValidateUtf8WithErrors(const char *buf, size_t len) const noexcept {
    avx512_utf8_checker checker{};
    const char* ptr = buf;
    const char* end = ptr + len;
    size_t count{0};
    for (; ptr + 64 <= end; ptr += 64) {
      const __m512i utf8 = _mm512_loadu_si512((const __m512i*)ptr);
      checker.check_next_input(utf8);
      if(checker.errors()) {
        if (count != 0) { count--; } // Sometimes the error is only detected in the next chunk
        result res = scalar::utf8::rewind_and_validate_with_errors(reinterpret_cast<const char*>(buf + count), len - count);
        res.count += count;
        return res;
      }
      count += 64;
    }
    {
      const __m512i utf8 = _mm512_maskz_loadu_epi8((1ULL<<(end - ptr))-1, (const __m512i*)ptr);
      checker.check_next_input(utf8);
      if(checker.errors()) {
        if (count != 0) { count--; } // Sometimes the error is only detected in the next chunk
        result res = scalar::utf8::rewind_and_validate_with_errors(reinterpret_cast<const char*>(buf + count), len - count);
        res.count += count;
        return res;
      } else {
        return result(error_code::SUCCESS, len);
      }
    }
}

TURBO_MUST_USE_RESULT bool Implementation::ValidateAscii(const char *buf, size_t len) const noexcept {
  return icelake::ValidateAscii(buf, len);
}

TURBO_MUST_USE_RESULT result Implementation::ValidateAsciiWithErrors(const char *buf, size_t len) const noexcept {
  const char* buf_orig = buf;
  const char* end = buf + len;
  const __m512i ascii = _mm512_set1_epi8((uint8_t)0x80);
  for (; buf + 64 <= end; buf += 64) {
    const __m512i input = _mm512_loadu_si512((const __m512i*)buf);
    __mmask64 notascii = _mm512_cmp_epu8_mask(input, ascii, _MM_CMPINT_NLT);
    if(notascii) {
      return result(error_code::TOO_LARGE, buf - buf_orig + _tzcnt_u64(notascii));
    }
  }
  {
    const __m512i input = _mm512_maskz_loadu_epi8((1ULL<<(end - buf))-1, (const __m512i*)buf);
    __mmask64 notascii = _mm512_cmp_epu8_mask(input, ascii, _MM_CMPINT_NLT);
    if(notascii) {
      return result(error_code::TOO_LARGE, buf - buf_orig + _tzcnt_u64(notascii));
    }
  }
  return result(error_code::SUCCESS, len);
}

TURBO_MUST_USE_RESULT bool Implementation::ValidateUtf16Le(const char16_t *buf, size_t len) const noexcept {
    const char16_t *end = buf + len;

    for(;buf + 32 <= end; ) {
      __m512i in = _mm512_loadu_si512((__m512i*)const_cast<char16_t*>(buf));
      __m512i diff = _mm512_sub_epi16(in, _mm512_set1_epi16(uint16_t(0xD800)));
      __mmask32 surrogates = _mm512_cmplt_epu16_mask(diff, _mm512_set1_epi16(uint16_t(0x0800)));
      if(surrogates) {
        __mmask32 highsurrogates = _mm512_cmplt_epu16_mask(diff, _mm512_set1_epi16(uint16_t(0x0400)));
        __mmask32 lowsurrogates = surrogates ^ highsurrogates;
        // high must be followed by low
        if ((highsurrogates << 1) != lowsurrogates) {
           return false;
        }
        bool ends_with_high = ((highsurrogates & 0x80000000) != 0);
        if(ends_with_high) {
          buf += 31; // advance only by 31 words so that we start with the high surrogate on the next round.
        } else {
          buf += 32;
        }
      } else {
        buf += 32;
      }
    }
    if(buf < end) {
      __m512i in = _mm512_maskz_loadu_epi16((1<<(end-buf))-1,(__m512i*)const_cast<char16_t*>(buf));
      __m512i diff = _mm512_sub_epi16(in, _mm512_set1_epi16(uint16_t(0xD800)));
      __mmask32 surrogates = _mm512_cmplt_epu16_mask(diff, _mm512_set1_epi16(uint16_t(0x0800)));
      if(surrogates) {
        __mmask32 highsurrogates = _mm512_cmplt_epu16_mask(diff, _mm512_set1_epi16(uint16_t(0x0400)));
        __mmask32 lowsurrogates = surrogates ^ highsurrogates;
        // high must be followed by low
        if ((highsurrogates << 1) != lowsurrogates) {
           return false;
        }
      }
    }
    return true;
}

TURBO_MUST_USE_RESULT bool Implementation::ValidateUtf16Be(const char16_t *buf, size_t len) const noexcept {
   const char16_t *end = buf + len;
   const __m512i byteflip = _mm512_setr_epi64(
            0x0607040502030001,
            0x0e0f0c0d0a0b0809,
            0x0607040502030001,
            0x0e0f0c0d0a0b0809,
            0x0607040502030001,
            0x0e0f0c0d0a0b0809,
            0x0607040502030001,
            0x0e0f0c0d0a0b0809
        );
    for(;buf + 32 <= end; ) {
      __m512i in = _mm512_shuffle_epi8(_mm512_loadu_si512((__m512i*)const_cast<char16_t*>(buf)), byteflip);
      __m512i diff = _mm512_sub_epi16(in, _mm512_set1_epi16(uint16_t(0xD800)));
      __mmask32 surrogates = _mm512_cmplt_epu16_mask(diff, _mm512_set1_epi16(uint16_t(0x0800)));
      if(surrogates) {
        __mmask32 highsurrogates = _mm512_cmplt_epu16_mask(diff, _mm512_set1_epi16(uint16_t(0x0400)));
        __mmask32 lowsurrogates = surrogates ^ highsurrogates;
        // high must be followed by low
        if ((highsurrogates << 1) != lowsurrogates) {
           return false;
        }
        bool ends_with_high = ((highsurrogates & 0x80000000) != 0);
        if(ends_with_high) {
          buf += 31; // advance only by 31 words so that we start with the high surrogate on the next round.
        } else {
          buf += 32;
        }
      } else {
        buf += 32;
      }
    }
    if(buf < end) {
      __m512i in = _mm512_shuffle_epi8(_mm512_maskz_loadu_epi16((1<<(end-buf))-1,(__m512i*)const_cast<char16_t*>(buf)), byteflip);
      __m512i diff = _mm512_sub_epi16(in, _mm512_set1_epi16(uint16_t(0xD800)));
      __mmask32 surrogates = _mm512_cmplt_epu16_mask(diff, _mm512_set1_epi16(uint16_t(0x0800)));
      if(surrogates) {
        __mmask32 highsurrogates = _mm512_cmplt_epu16_mask(diff, _mm512_set1_epi16(uint16_t(0x0400)));
        __mmask32 lowsurrogates = surrogates ^ highsurrogates;
        // high must be followed by low
        if ((highsurrogates << 1) != lowsurrogates) {
           return false;
        }
      }
    }
    return true;
}

TURBO_MUST_USE_RESULT result Implementation::ValidateUtf16LeWithErrors(const char16_t *buf, size_t len) const noexcept {
    const char16_t *start_buf = buf;
    const char16_t *end = buf + len;
    for(;buf + 32 <= end; ) {
      __m512i in = _mm512_loadu_si512((__m512i*)const_cast<char16_t*>(buf));
      __m512i diff = _mm512_sub_epi16(in, _mm512_set1_epi16(uint16_t(0xD800)));
      __mmask32 surrogates = _mm512_cmplt_epu16_mask(diff, _mm512_set1_epi16(uint16_t(0x0800)));
      if(surrogates) {
        __mmask32 highsurrogates = _mm512_cmplt_epu16_mask(diff, _mm512_set1_epi16(uint16_t(0x0400)));
        __mmask32 lowsurrogates = surrogates ^ highsurrogates;
        // high must be followed by low
        if ((highsurrogates << 1) != lowsurrogates) {
          uint32_t extra_low = _tzcnt_u32(lowsurrogates &~(highsurrogates << 1));
          uint32_t extra_high = _tzcnt_u32(highsurrogates &~(lowsurrogates >> 1));
          return result(error_code::SURROGATE, (buf - start_buf) + (extra_low < extra_high ? extra_low : extra_high));
        }
        bool ends_with_high = ((highsurrogates & 0x80000000) != 0);
        if(ends_with_high) {
          buf += 31; // advance only by 31 words so that we start with the high surrogate on the next round.
        } else {
          buf += 32;
        }
      } else {
        buf += 32;
      }
    }
    if(buf < end) {
      __m512i in = _mm512_maskz_loadu_epi16((1<<(end-buf))-1,(__m512i*)const_cast<char16_t*>(buf));
      __m512i diff = _mm512_sub_epi16(in, _mm512_set1_epi16(uint16_t(0xD800)));
      __mmask32 surrogates = _mm512_cmplt_epu16_mask(diff, _mm512_set1_epi16(uint16_t(0x0800)));
      if(surrogates) {
        __mmask32 highsurrogates = _mm512_cmplt_epu16_mask(diff, _mm512_set1_epi16(uint16_t(0x0400)));
        __mmask32 lowsurrogates = surrogates ^ highsurrogates;
        // high must be followed by low
        if ((highsurrogates << 1) != lowsurrogates) {
          uint32_t extra_low = _tzcnt_u32(lowsurrogates &~(highsurrogates << 1));
          uint32_t extra_high = _tzcnt_u32(highsurrogates &~(lowsurrogates >> 1));
          return result(error_code::SURROGATE, (buf - start_buf) + (extra_low < extra_high ? extra_low : extra_high));
        }
      }
    }
    return result(error_code::SUCCESS, len);
}

TURBO_MUST_USE_RESULT result Implementation::ValidateUtf16BeWithErrors(const char16_t *buf, size_t len) const noexcept {
    const char16_t *start_buf = buf;
    const char16_t *end = buf + len;
    const __m512i byteflip = _mm512_setr_epi64(
            0x0607040502030001,
            0x0e0f0c0d0a0b0809,
            0x0607040502030001,
            0x0e0f0c0d0a0b0809,
            0x0607040502030001,
            0x0e0f0c0d0a0b0809,
            0x0607040502030001,
            0x0e0f0c0d0a0b0809
        );
    for(;buf + 32 <= end; ) {
      __m512i in = _mm512_shuffle_epi8(_mm512_loadu_si512((__m512i*)const_cast<char16_t*>(buf)), byteflip);
      __m512i diff = _mm512_sub_epi16(in, _mm512_set1_epi16(uint16_t(0xD800)));
      __mmask32 surrogates = _mm512_cmplt_epu16_mask(diff, _mm512_set1_epi16(uint16_t(0x0800)));
      if(surrogates) {
        __mmask32 highsurrogates = _mm512_cmplt_epu16_mask(diff, _mm512_set1_epi16(uint16_t(0x0400)));
        __mmask32 lowsurrogates = surrogates ^ highsurrogates;
        // high must be followed by low
        if ((highsurrogates << 1) != lowsurrogates) {
          uint32_t extra_low = _tzcnt_u32(lowsurrogates &~(highsurrogates << 1));
          uint32_t extra_high = _tzcnt_u32(highsurrogates &~(lowsurrogates >> 1));
          return result(error_code::SURROGATE, (buf - start_buf) + (extra_low < extra_high ? extra_low : extra_high));
        }
        bool ends_with_high = ((highsurrogates & 0x80000000) != 0);
        if(ends_with_high) {
          buf += 31; // advance only by 31 words so that we start with the high surrogate on the next round.
        } else {
          buf += 32;
        }
      } else {
        buf += 32;
      }
    }
    if(buf < end) {
      __m512i in = _mm512_shuffle_epi8(_mm512_maskz_loadu_epi16((1<<(end-buf))-1,(__m512i*)const_cast<char16_t*>(buf)), byteflip);
      __m512i diff = _mm512_sub_epi16(in, _mm512_set1_epi16(uint16_t(0xD800)));
      __mmask32 surrogates = _mm512_cmplt_epu16_mask(diff, _mm512_set1_epi16(uint16_t(0x0800)));
      if(surrogates) {
        __mmask32 highsurrogates = _mm512_cmplt_epu16_mask(diff, _mm512_set1_epi16(uint16_t(0x0400)));
        __mmask32 lowsurrogates = surrogates ^ highsurrogates;
        // high must be followed by low
        if ((highsurrogates << 1) != lowsurrogates) {
          uint32_t extra_low = _tzcnt_u32(lowsurrogates &~(highsurrogates << 1));
          uint32_t extra_high = _tzcnt_u32(highsurrogates &~(lowsurrogates >> 1));
          return result(error_code::SURROGATE, (buf - start_buf) + (extra_low < extra_high ? extra_low : extra_high));
        }
      }
    }
    return result(error_code::SUCCESS, len);
}

TURBO_MUST_USE_RESULT bool Implementation::ValidateUtf32(const char32_t *buf, size_t len) const noexcept {
  const char32_t * tail = icelake::ValidateUtf32(buf, len);
  if (tail) {
    return scalar::utf32::validate(tail, len - (tail - buf));
  } else {
    return false;
  }
}

TURBO_MUST_USE_RESULT result Implementation::ValidateUtf32WithErrors(const char32_t *buf, size_t len) const noexcept {

    const char32_t* end = len >= 16 ? buf + len - 16 : nullptr;
    const char32_t* buf_orig = buf;
    while (buf <= end) {
      __m512i utf32 = _mm512_loadu_si512((const __m512i*)buf);
      __mmask16 outside_range = _mm512_cmp_epu32_mask(utf32, _mm512_set1_epi32(0x10ffff),
                                _MM_CMPINT_GT);
      if (outside_range) {
        return result(error_code::TOO_LARGE, buf - buf_orig + _tzcnt_u32(outside_range));
      }

      __m512i utf32_off = _mm512_add_epi32(utf32, _mm512_set1_epi32(0xffff2000));

      __mmask16 surrogate_range = _mm512_cmp_epu32_mask(utf32_off, _mm512_set1_epi32(0xfffff7ff),
                                _MM_CMPINT_GT);
      if (surrogate_range) {
        return result(error_code::SURROGATE, buf - buf_orig + _tzcnt_u32(surrogate_range));
      }
      buf += 16;
    }
    if(buf < buf_orig + len) {
      __m512i utf32 = _mm512_maskz_loadu_epi32(__mmask16((1<<(buf_orig + len - buf))-1),(const __m512i*)buf);
      __mmask16 outside_range = _mm512_cmp_epu32_mask(utf32, _mm512_set1_epi32(0x10ffff),
                                _MM_CMPINT_GT);
      if (outside_range) {
        return result(error_code::TOO_LARGE, buf - buf_orig + _tzcnt_u32(outside_range));
      }
      __m512i utf32_off = _mm512_add_epi32(utf32, _mm512_set1_epi32(0xffff2000));

      __mmask16 surrogate_range = _mm512_cmp_epu32_mask(utf32_off, _mm512_set1_epi32(0xfffff7ff),
                                _MM_CMPINT_GT);
      if (surrogate_range) {
        return result(error_code::SURROGATE, buf - buf_orig + _tzcnt_u32(surrogate_range));
      }
    }

    return result(error_code::SUCCESS, len);
}

TURBO_MUST_USE_RESULT size_t Implementation::ConvertUtf8ToUtf16Le(const char* buf, size_t len, char16_t* utf16_output) const noexcept {
  utf8_to_utf16_result ret = fast_avx512_convert_utf8_to_utf16<endianness::LITTLE>(buf, len, utf16_output);
  if (ret.second == nullptr) {
    return 0;
  }
  return ret.second - utf16_output;
}

TURBO_MUST_USE_RESULT size_t Implementation::ConvertUtf8ToUtf16Be(const char* buf, size_t len, char16_t* utf16_output) const noexcept {
  utf8_to_utf16_result ret = fast_avx512_convert_utf8_to_utf16<endianness::BIG>(buf, len, utf16_output);
  if (ret.second == nullptr) {
    return 0;
  }
  return ret.second - utf16_output;
}

TURBO_MUST_USE_RESULT result Implementation::ConvertUtf8ToUtf16LeWithErrors(const char* buf, size_t len, char16_t* utf16_output) const noexcept {
   return fast_avx512_convert_utf8_to_utf16_with_errors<endianness::LITTLE>(buf, len, utf16_output);
}

TURBO_MUST_USE_RESULT result Implementation::ConvertUtf8ToUtf16BeWithErrors(const char* buf, size_t len, char16_t* utf16_output) const noexcept {
   return fast_avx512_convert_utf8_to_utf16_with_errors<endianness::BIG>(buf, len, utf16_output);
}

TURBO_MUST_USE_RESULT size_t Implementation::ConvertValidUtf8ToUtf16Le(const char* buf, size_t len, char16_t* utf16_output) const noexcept {
  utf8_to_utf16_result ret = icelake::valid_utf8_to_fixed_length<endianness::LITTLE, char16_t>(buf, len, utf16_output);
  size_t saved_bytes = ret.second - utf16_output;
  const char* end = buf + len;
  if (ret.first == end) {
    return saved_bytes;
  }

  // Note: AVX512 procedure looks up 4 bytes forward, and
  //       correctly converts multi-byte chars even if their
  //       continuation bytes lie outsiede 16-byte window.
  //       It meas, we have to skip continuation bytes from
  //       the beginning ret.first, as they were already consumed.
  while (ret.first != end && ((uint8_t(*ret.first) & 0xc0) == 0x80)) {
      ret.first += 1;
  }

  if (ret.first != end) {
    const size_t scalar_saved_bytes = scalar::utf8_to_utf16::convert_valid<endianness::LITTLE>(
                                        ret.first, len - (ret.first - buf), ret.second);
    if (scalar_saved_bytes == 0) { return 0; }
    saved_bytes += scalar_saved_bytes;
  }

  return saved_bytes;
}

TURBO_MUST_USE_RESULT size_t Implementation::ConvertValidUtf8ToUtf16Be(const char* buf, size_t len, char16_t* utf16_output) const noexcept {
  utf8_to_utf16_result ret = icelake::valid_utf8_to_fixed_length<endianness::BIG, char16_t>(buf, len, utf16_output);
  size_t saved_bytes = ret.second - utf16_output;
  const char* end = buf + len;
  if (ret.first == end) {
    return saved_bytes;
  }

  // Note: AVX512 procedure looks up 4 bytes forward, and
  //       correctly converts multi-byte chars even if their
  //       continuation bytes lie outsiede 16-byte window.
  //       It meas, we have to skip continuation bytes from
  //       the beginning ret.first, as they were already consumed.
  while (ret.first != end && ((uint8_t(*ret.first) & 0xc0) == 0x80)) {
      ret.first += 1;
  }

  if (ret.first != end) {
    const size_t scalar_saved_bytes = scalar::utf8_to_utf16::convert_valid<endianness::BIG>(
                                        ret.first, len - (ret.first - buf), ret.second);
    if (scalar_saved_bytes == 0) { return 0; }
    saved_bytes += scalar_saved_bytes;
  }

  return saved_bytes;
}


TURBO_MUST_USE_RESULT size_t Implementation::ConvertUtf8ToUtf32(const char* buf, size_t len, char32_t* utf32_out) const noexcept {
  uint32_t * utf32_output = reinterpret_cast<uint32_t *>(utf32_out);
  utf8_to_utf32_result ret = icelake::validating_utf8_to_fixed_length<endianness::LITTLE, uint32_t>(buf, len, utf32_output);
  if (ret.second == nullptr)
    return 0;

  size_t saved_bytes = ret.second - utf32_output;
  const char* end = buf + len;
  if (ret.first == end) {
    return saved_bytes;
  }

  // Note: the AVX512 procedure looks up 4 bytes forward, and
  //       correctly converts multi-byte chars even if their
  //       continuation bytes lie outside 16-byte window.
  //       It means, we have to skip continuation bytes from
  //       the beginning ret.first, as they were already consumed.
  while (ret.first != end and ((uint8_t(*ret.first) & 0xc0) == 0x80)) {
      ret.first += 1;
  }

  if (ret.first != end) {
    const size_t scalar_saved_bytes = scalar::utf8_to_utf32::convert(
                                        ret.first, len - (ret.first - buf), utf32_out + saved_bytes);
    if (scalar_saved_bytes == 0) { return 0; }
    saved_bytes += scalar_saved_bytes;
  }

  return saved_bytes;
}

TURBO_MUST_USE_RESULT result Implementation::ConvertUtf8ToUtf32WithErrors(const char* buf, size_t len, char32_t* utf32) const noexcept {
  uint32_t * utf32_output = reinterpret_cast<uint32_t *>(utf32);
  auto ret = icelake::validating_utf8_to_fixed_length_with_constant_checks<endianness::LITTLE, uint32_t>(buf, len, utf32_output);
  if (!std::get<2>(ret)) {
    auto new_buf = std::get<0>(ret);
    // rewind_and_convert_with_errors will seek a potential error from new_buf onward,
    // with the ability to go back up to new_buf - buf bytes, and read len - (new_buf - buf) bytes forward.
    result res = scalar::utf8_to_utf32::rewind_and_convert_with_errors(new_buf - buf, new_buf, len - (new_buf - buf), reinterpret_cast<char32_t *>(std::get<1>(ret)));
    res.count += (std::get<0>(ret) - buf);
    return res;
  }
  size_t saved_bytes = std::get<1>(ret) - utf32_output;
  const char* end = buf + len;
  if (std::get<0>(ret) == end) {
    return {turbo::SUCCESS, saved_bytes};
  }

  // Note: the AVX512 procedure looks up 4 bytes forward, and
  //       correctly converts multi-byte chars even if their
  //       continuation bytes lie outside 16-byte window.
  //       It means, we have to skip continuation bytes from
  //       the beginning ret.first, as they were already consumed.
  while (std::get<0>(ret) != end and ((uint8_t(*std::get<0>(ret)) & 0xc0) == 0x80)) {
      std::get<0>(ret) += 1;
  }

  if (std::get<0>(ret) != end) {
    auto scalar_result = scalar::utf8_to_utf32::convert_with_errors(
                                        std::get<0>(ret), len - (std::get<0>(ret) - buf), reinterpret_cast<char32_t *>(utf32_output) + saved_bytes);
    if (scalar_result.error != turbo::SUCCESS) {
      scalar_result.count +=  (std::get<0>(ret) - buf);
    } else {
      scalar_result.count += saved_bytes;
    }
    return scalar_result;
  }

  return {turbo::SUCCESS, size_t(std::get<1>(ret) - utf32_output)};
}


TURBO_MUST_USE_RESULT size_t Implementation::ConvertValidUtf8ToUtf32(const char* buf, size_t len, char32_t* utf32_out) const noexcept {
  uint32_t * utf32_output = reinterpret_cast<uint32_t *>(utf32_out);
  utf8_to_utf32_result ret = icelake::valid_utf8_to_fixed_length<endianness::LITTLE, uint32_t>(buf, len, utf32_output);
  size_t saved_bytes = ret.second - utf32_output;
  const char* end = buf + len;
  if (ret.first == end) {
    return saved_bytes;
  }

  // Note: AVX512 procedure looks up 4 bytes forward, and
  //       correctly converts multi-byte chars even if their
  //       continuation bytes lie outsiede 16-byte window.
  //       It meas, we have to skip continuation bytes from
  //       the beginning ret.first, as they were already consumed.
  while (ret.first != end && ((uint8_t(*ret.first) & 0xc0) == 0x80)) {
      ret.first += 1;
  }

  if (ret.first != end) {
    const size_t scalar_saved_bytes = scalar::utf8_to_utf32::convert_valid(
                                        ret.first, len - (ret.first - buf), utf32_out + saved_bytes);
    if (scalar_saved_bytes == 0) { return 0; }
    saved_bytes += scalar_saved_bytes;
  }

  return saved_bytes;
}

TURBO_MUST_USE_RESULT size_t Implementation::ConvertUtf16LeToUtf8(const char16_t* buf, size_t len, char* utf8_output) const noexcept {
  size_t outlen;
  size_t inlen = utf16_to_utf8_avx512i<endianness::LITTLE>(buf, len, (unsigned char*)utf8_output, &outlen);
  if(inlen != len) { return 0; }
  return outlen;
}

TURBO_MUST_USE_RESULT size_t Implementation::ConvertUtf16BeToUtf8(const char16_t* buf, size_t len, char* utf8_output) const noexcept {
  size_t outlen;
  size_t inlen = utf16_to_utf8_avx512i<endianness::BIG>(buf, len, (unsigned char*)utf8_output, &outlen);
  if(inlen != len) { return 0; }
  return outlen;
}

TURBO_MUST_USE_RESULT result Implementation::ConvertUtf16LeToUtf8WithErrors(const char16_t* buf, size_t len, char* utf8_output) const noexcept {
  size_t outlen;
  size_t inlen = utf16_to_utf8_avx512i<endianness::LITTLE>(buf, len, (unsigned char*)utf8_output, &outlen);
  if(inlen != len) {
    result res = scalar::utf16_to_utf8::convert_with_errors<endianness::LITTLE>(buf + inlen, len - outlen, utf8_output + outlen);
    res.count += inlen;
    return res;
  }
  return {turbo::SUCCESS, outlen};
}

TURBO_MUST_USE_RESULT result Implementation::ConvertUtf16BeToUtf8WithErrors(const char16_t* buf, size_t len, char* utf8_output) const noexcept {
  size_t outlen;
  size_t inlen = utf16_to_utf8_avx512i<endianness::BIG>(buf, len, (unsigned char*)utf8_output, &outlen);
  if(inlen != len) {
    result res = scalar::utf16_to_utf8::convert_with_errors<endianness::BIG>(buf + inlen, len - outlen, utf8_output + outlen);
    res.count += inlen;
    return res;
  }
  return {turbo::SUCCESS, outlen};
}

TURBO_MUST_USE_RESULT size_t Implementation::ConvertValidUtf16LeToUtf8(const char16_t* buf, size_t len, char* utf8_output) const noexcept {
  return ConvertUtf16LeToUtf8(buf, len, utf8_output);
}

TURBO_MUST_USE_RESULT size_t Implementation::ConvertValidUtf16BeToUtf8(const char16_t* buf, size_t len, char* utf8_output) const noexcept {
  return ConvertUtf16BeToUtf8(buf, len, utf8_output);
}


TURBO_MUST_USE_RESULT size_t Implementation::ConvertUtf32ToUtf8(const char32_t* buf, size_t len, char* utf8_output) const noexcept {
  std::pair<const char32_t*, char*> ret = avx512_convert_utf32_to_utf8(buf, len, utf8_output);
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
  std::pair<result, char*> ret = icelake::avx512_convert_utf32_to_utf8_with_errors(buf, len, utf8_output);
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

TURBO_MUST_USE_RESULT size_t Implementation::ConvertValidUtf32ToUtf8(const char32_t* buf, size_t len, char* utf8_output) const noexcept {
  return ConvertUtf32ToUtf8(buf, len, utf8_output);
}

TURBO_MUST_USE_RESULT size_t Implementation::ConvertUtf32ToUtf16Le(const char32_t* buf, size_t len, char16_t* utf16_output) const noexcept {
  std::pair<const char32_t*, char16_t*> ret = avx512_convert_utf32_to_utf16<endianness::LITTLE>(buf, len, utf16_output);
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
  std::pair<const char32_t*, char16_t*> ret = avx512_convert_utf32_to_utf16<endianness::BIG>(buf, len, utf16_output);
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
  std::pair<result, char16_t*> ret = avx512_convert_utf32_to_utf16_with_errors<endianness::LITTLE>(buf, len, utf16_output);
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
  std::pair<result, char16_t*> ret = avx512_convert_utf32_to_utf16_with_errors<endianness::BIG>(buf, len, utf16_output);
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

TURBO_MUST_USE_RESULT size_t Implementation::ConvertUtf16LeToUtf32(const char16_t* buf, size_t len, char32_t* utf32_output) const noexcept {
  std::tuple<const char16_t*, char32_t*, bool> ret = icelake::ConvertUtf16ToUtf32<endianness::LITTLE>(buf, len, utf32_output);
  if (!std::get<2>(ret)) { return 0; }
  size_t saved_bytes = std::get<1>(ret) - utf32_output;
  if (std::get<0>(ret) != buf + len) {
    const size_t scalar_saved_bytes = scalar::utf16_to_utf32::convert<endianness::LITTLE>(
                                        std::get<0>(ret), len - (std::get<0>(ret) - buf), std::get<1>(ret));
    if (scalar_saved_bytes == 0) { return 0; }
    saved_bytes += scalar_saved_bytes;
  }
  return saved_bytes;
}

TURBO_MUST_USE_RESULT size_t Implementation::ConvertUtf16BeToUtf32(const char16_t* buf, size_t len, char32_t* utf32_output) const noexcept {
  std::tuple<const char16_t*, char32_t*, bool> ret = icelake::ConvertUtf16ToUtf32<endianness::BIG>(buf, len, utf32_output);
  if (!std::get<2>(ret)) { return 0; }
  size_t saved_bytes = std::get<1>(ret) - utf32_output;
  if (std::get<0>(ret) != buf + len) {
    const size_t scalar_saved_bytes = scalar::utf16_to_utf32::convert<endianness::BIG>(
                                        std::get<0>(ret), len - (std::get<0>(ret) - buf), std::get<1>(ret));
    if (scalar_saved_bytes == 0) { return 0; }
    saved_bytes += scalar_saved_bytes;
  }
  return saved_bytes;
}

TURBO_MUST_USE_RESULT result Implementation::ConvertUtf16LeToUtf32WithErrors(const char16_t* buf, size_t len, char32_t* utf32_output) const noexcept {
  std::tuple<const char16_t*, char32_t*, bool> ret = icelake::ConvertUtf16ToUtf32<endianness::LITTLE>(buf, len, utf32_output);
  if (!std::get<2>(ret)) {
    result scalar_res = scalar::utf16_to_utf32::convert_with_errors<endianness::LITTLE>(
                                        std::get<0>(ret), len - (std::get<0>(ret) - buf), std::get<1>(ret));
    scalar_res.count += (std::get<0>(ret) - buf);
    return scalar_res;
  }
  size_t saved_bytes = std::get<1>(ret) - utf32_output;
  if (std::get<0>(ret) != buf + len) {
    result scalar_res = scalar::utf16_to_utf32::convert_with_errors<endianness::LITTLE>(
                                        std::get<0>(ret), len - (std::get<0>(ret) - buf), std::get<1>(ret));
    if (scalar_res.error) {
      scalar_res.count += (std::get<0>(ret) - buf);
      return scalar_res;
    } else {
      scalar_res.count += saved_bytes;
      return scalar_res;
    }
  }
  return turbo::result(turbo::SUCCESS, saved_bytes);
}

TURBO_MUST_USE_RESULT result Implementation::ConvertUtf16BeToUtf32WithErrors(const char16_t* buf, size_t len, char32_t* utf32_output) const noexcept {
  std::tuple<const char16_t*, char32_t*, bool> ret = icelake::ConvertUtf16ToUtf32<endianness::BIG>(buf, len, utf32_output);
  if (!std::get<2>(ret)) {
    result scalar_res = scalar::utf16_to_utf32::convert_with_errors<endianness::BIG>(
                                        std::get<0>(ret), len - (std::get<0>(ret) - buf), std::get<1>(ret));
    scalar_res.count += (std::get<0>(ret) - buf);
    return scalar_res;
  }
  size_t saved_bytes = std::get<1>(ret) - utf32_output;
  if (std::get<0>(ret) != buf + len) {
    result scalar_res = scalar::utf16_to_utf32::convert_with_errors<endianness::BIG>(
                                        std::get<0>(ret), len - (std::get<0>(ret) - buf), std::get<1>(ret));
    if (scalar_res.error) {
      scalar_res.count += (std::get<0>(ret) - buf);
      return scalar_res;
    } else {
      scalar_res.count += saved_bytes;
      return scalar_res;
    }
  }
  return turbo::result(turbo::SUCCESS, saved_bytes);
}

TURBO_MUST_USE_RESULT size_t Implementation::ConvertValidUtf16LeToUtf32(const char16_t* buf, size_t len, char32_t* utf32_output) const noexcept {
  std::tuple<const char16_t*, char32_t*, bool> ret = icelake::ConvertUtf16ToUtf32<endianness::LITTLE>(buf, len, utf32_output);
  if (!std::get<2>(ret)) { return 0; }
  size_t saved_bytes = std::get<1>(ret) - utf32_output;
  if (std::get<0>(ret) != buf + len) {
    const size_t scalar_saved_bytes = scalar::utf16_to_utf32::convert<endianness::LITTLE>(
                                        std::get<0>(ret), len - (std::get<0>(ret) - buf), std::get<1>(ret));
    if (scalar_saved_bytes == 0) { return 0; }
    saved_bytes += scalar_saved_bytes;
  }
  return saved_bytes;
}

TURBO_MUST_USE_RESULT size_t Implementation::ConvertValidUtf16BeToUtf32(const char16_t* buf, size_t len, char32_t* utf32_output) const noexcept {
  std::tuple<const char16_t*, char32_t*, bool> ret = icelake::ConvertUtf16ToUtf32<endianness::BIG>(buf, len, utf32_output);
  if (!std::get<2>(ret)) { return 0; }
  size_t saved_bytes = std::get<1>(ret) - utf32_output;
  if (std::get<0>(ret) != buf + len) {
    const size_t scalar_saved_bytes = scalar::utf16_to_utf32::convert<endianness::BIG>(
                                        std::get<0>(ret), len - (std::get<0>(ret) - buf), std::get<1>(ret));
    if (scalar_saved_bytes == 0) { return 0; }
    saved_bytes += scalar_saved_bytes;
  }
  return saved_bytes;
}

void Implementation::ChangeEndiannessUtf16(const char16_t * input, size_t length, char16_t * output) const noexcept {
  size_t pos = 0;
  const __m512i byteflip = _mm512_setr_epi64(
            0x0607040502030001,
            0x0e0f0c0d0a0b0809,
            0x0607040502030001,
            0x0e0f0c0d0a0b0809,
            0x0607040502030001,
            0x0e0f0c0d0a0b0809,
            0x0607040502030001,
            0x0e0f0c0d0a0b0809
        );
  while (pos + 32 <= length) {
    __m512i utf16 = _mm512_loadu_si512((const __m512i*)(input + pos));
    utf16 = _mm512_shuffle_epi8(utf16, byteflip);
    _mm512_storeu_si512(output + pos, utf16);
    pos += 32;
  }
  if(pos < length) {
    __mmask32 m((1<< (length - pos))-1);
    __m512i utf16 = _mm512_maskz_loadu_epi16(m, (const __m512i*)(input + pos));
    utf16 = _mm512_shuffle_epi8(utf16, byteflip);
    _mm512_mask_storeu_epi16(output + pos, m, utf16);
  }
}


TURBO_MUST_USE_RESULT size_t Implementation::CountUtf16Le(const char16_t * input, size_t length) const noexcept {
  const char16_t* end = length >= 32 ? input + length - 32 : nullptr;
  const char16_t* ptr = input;

  const __m512i low = _mm512_set1_epi16((uint16_t)0xdc00);
  const __m512i high = _mm512_set1_epi16((uint16_t)0xdfff);

  size_t count{0};

  while (ptr <= end) {
    __m512i utf16 = _mm512_loadu_si512((const __m512i*)ptr);
    ptr += 32;
    uint64_t not_high_surrogate = static_cast<uint64_t>(_mm512_cmpgt_epu16_mask(utf16, high) | _mm512_cmplt_epu16_mask(utf16, low));
    count += count_ones(not_high_surrogate);
  }

  return count + scalar::utf16::count_code_points<endianness::LITTLE>(ptr, length - (ptr - input));
}

TURBO_MUST_USE_RESULT size_t Implementation::CountUtf16Be(const char16_t * input, size_t length) const noexcept {
  const char16_t* end = length >= 32 ? input + length - 32 : nullptr;
  const char16_t* ptr = input;

  const __m512i low = _mm512_set1_epi16((uint16_t)0xdc00);
  const __m512i high = _mm512_set1_epi16((uint16_t)0xdfff);

  size_t count{0};
  const __m512i byteflip = _mm512_setr_epi64(
            0x0607040502030001,
            0x0e0f0c0d0a0b0809,
            0x0607040502030001,
            0x0e0f0c0d0a0b0809,
            0x0607040502030001,
            0x0e0f0c0d0a0b0809,
            0x0607040502030001,
            0x0e0f0c0d0a0b0809
        );
  while (ptr <= end) {
    __m512i utf16 = _mm512_shuffle_epi8(_mm512_loadu_si512((__m512i*)const_cast<char16_t*>(ptr)), byteflip);
    ptr += 32;
    uint64_t not_high_surrogate = static_cast<uint64_t>(_mm512_cmpgt_epu16_mask(utf16, high) | _mm512_cmplt_epu16_mask(utf16, low));
    count += count_ones(not_high_surrogate);
  }

  return count + scalar::utf16::count_code_points<endianness::BIG>(ptr, length - (ptr - input));
}


TURBO_MUST_USE_RESULT size_t Implementation::CountUtf8(const char * input, size_t length) const noexcept {
  const char* end = length >= 64 ? input + length - 64 : nullptr;
  const char* ptr = input;

  const __m512i continuation = _mm512_set1_epi8(char(0b10111111));

  size_t count{0};

  while (ptr <= end) {
    __m512i utf8 = _mm512_loadu_si512((const __m512i*)ptr);
    ptr += 64;
    uint64_t continuation_bitmask = static_cast<uint64_t>(_mm512_cmple_epi8_mask(utf8, continuation));
    count += 64 - count_ones(continuation_bitmask);
  }

  return count + scalar::utf8::count_code_points(ptr, length - (ptr - input));
}


TURBO_MUST_USE_RESULT size_t Implementation::Utf8LengthFromUtf16Le(const char16_t * input, size_t length) const noexcept {
  const char16_t* end = length >= 32 ? input + length - 32 : nullptr;
  const char16_t* ptr = input;

  const __m512i v_007f = _mm512_set1_epi16((uint16_t)0x007f);
  const __m512i v_07ff = _mm512_set1_epi16((uint16_t)0x07ff);
  const __m512i v_dfff = _mm512_set1_epi16((uint16_t)0xdfff);
  const __m512i v_d800 = _mm512_set1_epi16((uint16_t)0xd800);

  size_t count{0};

  while (ptr <= end) {
    __m512i utf16 = _mm512_loadu_si512((const __m512i*)ptr);
    ptr += 32;
    __mmask32 ascii_bitmask = _mm512_cmple_epu16_mask(utf16, v_007f);
    __mmask32 two_bytes_bitmask = _mm512_mask_cmple_epu16_mask(~ascii_bitmask, utf16, v_07ff);
    __mmask32 not_one_two_bytes = ~(ascii_bitmask | two_bytes_bitmask);
    __mmask32 surrogates_bitmask = _mm512_mask_cmple_epu16_mask(not_one_two_bytes, utf16, v_dfff) & _mm512_mask_cmpge_epu16_mask(not_one_two_bytes, utf16, v_d800);

    size_t ascii_count = count_ones(ascii_bitmask);
    size_t two_bytes_count = count_ones(two_bytes_bitmask);
    size_t surrogate_bytes_count = count_ones(surrogates_bitmask);
    size_t three_bytes_count = 32 - ascii_count - two_bytes_count - surrogate_bytes_count;

    count += ascii_count + 2*two_bytes_count + 3*three_bytes_count + 2*surrogate_bytes_count;
  }

  return count + scalar::utf16::Utf8LengthFromUtf16<endianness::LITTLE>(ptr, length - (ptr - input));
}

TURBO_MUST_USE_RESULT size_t Implementation::Utf8LengthFromUtf16be(const char16_t * input, size_t length) const noexcept {
  const char16_t* end = length >= 32 ? input + length - 32 : nullptr;
  const char16_t* ptr = input;

  const __m512i v_007f = _mm512_set1_epi16((uint16_t)0x007f);
  const __m512i v_07ff = _mm512_set1_epi16((uint16_t)0x07ff);
  const __m512i v_dfff = _mm512_set1_epi16((uint16_t)0xdfff);
  const __m512i v_d800 = _mm512_set1_epi16((uint16_t)0xd800);

  size_t count{0};
  const __m512i byteflip = _mm512_setr_epi64(
            0x0607040502030001,
            0x0e0f0c0d0a0b0809,
            0x0607040502030001,
            0x0e0f0c0d0a0b0809,
            0x0607040502030001,
            0x0e0f0c0d0a0b0809,
            0x0607040502030001,
            0x0e0f0c0d0a0b0809
        );
  while (ptr <= end) {
    __m512i utf16 = _mm512_loadu_si512((const __m512i*)ptr);
    utf16 = _mm512_shuffle_epi8(utf16, byteflip);
    ptr += 32;
    __mmask32 ascii_bitmask = _mm512_cmple_epu16_mask(utf16, v_007f);
    __mmask32 two_bytes_bitmask = _mm512_mask_cmple_epu16_mask(~ascii_bitmask, utf16, v_07ff);
    __mmask32 not_one_two_bytes = ~(ascii_bitmask | two_bytes_bitmask);
    __mmask32 surrogates_bitmask = _mm512_mask_cmple_epu16_mask(not_one_two_bytes, utf16, v_dfff) & _mm512_mask_cmpge_epu16_mask(not_one_two_bytes, utf16, v_d800);

    size_t ascii_count = count_ones(ascii_bitmask);
    size_t two_bytes_count = count_ones(two_bytes_bitmask);
    size_t surrogate_bytes_count = count_ones(surrogates_bitmask);
    size_t three_bytes_count = 32 - ascii_count - two_bytes_count - surrogate_bytes_count;
    count += ascii_count + 2*two_bytes_count + 3*three_bytes_count + 2*surrogate_bytes_count;
  }

  return count + scalar::utf16::Utf8LengthFromUtf16<endianness::BIG>(ptr, length - (ptr - input));
}

TURBO_MUST_USE_RESULT size_t Implementation::Utf32LengthFromUtf16Le(const char16_t * input, size_t length) const noexcept {
  return Implementation::CountUtf16Le(input, length);
}

TURBO_MUST_USE_RESULT size_t Implementation::Utf32LengthFromUtf16Be(const char16_t * input, size_t length) const noexcept {
  return Implementation::CountUtf16Be(input, length);
}

TURBO_MUST_USE_RESULT size_t Implementation::Utf16LengthFromUtf8(const char * input, size_t length) const noexcept {
    size_t pos = 0;
    size_t count = 0;
    // This algorithm could no doubt be improved!
    for(;pos + 64 <= length; pos += 64) {
      __m512i utf8 = _mm512_loadu_si512((const __m512i*)(input+pos));
      uint64_t utf8_continuation_mask = _mm512_cmple_epi8_mask(utf8, _mm512_set1_epi8(-65+1));
      // We count one word for anything that is not a continuation (so
      // leading bytes).
      count += 64 - count_ones(utf8_continuation_mask);
      uint64_t utf8_4byte = _mm512_cmpge_epu8_mask(utf8, _mm512_set1_epi8(int8_t(240)));
      count += count_ones(utf8_4byte);
    }
    return count + scalar::utf8::Utf16LengthFromUtf8(input + pos, length - pos);
}

TURBO_MUST_USE_RESULT size_t Implementation::Utf8LengthFromUtf32(const char32_t * input, size_t length) const noexcept {
  const char32_t* end = length >= 16 ? input + length - 16 : nullptr;
  const char32_t* ptr = input;

  const __m512i v_0000_007f = _mm512_set1_epi32((uint32_t)0x7f);
  const __m512i v_0000_07ff = _mm512_set1_epi32((uint32_t)0x7ff);
  const __m512i v_0000_ffff = _mm512_set1_epi32((uint32_t)0x0000ffff);

  size_t count{0};

  while (ptr <= end) {
    __m512i utf32 = _mm512_loadu_si512((const __m512i*)ptr);
    ptr += 16;
    __mmask16 ascii_bitmask = _mm512_cmple_epu32_mask(utf32, v_0000_007f);
    __mmask16 two_bytes_bitmask = _mm512_mask_cmple_epu32_mask(_knot_mask16(ascii_bitmask), utf32, v_0000_07ff);
    __mmask16 three_bytes_bitmask = _mm512_mask_cmple_epu32_mask(_knot_mask16(_mm512_kor(ascii_bitmask, two_bytes_bitmask)), utf32, v_0000_ffff);

    size_t ascii_count = count_ones(ascii_bitmask);
    size_t two_bytes_count = count_ones(two_bytes_bitmask);
    size_t three_bytes_count = count_ones(three_bytes_bitmask);
    size_t four_bytes_count = 16 - ascii_count - two_bytes_count - three_bytes_count;
    count += ascii_count + 2*two_bytes_count + 3*three_bytes_count + 4*four_bytes_count;
  }

  return count + scalar::utf32::Utf8LengthFromUtf32(ptr, length - (ptr - input));
}

TURBO_MUST_USE_RESULT size_t Implementation::Utf16LengthFromUtf32(const char32_t * input, size_t length) const noexcept {
  const char32_t* end = length >= 16 ? input + length - 16 : nullptr;
  const char32_t* ptr = input;

  const __m512i v_0000_ffff = _mm512_set1_epi32((uint32_t)0x0000ffff);

  size_t count{0};

  while (ptr <= end) {
    __m512i utf32 = _mm512_loadu_si512((const __m512i*)ptr);
    ptr += 16;
    __mmask16 surrogates_bitmask = _mm512_cmpgt_epu32_mask(utf32, v_0000_ffff);

    count += 16 + count_ones(surrogates_bitmask);
  }

  return count + scalar::utf32::Utf16LengthFromUtf32(ptr, length - (ptr - input));
}

TURBO_MUST_USE_RESULT size_t Implementation::Utf32LengthFromUtf8(const char * input, size_t length) const noexcept {
  return Implementation::CountUtf8(input, length);
}

} // namespace TURBO_UNICODE_IMPLEMENTATION
} // namespace turbo

#include "turbo/unicode/icelake/end.h"
