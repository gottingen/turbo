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

#include <turbo/synchronization/internal/futex_waiter.h>

#ifdef TURBO_INTERNAL_HAVE_FUTEX_WAITER

#include <atomic>
#include <cstdint>
#include <cerrno>

#include <turbo/base/config.h>
#include <turbo/base/internal/raw_logging.h>
#include <turbo/base/internal/thread_identity.h>
#include <turbo/base/optimization.h>
#include <turbo/synchronization/internal/kernel_timeout.h>
#include <turbo/synchronization/internal/futex.h>

namespace turbo {
TURBO_NAMESPACE_BEGIN
namespace synchronization_internal {

#ifdef TURBO_INTERNAL_NEED_REDUNDANT_CONSTEXPR_DECL
constexpr char FutexWaiter::kName[];
#endif

int FutexWaiter::WaitUntil(std::atomic<int32_t>* v, int32_t val,
                           KernelTimeout t) {
#ifdef CLOCK_MONOTONIC
  constexpr bool kHasClockMonotonic = true;
#else
  constexpr bool kHasClockMonotonic = false;
#endif

  // We can't call Futex::WaitUntil() here because the prodkernel implementation
  // does not know about KernelTimeout::SupportsSteadyClock().
  if (!t.has_timeout()) {
    return Futex::Wait(v, val);
  } else if (kHasClockMonotonic && KernelTimeout::SupportsSteadyClock() &&
             t.is_relative_timeout()) {
    auto rel_timespec = t.MakeRelativeTimespec();
    return Futex::WaitRelativeTimeout(v, val, &rel_timespec);
  } else {
    auto abs_timespec = t.MakeAbsTimespec();
    return Futex::WaitAbsoluteTimeout(v, val, &abs_timespec);
  }
}

bool FutexWaiter::Wait(KernelTimeout t) {
  // Loop until we can atomically decrement futex from a positive
  // value, waiting on a futex while we believe it is zero.
  // Note that, since the thread ticker is just reset, we don't need to check
  // whether the thread is idle on the very first pass of the loop.
  bool first_pass = true;
  while (true) {
    int32_t x = futex_.load(std::memory_order_relaxed);
    while (x != 0) {
      if (!futex_.compare_exchange_weak(x, x - 1,
                                        std::memory_order_acquire,
                                        std::memory_order_relaxed)) {
        continue;  // Raced with someone, retry.
      }
      return true;  // Consumed a wakeup, we are done.
    }

    if (!first_pass) MaybeBecomeIdle();
    const int err = WaitUntil(&futex_, 0, t);
    if (err != 0) {
      if (err == -EINTR || err == -EWOULDBLOCK) {
        // Do nothing, the loop will retry.
      } else if (err == -ETIMEDOUT) {
        return false;
      } else {
        TURBO_RAW_LOG(FATAL, "Futex operation failed with error %d\n", err);
      }
    }
    first_pass = false;
  }
}

void FutexWaiter::Post() {
  if (futex_.fetch_add(1, std::memory_order_release) == 0) {
    // We incremented from 0, need to wake a potential waiter.
    Poke();
  }
}

void FutexWaiter::Poke() {
  // Wake one thread waiting on the futex.
  const int err = Futex::Wake(&futex_, 1);
  if (TURBO_UNLIKELY(err < 0)) {
    TURBO_RAW_LOG(FATAL, "Futex operation failed with error %d\n", err);
  }
}

}  // namespace synchronization_internal
TURBO_NAMESPACE_END
}  // namespace turbo

#endif  // TURBO_INTERNAL_HAVE_FUTEX_WAITER
