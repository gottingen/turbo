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

#ifndef TURBO_BASE_INTERNAL_ERRNO_SAVER_H_
#define TURBO_BASE_INTERNAL_ERRNO_SAVER_H_

#include <cerrno>

#include <turbo/base/config.h>

namespace turbo {
TURBO_NAMESPACE_BEGIN
namespace base_internal {

// `ErrnoSaver` captures the value of `errno` upon construction and restores it
// upon deletion.  It is used in low-level code and must be super fast.  Do not
// add instrumentation, even in debug modes.
class ErrnoSaver {
 public:
  ErrnoSaver() : saved_errno_(errno) {}
  ~ErrnoSaver() { errno = saved_errno_; }
  int operator()() const { return saved_errno_; }

 private:
  const int saved_errno_;
};

}  // namespace base_internal
TURBO_NAMESPACE_END
}  // namespace turbo

#endif  // TURBO_BASE_INTERNAL_ERRNO_SAVER_H_
