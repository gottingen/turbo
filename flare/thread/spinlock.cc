
/****************************************************************
 * Copyright (c) 2022, liyinbin
 * All rights reserved.
 * Author by liyinbin (jeff.li) lijippy@163.com
 *****************************************************************/



#include "flare/thread/spinlock.h"

#if defined(__x86_64__)
#define FLARE_CPU_RELAX() asm volatile("pause" ::: "memory")
#else
#define FLARE_CPU_RELAX() sched_yield()
#endif

namespace flare {

    void spinlock::lock_slow() noexcept {
        do {
            // Test ...
            while (locked_.load(std::memory_order_relaxed)) {
                FLARE_CPU_RELAX();
            }

            // ... and set.
        } while (locked_.exchange(true, std::memory_order_acquire));
    }
}  // namespace flare