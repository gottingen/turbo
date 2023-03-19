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

#include "turbo/unicode/error.h"
#include "turbo/base/turbo_error.h"

//TODO(jeff) fill error reason
TURBO_REGISTER_ERRNO(turbo::HEADER_BITS, "HEADER_BITS");
TURBO_REGISTER_ERRNO(turbo::TOO_SHORT, "TOO_SHORT");
TURBO_REGISTER_ERRNO(turbo::TOO_LONG, "TOO_LONG");
TURBO_REGISTER_ERRNO(turbo::OVERLONG, "OVERLONG");
TURBO_REGISTER_ERRNO(turbo::TOO_LARGE, "TOO_LARGE");
TURBO_REGISTER_ERRNO(turbo::SURROGATE, "SURROGATE");
TURBO_REGISTER_ERRNO(turbo::OTHER, "OTHER");