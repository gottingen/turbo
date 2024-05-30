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
// -----------------------------------------------------------------------------
// File: clock.h
// -----------------------------------------------------------------------------
//
// This header file contains utility functions for working with the system-wide
// realtime clock. For descriptions of the main time abstractions used within
// this header file, consult the time.h header file.
#ifndef TURBO_TIME_CLOCK_H_
#define TURBO_TIME_CLOCK_H_

#include <cstdint>

#include <turbo/base/config.h>
#include <turbo/base/macros.h>
#include <turbo/times/time.h>

namespace turbo {
TURBO_NAMESPACE_BEGIN

// Now()
//
// Returns the current time, expressed as an `turbo::Time` absolute time value.
//turbo::Time Now();

// GetCurrentTimeNanos()
//
// Returns the current time, expressed as a count of nanoseconds since the Unix
// Epoch (https://en.wikipedia.org/wiki/Unix_time). Prefer `turbo::Time::current_time()` instead
// for all but the most performance-sensitive cases (i.e. when you are calling
// this function hundreds of thousands of times per second).
int64_t GetCurrentTimeNanos();

// SleepFor()
//
// Sleeps for the specified duration, expressed as an `turbo::Duration`.
//
// Notes:
// * Signal interruptions will not reduce the sleep duration.
// * Returns immediately when passed a nonpositive duration.
void SleepFor(turbo::Duration duration);

TURBO_NAMESPACE_END
}  // namespace turbo

// -----------------------------------------------------------------------------
// Implementation Details
// -----------------------------------------------------------------------------

// In some build configurations we pass --detect-odr-violations to the
// gold linker.  This causes it to flag weak symbol overrides as ODR
// violations.  Because ODR only applies to C++ and not C,
// --detect-odr-violations ignores symbols not mangled with C++ names.
// By changing our extension points to be extern "C", we dodge this
// check.
extern "C" {
TURBO_DLL void TURBO_INTERNAL_C_SYMBOL(TurboInternalSleepFor)(
    turbo::Duration duration);
}  // extern "C"

inline void turbo::SleepFor(turbo::Duration duration) {
  TURBO_INTERNAL_C_SYMBOL(TurboInternalSleepFor)(duration);
}

#endif  // TURBO_TIME_CLOCK_H_
