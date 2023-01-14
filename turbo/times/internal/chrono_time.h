
/****************************************************************
 * Copyright (c) 2022, liyinbin
 * All rights reserved.
 * Author by liyinbin (jeff.li) lijippy@163.com
 *****************************************************************/
#ifndef TURBO_TIMES_INTERNAL_CHRONO_TIME_H_
#define TURBO_TIMES_INTERNAL_CHRONO_TIME_H_

namespace turbo::times_internal {

    static inline int64_t get_current_time_nanos_from_system() {
        return std::chrono::duration_cast<std::chrono::nanoseconds>(
                std::chrono::system_clock::now() -
                std::chrono::system_clock::from_time_t(0)).count();
    }

}  // namespace turbo::times_internal

#endif  // TURBO_TIMES_INTERNAL_CHRONO_TIME_H_
