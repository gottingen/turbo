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

// The OS-specific header included below must provide two calls:
// TurboInternalSpinLockDelay() and TurboInternalSpinLockWake().
// See spinlock_wait.h for the specs.

#include <atomic>
#include <cstdint>

#include <turbo/base/internal/spinlock_wait.h>

#if defined(_WIN32)
#include <turbo/base/internal/spinlock_win32.inc>
#elif defined(__linux__)
#include <turbo/base/internal/spinlock_linux.inc>
#elif defined(__akaros__)
#include <turbo/base/internal/spinlock_akaros.inc>
#else
#include <turbo/base/internal/spinlock_posix.inc>
#endif

namespace turbo {
TURBO_NAMESPACE_BEGIN
namespace base_internal {

// See spinlock_wait.h for spec.
uint32_t SpinLockWait(std::atomic<uint32_t> *w, int n,
                      const SpinLockWaitTransition trans[],
                      base_internal::SchedulingMode scheduling_mode) {
  int loop = 0;
  for (;;) {
    uint32_t v = w->load(std::memory_order_acquire);
    int i;
    for (i = 0; i != n && v != trans[i].from; i++) {
    }
    if (i == n) {
      SpinLockDelay(w, v, ++loop, scheduling_mode);  // no matching transition
    } else if (trans[i].to == v ||                   // null transition
               w->compare_exchange_strong(v, trans[i].to,
                                          std::memory_order_acquire,
                                          std::memory_order_relaxed)) {
      if (trans[i].done) return v;
    }
  }
}

static std::atomic<uint64_t> delay_rand;

// Return a suggested delay in nanoseconds for iteration number "loop"
int SpinLockSuggestedDelayNS(int loop) {
  // Weak pseudo-random number generator to get some spread between threads
  // when many are spinning.
  uint64_t r = delay_rand.load(std::memory_order_relaxed);
  r = 0x5deece66dLL * r + 0xb;   // numbers from nrand48()
  delay_rand.store(r, std::memory_order_relaxed);

  if (loop < 0 || loop > 32) {   // limit loop to 0..32
    loop = 32;
  }
  const int kMinDelay = 128 << 10;  // 128us
  // Double delay every 8 iterations, up to 16x (2ms).
  int delay = kMinDelay << (loop / 8);
  // Randomize in delay..2*delay range, for resulting 128us..4ms range.
  return delay | ((delay - 1) & static_cast<int>(r));
}

}  // namespace base_internal
TURBO_NAMESPACE_END
}  // namespace turbo
