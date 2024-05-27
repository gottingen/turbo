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

#include <turbo/strings/internal/memutil.h>

#include <cstdlib>

#include <turbo/strings/ascii.h>

namespace turbo {
TURBO_NAMESPACE_BEGIN
namespace strings_internal {

int memcasecmp(const char* s1, const char* s2, size_t len) {
  const unsigned char* us1 = reinterpret_cast<const unsigned char*>(s1);
  const unsigned char* us2 = reinterpret_cast<const unsigned char*>(s2);

  for (size_t i = 0; i < len; i++) {
    unsigned char c1 = us1[i];
    unsigned char c2 = us2[i];
    // If bytes are the same, they will be the same when converted to lower.
    // So we only need to convert if bytes are not equal.
    // NOTE(b/308193381): We do not use `turbo::ascii_tolower` here in order
    // to avoid its lookup table and improve performance.
    if (c1 != c2) {
      c1 = c1 >= 'A' && c1 <= 'Z' ? c1 - 'A' + 'a' : c1;
      c2 = c2 >= 'A' && c2 <= 'Z' ? c2 - 'A' + 'a' : c2;
      const int diff = int{c1} - int{c2};
      if (diff != 0) return diff;
    }
  }
  return 0;
}

}  // namespace strings_internal
TURBO_NAMESPACE_END
}  // namespace turbo
