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

// This file is a no-op if the required LowLevelAlloc support is missing.
#include <turbo/base/internal/low_level_alloc.h>
#ifndef TURBO_LOW_LEVEL_ALLOC_MISSING

#include <turbo/synchronization/internal/per_thread_sem.h>

#include <atomic>

#include <turbo/base/attributes.h>
#include <turbo/base/internal/thread_identity.h>
#include <turbo/synchronization/internal/waiter.h>

namespace turbo {
TURBO_NAMESPACE_BEGIN
namespace synchronization_internal {

void PerThreadSem::SetThreadBlockedCounter(std::atomic<int> *counter) {
  base_internal::ThreadIdentity *identity;
  identity = GetOrCreateCurrentThreadIdentity();
  identity->blocked_count_ptr = counter;
}

std::atomic<int> *PerThreadSem::GetThreadBlockedCounter() {
  base_internal::ThreadIdentity *identity;
  identity = GetOrCreateCurrentThreadIdentity();
  return identity->blocked_count_ptr;
}

void PerThreadSem::Tick(base_internal::ThreadIdentity *identity) {
  const int ticker =
      identity->ticker.fetch_add(1, std::memory_order_relaxed) + 1;
  const int wait_start = identity->wait_start.load(std::memory_order_relaxed);
  const bool is_idle = identity->is_idle.load(std::memory_order_relaxed);
  if (wait_start && (ticker - wait_start > Waiter::kIdlePeriods) && !is_idle) {
    // Wakeup the waiting thread since it is time for it to become idle.
    TURBO_INTERNAL_C_SYMBOL(TurboInternalPerThreadSemPoke)(identity);
  }
}

}  // namespace synchronization_internal
TURBO_NAMESPACE_END
}  // namespace turbo

extern "C" {

TURBO_ATTRIBUTE_WEAK void TURBO_INTERNAL_C_SYMBOL(TurboInternalPerThreadSemInit)(
    turbo::base_internal::ThreadIdentity *identity) {
  new (turbo::synchronization_internal::Waiter::GetWaiter(identity))
      turbo::synchronization_internal::Waiter();
}

TURBO_ATTRIBUTE_WEAK void TURBO_INTERNAL_C_SYMBOL(TurboInternalPerThreadSemPost)(
    turbo::base_internal::ThreadIdentity *identity) {
  turbo::synchronization_internal::Waiter::GetWaiter(identity)->Post();
}

TURBO_ATTRIBUTE_WEAK void TURBO_INTERNAL_C_SYMBOL(TurboInternalPerThreadSemPoke)(
    turbo::base_internal::ThreadIdentity *identity) {
  turbo::synchronization_internal::Waiter::GetWaiter(identity)->Poke();
}

TURBO_ATTRIBUTE_WEAK bool TURBO_INTERNAL_C_SYMBOL(TurboInternalPerThreadSemWait)(
    turbo::synchronization_internal::KernelTimeout t) {
  bool timeout = false;
  turbo::base_internal::ThreadIdentity *identity;
  identity = turbo::synchronization_internal::GetOrCreateCurrentThreadIdentity();

  // Ensure wait_start != 0.
  int ticker = identity->ticker.load(std::memory_order_relaxed);
  identity->wait_start.store(ticker ? ticker : 1, std::memory_order_relaxed);
  identity->is_idle.store(false, std::memory_order_relaxed);

  if (identity->blocked_count_ptr != nullptr) {
    // Increment count of threads blocked in a given thread pool.
    identity->blocked_count_ptr->fetch_add(1, std::memory_order_relaxed);
  }

  timeout =
      !turbo::synchronization_internal::Waiter::GetWaiter(identity)->Wait(t);

  if (identity->blocked_count_ptr != nullptr) {
    identity->blocked_count_ptr->fetch_sub(1, std::memory_order_relaxed);
  }

  identity->is_idle.store(false, std::memory_order_relaxed);
  identity->wait_start.store(0, std::memory_order_relaxed);
  return !timeout;
}

}  // extern "C"

#endif  // TURBO_LOW_LEVEL_ALLOC_MISSING
