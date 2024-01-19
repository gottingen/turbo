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

#include "turbo/event/internal/common_timer.h"

namespace turbo {


    Status CommonTimer::start(TimerOptions options) {
        if (_running) {
            return turbo::ok_status();
        }
        _running = true;
        _stop = false;
        auto rs = _timer_thread.start(&options);
        if (!rs.ok()) {
            _running = false;
            return rs;
        }
        return turbo::ok_status();
    }

    [[nodiscard]] TimerId CommonTimer::run_at(timer_task_fn_t&& fn, void *arg, const turbo::Time &abstime) {
        return _timer_thread.schedule(std::move(fn), arg, abstime);
    }

    [[nodiscard]] TimerId CommonTimer::run_after(timer_task_fn_t&& fn, void *arg, const turbo::Duration &du) {
        return _timer_thread.schedule(std::move(fn), arg, Time::time_now() + du);
    }

    [[maybe_unused]] turbo::Status CommonTimer::cancel(TimerId id)  {
        return _timer_thread.unschedule(id);
    }

    void CommonTimer::stop() {
        _stop = true;
        _running = false;
    }

    void CommonTimer::join() {
        _timer_thread.stop_and_join();
    }

    bool CommonTimer::running() const {
        return _running;
    }

    static CommonTimer *common_timer_ptr = nullptr;

    struct TimerfdInitializer {
        TimerfdInitializer() {
            common_timer_ptr = new CommonTimer();
            if (!common_timer_ptr) {
                TLOG_CRITICAL("Fail to create timerfd {}", errno);
                return;
            }
            auto rs = common_timer_ptr->start(TimerOptions{});
            if (!rs.ok()) {
                TLOG_CRITICAL("Fail to start timerfd {}", rs.to_string());
                delete common_timer_ptr;
                common_timer_ptr = nullptr;
            }
        }

        ~TimerfdInitializer() {
            delete common_timer_ptr;
            common_timer_ptr = nullptr;
        }
    };

    TimerfdInitializer timerfd_initializer [[maybe_unused]];

    TimerDispatcher *global_common_timer_dispatcher() {
        return common_timer_ptr;
    }

}  // namespace turbo