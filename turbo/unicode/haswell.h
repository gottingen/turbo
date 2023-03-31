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

#ifndef TURBO_UNICODE_HASWELL_H_
#define TURBO_UNICODE_HASWELL_H_

#ifdef TURBO_UNICODE_WESTMERE_H_
#error "haswell.h must be included before westmere.h"
#endif
#ifdef TURBO_UNICODE_FALLBACK_H_
#error "haswell.h must be included before fallback.h"
#endif

#include "turbo/unicode/internal/config.h"

// Default Haswell to on if this is x86-64. Even if we're not compiled for it, it could be selected
// at runtime.
#ifndef TURBO_UNICODE_IMPLEMENTATION_HASWELL
//
// You do not want to restrict it like so: defined(TURBO_PROCESSOR_X86_64) && __AVX2__
// because we want to rely on *runtime dispatch*.
//
#if TURBO_UNICODE_CAN_ALWAYS_RUN_ICELAKE
#define TURBO_UNICODE_IMPLEMENTATION_HASWELL 0
#elif defined(TURBO_PROCESSOR_X86_64)
#define TURBO_UNICODE_IMPLEMENTATION_HASWELL 1
#else
#define TURBO_UNICODE_IMPLEMENTATION_HASWELL 0
#endif

#endif

#if defined(TURBO_PROCESSOR_X86_64) && (TURBO_UNICODE_IMPLEMENTATION_HASWELL) && (__AVX2__)
#define TURBO_UNICODE_CAN_ALWAYS_RUN_HASWELL 1
#else
#define TURBO_UNICODE_CAN_ALWAYS_RUN_HASWELL 0
#endif

#if TURBO_UNICODE_IMPLEMENTATION_HASWELL

#define TURBO_TARGET_HASWELL TURBO_TARGET_REGION("avx2,bmi,pclmul,lzcnt")

namespace turbo {
/**
 * Implementation for Haswell (Intel AVX2).
 */
namespace haswell {
} // namespace haswell
} // namespace turbo

//
// These two need to be included outside TURBO_TARGET_REGION
//
#include "turbo/unicode/haswell/implementation.h"
#include "turbo/unicode/haswell/intrinsics.h"

//
// The rest need to be inside the region
//
#include "turbo/unicode/haswell/begin.h"
// Declarations
#include "turbo/unicode/haswell/bitmanipulation.h"
#include "turbo/unicode/haswell/simd.h"

#include "turbo/unicode/haswell/end.h"

#endif // TURBO_UNICODE_IMPLEMENTATION_HASWELL
#endif // TURBO_UNICODE_HASWELL_H_
