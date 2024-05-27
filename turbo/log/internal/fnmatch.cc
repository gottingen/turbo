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

#include <turbo/log/internal/fnmatch.h>

#include <cstddef>

#include <turbo/base/config.h>
#include <turbo/strings/string_view.h>

namespace turbo {
TURBO_NAMESPACE_BEGIN
namespace log_internal {
bool FNMatch(turbo::string_view pattern, turbo::string_view str) {
  bool in_wildcard_match = false;
  while (true) {
    if (pattern.empty()) {
      // `pattern` is exhausted; succeed if all of `str` was consumed matching
      // it.
      return in_wildcard_match || str.empty();
    }
    if (str.empty()) {
      // `str` is exhausted; succeed if `pattern` is empty or all '*'s.
      return pattern.find_first_not_of('*') == pattern.npos;
    }
    switch (pattern.front()) {
      case '*':
        pattern.remove_prefix(1);
        in_wildcard_match = true;
        break;
      case '?':
        pattern.remove_prefix(1);
        str.remove_prefix(1);
        break;
      default:
        if (in_wildcard_match) {
          turbo::string_view fixed_portion = pattern;
          const size_t end = fixed_portion.find_first_of("*?");
          if (end != fixed_portion.npos) {
            fixed_portion = fixed_portion.substr(0, end);
          }
          const size_t match = str.find(fixed_portion);
          if (match == str.npos) {
            return false;
          }
          pattern.remove_prefix(fixed_portion.size());
          str.remove_prefix(match + fixed_portion.size());
          in_wildcard_match = false;
        } else {
          if (pattern.front() != str.front()) {
            return false;
          }
          pattern.remove_prefix(1);
          str.remove_prefix(1);
        }
        break;
    }
  }
}
}  // namespace log_internal
TURBO_NAMESPACE_END
}  // namespace turbo
