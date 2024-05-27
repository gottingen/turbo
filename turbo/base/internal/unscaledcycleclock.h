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
// UnscaledCycleClock
//    An UnscaledCycleClock yields the value and frequency of a cycle counter
//    that increments at a rate that is approximately constant.
//    This class is for internal use only, you should consider using CycleClock
//    instead.
//
// Notes:
// The cycle counter frequency is not necessarily the core clock frequency.
// That is, CycleCounter cycles are not necessarily "CPU cycles".
//
// An arbitrary offset may have been added to the counter at power on.
//
// On some platforms, the rate and offset of the counter may differ
// slightly when read from different CPUs of a multiprocessor.  Usually,
// we try to ensure that the operating system adjusts values periodically
// so that values agree approximately.   If you need stronger guarantees,
// consider using alternate interfaces.
//
// The CPU is not required to maintain the ordering of a cycle counter read
// with respect to surrounding instructions.

#ifndef TURBO_BASE_INTERNAL_UNSCALEDCYCLECLOCK_H_
#define TURBO_BASE_INTERNAL_UNSCALEDCYCLECLOCK_H_

#include <cstdint>

#if defined(__APPLE__)
#include <TargetConditionals.h>
#endif

#include <turbo/base/config.h>
#include <turbo/base/internal/unscaledcycleclock_config.h>

#if TURBO_USE_UNSCALED_CYCLECLOCK

namespace turbo {
TURBO_NAMESPACE_BEGIN
namespace time_internal {
class UnscaledCycleClockWrapperForGetCurrentTime;
}  // namespace time_internal

namespace base_internal {
class CycleClock;
class UnscaledCycleClockWrapperForInitializeFrequency;

class UnscaledCycleClock {
 private:
  UnscaledCycleClock() = delete;

  // Return the value of a cycle counter that counts at a rate that is
  // approximately constant.
  static int64_t Now();

  // Return the how much UnscaledCycleClock::Now() increases per second.
  // This is not necessarily the core CPU clock frequency.
  // It may be the nominal value report by the kernel, rather than a measured
  // value.
  static double Frequency();

  // Allowed users
  friend class base_internal::CycleClock;
  friend class time_internal::UnscaledCycleClockWrapperForGetCurrentTime;
  friend class base_internal::UnscaledCycleClockWrapperForInitializeFrequency;
};

#if defined(__x86_64__)

inline int64_t UnscaledCycleClock::Now() {
  uint64_t low, high;
  __asm__ volatile("rdtsc" : "=a"(low), "=d"(high));
  return static_cast<int64_t>((high << 32) | low);
}

#endif

}  // namespace base_internal
TURBO_NAMESPACE_END
}  // namespace turbo

#endif  // TURBO_USE_UNSCALED_CYCLECLOCK

#endif  // TURBO_BASE_INTERNAL_UNSCALEDCYCLECLOCK_H_
