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

#include <turbo/strings/internal/str_format/output.h>

#include <errno.h>
#include <cstring>

namespace turbo {
TURBO_NAMESPACE_BEGIN
namespace str_format_internal {

namespace {
struct ClearErrnoGuard {
  ClearErrnoGuard() : old_value(errno) { errno = 0; }
  ~ClearErrnoGuard() {
    if (!errno) errno = old_value;
  }
  int old_value;
};
}  // namespace

void BufferRawSink::Write(string_view v) {
  size_t to_write = std::min(v.size(), size_);
  std::memcpy(buffer_, v.data(), to_write);
  buffer_ += to_write;
  size_ -= to_write;
  total_written_ += v.size();
}

void FILERawSink::Write(string_view v) {
  while (!v.empty() && !error_) {
    // Reset errno to zero in case the libc implementation doesn't set errno
    // when a failure occurs.
    ClearErrnoGuard guard;

    if (size_t result = std::fwrite(v.data(), 1, v.size(), output_)) {
      // Some progress was made.
      count_ += result;
      v.remove_prefix(result);
    } else {
      if (errno == EINTR) {
        continue;
      } else if (errno) {
        error_ = errno;
      } else if (std::ferror(output_)) {
        // Non-POSIX compliant libc implementations may not set errno, so we
        // have check the streams error indicator.
        error_ = EBADF;
      } else {
        // We're likely on a non-POSIX system that encountered EINTR but had no
        // way of reporting it.
        continue;
      }
    }
  }
}

}  // namespace str_format_internal
TURBO_NAMESPACE_END
}  // namespace turbo
