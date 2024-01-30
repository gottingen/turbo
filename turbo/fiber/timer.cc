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

#include "turbo/fiber/timer.h"

namespace turbo {

    FiberTimer::~FiberTimer() {
        if(_timer_id != INVALID_TIMER_ID) {
            TURBO_RAW_LOG(FATAL,  "FiberTimer destructed without cancel or detach");
            std::terminate();
        }
    }
    turbo::Status FiberTimer::run_at(const turbo::Time &abstime,
                        turbo::timer_task_fn_t&& on_timer, void *arg) {
        if(_timer_id != INVALID_TIMER_ID) {
            return turbo::already_exists_error("");
        }
        auto duration = abstime - turbo::time_now();
        auto rs = check_duration_possible(duration);
        if (TURBO_UNLIKELY(!rs.ok())) {
            return rs;
        }

        auto tmp = turbo::fiber_internal::fiber_timer_add(abstime, std::move(on_timer), arg);
        if (TURBO_UNLIKELY(!tmp.ok())) {
            return tmp.status();
        }
        _timer_id = tmp.value();
        return turbo::ok_status();
    }

    turbo::Status FiberTimer::run_after(const turbo::Duration &duration,
                            turbo::timer_task_fn_t&& on_timer, void *arg) {
        if(_timer_id != INVALID_TIMER_ID) {
            return turbo::already_exists_error("");
        }
        auto rs = check_duration_possible(duration);
        if (TURBO_UNLIKELY(!rs.ok())) {
            return rs;
        }
        auto abstime = turbo::time_now() + duration;
        auto tmp = turbo::fiber_internal::fiber_timer_add(abstime, std::move(on_timer), arg);
        if (TURBO_UNLIKELY(!tmp.ok())) {
            return tmp.status();
        }
        _timer_id = tmp.value();
        return turbo::ok_status();
    }

    void FiberTimer::cancel() {
        if (is_valid()) {
            turbo::fiber_internal::fiber_timer_del(_timer_id);
            _timer_id = INVALID_TIMER_ID;
        }
    }

    turbo::Status FiberTimer::check_duration_possible(const turbo::Duration &duration) const {
        if(is_valid()) {
            return turbo::already_exists_error("");
        }
        if (duration < MIN_DURATION) {
            return turbo::make_status(kEINVAL, "duration to short");
        }
        return turbo::ok_status();
    }
}