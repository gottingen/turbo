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

// The implementation of CycleClock::Frequency.
//
// NOTE: only i386 and x86_64 have been well tested.
// PPC, sparc, alpha, and ia64 are based on
//    http://peter.kuscsik.com/wordpress/?p=14
// with modifications by m3b.  See also
//    https://setisvn.ssl.berkeley.edu/svn/lib/fftw-3.0.1/kernel/cycle.h

#include <turbo/base/internal/cycleclock.h>

#include <atomic>
#include <chrono>  // NOLINT(build/c++11)

#include <turbo/base/attributes.h>
#include <turbo/base/config.h>
#include <turbo/base/internal/unscaledcycleclock.h>

namespace turbo {
TURBO_NAMESPACE_BEGIN
namespace base_internal {

#if TURBO_USE_UNSCALED_CYCLECLOCK

#ifdef TURBO_INTERNAL_NEED_REDUNDANT_CONSTEXPR_DECL
constexpr int32_t CycleClock::kShift;
constexpr double CycleClock::kFrequencyScale;
#endif

TURBO_CONST_INIT std::atomic<CycleClockSourceFunc>
    CycleClock::cycle_clock_source_{nullptr};

void CycleClockSource::Register(CycleClockSourceFunc source) {
  // Corresponds to the load(std::memory_order_acquire) in LoadCycleClockSource.
  CycleClock::cycle_clock_source_.store(source, std::memory_order_release);
}

#ifdef _WIN32
int64_t CycleClock::Now() {
  auto fn = LoadCycleClockSource();
  if (fn == nullptr) {
    return base_internal::UnscaledCycleClock::Now() >> kShift;
  }
  return fn() >> kShift;
}
#endif

#else

int64_t CycleClock::Now() {
  return std::chrono::duration_cast<std::chrono::nanoseconds>(
             std::chrono::steady_clock::now().time_since_epoch())
      .count();
}

double CycleClock::Frequency() {
  return 1e9;
}

#endif

}  // namespace base_internal
TURBO_NAMESPACE_END
}  // namespace turbo
