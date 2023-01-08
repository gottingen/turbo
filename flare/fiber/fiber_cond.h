
/****************************************************************
 * Copyright (c) 2022, liyinbin
 * All rights reserved.
 * Author by liyinbin (jeff.li) lijippy@163.com
 *****************************************************************/

#ifndef FLARE_FIBER_FIBER_COND_H_
#define FLARE_FIBER_FIBER_COND_H_

#include "flare/fiber/internal/fiber_cond.h"
#include "flare/fiber/fiber_mutex.h"

namespace flare {
    class fiber_cond {
        FLARE_DISALLOW_COPY_AND_ASSIGN(fiber_cond);

    public:
        typedef fiber_cond_t *native_handler_type;

        fiber_cond() {
            FLARE_CHECK_EQ(0, fiber_cond_init(&_cond, nullptr));
        }

        ~fiber_cond() {
            FLARE_CHECK_EQ(0, fiber_cond_destroy(&_cond));
        }

        native_handler_type native_handler() { return &_cond; }

        void wait(std::unique_lock<flare::fiber_mutex> &lock) {
            fiber_cond_wait(&_cond, lock.mutex()->native_handler());
        }

        void wait(std::unique_lock<fiber_mutex_t> &lock) {
            fiber_cond_wait(&_cond, lock.mutex());
        }

        // Unlike std::condition_variable, we return ETIMEDOUT when time expires
        // rather than std::timeout
        int wait_for(std::unique_lock<flare::fiber_mutex> &lock,
                     long timeout_us) {
            return wait_until(lock,  flare::time_point::future_unix_micros(timeout_us).to_timespec());
        }

        int wait_for(std::unique_lock<fiber_mutex_t> &lock,
                     long timeout_us) {
            return wait_until(lock,  flare::time_point::future_unix_micros(timeout_us).to_timespec());
        }

        int wait_until(std::unique_lock<flare::fiber_mutex> &lock,
                       timespec duetime) {
            const int rc = fiber_cond_timedwait(
                    &_cond, lock.mutex()->native_handler(), &duetime);
            return rc == ETIMEDOUT ? ETIMEDOUT : 0;
        }

        int wait_until(std::unique_lock<fiber_mutex_t> &lock,
                       timespec duetime) {
            const int rc = fiber_cond_timedwait(
                    &_cond, lock.mutex(), &duetime);
            return rc == ETIMEDOUT ? ETIMEDOUT : 0;
        }

        void notify_one() {
            fiber_cond_signal(&_cond);
        }

        void notify_all() {
            fiber_cond_broadcast(&_cond);
        }

    private:
        fiber_cond_t _cond;
    };

}  // namespace flare

#endif // FLARE_FIBER_FIBER_COND_H_
