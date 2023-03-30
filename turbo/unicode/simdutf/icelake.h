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

#ifndef SIMDUTF_ICELAKE_H
#define SIMDUTF_ICELAKE_H


#include "turbo/unicode/simdutf/portability.h"

#ifdef __has_include
// How do we detect that a compiler supports vbmi2?
// For sure if the following header is found, we are ok?
#if __has_include(<avx512vbmi2intrin.h>)
#define SIMDUTF_COMPILER_SUPPORTS_VBMI2 1
#endif
#endif

#ifdef _MSC_VER
#if _MSC_VER >= 1920
// Visual Studio 2019 and up support VBMI2 under x64 even if the header
// avx512vbmi2intrin.h is not found.
#define SIMDUTF_COMPILER_SUPPORTS_VBMI2 1
#endif
#endif

// We allow icelake on x64 as long as the compiler is known to support VBMI2.
#ifndef SIMDUTF_IMPLEMENTATION_ICELAKE
#define SIMDUTF_IMPLEMENTATION_ICELAKE ((SIMDUTF_IS_X86_64) && (SIMDUTF_COMPILER_SUPPORTS_VBMI2))
#endif

// To see why  (__BMI__) && (__PCLMUL__) && (__LZCNT__) are not part of this next line, see
// https://github.com/simdutf/simdutf/issues/1247
#define SIMDUTF_CAN_ALWAYS_RUN_ICELAKE ((SIMDUTF_IMPLEMENTATION_ICELAKE) && (SIMDUTF_IS_X86_64) && (__AVX2__) && (SIMDUTF_HAS_AVX512F && \
                                         SIMDUTF_HAS_AVX512DQ && \
                                         SIMDUTF_HAS_AVX512VL && \
                                           SIMDUTF_HAS_AVX512VBMI2) && (!SIMDUTF_IS_32BITS))

#if SIMDUTF_IMPLEMENTATION_ICELAKE
#if SIMDUTF_CAN_ALWAYS_RUN_ICELAKE
#define SIMDUTF_TARGET_ICELAKE
#else
#define SIMDUTF_TARGET_ICELAKE SIMDUTF_TARGET_REGION("avx512f,avx512dq,avx512cd,avx512bw,avx512vbmi,avx512vbmi2,avx512vl,avx2,bmi,bmi2,pclmul,lzcnt")
#endif

namespace simdutf {
namespace icelake {
} // namespace icelake
} // namespace simdutf



//
// These two need to be included outside SIMDUTF_TARGET_REGION
//
#include "turbo/unicode/icelake/intrinsics.h"
#include "turbo/unicode/icelake/implementation.h"

//
// The rest need to be inside the region
//
#include "turbo/unicode/icelake/begin.h"
// Declarations
#include "turbo/unicode/icelake/bitmanipulation.h"
#include "turbo/unicode/icelake/end.h"



#endif // SIMDUTF_IMPLEMENTATION_ICELAKE
#endif // SIMDUTF_ICELAKE_H
