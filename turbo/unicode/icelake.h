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

#ifndef TURBO_UNICODE_ICELAKE_H_
#define TURBO_UNICODE_ICELAKE_H_


#include "turbo/unicode/internal/config.h"

#ifdef __has_include
// How do we detect that a compiler supports vbmi2?
// For sure if the following header is found, we are ok?
#if __has_include(<avx512vbmi2intrin.h>)
#define TURBO_UNICODE_COMPILER_SUPPORTS_VBMI2 1
#endif
#endif

#ifdef _MSC_VER
#if _MSC_VER >= 1920
// Visual Studio 2019 and up support VBMI2 under x64 even if the header
// avx512vbmi2intrin.h is not found.
#define TURBO_UNICODE_COMPILER_SUPPORTS_VBMI2 1
#endif
#endif

// We allow icelake on x64 as long as the compiler is known to support VBMI2.
#if !defined(TURBO_UNICODE_IMPLEMENTATION_ICELAKE) && (defined(TURBO_PROCESSOR_X86_64) && (TURBO_UNICODE_COMPILER_SUPPORTS_VBMI2))
#define TURBO_UNICODE_IMPLEMENTATION_ICELAKE 1
#else
#define TURBO_UNICODE_IMPLEMENTATION_ICELAKE 0
#endif

#if ((TURBO_UNICODE_IMPLEMENTATION_ICELAKE) && defined(TURBO_PROCESSOR_X86_64) && (__AVX2__) && (TURBO_WITH_AVX512F && \
                                         TURBO_WITH_AVX512DQ && \
                                         TURBO_HAVE_AVX512VL && \
                                           TURBO_HAVE_AVX512VBMI2))
#define TURBO_UNICODE_CAN_ALWAYS_RUN_ICELAKE 1
#else
#define TURBO_UNICODE_CAN_ALWAYS_RUN_ICELAKE 0
#endif

#if TURBO_UNICODE_IMPLEMENTATION_ICELAKE
#if TURBO_UNICODE_CAN_ALWAYS_RUN_ICELAKE
#define TURBO_UNICODE_TARGET_ICELAKE
#else
#define TURBO_UNICODE_TARGET_ICELAKE TURBO_TARGET_REGION("avx512f,avx512dq,avx512cd,avx512bw,avx512vbmi,avx512vbmi2,avx512vl,avx2,bmi,bmi2,pclmul,lzcnt")
#endif

namespace turbo {
namespace icelake {
} // namespace icelake
} // namespace turbo



//
// These two need to be included outside TURBO_TARGET_REGION
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



#endif // TURBO_UNICODE_IMPLEMENTATION_ICELAKE
#endif // TURBO_UNICODE_ICELAKE_H_
