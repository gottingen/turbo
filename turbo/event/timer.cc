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
// Created by jeff on 24-1-20.
//
#include "turbo/platform/port.h"
#include "turbo/event/timer.h"
#include "turbo/event/internal/timerfd.h"
#include "turbo/event/internal/common_timer.h"

namespace turbo {
    TimerDispatcher *global_timer_dispatcher() {
        auto ptr = global_timerfd_dispatcher();
        if (ptr) {
            return ptr;
        }
        return global_common_timer_dispatcher();
    }

    [[nodiscard]]  TimerId run_at(timer_task_fn_t&& fn, void *arg, const turbo::Time &abstime) {
        return global_timer_dispatcher()->run_at(std::move(fn), arg, abstime);
    }

    [[nodiscard]]  TimerId run_after(timer_task_fn_t&& fn, void *arg, const turbo::Duration &du) {
        return global_timer_dispatcher()->run_after(std::move(fn), arg, du);
    }

    [[maybe_unused]]  turbo::Status cancel(TimerId id) {
        return global_timer_dispatcher()->cancel(id);
    }
}