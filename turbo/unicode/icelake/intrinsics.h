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

#ifndef TURBO_UNICODE_ICELAKE_INTRINSICS_H_
#define TURBO_UNICODE_ICELAKE_INTRINSICS_H_

#include "turbo/unicode/utf.h"

#ifdef _MSC_VER
// under clang within visual studio, this will include <x86intrin.h>
#include <intrin.h>  // visual studio or clang
#include <immintrin.h>
#else


#include <x86intrin.h> // elsewhere

#ifndef _tzcnt_u64
#define _tzcnt_u64(x) __tzcnt_u64(x)
#endif // _tzcnt_u64
#endif // _MSC_VER

#if defined(__GNUC__) && !defined(__clang__) && __GNUC__ == 8
#pragma GCC push_options
#pragma GCC target("avx512f")
/**
 * GCC 8 fails to provide _mm512_set_epi8. We roll our own.
 */
inline __m512i _mm512_set_epi8(uint8_t a0, uint8_t a1, uint8_t a2, uint8_t a3, uint8_t a4, uint8_t a5, uint8_t a6, uint8_t a7, uint8_t a8, uint8_t a9, uint8_t a10, uint8_t a11, uint8_t a12, uint8_t a13, uint8_t a14, uint8_t a15, uint8_t a16, uint8_t a17, uint8_t a18, uint8_t a19, uint8_t a20, uint8_t a21, uint8_t a22, uint8_t a23, uint8_t a24, uint8_t a25, uint8_t a26, uint8_t a27, uint8_t a28, uint8_t a29, uint8_t a30, uint8_t a31, uint8_t a32, uint8_t a33, uint8_t a34, uint8_t a35, uint8_t a36, uint8_t a37, uint8_t a38, uint8_t a39, uint8_t a40, uint8_t a41, uint8_t a42, uint8_t a43, uint8_t a44, uint8_t a45, uint8_t a46, uint8_t a47, uint8_t a48, uint8_t a49, uint8_t a50, uint8_t a51, uint8_t a52, uint8_t a53, uint8_t a54, uint8_t a55, uint8_t a56, uint8_t a57, uint8_t a58, uint8_t a59, uint8_t a60, uint8_t a61, uint8_t a62, uint8_t a63) {
  return _mm512_set_epi64(uint64_t(a7) + (uint64_t(a6) << 8) + (uint64_t(a5) << 16) + (uint64_t(a4) << 24) + (uint64_t(a3) << 32) + (uint64_t(a2) << 40) + (uint64_t(a1) << 48) + (uint64_t(a0) << 56),
                          uint64_t(a15) + (uint64_t(a14) << 8) + (uint64_t(a13) << 16) + (uint64_t(a12) << 24) + (uint64_t(a11) << 32) + (uint64_t(a10) << 40) + (uint64_t(a9) << 48) + (uint64_t(a8) << 56),
                          uint64_t(a23) + (uint64_t(a22) << 8) + (uint64_t(a21) << 16) + (uint64_t(a20) << 24) + (uint64_t(a19) << 32) + (uint64_t(a18) << 40) + (uint64_t(a17) << 48) + (uint64_t(a16) << 56),
                          uint64_t(a31) + (uint64_t(a30) << 8) + (uint64_t(a29) << 16) + (uint64_t(a28) << 24) + (uint64_t(a27) << 32) + (uint64_t(a26) << 40) + (uint64_t(a25) << 48) + (uint64_t(a24) << 56),
                          uint64_t(a39) + (uint64_t(a38) << 8) + (uint64_t(a37) << 16) + (uint64_t(a36) << 24) + (uint64_t(a35) << 32) + (uint64_t(a34) << 40) + (uint64_t(a33) << 48) + (uint64_t(a32) << 56),
                          uint64_t(a47) + (uint64_t(a46) << 8) + (uint64_t(a45) << 16) + (uint64_t(a44) << 24) + (uint64_t(a43) << 32) + (uint64_t(a42) << 40) + (uint64_t(a41) << 48) + (uint64_t(a40) << 56),
                          uint64_t(a55) + (uint64_t(a54) << 8) + (uint64_t(a53) << 16) + (uint64_t(a52) << 24) + (uint64_t(a51) << 32) + (uint64_t(a50) << 40) + (uint64_t(a49) << 48) + (uint64_t(a48) << 56),
                          uint64_t(a63) + (uint64_t(a62) << 8) + (uint64_t(a61) << 16) + (uint64_t(a60) << 24) + (uint64_t(a59) << 32) + (uint64_t(a58) << 40) + (uint64_t(a57) << 48) + (uint64_t(a56) << 56));
}
#pragma GCC pop_options
#endif // GCC8

#endif // TURBO_UNICODE_ICELAKE_INTRINSICS_H_
