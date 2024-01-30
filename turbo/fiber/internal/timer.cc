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

#include "turbo/fiber/internal/timer.h"
#include "turbo/fiber/internal/schedule_group.h"
#include "turbo/status/result_status.h"
#include "turbo/status/status.h"

namespace turbo {
    template Status init_timer_thread<fiber_timer_thread>(const TimerOptions *options);
    template TimerThread * get_timer_thread<fiber_timer_thread>();
}
namespace turbo::fiber_internal {

    extern turbo::fiber_internal::ScheduleGroup * get_or_new_task_control();

    extern turbo::fiber_internal::ScheduleGroup * get_task_control();

    turbo::ResultStatus<TimerId> fiber_timer_add(const turbo::Time &abstime,
                                  turbo::timer_task_fn_t&& on_timer, void *arg) {
        turbo::fiber_internal::ScheduleGroup *c = turbo::fiber_internal::get_or_new_task_control();
        if (c == nullptr) {
            return turbo::make_status(kENOMEM);
        }
        // already init by ScheduleGroup::init
        auto *tt = get_fiber_timer_thread();
        if (tt == nullptr) {
            return turbo::make_status(kENOMEM);
        }
        const auto tmp = tt->schedule(std::move(on_timer), arg, abstime);
        if (tmp != INVALID_TIMER_ID) {
            return tmp;
        }
        return make_status(kESTOP);
    }

    turbo::Status fiber_timer_del(TimerId id) {
        turbo::fiber_internal::ScheduleGroup *c = turbo::fiber_internal::get_task_control();
        if (c != nullptr) {
            turbo::TimerThread *tt = get_fiber_timer_thread();
            if (tt == nullptr) {
                return turbo::make_status(kEINVAL);
            }
            const auto state = tt->unschedule(id);
            if (state.ok() || state.code() == kESTOP) {
                return turbo::ok_status();
            }
        }
        return turbo::make_status(kEINVAL);
    }

}