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

#ifndef TURBO_PLATFORM_CONFIG_ATTRIBUTE_HARDWARE_H_
#define TURBO_PLATFORM_CONFIG_ATTRIBUTE_HARDWARE_H_

#include "turbo/platform/config/compiler_traits.h"


// ------------------------------------------------------------------------
// TURBO_SSE
// Visual C Processor Packs define _MSC_FULL_VER and are needed for SSE
// Intel C also has SSE support.
// TURBO_SSE is used to select FPU or SSE versions in hw_select.inl
//
// TURBO_SSE defines the level of SSE support:
//  0 indicates no SSE support
//  1 indicates SSE1 is supported
//  2 indicates SSE2 is supported
//  3 indicates SSE3 (or greater) is supported
//
// Note: SSE support beyond SSE3 can't be properly represented as a single
// version number.  Instead users should use specific SSE defines (e.g.
// TURBO_SSE4_2) to detect what specific support is available.  TURBO_SSE being
// equal to 3 really only indicates that SSE3 or greater is supported.
#ifndef TURBO_SSE
#if defined(TURBO_COMPILER_GNUC) || defined(TURBO_COMPILER_CLANG)
#if defined(__SSE3__)
#define TURBO_SSE 3
#elif defined(__SSE2__)
#define TURBO_SSE 2
#elif defined(__SSE__) && __SSE__
#define TURBO_SSE 1
#else
#define TURBO_SSE 0
#endif
#elif (defined(TURBO_SSE3) && TURBO_SSE3) || defined TURBO_PLATFORM_XBOXONE || defined CS_UNDEFINED_STRING
#define TURBO_SSE 3
#elif defined(TURBO_SSE2) && TURBO_SSE2
#define TURBO_SSE 2
#elif defined(TURBO_PROCESSOR_X86) && defined(_MSC_FULL_VER) && !defined(__NOSSE__) && defined(_M_IX86_FP)
#define TURBO_SSE _M_IX86_FP
#elif defined(TURBO_PROCESSOR_X86) && defined(TURBO_COMPILER_INTEL) && !defined(__NOSSE__)
#define TURBO_SSE 1
#elif defined(TURBO_PROCESSOR_X86_64)
// All x64 processors support SSE2 or higher
#define TURBO_SSE 2
#else
#define TURBO_SSE 0
#endif
#endif

// ------------------------------------------------------------------------
// We define separate defines for SSE support beyond SSE1.  These defines
// are particularly useful for detecting SSE4.x features since there isn't
// a single concept of SSE4.
//
// The following SSE defines are always defined.  0 indicates the
// feature/level of SSE is not supported, and 1 indicates support is
// available.
#ifndef TURBO_SSE2
#if TURBO_SSE >= 2
#define TURBO_SSE2 1
#else
#define TURBO_SSE2 0
#endif
#endif
#ifndef TURBO_SSE3
#if TURBO_SSE >= 3
#define TURBO_SSE3 1
#else
#define TURBO_SSE3 0
#endif
#endif
#ifndef TURBO_SSSE3
#if defined __SSSE3__ || defined TURBO_PLATFORM_XBOXONE || defined CS_UNDEFINED_STRING
#define TURBO_SSSE3 1
#else
#define TURBO_SSSE3 0
#endif
#endif
#ifndef TURBO_SSE4_1
#if defined __SSE4_1__ || defined TURBO_PLATFORM_XBOXONE || defined CS_UNDEFINED_STRING
#define TURBO_SSE4_1 1
#else
#define TURBO_SSE4_1 0
#endif
#endif
#ifndef TURBO_SSE4_2
#if defined __SSE4_2__ || defined TURBO_PLATFORM_XBOXONE || defined CS_UNDEFINED_STRING
#define TURBO_SSE4_2 1
#else
#define TURBO_SSE4_2 0
#endif
#endif
#ifndef TURBO_SSE4A
#if defined __SSE4A__ || defined TURBO_PLATFORM_XBOXONE || defined CS_UNDEFINED_STRING
#define TURBO_SSE4A 1
#else
#define TURBO_SSE4A 0
#endif
#endif

// ------------------------------------------------------------------------
// TURBO_NEON
// TURBO_NEON may be used to determine if NEON is supported.
#ifndef TURBO_NEON
#if defined(__ARM_NEON__) || defined(__ARM_NEON)
#define TURBO_NEON 1
#else
#define TURBO_NEON 0
#endif
#endif


// ------------------------------------------------------------------------
// TURBO_AVX
// TURBO_AVX may be used to determine if Advanced Vector Extensions are available for the target architecture
//
// TURBO_AVX defines the level of AVX support:
//  0 indicates no AVX support
//  1 indicates AVX1 is supported
//  2 indicates AVX2 is supported
#ifndef TURBO_AVX
#if defined __AVX2__
#define TURBO_AVX 2
#elif defined __AVX__ || defined TURBO_PLATFORM_XBOXONE || defined CS_UNDEFINED_STRING
#define TURBO_AVX 1
#else
#define TURBO_AVX 0
#endif
#endif
#ifndef TURBO_AVX2
#if TURBO_AVX >= 2
#define TURBO_AVX2 1
#else
#define TURBO_AVX2 0
#endif
#endif

// TURBO_FP128 may be used to determine if __float128 is a supported type for use. This type is enabled by a GCC extension (_GLIBCXX_USE_FLOAT128)
// but has support by some implementations of clang (__FLOAT128__)
// PS4 does not support __float128 as of SDK 5.500 https://ps4.siedev.net/resources/documents/SDK/5.500/CPU_Compiler_ABI-Overview/0003.html
#ifndef TURBO_FP128
#if (defined __FLOAT128__ || defined _GLIBCXX_USE_FLOAT128) && !defined(TURBO_PLATFORM_SONY)
#define TURBO_FP128 1
#else
#define TURBO_FP128 0
#endif
#endif

// TURBO_FP16C may be used to determine the existence of float <-> half conversion operations on an x86 CPU.
// (For example to determine if _mm_cvtph_ps or _mm_cvtps_ph could be used.)
#ifndef TURBO_FP16C
#if defined __F16C__ || defined TURBO_PLATFORM_XBOXONE || defined CS_UNDEFINED_STRING
#define TURBO_FP16C 1
#else
#define TURBO_FP16C 0
#endif
#endif

// ------------------------------------------------------------------------
// TURBO_FMA3
// TURBO_FMA3 may be used to determine if Fused Multiply Add operations are available for the target architecture
// __FMA__ is defined only by GCC, Clang, and ICC; MSVC only defines __AVX__ and __AVX2__
// FMA3 was introduced alongside AVX2 on Intel Haswell
// All AMD processors support FMA3 if AVX2 is also supported
//
// TURBO_FMA3 defines the level of FMA3 support:
//  0 indicates no FMA3 support
//  1 indicates FMA3 is supported
#ifndef TURBO_FMA3
#if defined(__FMA__) || TURBO_AVX2 >= 1
#define TURBO_FMA3 1
#else
#define TURBO_FMA3 0
#endif
#endif


#endif // TURBO_PLATFORM_CONFIG_ATTRIBUTE_HARDWARE_H_
