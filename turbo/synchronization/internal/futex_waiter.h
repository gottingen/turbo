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

#ifndef TURBO_SYNCHRONIZATION_INTERNAL_FUTEX_WAITER_H_
#define TURBO_SYNCHRONIZATION_INTERNAL_FUTEX_WAITER_H_

#include <atomic>
#include <cstdint>

#include <turbo/base/config.h>
#include <turbo/synchronization/internal/kernel_timeout.h>
#include <turbo/synchronization/internal/futex.h>
#include <turbo/synchronization/internal/waiter_base.h>

#ifdef TURBO_INTERNAL_HAVE_FUTEX

namespace turbo {
TURBO_NAMESPACE_BEGIN
namespace synchronization_internal {

#define TURBO_INTERNAL_HAVE_FUTEX_WAITER 1

class FutexWaiter : public WaiterCrtp<FutexWaiter> {
 public:
  FutexWaiter() : futex_(0) {}

  bool Wait(KernelTimeout t);
  void Post();
  void Poke();

  static constexpr char kName[] = "FutexWaiter";

 private:
  // Atomically check that `*v == val`, and if it is, then sleep until the
  // timeout `t` has been reached, or until woken by `Wake()`.
  static int WaitUntil(std::atomic<int32_t>* v, int32_t val,
                       KernelTimeout t);

  // Futexes are defined by specification to be 32-bits.
  // Thus std::atomic<int32_t> must be just an int32_t with lockfree methods.
  std::atomic<int32_t> futex_;
  static_assert(sizeof(int32_t) == sizeof(futex_), "Wrong size for futex");
};

}  // namespace synchronization_internal
TURBO_NAMESPACE_END
}  // namespace turbo

#endif  // TURBO_INTERNAL_HAVE_FUTEX

#endif  // TURBO_SYNCHRONIZATION_INTERNAL_FUTEX_WAITER_H_
