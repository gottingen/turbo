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

#include "turbo/unicode/internal/config.h"
#include "turbo/utf/encoding_types.h"
#include "turbo/unicode/error.h"

// Public API
#include "turbo/unicode/implementation.h"


// Implementation-internal files (must be included before the implementations themselves, to keep
// amalgamation working--otherwise, the first time a file is included, it might be put inside the
// #ifdef TURBO_UNICODE_IMPLEMENTATION_ARM64/FALLBACK/etc., which means the other implementations can't
// compile unless that implementation is turned on).
#include "turbo/unicode/internal/isadetection.h"


#endif // TURBO_UNICODE_UTF_H_
