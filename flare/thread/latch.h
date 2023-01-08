
/****************************************************************
 * Copyright (c) 2022, liyinbin
 * All rights reserved.
 * Author by liyinbin (jeff.li) lijippy@163.com
 *****************************************************************/



#ifndef FLARE_THREAD_LATCH_H_
#define FLARE_THREAD_LATCH_H_

#include <condition_variable>
#include <mutex>
#include <atomic>
#include <memory>
#include "flare/times/time.h"
#include "flare/log/logging.h"

namespace flare {

    class latch {
    public:
        explicit latch(uint32_t count = 0);

        // Decrement internal counter. If it reaches zero, wake up all waiters.
        void count_down(uint32_t update = 1);

        void count_up(uint32_t update = 1);

        // Test if the latch's internal counter has become zero.
        bool try_wait() const noexcept;

        // Wait until `latch`'s internal counter reached zero.
        void wait() const;

        bool wait_for(const flare::duration &d) {
            std::chrono::microseconds timeout = d.to_chrono_microseconds();
            std::unique_lock lk(_data->mutex);
            FLARE_CHECK_GE(_data->count, 0ul);
            return _data->cond.wait_for(lk, timeout, [this] { return _data->count == 0; });
        }

        bool wait_until(const flare::time_point &deadline) {
            auto d = deadline.to_chrono_time();
            std::unique_lock lk(_data->mutex);
            FLARE_CHECK_GE(_data->count, 0ul);
            return _data->cond.wait_until(lk, d, [this] { return _data->count == 0; });
        }

        // Shorthand for `count_down(); wait();`
        void arrive_and_wait(std::ptrdiff_t update = 1);

    private:
        struct inner_data {
            std::mutex mutex;
            std::condition_variable cond;
            std::atomic<uint32_t> count{0};
        };
        std::shared_ptr<inner_data> _data;
    };

}  // namespace flare
#endif  // FLARE_THREAD_LATCH_H_
