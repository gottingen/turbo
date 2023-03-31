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

#ifndef TURBO_UNICODE_FALLBACK_H_
#define TURBO_UNICODE_FALLBACK_H_

#include "turbo/unicode/internal/config.h"

// Note that fallback.h is always imported last.

// Default Fallback to on unless a builtin implementation has already been selected.
#ifndef TURBO_UNICODE_IMPLEMENTATION_FALLBACK
#if TURBO_UNICODE_CAN_ALWAYS_RUN_ARM64 || TURBO_UNICODE_CAN_ALWAYS_RUN_ICELAKE || TURBO_UNICODE_CAN_ALWAYS_RUN_HASWELL || TURBO_UNICODE_CAN_ALWAYS_RUN_WESTMERE || TURBO_CAN_ALWAYS_RUN_PPC64
#define TURBO_UNICODE_IMPLEMENTATION_FALLBACK 0
#else
#define TURBO_UNICODE_IMPLEMENTATION_FALLBACK 1
#endif
#endif

#define TURBO_UNICODE_CAN_ALWAYS_RUN_FALLBACK (TURBO_UNICODE_IMPLEMENTATION_FALLBACK)

#if TURBO_UNICODE_IMPLEMENTATION_FALLBACK

namespace turbo {
/**
 * Fallback implementation (runs on any machine).
 */
namespace fallback {
} // namespace fallback
} // namespace turbo

#include "turbo/unicode/fallback/implementation.h"

#include "turbo/unicode/fallback/begin.h"

// Declarations
#include "turbo/unicode/fallback/bitmanipulation.h"

#include "turbo/unicode/fallback/end.h"

#endif // TURBO_UNICODE_IMPLEMENTATION_FALLBACK
#endif // TURBO_UNICODE_FALLBACK_H_
