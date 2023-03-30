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

#ifndef SIMDUTF_FALLBACK_H
#define SIMDUTF_FALLBACK_H

#include "turbo/unicode/simdutf/portability.h"

// Note that fallback.h is always imported last.

// Default Fallback to on unless a builtin implementation has already been selected.
#ifndef SIMDUTF_IMPLEMENTATION_FALLBACK
#if SIMDUTF_CAN_ALWAYS_RUN_ARM64 || SIMDUTF_CAN_ALWAYS_RUN_ICELAKE || SIMDUTF_CAN_ALWAYS_RUN_HASWELL || SIMDUTF_CAN_ALWAYS_RUN_WESTMERE || SIMDUTF_CAN_ALWAYS_RUN_PPC64
#define SIMDUTF_IMPLEMENTATION_FALLBACK 0
#else
#define SIMDUTF_IMPLEMENTATION_FALLBACK 1
#endif
#endif

#define SIMDUTF_CAN_ALWAYS_RUN_FALLBACK (SIMDUTF_IMPLEMENTATION_FALLBACK)

#if SIMDUTF_IMPLEMENTATION_FALLBACK

namespace simdutf {
/**
 * Fallback implementation (runs on any machine).
 */
namespace fallback {
} // namespace fallback
} // namespace simdutf

#include "turbo/unicode/fallback/implementation.h"

#include "turbo/unicode/fallback/begin.h"

// Declarations
#include "turbo/unicode/fallback/bitmanipulation.h"

#include "turbo/unicode/fallback/end.h"

#endif // SIMDUTF_IMPLEMENTATION_FALLBACK
#endif // SIMDUTF_FALLBACK_H
