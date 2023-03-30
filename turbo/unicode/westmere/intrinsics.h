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

#ifndef SIMDUTF_WESTMERE_INTRINSICS_H
#define SIMDUTF_WESTMERE_INTRINSICS_H

#ifdef SIMDUTF_VISUAL_STUDIO
// under clang within visual studio, this will include <x86intrin.h>
#include <intrin.h> // visual studio or clang
#else

#if SIMDUTF_GCC11ORMORE
// We should not get warnings while including <x86intrin.h> yet we do
// under some versions of GCC.
// If the x86intrin.h header has uninitialized values that are problematic,
// it is a GCC issue, we want to ignore these warnigns.
SIMDUTF_DISABLE_GCC_WARNING(-Wuninitialized)
#endif

#include <x86intrin.h> // elsewhere


#if SIMDUTF_GCC11ORMORE
// cancels the suppression of the -Wuninitialized
SIMDUTF_POP_DISABLE_WARNINGS
#endif

#endif // SIMDUTF_VISUAL_STUDIO


#ifdef SIMDUTF_CLANG_VISUAL_STUDIO
/**
 * You are not supposed, normally, to include these
 * headers directly. Instead you should either include intrin.h
 * or x86intrin.h. However, when compiling with clang
 * under Windows (i.e., when _MSC_VER is set), these headers
 * only get included *if* the corresponding features are detected
 * from macros:
 */
#include <smmintrin.h>  // for _mm_alignr_epi8
#include <wmmintrin.h>  // for  _mm_clmulepi64_si128
#endif



#endif // SIMDUTF_WESTMERE_INTRINSICS_H
