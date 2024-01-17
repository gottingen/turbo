// Copyright 2023 The Elastic-AI Authors.
// part of Elastic AI Search
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//      https://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.
//
//
// Created by jeff on 24-1-3.
//
#ifndef TURBO_FIBER_FIBER_COND_H_
#define TURBO_FIBER_FIBER_COND_H_

#include "turbo/fiber/internal/fiber_cond.h"
#include "turbo/fiber/fiber_mutex.h"
#include "turbo/times/clock.h"

namespace turbo {

    /**
     * @ingroup turbo_fiber
     * @brief fiber condition variable
     */
    class FiberCond {
        TURBO_NON_COPYABLE(FiberCond);

    public:
        typedef turbo::fiber_internal::fiber_cond_t *native_handler_type;
        typedef turbo::fiber_internal::fiber_mutex_t fiber_mutex_t;

        FiberCond() {
            fiber_internal::fiber_cond_init(&_cond, nullptr);
        }

        ~FiberCond() {
            fiber_internal::fiber_cond_destroy(&_cond);
        }

        native_handler_type native_handler() { return &_cond; }

        turbo::Status wait(std::unique_lock<FiberMutex> &lock) {
            return fiber_internal::fiber_cond_wait(&_cond, lock.mutex()->native_handler());
        }

        turbo::Status wait(std::unique_lock<fiber_mutex_t> &lock) {
            return fiber_internal::fiber_cond_wait(&_cond, lock.mutex());
        }

        // Unlike std::condition_variable, we return ETIMEDOUT when time expires
        // rather than std::timeout
        turbo::Status wait_for(std::unique_lock<FiberMutex> &lock,
                     long timeout_us) {
            return wait_until(lock,  turbo::microseconds_from_now(timeout_us));
        }

        turbo::Status wait_for(std::unique_lock<fiber_mutex_t> &lock, long timeout_us) {
            return wait_until(lock,  turbo::microseconds_from_now(timeout_us));
        }

        turbo::Status wait_until(std::unique_lock<FiberMutex> &lock,
                       turbo::Time duetime) {
           return fiber_internal::fiber_cond_timedwait(
                    &_cond, lock.mutex()->native_handler(), duetime);
        }

        turbo::Status wait_until(std::unique_lock<fiber_mutex_t> &lock,
                                 turbo::Time duetime) {
            return  fiber_internal::fiber_cond_timedwait(
                    &_cond, lock.mutex(), duetime);
        }

        void notify_one() {
            fiber_internal::fiber_cond_signal(&_cond);
        }

        void notify_all() {
            fiber_internal::fiber_cond_broadcast(&_cond);
        }

    private:
        turbo::fiber_internal::fiber_cond_t _cond;
    };

}  // namespace turbo

#endif // TURBO_FIBER_FIBER_COND_H_
