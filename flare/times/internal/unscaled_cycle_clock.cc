
/****************************************************************
 * Copyright (c) 2022, liyinbin
 * All rights reserved.
 * Author by liyinbin (jeff.li) lijippy@163.com
 *****************************************************************/

#include "flare/times/internal/unscaled_cycle_clock.h"

#if FLARE_USE_UNSCALED_CYCLECLOCK

#if defined(_WIN32)
#include <intrin.h>
#endif

#if defined(__powerpc__) || defined(__ppc__)
#include <sys/platform/ppc.h>
#endif

#include "flare/system/sysinfo.h"

namespace flare::times_internal {

#if defined(__i386__)

    int64_t unscaled_cycle_clock::now() {
      int64_t ret;
      __asm__ volatile("rdtsc" : "=A"(ret));
      return ret;
    }

    double unscaled_cycle_clock::frequency() {
      return sysinfo::nominal_cpu_frequency();
    }

#elif defined(__x86_64__)

    int64_t unscaled_cycle_clock::now() {
        uint64_t low, high;
        __asm__ volatile("rdtsc" : "=a"(low), "=d"(high));
        return (high << 32) | low;
    }

    double unscaled_cycle_clock::frequency() {
        return sysinfo::nominal_cpu_frequency();
    }

#elif defined(__powerpc__) || defined(__ppc__)

    int64_t unscaled_cycle_clock::now() {
      return __ppc_get_timebase();
    }

    double unscaled_cycle_clock::frequency() {
      return __ppc_get_timebase_freq();
    }

#elif defined(__aarch64__)

    // System timer of ARMv8 runs at a different frequency than the CPU's.
    // The frequency is fixed, typically in the range 1-50MHz.  It can be
    // read at CNTFRQ special register.  We assume the OS has set up
    // the virtual timer properly.
    int64_t unscaled_cycle_clock::now() {
      int64_t virtual_timer_value;
      asm volatile("mrs %0, cntvct_el0" : "=r"(virtual_timer_value));
      return virtual_timer_value;
    }

    double unscaled_cycle_clock::frequency() {
      uint64_t aarch64_timer_frequency;
      asm volatile("mrs %0, cntfrq_el0" : "=r"(aarch64_timer_frequency));
      return aarch64_timer_frequency;
    }

#elif defined(_M_IX86) || defined(_M_X64)

#pragma intrinsic(__rdtsc)

    int64_t unscaled_cycle_clock::now() {
      return __rdtsc();
    }

    double unscaled_cycle_clock::frequency() {
      return sysinfo::nominal_cpu_frequency();
    }

#endif
}  // namespace flare::times_internal

#endif  // FLARE_USE_UNSCALED_CYCLECLOCK
