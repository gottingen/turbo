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

#ifndef TURBO_UTF_ERROR_H_
#define TURBO_UTF_ERROR_H_

#include <cstddef>

namespace turbo {

enum class UtfError {
  SUCCESS = 0,
  HEADER_BITS,  // Any byte must have fewer than 5 header bits.
  TOO_SHORT,    // The leading byte must be followed by N-1 continuation bytes, where N is the UTF-8 character length
                // This is also the error when the input is truncated.
  TOO_LONG,     // We either have too many consecutive continuation bytes or the string starts with a continuation byte.
  OVERLONG,     // The decoded character must be above U+7F for two-byte characters, U+7FF for three-byte characters,
                // and U+FFFF for four-byte characters.
  TOO_LARGE,    // The decoded character must be less than or equal to U+10FFFF OR less than or equal than U+7F for ASCII.
  SURROGATE,    // The decoded character must be not be in U+D800...DFFF (UTF-8 or UTF-32) OR
                // a high surrogate must be followed by a low surrogate and a low surrogate must be preceded by a high surrogate (UTF-16)
  OTHER         // Not related to validation/transcoding.
};


struct UtfResult {
    UtfError error{UtfError::SUCCESS};
    size_t count{0};     //!< In case of error, indicates the position of the error. In case of success, indicates the number of words validated/written.

    UtfResult() = default;

    UtfResult(UtfError error, size_t count) : error{error}, count{count} {};
};

}
#endif  // TURBO_UTF_ERROR_H_
