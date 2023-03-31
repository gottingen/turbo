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

#ifndef SIMDUTF_WESTMERE_H
#define SIMDUTF_WESTMERE_H

#ifdef SIMDUTF_FALLBACK_H
#error "westmere.h must be included before fallback.h"
#endif

#include "turbo/unicode/simdutf/portability.h"

// Default Westmere to on if this is x86-64, unless we'll always select Haswell.
#ifndef TURBO_UNICODE_IMPLEMENTATION_WESTMERE
//
// You do not want to set it to (SIMDUTF_IS_X86_64 && !SIMDUTF_REQUIRES_HASWELL)
// because you want to rely on runtime dispatch!
//
#if SIMDUTF_CAN_ALWAYS_RUN_ICELAKE || SIMDUTF_CAN_ALWAYS_RUN_HASWELL
#define TURBO_UNICODE_IMPLEMENTATION_WESTMERE 0
#else
#define TURBO_UNICODE_IMPLEMENTATION_WESTMERE (SIMDUTF_IS_X86_64)
#endif

#endif

#define SIMDUTF_CAN_ALWAYS_RUN_WESTMERE (TURBO_UNICODE_IMPLEMENTATION_WESTMERE && SIMDUTF_IS_X86_64 && __SSE4_2__ && __PCLMUL__)

#if TURBO_UNICODE_IMPLEMENTATION_WESTMERE

#define SIMDUTF_TARGET_WESTMERE SIMDUTF_TARGET_REGION("sse4.2,pclmul")

namespace turbo {
/**
 * Implementation for Westmere (Intel SSE4.2).
 */
namespace westmere {
} // namespace westmere
} // namespace turbo

//
// These two need to be included outside SIMDUTF_TARGET_REGION
//
#include "turbo/unicode/westmere/implementation.h"
#include "turbo/unicode/westmere/intrinsics.h"

//
// The rest need to be inside the region
//
#include "turbo/unicode/westmere/begin.h"

// Declarations
#include "turbo/unicode/westmere/bitmanipulation.h"
#include "turbo/unicode/westmere/simd.h"

#include "turbo/unicode/westmere/end.h"

#endif // TURBO_UNICODE_IMPLEMENTATION_WESTMERE
#endif // SIMDUTF_WESTMERE_COMMON_H
