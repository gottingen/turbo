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

#ifndef SIMDUTF_HASWELL_H
#define SIMDUTF_HASWELL_H

#ifdef SIMDUTF_WESTMERE_H
#error "haswell.h must be included before westmere.h"
#endif
#ifdef SIMDUTF_FALLBACK_H
#error "haswell.h must be included before fallback.h"
#endif

#include "turbo/unicode/simdutf/portability.h"

// Default Haswell to on if this is x86-64. Even if we're not compiled for it, it could be selected
// at runtime.
#ifndef SIMDUTF_IMPLEMENTATION_HASWELL
//
// You do not want to restrict it like so: SIMDUTF_IS_X86_64 && __AVX2__
// because we want to rely on *runtime dispatch*.
//
#if SIMDUTF_CAN_ALWAYS_RUN_ICELAKE
#define SIMDUTF_IMPLEMENTATION_HASWELL 0
#else
#define SIMDUTF_IMPLEMENTATION_HASWELL (SIMDUTF_IS_X86_64)
#endif

#endif
// To see why  (__BMI__) && (__PCLMUL__) && (__LZCNT__) are not part of this next line, see
// https://github.com/simdutf/simdutf/issues/1247
#define SIMDUTF_CAN_ALWAYS_RUN_HASWELL ((SIMDUTF_IMPLEMENTATION_HASWELL) && (SIMDUTF_IS_X86_64) && (__AVX2__))

#if SIMDUTF_IMPLEMENTATION_HASWELL

#define SIMDUTF_TARGET_HASWELL SIMDUTF_TARGET_REGION("avx2,bmi,pclmul,lzcnt")

namespace simdutf {
/**
 * Implementation for Haswell (Intel AVX2).
 */
namespace haswell {
} // namespace haswell
} // namespace simdutf

//
// These two need to be included outside SIMDUTF_TARGET_REGION
//
#include "turbo/unicode/simdutf/haswell/implementation.h"
#include "turbo/unicode/simdutf/haswell/intrinsics.h"

//
// The rest need to be inside the region
//
#include "turbo/unicode/simdutf/haswell/begin.h"
// Declarations
#include "turbo/unicode/simdutf/haswell/bitmanipulation.h"
#include "turbo/unicode/simdutf/haswell/simd.h"

#include "turbo/unicode/simdutf/haswell/end.h"

#endif // SIMDUTF_IMPLEMENTATION_HASWELL
#endif // SIMDUTF_HASWELL_COMMON_H
