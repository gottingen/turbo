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

#ifndef TURBO_UNICODE_UTF_PPC64_H_
#define TURBO_UNICODE_UTF_PPC64_H_

#ifdef TURBO_UNICODE_UTF_FALLBACK_H_
#error "ppc64.h must be included before fallback.h"
#endif

#include "turbo/unicode/utf/portability.h"

#ifndef TURBO_UNICODE_IMPLEMENTATION_PPC64
#define TURBO_UNICODE_IMPLEMENTATION_PPC64 (TURBO_PROCESSOR_POWERPC_64)
#endif
#define TURBO_UNICODE_CAN_ALWAYS_RUN_PPC64 TURBO_UNICODE_IMPLEMENTATION_PPC64 && TURBO_PROCESSOR_POWERPC_64


#include "turbo/unicode/utf/internal/isadetection.h"

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

#endif // TURBO_UNICODE_UTF_PPC64_H_
