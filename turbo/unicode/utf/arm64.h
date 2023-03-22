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

#ifndef TURBO_UNICODE_UTF_ARM64_H_
#define TURBO_UNICODE_UTF_ARM64_H_

#ifdef TURBO_UNICODE_UTF_FALLBACK_H_
#error "arm64.h must be included before fallback.h"
#endif

#include "turbo/unicode/utf/common_defs.h"

#ifndef TURBO_UNICODE_IMPLEMENTATION_ARM64
#define TURBO_UNICODE_IMPLEMENTATION_ARM64 (TURBO_PROCESSOR_ARM64)
#endif
#define TURBO_UNICODE_CAN_ALWAYS_RUN_ARM64 TURBO_UNICODE_IMPLEMENTATION_ARM64 && defined(TURBO_PROCESSOR_ARM64)


#include "turbo/unicode/utf/internal/isadetection.h"

#if TURBO_UNICODE_IMPLEMENTATION_ARM64

namespace turbo {
/**
 * Implementation for NEON (ARMv8).
 */
namespace arm64 {
} // namespace arm64
} // namespace turbo

#include "turbo/unicode/utf/arm64/implementation.h"

#include "turbo/unicode/utf/arm64/begin.h"

// Declarations
#include "turbo/unicode/utf/arm64/intrinsics.h"
#include "turbo/unicode/utf/arm64/bitmanipulation.h"
#include "turbo/unicode/utf/arm64/simd.h"

#include "turbo/unicode/utf/arm64/end.h"

#endif // TURBO_UNICODE_IMPLEMENTATION_ARM64

#endif // TURBO_UNICODE_UTF_ARM64_H_
