
/****************************************************************
 * Copyright (c) 2022, liyinbin
 * All rights reserved.
 * Author by liyinbin (jeff.li) lijippy@163.com
 *****************************************************************/
#ifndef FLARE_TIMES_INTERNAL_CHRONO_POSIX_TIME_H_
#define FLARE_TIMES_INTERNAL_CHRONO_POSIX_TIME_H_

#include <sys/time.h>
#include <ctime>
#include <cstdint>
#include "flare/log/logging.h"

namespace flare::times_internal {

    static inline int64_t get_current_time_nanos_from_system() {
        const int64_t kNanosPerSecond = 1000 * 1000 * 1000;
        struct timespec ts;
        FLARE_CHECK(clock_gettime(CLOCK_REALTIME, &ts) == 0)<<
               "Failed to read real-time clock.";
        return (int64_t{ts.tv_sec} * kNanosPerSecond +
                int64_t{ts.tv_nsec});
    }

}  // namespace flare::times_internal

#endif  // FLARE_TIMES_INTERNAL_CHRONO_POSIX_TIME_H_
