
/****************************************************************
 * Copyright (c) 2022, liyinbin
 * All rights reserved.
 * Author by liyinbin (jeff.li) lijippy@163.com
 *****************************************************************/

#include "flare/times/internal/cycle_clock.h"
#include <atomic>
#include <chrono>  // NOLINT(build/c++11)
#include "flare/times/internal/unscaled_cycle_clock.h"

namespace flare::times_internal {

#if FLARE_USE_UNSCALED_CYCLECLOCK

    namespace {

#ifdef NDEBUG
#ifdef FLARE_INTERNAL_UNSCALED_CYCLECLOCK_FREQUENCY_IS_CPU_FREQUENCY
// Not debug mode and the unscaled_cycle_clock frequency is the CPU
// frequency.  Scale the flare::cycle_clock to prevent overflow if someone
// tries to represent the time as cycles since the Unix epoch.
        static constexpr int32_t kShift = 1;
#else
        // Not debug mode and the unscaled_cycle_clock isn't operating at the
        // raw CPU frequency. There is no need to do any scaling, so don't
        // needlessly sacrifice precision.
        static constexpr int32_t kShift = 0;
#endif
#else
        // In debug mode use a different shift to discourage depending on a
        // particular shift value.
        static constexpr int32_t kShift = 2;
#endif

        static constexpr double kFrequencyScale = 1.0 / (1 << kShift);
        static std::atomic<cycle_clock_source_func> static_cycle_clock_source;

        cycle_clock_source_func load_cycle_clock_source() {
            // Optimize for the common case (no callback) by first doing a relaxed load;
            // this is significantly faster on non-x86 platforms.
            if (static_cycle_clock_source.load(std::memory_order_relaxed) == nullptr) {
                return nullptr;
            }
            // This corresponds to the store(std::memory_order_release) in
            // CycleClockSource::Register, and makes sure that any updates made prior to
            // registering the callback are visible to this thread before the callback is
            // invoked.
            return static_cycle_clock_source.load(std::memory_order_acquire);
        }

    }  // namespace

    int64_t cycle_clock::now() {
        auto fn = load_cycle_clock_source();
        if (fn == nullptr) {
            return unscaled_cycle_clock::now() >> kShift;
        }
        return fn() >> kShift;
    }

    double cycle_clock::frequency() {
        return kFrequencyScale * unscaled_cycle_clock::frequency();
    }

    void cycle_clock_source::clock_register(cycle_clock_source_func source) {
        // Corresponds to the load(std::memory_order_acquire) in LoadCycleClockSource.
        static_cycle_clock_source.store(source, std::memory_order_release);
    }

#else

    int64_t cycle_clock::now() {
      return std::chrono::duration_cast<std::chrono::nanoseconds>(
                 std::chrono::steady_clock::now().time_since_epoch())
          .count();
    }

    double cycle_clock::frequency() {
      return 1e9;
    }

#endif

}  // namespace flare::times_internal
