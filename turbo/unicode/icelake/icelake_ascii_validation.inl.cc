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

// file included directly

bool ValidateAscii(const char* buf, size_t len) {
  const char* end = buf + len;
  const __m512i ascii = _mm512_set1_epi8((uint8_t)0x80);
  __m512i running_or = _mm512_setzero_si512();
  for (; buf + 64 <= end; buf += 64) {
    const __m512i utf8 = _mm512_loadu_si512((const __m512i*)buf);
    running_or = _mm512_ternarylogic_epi32(running_or, utf8, ascii, 0xf8); // running_or | (utf8 & ascii)
  }
  if(buf < end) {
     const __m512i utf8 = _mm512_maskz_loadu_epi8((uint64_t(1) << (end-buf)) - 1,(const __m512i*)buf);
    running_or = _mm512_ternarylogic_epi32(running_or, utf8, ascii, 0xf8); // running_or | (utf8 & ascii)
  }
  return (_mm512_test_epi8_mask(running_or, running_or) == 0);
}
