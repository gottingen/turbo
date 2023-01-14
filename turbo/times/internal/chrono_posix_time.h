
/****************************************************************
 * Copyright (c) 2022, liyinbin
 * All rights reserved.
 * Author by liyinbin (jeff.li) lijippy@163.com
 *****************************************************************/
#ifndef TURBO_TIMES_INTERNAL_CHRONO_POSIX_TIME_H_
#define TURBO_TIMES_INTERNAL_CHRONO_POSIX_TIME_H_

#include <sys/time.h>
#include <ctime>
#include <cstdint>
#include "turbo/log/logging.h"

namespace turbo::times_internal {

    static inline int64_t get_current_time_nanos_from_system() {
        const int64_t kNanosPerSecond = 1000 * 1000 * 1000;
        struct timespec ts;
        TURBO_CHECK(clock_gettime(CLOCK_REALTIME, &ts) == 0)<<
               "Failed to read real-time clock.";
        return (int64_t{ts.tv_sec} * kNanosPerSecond +
                int64_t{ts.tv_nsec});
    }

}  // namespace turbo::times_internal

#endif  // TURBO_TIMES_INTERNAL_CHRONO_POSIX_TIME_H_
