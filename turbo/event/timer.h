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
// Created by jeff on 24-1-19.
//

#ifndef TURBO_EVENT_TIMER_H_
#define TURBO_EVENT_TIMER_H_

#include <string>
#include "turbo/status/status.h"
#include "turbo/fiber/fiber.h"
#include "turbo/event/once_task.h"
#include "turbo/event/types.h"

namespace turbo {

    class TimerDispatcher {
    public:

        virtual ~TimerDispatcher() = default;

        virtual Status start(TimerOptions options = TimerOptions()) = 0;

        [[nodiscard]] virtual TimerId run_at(timer_task_fn_t&& fn, void *arg, const turbo::Time &abstime) =0;

        [[nodiscard]] virtual TimerId run_after(timer_task_fn_t&& fn, void *arg, const turbo::Duration &du) = 0;

        [[maybe_unused]] virtual turbo::Status cancel(TimerId id) = 0;

        virtual void stop() = 0;

        virtual void join() = 0;

        virtual bool running() const = 0;

        virtual std::string name() const = 0;
    };

    TimerDispatcher *global_timer_dispatcher();

    ///////////////////////// public global timer interface /////////////////////////
    [[nodiscard]]  TimerId run_at(timer_task_fn_t&& fn, void *arg, const turbo::Time &abstime);

    [[nodiscard]]  TimerId run_after(timer_task_fn_t&& fn, void *arg, const turbo::Duration &du);

    [[maybe_unused]]  turbo::Status cancel(TimerId id);
}  // namespace turbo
#endif  // TURBO_EVENT_TIMER_H_
