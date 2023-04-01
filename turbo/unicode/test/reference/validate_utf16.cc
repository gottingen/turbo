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

#include "validate_utf16.h"

#ifndef TURBO_IS_BIG_ENDIAN
#error "TURBO_IS_BIG_ENDIAN should be defined."
#endif

namespace turbo {
namespace tests {
namespace reference {

TURBO_MUST_USE_RESULT bool ValidateUtf16(const char16_t *buf, size_t len) noexcept {
  const char16_t* curr = buf;
  const char16_t* end = buf + len;

  while (curr != end) {
#if TURBO_IS_BIG_ENDIAN
      // By convention, we always take as an input an UTF-16LE.
      const uint16_t W1 = uint16_t((uint16_t(*curr) << 8) | (uint16_t(*curr) >> 8));
#else
      const uint16_t W1 = *curr;
#endif

      curr += 1;

      if (W1 < 0xd800 || W1 > 0xdfff) { // fast path, code point is equal to character's value
        continue;
      }

      if (W1 > 0xdbff) { // W1 must be in range 0xd800 .. 0xdbff
        return false;
      }

      if (curr == end) { // required the next word, but we're already at the end of data
        return false;
      }
#if TURBO_IS_BIG_ENDIAN
      // By convention, we always take as an input an UTF-16LE.
      const uint16_t W2 = uint16_t((uint16_t(*curr) << 8) | (uint16_t(*curr) >> 8));
#else
      const uint16_t W2 = *curr;
#endif

      if (W2 < 0xdc00 || W2 > 0xdfff) // W2 = 0xdc00 .. 0xdfff
        return false;

      curr += 1;
  }

  return true;
}

} // namespace reference
} // namespace tests
} // namespace turbo


