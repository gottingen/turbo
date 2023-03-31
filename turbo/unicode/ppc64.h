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

#ifndef TURBO_UNICODE_PPC64_H_
#define TURBO_UNICODE_PPC64_H_

#ifdef TURBO_UNICODE_FALLBACK_H_
#error "ppc64.h must be included before fallback.h"
#endif

#include "turbo/unicode/internal/config.h"

#if !defined(TURBO_UNICODE_IMPLEMENTATION_PPC64) && defined(TURBO_PROCESSOR_POWERPC)
#define TURBO_UNICODE_IMPLEMENTATION_PPC64 1
#else
#define TURBO_UNICODE_IMPLEMENTATION_PPC64 0
#endif

#if defined(TURBO_PROCESSOR_POWERPC)
#define TURBO_CAN_ALWAYS_RUN_PPC64 TURBO_UNICODE_IMPLEMENTATION_PPC64
#else
#define TURBO_CAN_ALWAYS_RUN_PPC64 0
#endif

#include "turbo/unicode/internal/isadetection.h"

#if TURBO_UNICODE_IMPLEMENTATION_PPC64

namespace turbo {
/**
 * Implementation for ALTIVEC (PPC64).
 */
namespace ppc64 {
} // namespace ppc64
} // namespace turbo

#include "turbo/unicode/ppc64/implementation.h"

#include "turbo/unicode/ppc64/begin.h"

// Declarations
#include "turbo/unicode/ppc64/intrinsics.h"
#include "turbo/unicode/ppc64/bitmanipulation.h"
#include "turbo/unicode/ppc64/simd.h"

#include "turbo/unicode/ppc64/end.h"

#endif // TURBO_UNICODE_IMPLEMENTATION_PPC64

#endif // TURBO_UNICODE_PPC64_H_
