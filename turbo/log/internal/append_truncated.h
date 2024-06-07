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

#ifndef TURBO_LOG_INTERNAL_APPEND_TRUNCATED_H_
#define TURBO_LOG_INTERNAL_APPEND_TRUNCATED_H_

#include <cstddef>
#include <cstring>

#include <turbo/base/config.h>
#include <turbo/strings/string_view.h>
#include <turbo/container/span.h>

namespace turbo {
TURBO_NAMESPACE_BEGIN
namespace log_internal {
// Copies into `dst` as many bytes of `src` as will fit, then truncates the
// copied bytes from the front of `dst` and returns the number of bytes written.
inline size_t AppendTruncated(std::string_view src, turbo::Span<char> &dst) {
  if (src.size() > dst.size()) src = src.substr(0, dst.size());
  memcpy(dst.data(), src.data(), src.size());
  dst.remove_prefix(src.size());
  return src.size();
}
// Likewise, but `n` copies of `c`.
inline size_t AppendTruncated(char c, size_t n, turbo::Span<char> &dst) {
  if (n > dst.size()) n = dst.size();
  memset(dst.data(), c, n);
  dst.remove_prefix(n);
  return n;
}
}  // namespace log_internal
TURBO_NAMESPACE_END
}  // namespace turbo

#endif  // TURBO_LOG_INTERNAL_APPEND_TRUNCATED_H_
