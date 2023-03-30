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

#ifndef SIMDUTF_ARM64_H
#define SIMDUTF_ARM64_H

#ifdef SIMDUTF_FALLBACK_H
#error "arm64.h must be included before fallback.h"
#endif

#include "turbo/unicode/simdutf/portability.h"

#ifndef SIMDUTF_IMPLEMENTATION_ARM64
#define SIMDUTF_IMPLEMENTATION_ARM64 (SIMDUTF_IS_ARM64)
#endif
#define SIMDUTF_CAN_ALWAYS_RUN_ARM64 SIMDUTF_IMPLEMENTATION_ARM64 && SIMDUTF_IS_ARM64


#include "turbo/unicode/simdutf/internal/isadetection.h"

#if SIMDUTF_IMPLEMENTATION_ARM64

namespace simdutf {
/**
 * Implementation for NEON (ARMv8).
 */
namespace arm64 {
} // namespace arm64
} // namespace simdutf

#include "turbo/unicode/simdutf/arm64/implementation.h"

#include "turbo/unicode/simdutf/arm64/begin.h"

// Declarations
#include "turbo/unicode/simdutf/arm64/intrinsics.h"
#include "turbo/unicode/simdutf/arm64/bitmanipulation.h"
#include "turbo/unicode/simdutf/arm64/simd.h"

#include "turbo/unicode/simdutf/arm64/end.h"

#endif // SIMDUTF_IMPLEMENTATION_ARM64

#endif // SIMDUTF_ARM64_H
