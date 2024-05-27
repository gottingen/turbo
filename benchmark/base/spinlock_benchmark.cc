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

// See also //turbo/synchronization:mutex_benchmark for a comparison of SpinLock
// and Mutex performance under varying levels of contention.

#include <turbo/base/internal/raw_logging.h>
#include <turbo/base/internal/scheduling_mode.h>
#include <turbo/base/internal/spinlock.h>
#include <turbo/base/no_destructor.h>
#include <turbo/synchronization/internal/create_thread_identity.h>
#include <benchmark/benchmark.h>

namespace {

template <turbo::base_internal::SchedulingMode scheduling_mode>
static void BM_TryLock(benchmark::State& state) {
  // Ensure a ThreadIdentity is installed so that KERNEL_ONLY has an effect.
  TURBO_INTERNAL_CHECK(
      turbo::synchronization_internal::GetOrCreateCurrentThreadIdentity() !=
          nullptr,
      "GetOrCreateCurrentThreadIdentity() failed");

  static turbo::NoDestructor<turbo::base_internal::SpinLock> spinlock(
      scheduling_mode);
  for (auto _ : state) {
    if (spinlock->TryLock()) spinlock->Unlock();
  }
}

template <turbo::base_internal::SchedulingMode scheduling_mode>
static void BM_SpinLock(benchmark::State& state) {
  // Ensure a ThreadIdentity is installed so that KERNEL_ONLY has an effect.
  TURBO_INTERNAL_CHECK(
      turbo::synchronization_internal::GetOrCreateCurrentThreadIdentity() !=
          nullptr,
      "GetOrCreateCurrentThreadIdentity() failed");

  static turbo::NoDestructor<turbo::base_internal::SpinLock> spinlock(
      scheduling_mode);
  for (auto _ : state) {
    turbo::base_internal::SpinLockHolder holder(spinlock.get());
  }
}

BENCHMARK_TEMPLATE(BM_SpinLock,
                   turbo::base_internal::SCHEDULE_KERNEL_ONLY)
    ->UseRealTime()
    ->Threads(1)
    ->ThreadPerCpu();

BENCHMARK_TEMPLATE(BM_SpinLock,
                   turbo::base_internal::SCHEDULE_COOPERATIVE_AND_KERNEL)
    ->UseRealTime()
    ->Threads(1)
    ->ThreadPerCpu();

BENCHMARK_TEMPLATE(BM_TryLock, turbo::base_internal::SCHEDULE_KERNEL_ONLY)
    ->UseRealTime()
    ->Threads(1)
    ->ThreadPerCpu();

BENCHMARK_TEMPLATE(BM_TryLock,
                   turbo::base_internal::SCHEDULE_COOPERATIVE_AND_KERNEL)
    ->UseRealTime()
    ->Threads(1)
    ->ThreadPerCpu();

}  // namespace
