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
#ifndef TURBO_FIBER_FIBER_LATCH_H_
#define TURBO_FIBER_FIBER_LATCH_H_

#include "turbo/fiber/internal/fiber.h"
#include <atomic>

namespace turbo {

    /**
     * @ingroup turbo_fiber
     * @brief A synchronization primitive to wait for multiple signallers.
     */
    class FiberLatch {
    public:
        FiberLatch(int initial_count = 1);

        ~FiberLatch();

        // Increase current counter by |v|
        void add_count(int v = 1);

        // Reset the counter to |v|
        void reset(int v = 1);

        // Decrease the counter by |sig|
        void signal(int sig = 1);

        // Block current thread until the counter reaches 0.
        // Returns 0 on success, error code otherwise.
        // This method never returns EINTR.
        int wait();

        // Block the current thread until the counter reaches 0 or duetime has expired
        // Returns 0 on success, error code otherwise. ETIMEDOUT is for timeout.
        // This method never returns EINTR.
        int timed_wait(const timespec *duetime);

    private:
        int *_event;
        bool _wait_was_invoked;
    };

}  // namespace turbo

#endif  // TURBO_FIBER_FIBER_LATCH_H_
