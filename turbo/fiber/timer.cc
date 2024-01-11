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
        cancel();
    }
    turbo::Status FiberTimer::run_at(const turbo::Time &abstime,
                        const turbo::timer_task_fn_t& on_timer, void *arg) {
        auto duration = abstime - turbo::time_now();
        auto rs = check_duration_possible(duration);
        if (TURBO_UNLIKELY(!rs.ok())) {
            return rs;
        }
        _arg = arg;
        _on_timer = on_timer;
        auto tmp = turbo::fiber_internal::fiber_timer_add(abstime, on_timer_call, this);
        if (TURBO_UNLIKELY(!tmp.ok())) {
            return tmp.status();
        }
        _timer_id = tmp.value();
        return turbo::ok_status();
    }

    turbo::Status FiberTimer::run_after(const turbo::Duration &duration,
                            const turbo::timer_task_fn_t& on_timer, void *arg) {
        auto rs = check_duration_possible(duration);
        if (TURBO_UNLIKELY(!rs.ok())) {
            return rs;
        }
        _arg = arg;
        _on_timer = on_timer;
        auto abstime = turbo::time_now() + duration;
        auto tmp = turbo::fiber_internal::fiber_timer_add(abstime, on_timer_call, this);
        if (TURBO_UNLIKELY(!tmp.ok())) {
            return tmp.status();
        }
        _timer_id = tmp.value();
        return turbo::ok_status();
    }

    turbo::Status FiberTimer::run_every(const turbo::Duration &duration,
                            const turbo::timer_task_fn_t& on_timer, void *arg) {
        auto rs = check_duration_possible(duration);
        if (TURBO_UNLIKELY(!rs.ok())) {
            return rs;
        }
        _arg = arg;
        _on_timer = on_timer;
        _duration = duration;
        auto abstime = turbo::time_now() + duration;
        auto tmp = turbo::fiber_internal::fiber_timer_add(abstime, on_timer_call, this);
        if (TURBO_UNLIKELY(!tmp.ok())) {
            return tmp.status();
        }
        _timer_id = tmp.value();
        return turbo::ok_status();

    }

    void FiberTimer::cancel() {
        if (is_valid()) {
            turbo::fiber_internal::fiber_timer_del(_timer_id);
            _timer_id = INVALID_FIBER_TIMER_ID;
        }
    }

    turbo::Status FiberTimer::check_duration_possible(const turbo::Duration &duration) const {
        if(is_valid()) {
            return turbo::already_exists_error("");
        }
        if (duration < MIN_DURATION) {
            return turbo::invalid_argument_error("duration too short");
        }
        return turbo::ok_status();
    }

    void FiberTimer::on_timer_call(void *arg) {
        auto *timer = static_cast<FiberTimer *>(arg);
        timer->_on_timer(timer->_arg);
        timer->_triggered = true;
        timer->_timer_id = INVALID_FIBER_TIMER_ID;
        if(timer->_duration > MIN_DURATION) {
            ++timer->_repeat;
            timer->run_after(timer->_duration, std::move(timer->_on_timer), timer->_arg);
        }
    }
}