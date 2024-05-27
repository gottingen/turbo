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
//
// UTF8 utilities, implemented to reduce dependencies.

#ifndef TURBO_STRINGS_INTERNAL_UTF8_H_
#define TURBO_STRINGS_INTERNAL_UTF8_H_

#include <cstddef>
#include <cstdint>

#include <turbo/base/config.h>

namespace turbo {
TURBO_NAMESPACE_BEGIN
namespace strings_internal {

// For Unicode code points 0 through 0x10FFFF, EncodeUTF8Char writes
// out the UTF-8 encoding into buffer, and returns the number of chars
// it wrote.
//
// As described in https://tools.ietf.org/html/rfc3629#section-3 , the encodings
// are:
//    00 -     7F : 0xxxxxxx
//    80 -    7FF : 110xxxxx 10xxxxxx
//   800 -   FFFF : 1110xxxx 10xxxxxx 10xxxxxx
// 10000 - 10FFFF : 11110xxx 10xxxxxx 10xxxxxx 10xxxxxx
//
// Values greater than 0x10FFFF are not supported and may or may not write
// characters into buffer, however never will more than kMaxEncodedUTF8Size
// bytes be written, regardless of the value of utf8_char.
enum { kMaxEncodedUTF8Size = 4 };
size_t EncodeUTF8Char(char *buffer, char32_t utf8_char);

}  // namespace strings_internal
TURBO_NAMESPACE_END
}  // namespace turbo

#endif  // TURBO_STRINGS_INTERNAL_UTF8_H_
