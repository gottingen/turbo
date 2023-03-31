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

#ifndef TURBO_UNICODE_INTERNAL_CONFIG_H_
#define TURBO_UNICODE_INTERNAL_CONFIG_H_

#include <cstddef>
#include <cstdint>
#include <cstdlib>
#include <cfloat>
#include <cassert>
#ifndef _WIN32
// strcasecmp, strncasecmp
#include <strings.h>
#endif
#include <cassert>
#include "turbo/platform/port.h"
#ifdef _MSC_VER
// https://en.wikipedia.org/wiki/C_alternative_tokens
// This header should have no effect, except maybe
// under Visual Studio.
#include <iso646.h>
#endif

#endif // TURBO_UNICODE_INTERNAL_CONFIG_H_
