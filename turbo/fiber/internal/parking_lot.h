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
// Created by jeff on 23-12-16.
//

#ifndef TURBO_FIBER_INTERNAL_PARKING_LOT_H_
#define TURBO_FIBER_INTERNAL_PARKING_LOT_H_

#include <atomic>
#include "turbo/concurrent/spinlock_wait.h"
#include "turbo/platform/port.h"

namespace turbo::fiber_internal {

    // Park idle workers.
    class TURBO_CACHE_LINE_ALIGNED ParkingLot {
    public:
        class State {
        public:
            State() : val(0) {}

            bool stopped() const { return val & 1; }

        private:
            friend class ParkingLot;

            State(int val) : val(val) {}

            int val;
        };

        ParkingLot() : _pending_signal(0) {}

        // Wake up at most `num_task' workers.
        // Returns #workers woken up.
        int signal(int num_task) {
            _pending_signal.fetch_add((num_task << 1));
            return _pending_signal.wake(num_task);
        }

        // Get a state for later wait().
        State get_state() {
            return _pending_signal.load();
        }

        // Wait for tasks.
        // If the `expected_state' does not match, wait() may finish directly.
        void wait(const State &expected_state) {
            _pending_signal.wait(expected_state.val);
        }

        // Wakeup suspended wait() and make them unwaitable ever.
        void stop() {
            _pending_signal.fetch_or(1);
            _pending_signal.wake(10000);
        }

    private:
        turbo::SpinWaiter _pending_signal;
    };

}  // namespace turbo::fiber_internal

#endif  // TURBO_FIBER_INTERNAL_PARKING_LOT_H_
