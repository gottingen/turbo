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

#ifndef TURBO_SYNCHRONIZATION_INTERNAL_WAITER_BASE_H_
#define TURBO_SYNCHRONIZATION_INTERNAL_WAITER_BASE_H_

#include <turbo/base/config.h>
#include <turbo/base/internal/thread_identity.h>
#include <turbo/synchronization/internal/kernel_timeout.h>

namespace turbo {
TURBO_NAMESPACE_BEGIN
namespace synchronization_internal {

// `Waiter` is a platform specific semaphore implementation that `PerThreadSem`
// waits on to implement blocking in `turbo::Mutex`.  Implementations should
// inherit from `WaiterCrtp` and must implement `Wait()`, `Post()`, and `Poke()`
// as described in `WaiterBase`.  `waiter.h` selects the implementation and uses
// static-dispatch for performance.
class WaiterBase {
 public:
  WaiterBase() = default;

  // Not copyable or movable
  WaiterBase(const WaiterBase&) = delete;
  WaiterBase& operator=(const WaiterBase&) = delete;

  // Blocks the calling thread until a matching call to `Post()` or
  // `t` has passed. Returns `true` if woken (`Post()` called),
  // `false` on timeout.
  //
  // bool Wait(KernelTimeout t);

  // Restart the caller of `Wait()` as with a normal semaphore.
  //
  // void Post();

  // If anyone is waiting, wake them up temporarily and cause them to
  // call `MaybeBecomeIdle()`. They will then return to waiting for a
  // `Post()` or timeout.
  //
  // void Poke();

  // Returns the name of this implementation. Used only for debugging.
  //
  // static constexpr char kName[];

  // How many periods to remain idle before releasing resources
#ifndef TURBO_HAVE_THREAD_SANITIZER
  static constexpr int kIdlePeriods = 60;
#else
  // Memory consumption under ThreadSanitizer is a serious concern,
  // so we release resources sooner. The value of 1 leads to 1 to 2 second
  // delay before marking a thread as idle.
  static constexpr int kIdlePeriods = 1;
#endif

 protected:
  static void MaybeBecomeIdle();
};

template <typename T>
class WaiterCrtp : public WaiterBase {
 public:
  // Returns the Waiter associated with the identity.
  static T* GetWaiter(base_internal::ThreadIdentity* identity) {
    static_assert(
        sizeof(T) <= sizeof(base_internal::ThreadIdentity::WaiterState),
        "Insufficient space for Waiter");
    return reinterpret_cast<T*>(identity->waiter_state.data);
  }
};

}  // namespace synchronization_internal
TURBO_NAMESPACE_END
}  // namespace turbo

#endif  // TURBO_SYNCHRONIZATION_INTERNAL_WAITER_BASE_H_
