
/****************************************************************
 * Copyright (c) 2022, liyinbin
 * All rights reserved.
 * Author by liyinbin (jeff.li) lijippy@163.com
 *****************************************************************/

#include "flare/fiber/this_fiber.h"
#include "flare/fiber/internal/fiber_worker.h"
#include "flare/times/time.h"


namespace flare::fiber_internal {
    extern FLARE_THREAD_LOCAL fiber_worker *tls_task_group;
}  // namespace flare::fiber_internal

namespace flare {

    int fiber_yield() {
        flare::fiber_internal::fiber_worker *g = flare::fiber_internal::tls_task_group;
        if (nullptr != g && !g->is_current_pthread_task()) {
            flare::fiber_internal::fiber_worker::yield(&g);
            return 0;
        }
        // pthread_yield is not available on MAC
        return sched_yield();
    }

    int fiber_sleep_until(const int64_t &expires_at_us) {
        int64_t now = flare::get_current_time_micros();
        int64_t expires_in_us = expires_at_us - now;
        if (expires_in_us > 0) {
            return fiber_sleep_for(expires_in_us);
        } else {
            return fiber_yield();
        }

    }

    int fiber_sleep_for(const int64_t &expires_in_us) {
        flare::fiber_internal::fiber_worker *g = flare::fiber_internal::tls_task_group;
        if (nullptr != g && !g->is_current_pthread_task()) {
            return flare::fiber_internal::fiber_worker::usleep(&g, expires_in_us);
        }
        return ::usleep(expires_in_us);
    }

    uint64_t get_fiber_id() {
        flare::fiber_internal::fiber_worker *g = flare::fiber_internal::tls_task_group;
        if (nullptr != g && !g->is_current_pthread_task()) {
            return static_cast<uint64_t>(g->current_fid());
        }
        return static_cast<uint64_t>(INVALID_FIBER_ID);
    }
}  // namespace flare
