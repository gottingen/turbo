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

// UTF8 utilities, implemented to reduce dependencies.

#include <turbo/strings/internal/utf8.h>

namespace turbo {
TURBO_NAMESPACE_BEGIN
namespace strings_internal {

size_t EncodeUTF8Char(char *buffer, char32_t utf8_char) {
  if (utf8_char <= 0x7F) {
    *buffer = static_cast<char>(utf8_char);
    return 1;
  } else if (utf8_char <= 0x7FF) {
    buffer[1] = static_cast<char>(0x80 | (utf8_char & 0x3F));
    utf8_char >>= 6;
    buffer[0] = static_cast<char>(0xC0 | utf8_char);
    return 2;
  } else if (utf8_char <= 0xFFFF) {
    buffer[2] = static_cast<char>(0x80 | (utf8_char & 0x3F));
    utf8_char >>= 6;
    buffer[1] = static_cast<char>(0x80 | (utf8_char & 0x3F));
    utf8_char >>= 6;
    buffer[0] = static_cast<char>(0xE0 | utf8_char);
    return 3;
  } else {
    buffer[3] = static_cast<char>(0x80 | (utf8_char & 0x3F));
    utf8_char >>= 6;
    buffer[2] = static_cast<char>(0x80 | (utf8_char & 0x3F));
    utf8_char >>= 6;
    buffer[1] = static_cast<char>(0x80 | (utf8_char & 0x3F));
    utf8_char >>= 6;
    buffer[0] = static_cast<char>(0xF0 | utf8_char);
    return 4;
  }
}

}  // namespace strings_internal
TURBO_NAMESPACE_END
}  // namespace turbo
