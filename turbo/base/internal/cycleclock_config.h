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

#ifndef TURBO_BASE_INTERNAL_CYCLECLOCK_CONFIG_H_
#define TURBO_BASE_INTERNAL_CYCLECLOCK_CONFIG_H_

#include <cstdint>

#include <turbo/base/config.h>
#include <turbo/base/internal/inline_variable.h>
#include <turbo/base/internal/unscaledcycleclock_config.h>

namespace turbo {
TURBO_NAMESPACE_BEGIN
namespace base_internal {

#if TURBO_USE_UNSCALED_CYCLECLOCK
#ifdef NDEBUG
#ifdef TURBO_INTERNAL_UNSCALED_CYCLECLOCK_FREQUENCY_IS_CPU_FREQUENCY
// Not debug mode and the UnscaledCycleClock frequency is the CPU
// frequency.  Scale the CycleClock to prevent overflow if someone
// tries to represent the time as cycles since the Unix epoch.
TURBO_INTERNAL_INLINE_CONSTEXPR(int32_t, kCycleClockShift, 1);
#else
// Not debug mode and the UnscaledCycleClock isn't operating at the
// raw CPU frequency. There is no need to do any scaling, so don't
// needlessly sacrifice precision.
TURBO_INTERNAL_INLINE_CONSTEXPR(int32_t, kCycleClockShift, 0);
#endif
#else   // NDEBUG
// In debug mode use a different shift to discourage depending on a
// particular shift value.
TURBO_INTERNAL_INLINE_CONSTEXPR(int32_t, kCycleClockShift, 2);
#endif  // NDEBUG

TURBO_INTERNAL_INLINE_CONSTEXPR(double, kCycleClockFrequencyScale,
                               1.0 / (1 << kCycleClockShift));
#endif  //  TURBO_USE_UNSCALED_CYCLECLOC

}  // namespace base_internal
TURBO_NAMESPACE_END
}  // namespace turbo

#endif  // TURBO_BASE_INTERNAL_CYCLECLOCK_CONFIG_H_
