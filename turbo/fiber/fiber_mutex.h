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
#ifndef TURBO_FIBER_FIBER_MUTEX_H_
#define TURBO_FIBER_FIBER_MUTEX_H_

#include "turbo/fiber/internal/mutex.h"
#include "turbo/log/logging.h"

namespace turbo {

    /**
     * @ingroup turbo_fiber
     * @brief FiberMutex is a mutex for fiber.
     *        in turbo fiber, std::mutex will block the whole worker thread,
     *        FiberMutex will only block the fiber. Another feature is that
     *        FiberMutex will capture the system std::mutex, and replace the
     *        std::mutex with FiberMutex, so that the std::mutex will be
     *        compatible with turbo fiber.
     */
    class FiberMutex {
    public:
        typedef turbo::fiber_internal::fiber_mutex_t *native_handler_type;

        FiberMutex() {
            auto ec = fiber_internal::fiber_mutex_init(&_mutex, nullptr);
            if (!ec.ok()) {
                TLOG_CRITICAL("FiberMutex constructor failed: {}", ec.message());
                throw std::system_error(std::error_code(ec.code(), std::system_category()), "FiberMutex constructor failed");
            }
        }

        ~FiberMutex() {
            fiber_internal::fiber_mutex_destroy(&_mutex);
        }

        native_handler_type native_handler() { return &_mutex; }

        void lock() {
            auto ec = fiber_internal::fiber_mutex_lock(&_mutex);
            if (!ec.ok()) {
                TLOG_CRITICAL("FiberMutex lock failed: {}", ec.to_string());
                throw std::system_error(std::error_code(ec.code(), std::system_category()), "FiberMutex lock failed");
            }
        }

        void unlock() { fiber_internal::fiber_mutex_unlock(&_mutex); }

        bool try_lock() { return fiber_internal::fiber_mutex_trylock(&_mutex).ok(); }
        // TODO(jeff.li): Complement interfaces for C++11
    private:
        TURBO_NON_COPYABLE(FiberMutex);

        turbo::fiber_internal::fiber_mutex_t _mutex;
    };
}  // namespace turbo

#endif // TURBO_FIBER_FIBER_MUTEX_H_
