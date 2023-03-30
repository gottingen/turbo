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

#ifndef SIMDUTF_PPC64_H
#define SIMDUTF_PPC64_H

#ifdef SIMDUTF_FALLBACK_H
#error "ppc64.h must be included before fallback.h"
#endif

#include "turbo/unicode/simdutf/portability.h"

#ifndef SIMDUTF_IMPLEMENTATION_PPC64
#define SIMDUTF_IMPLEMENTATION_PPC64 (SIMDUTF_IS_PPC64)
#endif
#define SIMDUTF_CAN_ALWAYS_RUN_PPC64 SIMDUTF_IMPLEMENTATION_PPC64 && SIMDUTF_IS_PPC64


#include "turbo/unicode/internal/isadetection.h"

#if SIMDUTF_IMPLEMENTATION_PPC64

namespace simdutf {
/**
 * Implementation for ALTIVEC (PPC64).
 */
namespace ppc64 {
} // namespace ppc64
} // namespace simdutf

#include "simdutf/ppc64/implementation.h"

#include "simdutf/ppc64/begin.h"

// Declarations
#include "simdutf/ppc64/intrinsics.h"
#include "simdutf/ppc64/bitmanipulation.h"
#include "simdutf/ppc64/simd.h"

#include "simdutf/ppc64/end.h"

#endif // SIMDUTF_IMPLEMENTATION_PPC64

#endif // SIMDUTF_PPC64_H
