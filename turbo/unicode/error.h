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

#ifndef TURBO_UNICODE_ERROR_H_
#define TURBO_UNICODE_ERROR_H_

#include "turbo/base/status.h"

namespace turbo {

    // Any byte must have fewer than 5 header bits.
    static constexpr StatusCode HEADER_BITS = 20;
    // The leading byte must be followed by N-1 continuation bytes, where N is the UTF-8 character length
    // This is also the error when the input is truncated.
    static constexpr StatusCode TOO_SHORT = 21;
    // We either have too many consecutive continuation bytes or the string starts with a continuation byte.
    static constexpr StatusCode TOO_LONG = 22;
    // The decoded character must be above U+7F for two-byte characters, U+7FF for three-byte characters,
    // and U+FFFF for four-byte characters.
    static constexpr StatusCode OVERLONG = 23;
    // The decoded character must be less than or equal to U+10FFFF OR less than or equal than U+7F for ASCII.
    static constexpr StatusCode TOO_LARGE = 24;
    // The decoded character must be not be in U+D800...DFFF (UTF-8 or UTF-32) OR
    // a high surrogate must be followed by a low surrogate and a low surrogate must be preceded by a high surrogate (UTF-16)
    static constexpr StatusCode SURROGATE = 25;
    // Not related to validation/transcoding.
    static constexpr StatusCode OTHER = 26;
}  // namespace turbo

#endif  // TURBO_UNICODE_ERROR_H_
