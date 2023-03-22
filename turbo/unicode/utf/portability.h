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

#ifndef SIMDUTF_PORTABILITY_H
#define SIMDUTF_PORTABILITY_H

#include <cstddef>
#include <cstdint>
#include <cstdlib>
#include <cfloat>
#include <cassert>
#ifndef _WIN32
// strcasecmp, strncasecmp
#include <strings.h>
#endif

/**
 * At this point in time, TURBO_IS_BIG_ENDIAN is defined.
 */

#ifdef _MSC_VER
#define SIMDUTF_VISUAL_STUDIO 1
/**
 * We want to differentiate carefully between
 * clang under visual studio and regular visual
 * studio.
 *
 * Under clang for Windows, we enable:
 *  * target pragmas so that part and only part of the
 *     code gets compiled for advanced instructions.
 *
 */
#ifdef __clang__
// clang under visual studio
#define SIMDUTF_CLANG_VISUAL_STUDIO 1
#else
// just regular visual studio (best guess)
#define SIMDUTF_REGULAR_VISUAL_STUDIO 1
#endif // __clang__
#endif // _MSC_VER

#ifdef SIMDUTF_REGULAR_VISUAL_STUDIO
// https://en.wikipedia.org/wiki/C_alternative_tokens
// This header should have no effect, except maybe
// under Visual Studio.
#include <iso646.h>
#endif

#if defined(__PPC64__) || defined(_M_PPC64)
//#define SIMDUTF_IS_PPC64 1
// The simdutf library does yet support SIMD acceleration under
// POWER processors. Please see https://github.com/lemire/simdutf/issues/51
#elif defined(__s390__)
// s390 IBM system. Big endian.
#else
// The simdutf library is designed
// for 64-bit processors and it seems that you are not
// compiling for a known 64-bit platform. Please
// use a 64-bit target such as x64 or 64-bit ARM for best performance.

// We do not support 32-bit platforms, but it can be
// handy to identify them.
#if defined(_M_IX86) || defined(__i386__)
#define SIMDUTF_IS_X86_32BITS 1
#elif defined(__arm__) || defined(_M_ARM)
#define SIMDUTF_IS_ARM_32BITS 1
#elif defined(__PPC__) || defined(_M_PPC)
#define SIMDUTF_IS_PPC_32BITS 1
#endif

#endif // defined(__x86_64__) || defined(_M_AMD64)


// this is almost standard?
#define SIMDUTF_STRINGIFY_IMPLEMENTATION_(a) #a
#define SIMDUTF_STRINGIFY(a) SIMDUTF_STRINGIFY_IMPLEMENTATION_(a)

// Our fast kernels require 64-bit systems.
//
// On 32-bit x86, we lack 64-bit popcnt, lzcnt, blsr instructions.
// Furthermore, the number of SIMD registers is reduced.
//
// On 32-bit ARM, we would have smaller registers.
//
// The simdutf users should still have the fallback kernel. It is
// slower, but it should run everywhere.

//
// Enable valid runtime implementations, and select SIMDUTF_BUILTIN_IMPLEMENTATION
//

// We are going to use runtime dispatch.
#ifdef SIMDUTF_IS_X86_64
#ifdef __clang__
// clang does not have GCC push pop
// warning: clang attribute push can't be used within a namespace in clang up
// til 8.0 so SIMDUTF_TARGET_REGION and SIMDUTF_UNTARGET_REGION must be *outside* of a
// namespace.
#define SIMDUTF_TARGET_REGION(T)                                                       \
  _Pragma(SIMDUTF_STRINGIFY(                                                           \
      clang attribute push(__attribute__((target(T))), apply_to = function)))
#define SIMDUTF_UNTARGET_REGION _Pragma("clang attribute pop")
#elif defined(__GNUC__)
// GCC is easier
#define SIMDUTF_TARGET_REGION(T)                                                       \
  _Pragma("GCC push_options") _Pragma(SIMDUTF_STRINGIFY(GCC target(T)))
#define SIMDUTF_UNTARGET_REGION _Pragma("GCC pop_options")
#endif // clang then gcc

#endif // x86

// Default target region macros don't do anything.
#ifndef SIMDUTF_TARGET_REGION
#define SIMDUTF_TARGET_REGION(T)
#define SIMDUTF_UNTARGET_REGION
#endif


#if defined(__GNUC__) && !defined(__clang__)
#if __GNUC__ >= 11
#define SIMDUTF_GCC11ORMORE 1
#endif //  __GNUC__ >= 11
#endif // defined(__GNUC__) && !defined(__clang__)


#endif // SIMDUTF_PORTABILITY_H
