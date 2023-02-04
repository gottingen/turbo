// Copyright 2022 The Turbo Authors
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//     https://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.

#ifndef TURBO_LOG_INTERNAL_APPEND_TRUNCATED_H_
#define TURBO_LOG_INTERNAL_APPEND_TRUNCATED_H_

#include <cstddef>
#include <cstring>

#include "turbo/meta/span.h"
#include "turbo/platform/port.h"
#include "turbo/strings/string_view.h"

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
