
/****************************************************************
 * Copyright (c) 2022, liyinbin
 * All rights reserved.
 * Author by liyinbin (jeff.li) lijippy@163.com
 *****************************************************************/
#ifndef FLARE_TIMES_INTERNAL_CHRONO_TIME_H_
#define FLARE_TIMES_INTERNAL_CHRONO_TIME_H_

namespace flare::times_internal {

    static inline int64_t get_current_time_nanos_from_system() {
        return std::chrono::duration_cast<std::chrono::nanoseconds>(
                std::chrono::system_clock::now() -
                std::chrono::system_clock::from_time_t(0)).count();
    }

}  // namespace flare::times_internal

#endif  // FLARE_TIMES_INTERNAL_CHRONO_TIME_H_
