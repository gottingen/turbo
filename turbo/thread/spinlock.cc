
/****************************************************************
 * Copyright (c) 2022, liyinbin
 * All rights reserved.
 * Author by liyinbin (jeff.li) lijippy@163.com
 *****************************************************************/



#include "turbo/thread/spinlock.h"

#if defined(__x86_64__)
#define TURBO_CPU_RELAX() asm volatile("pause" ::: "memory")
#else
#define TURBO_CPU_RELAX() sched_yield()
#endif

namespace turbo {

    void spinlock::lock_slow() noexcept {
        do {
            // Test ...
            while (locked_.load(std::memory_order_relaxed)) {
                TURBO_CPU_RELAX();
            }

            // ... and set.
        } while (locked_.exchange(true, std::memory_order_acquire));
    }
}  // namespace turbo