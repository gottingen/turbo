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

#ifndef TURBO_FIBER_TIMER_H_
#define TURBO_FIBER_TIMER_H_

#include "turbo/fiber/internal/timer.h"

namespace turbo {

    class FiberTimer {
    public:
        static constexpr turbo::Duration MIN_DURATION = turbo::Duration::microseconds(2);

        FiberTimer() = default;

        ~FiberTimer();

        turbo::Status run_at(const turbo::Time &abstime,
                             const turbo::timer_task_fn_t &on_timer, void *arg);

        turbo::Status run_after(const turbo::Duration &duration,
                                const turbo::timer_task_fn_t &on_timer, void *arg);

        turbo::Status run_every(const turbo::Duration &duration,
                                const turbo::timer_task_fn_t &on_timer, void *arg);

        void cancel();

        bool is_valid() const;

        void detach() {
            _timer_id = INVALID_TIMER_ID;
        }

        const turbo::Duration & duration() const { return _duration; }

        size_t repeat() const { return _repeat; }

        bool  triggered() const { return _triggered; }

        bool is_repeated() const { return _duration != turbo::Duration::zero(); }
    private:
        turbo::Status check_duration_possible(const turbo::Duration &duration) const;

        static void on_timer_call(void *arg);

        void *_arg{nullptr};
        bool _triggered{false};
        timer_task_fn_t _on_timer;
        turbo::Duration _duration;
        size_t _repeat{0};
        TimerId _timer_id{INVALID_TIMER_ID};
    };

    /// inlines

    bool FiberTimer::is_valid() const {
        return _timer_id != INVALID_TIMER_ID;
    }
}  // namespace turbo
#endif  // TURBO_FIBER_TIMER_H_
