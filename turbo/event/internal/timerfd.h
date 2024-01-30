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

#ifndef TURBO_EVENT_INTERNAL_TIMERFD_H_
#define TURBO_EVENT_INTERNAL_TIMERFD_H_

#include "turbo/platform/port.h"

#if defined(TURBO_PLATFORM_LINUX)

#include "turbo/event/internal/timer_core.h"
#include "turbo/event/channel.h"
#include "turbo/event/event_dispatcher.h"
#include "turbo/event/timer.h"

namespace turbo {

    class Timerfd : public TimerDispatcher {
    public:
        Timerfd() = default;

        ~Timerfd() override;

        turbo::Status start(TimerOptions options) override;

        [[nodiscard]] TimerId run_at(timer_task_fn_t &&fn, void *arg, const turbo::Time &abstime) override;

        [[nodiscard]] TimerId run_after(timer_task_fn_t &&fn, void *arg, const turbo::Duration &du) override;

        [[maybe_unused]] turbo::Status cancel(TimerId id) override;

        void stop() override;

        void join() override;

        bool running() const override;

        std::string name() const override {
            return "Timerfd";
        }

    private:
        // NOLINT NEXT LINE
        TURBO_NON_COPYABLE(Timerfd);

        static void timer_callback(EventChannel *channel,int event);

        void reset_timerfd(const turbo::Time &abstime);

        bool stop_{false};
        TimerCore _timer_core;
        int _fd{-1};
        EventChannelId _cid{DEFAULT_EVENT_CHANNEL_ID};
        EventDispatcher _dispatcher;
    };

    TimerDispatcher *global_common_timer_dispatcher();

}  // namespace turbo
#endif // TURBO_PLATFORM_LINUX
namespace turbo {
    TimerDispatcher *global_timerfd_dispatcher();
}  // namespace turbo
#endif  // TURBO_EVENT_INTERNAL_TIMERFD_H_
