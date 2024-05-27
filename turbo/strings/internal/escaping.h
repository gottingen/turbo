// Copyright (C) 2024 EA group inc.
// Author: Jeff.li lijippy@163.com
// All rights reserved.
// This program is free software: you can redistribute it and/or modify
// it under the terms of the GNU Affero General Public License as published
// by the Free Software Foundation, either version 3 of the License, or
// (at your option) any later version.
//
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU Affero General Public License for more details.
//
// You should have received a copy of the GNU Affero General Public License
// along with this program.  If not, see <https://www.gnu.org/licenses/>.
//

#ifndef TURBO_STRINGS_INTERNAL_ESCAPING_H_
#define TURBO_STRINGS_INTERNAL_ESCAPING_H_

#include <cassert>

#include <turbo/strings/internal/resize_uninitialized.h>

namespace turbo {
TURBO_NAMESPACE_BEGIN
namespace strings_internal {

TURBO_CONST_INIT extern const char kBase64Chars[];
TURBO_CONST_INIT extern const char kWebSafeBase64Chars[];

// Calculates the length of a Base64 encoding (RFC 4648) of a string of length
// `input_len`, with or without padding per `do_padding`. Note that 'web-safe'
// encoding (section 5 of the RFC) does not change this length.
size_t CalculateBase64EscapedLenInternal(size_t input_len, bool do_padding);

// Base64-encodes `src` using the alphabet provided in `base64` (which
// determines whether to do web-safe encoding or not) and writes the result to
// `dest`. If `do_padding` is true, `dest` is padded with '=' chars until its
// length is a multiple of 3. Returns the length of `dest`.
size_t Base64EscapeInternal(const unsigned char* src, size_t szsrc, char* dest,
                            size_t szdest, const char* base64, bool do_padding);
template <typename String>
void Base64EscapeInternal(const unsigned char* src, size_t szsrc, String* dest,
                          bool do_padding, const char* base64_chars) {
  const size_t calc_escaped_size =
      CalculateBase64EscapedLenInternal(szsrc, do_padding);
  STLStringResizeUninitialized(dest, calc_escaped_size);

  const size_t escaped_len = Base64EscapeInternal(
      src, szsrc, &(*dest)[0], dest->size(), base64_chars, do_padding);
  assert(calc_escaped_size == escaped_len);
  dest->erase(escaped_len);
}

}  // namespace strings_internal
TURBO_NAMESPACE_END
}  // namespace turbo

#endif  // TURBO_STRINGS_INTERNAL_ESCAPING_H_
