// Copyright 2020 The Turbo Authors.
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//      https://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.

#ifndef TURBO_TIME_CLOCK_H_
#define TURBO_TIME_CLOCK_H_

#include "turbo/platform/port.h"
#include "turbo/times/time.h"

namespace turbo {

    /**
     * @ingroup turbo_times_clock
     * @brief Returns the current time, expressed as an `turbo::Time` absolute time value.
     * @return
     */
    turbo::Time time_now();

    /**
     * @ingroup turbo_times_clock
     * @brief Returns the current time, expressed as a count of nanoseconds since the Unix Epoch.
     *        Prefer `turbo::time_now()` instead for all but the most performance-sensitive cases
     *        (i.e. when you are calling this function hundreds of thousands of times per second).
     *        @see `turbo::time_now()`
     *        @see `turbo::get_current_time_nanos()`
     * @return
     */
    int64_t get_current_time_nanos();

    inline int64_t get_current_time_micros() {
        return get_current_time_nanos() / 1000;
    }

    /**
     * @ingroup turbo_times_clock
     * @brief Returns the current time, expressed as a count of milliseconds since the Unix Epoch.
     *        Prefer `turbo::time_now()` instead for all but the most performance-sensitive cases
     *        (i.e. when you are calling this function hundreds of thousands of times per second).
     *        @see `turbo::time_now()`
     *        @see `turbo::get_current_time_nanos()`
     * @return
     */
    inline int64_t get_current_time_millis() {
        return get_current_time_nanos() / 1000000;
    }

    /**
     * @ingroup turbo_times_clock
     * @brief Returns the current time, expressed as a count of seconds since the Unix Epoch.
     *        Prefer `turbo::time_now()` instead for all but the most performance-sensitive cases
     *        (i.e. when you are calling this function hundreds of thousands of times per second).
     *        @see `turbo::time_now()`
     *        @see `turbo::get_current_time_nanos()`
     * @return
     */
    inline int64_t get_current_time_seconds() {
        return get_current_time_nanos() / 1000000000;
    }

    /**
     * @ingroup turbo_times_clock
     * @brief Sleeps for the specified duration, expressed as an `turbo::Duration`.
     * @note Signal interruptions will not reduce the sleep duration.
     *       Returns immediately when passed a nonpositive duration.
     *       @see `turbo::Duration`
     * @param duration The duration to sleep for.
     */
    void sleep_for(turbo::Duration duration);

    /**
     * @ingroup turbo_times_clock
     * @brief Sleeps until the specified time, expressed as an `turbo::Time`.
     * @note Signal interruptions will not reduce the sleep duration.
     *       Returns immediately when passed a time in the past.
     *       @see `turbo::Time`
     * @param time The time to sleep until.
     */
    inline void sleep_until(turbo::Time time) {
        sleep_for(time - time_now());
    }

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
void TURBO_INTERNAL_C_SYMBOL(TurboInternalSleepFor)(turbo::Duration duration);
}  // extern "C"

inline void turbo::sleep_for(turbo::Duration duration) {
    TURBO_INTERNAL_C_SYMBOL(TurboInternalSleepFor)(duration);
}

#endif  // TURBO_TIME_CLOCK_H_
