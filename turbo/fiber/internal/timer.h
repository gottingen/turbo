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
// Created by jeff on 24-1-11.
//

#ifndef TURBO_FIBER_INTERNAL_TIMER_H_
#define TURBO_FIBER_INTERNAL_TIMER_H_

#include "turbo/times/timer_thread.h"
#include "turbo/fiber/internal/types.h"
#include "turbo/base/result_status.h"

namespace turbo {
    struct fiber_timer_thread{};
    extern template turbo::Status init_timer_thread<fiber_timer_thread>(const TimerThreadOptions *options = nullptr);
    extern template TimerThread * get_timer_thread<fiber_timer_thread>();
}

namespace turbo::fiber_internal {

    inline turbo::Status init_fiber_timer_thread(const TimerThreadOptions *options = nullptr) {
        return turbo::init_timer_thread<fiber_timer_thread>(options);
    }
    inline TimerThread *get_fiber_timer_thread() {
        return turbo::get_timer_thread<fiber_timer_thread>();
    }

    // Run `on_timer(arg)' at or after real-time `abstime'. Put identifier of the
    // timer into *id.
    // Return 0 on success, errno otherwise.
    turbo::ResultStatus<fiber_timer_id> fiber_timer_add(const turbo::Time &abstime,
                        turbo::timer_task_fn_t&& on_timer, void *arg);

    // Unschedule the timer associated with `id'.
    // Returns: 0 - exist & not-run; 1 - still running; EINVAL - not exist.
    turbo::Status fiber_timer_del(fiber_timer_id id);

}  // namespace turbo::fiber_internal
#endif  // TURBO_FIBER_INTERNAL_TIMER_H_
