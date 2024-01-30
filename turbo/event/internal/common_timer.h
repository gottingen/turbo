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

#ifndef TURBO_EVENT_INTERNAL_COMMON_TIMER_H_
#define TURBO_EVENT_INTERNAL_COMMON_TIMER_H_

#include "turbo/platform/port.h"
#include "turbo/event/timer.h"
#include "turbo/event/internal/timer_thread.h"

namespace turbo {

    class CommonTimer : public TimerDispatcher {
    public:
        CommonTimer() = default;
        virtual ~CommonTimer() = default;

        Status start(TimerOptions options = TimerOptions()) override;

        [[nodiscard]] TimerId run_at(timer_task_fn_t&& fn, void *arg, const turbo::Time &abstime) override;

        [[nodiscard]] TimerId run_after(timer_task_fn_t&& fn, void *arg, const turbo::Duration &du) override;

        [[maybe_unused]] turbo::Status cancel(TimerId id) override;

        void stop() override;

        void join() override;

        bool running() const override;

        std::string name() const override {
            return "CommonTimer";
        }
    private:
        // NOLINT NEXT LINE
        TURBO_NON_COPYABLE(CommonTimer);
        bool _stop{false};
        bool _running{false};
        TimerThread _timer_thread;
    };

}  // namespace turbo
#endif  // TURBO_EVENT_INTERNAL_COMMON_TIMER_H_
