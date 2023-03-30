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

#ifndef TURBO_UNICODE_UTF_H_
#define TURBO_UNICODE_UTF_H_

#include <cstring>

#include "turbo/unicode/simdutf/compiler_check.h"
#include "turbo/unicode/simdutf/common_defs.h"
#include "turbo/unicode/simdutf/encoding_types.h"
#include "turbo/unicode/simdutf/error.h"

SIMDUTF_PUSH_DISABLE_WARNINGS
SIMDUTF_DISABLE_UNDESIRED_WARNINGS

// Public API
#include "simdutf/simdutf_version.h"
#include "simdutf/implementation.h"


// Implementation-internal files (must be included before the implementations themselves, to keep
// amalgamation working--otherwise, the first time a file is included, it might be put inside the
// #ifdef SIMDUTF_IMPLEMENTATION_ARM64/FALLBACK/etc., which means the other implementations can't
// compile unless that implementation is turned on).
#include "simdutf/internal/isadetection.h"


SIMDUTF_POP_DISABLE_WARNINGS

#endif // TURBO_UNICODE_UTF_H_
