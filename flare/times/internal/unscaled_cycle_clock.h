
/****************************************************************
 * Copyright (c) 2022, liyinbin
 * All rights reserved.
 * Author by liyinbin (jeff.li) lijippy@163.com
 *****************************************************************///
// unscaled_cycle_clock
//    An unscaled_cycle_clock yields the value and frequency of a cycle counter
//    that increments at a rate that is approximately constant.
//    This class is for internal / whitelisted use only, you should consider
//    using cycle_clock instead.
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

#ifndef FLARE_TIME_UNSCALEDCYCLECLOCK_H_
#define FLARE_TIME_UNSCALEDCYCLECLOCK_H_

#include <cstdint>

#if defined(__APPLE__)

#include <TargetConditionals.h>

#endif

#include "flare/base/profile.h"

// The following platforms have an implementation of a hardware counter.
#if defined(__i386__) || defined(__x86_64__) || defined(__aarch64__) || \
  defined(__powerpc__) || defined(__ppc__) || \
  defined(_M_IX86) || defined(_M_X64)
#define FLARE_HAVE_UNSCALED_CYCLECLOCK_IMPLEMENTATION 1
#else
#define FLARE_HAVE_UNSCALED_CYCLECLOCK_IMPLEMENTATION 0
#endif

// The following platforms often disable access to the hardware
// counter (through a sandbox) even if the underlying hardware has a
// usable counter. The CycleTimer interface also requires a *scaled*
// cycle_clock that runs at atleast 1 MHz. We've found some Android
// ARM64 devices where this is not the case, so we disable it by
// default on Android ARM64.
#if defined(__native_client__) || \
    (defined(TARGET_OS_IPHONE) && TARGET_OS_IPHONE) || \
    (defined(__ANDROID__) && defined(__aarch64__))
#define FLARE_USE_UNSCALED_CYCLECLOCK_DEFAULT 0
#else
#define FLARE_USE_UNSCALED_CYCLECLOCK_DEFAULT 1
#endif

// unscaled_cycle_clock is an optional internal feature.
// Use "#if FLARE_USE_UNSCALED_CYCLECLOCK" to test for its presence.
// Can be overridden at compile-time via -DFLARE_USE_UNSCALED_CYCLECLOCK=0|1
#if !defined(FLARE_USE_UNSCALED_CYCLECLOCK)
#define FLARE_USE_UNSCALED_CYCLECLOCK               \
  (FLARE_HAVE_UNSCALED_CYCLECLOCK_IMPLEMENTATION && \
   FLARE_USE_UNSCALED_CYCLECLOCK_DEFAULT)
#endif

#if FLARE_USE_UNSCALED_CYCLECLOCK

// This macro can be used to test if unscaled_cycle_clock::frequency()
// is nominal_cpu_frequency() on a particular platform.
#if  (defined(__i386__) || defined(__x86_64__) || \
      defined(_M_IX86) || defined(_M_X64))
#define FLARE_INTERNAL_UNSCALED_CYCLECLOCK_FREQUENCY_IS_CPU_FREQUENCY
#endif

namespace flare {

    class unscaled_cycle_clock_wrapper_for_get_current_time;

    class unscaled_cycle_clock_wrapper_for_initialize_frequency;
    namespace times_internal {


        class cycle_clock;

        class unscaled_cycle_clock {
        private:
            unscaled_cycle_clock() = delete;

            // Return the value of a cycle counter that counts at a rate that is
            // approximately constant.
            static int64_t now();

            // Return the how much unscaled_cycle_clock::now() increases per second.
            // This is not necessarily the core CPU clock frequency.
            // It may be the nominal value report by the kernel, rather than a measured
            // value.
            static double frequency();

            // Whitelisted friends.
            friend class cycle_clock;

            friend class flare::unscaled_cycle_clock_wrapper_for_get_current_time;

            friend class flare::unscaled_cycle_clock_wrapper_for_initialize_frequency;
        };

    }  // namespace times_internal
}  // namespace flare

#endif  // FLARE_USE_UNSCALED_CYCLECLOCK

#endif  // FLARE_TIME_UNSCALEDCYCLECLOCK_H_
