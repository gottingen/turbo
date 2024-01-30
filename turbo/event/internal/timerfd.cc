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
#include "turbo/event/internal/timerfd.h"

#if defined(TURBO_PLATFORM_LINUX)

#include "turbo/event/internal/epoll_poller.h"
#include <sys/timerfd.h>

namespace turbo {

    Timerfd::~Timerfd() {
        this->stop();
        this->join();
        return_event_channel(_cid);
        if (_fd != -1) {
            ::close(_fd);
        }
    }

    turbo::Status Timerfd::start(TimerOptions options) {
        auto rs = _timer_core.initialize(&options);
        if (!rs.ok()) {
            TLOG_CRITICAL("Fail to initialize timer core, {}", rs.to_string());
            return rs;
        }
        _fd = timerfd_create(CLOCK_MONOTONIC, TFD_NONBLOCK);
        if (_fd < 0) {
            return make_status();
        }
        auto cptr = get_event_channel(&_cid);
        if (!cptr) {
            return make_status();
        }
        cptr->read_callback = timer_callback;
        cptr->user_data = this;
        cptr->fd = _fd;
        rs = _dispatcher.start(nullptr, true);
        if (!rs.ok()) {
            TLOG_CRITICAL("Fail to start dispatcher, {}", rs.to_string());
            return rs;
        }
        if (_dispatcher.add_poll_in(_cid, _fd) != 0) {
            TLOG_CRITICAL("Fail to add timerfd to dispatcher");
            return make_status();
        }
        return turbo::ok_status();
    }

    [[nodiscard]] TimerId Timerfd::run_at(timer_task_fn_t &&fn, void *arg, const turbo::Time &abstime) {
        if (!running()) {
            return INVALID_TIMER_ID;
        }

        auto [id, earlier] = _timer_core.schedule(std::move(fn), arg, abstime);
        if (id != INVALID_TIMER_ID && earlier) {
            reset_timerfd(abstime);
        }
        return id;
    }

    [[maybe_unused]] turbo::Status Timerfd::cancel(TimerId id) {
        return _timer_core.unschedule(id);
    }

    [[nodiscard]] TimerId Timerfd::run_after(timer_task_fn_t &&fn, void *arg, const turbo::Duration &du) {
        return run_at(std::move(fn), arg, turbo::Time::time_now() + du);
    }

    void Timerfd::stop() {
        if (_fd != -1 && !stop_) {
            _dispatcher.remove_poll_in(_fd);
            _timer_core.stop();
            _dispatcher.stop();
            stop_ = true;
        }
    }

    void Timerfd::join() {
        _dispatcher.join();
    }

    bool Timerfd::running() const {
        return _fd >= 0 && _dispatcher.running();
    }

    void Timerfd::reset_timerfd(const turbo::Time &abstime) {
        if (abstime == turbo::Time::infinite_future()) {
            return;
        }
        static constexpr timespec zero_spec = {0, 0};
        struct itimerspec newValue{zero_spec, zero_spec};
        struct itimerspec oldValue{zero_spec, zero_spec};
        static constexpr Duration two_us = Duration::microseconds(2);
        auto du = abstime - turbo::Time::time_now();
        if (du < two_us) {
            du = two_us;
        }
        newValue.it_value = du.to_timespec();
        int ret = ::timerfd_settime(_fd, 0, &newValue, &oldValue);
        if (ret != 0) {
            TLOG_CRITICAL("Fail to set timerfd, du:{} error:{}", du.to_string(), strerror(errno));
        }
    }

    void Timerfd::timer_callback(EventChannel *channel, int event) {
        if (event & EPOLLIN) {
            uint64_t exp;
            auto fd = channel->fd;
            ssize_t s = read(fd, &exp, sizeof(uint64_t));
            if (s != sizeof(uint64_t)) {
                TLOG_CRITICAL("Fail to read size {} timerfd fd:{} error:{}", s, fd, strerror(errno));
            }
            auto tfd = reinterpret_cast<Timerfd *>(channel->user_data);
            tfd->_timer_core.run_timer_tasks();
            auto next_run_time = tfd->_timer_core.next_run_time();
            if (next_run_time != turbo::Time::infinite_future()) {
                tfd->reset_timerfd(next_run_time);
            }
        }
    }

    static Timerfd *timer_ptr = nullptr;

    struct TimerfdInitializer {
        TimerfdInitializer() {
            timer_ptr = new Timerfd();
            if (!timer_ptr) {
                TLOG_CRITICAL("Fail to create timerfd {}", errno);
                return;
            }
            auto rs = timer_ptr->start(TimerOptions{});
            if (!rs.ok()) {
                TLOG_CRITICAL("Fail to start timerfd {}", rs.to_string());
                delete timer_ptr;
                timer_ptr = nullptr;
            }
        }

        ~TimerfdInitializer() {
            delete timer_ptr;
            timer_ptr = nullptr;
        }
    };

    TimerfdInitializer timerfd_initializer [[maybe_unused]];

    TimerDispatcher *global_timerfd_dispatcher() {
        return timer_ptr;
    }

}  // namespace turbo
#else
namespace turbo {
    TimerDispatcher *global_timerfd_dispatcher() {
        return nullptr;
    }
}  // namespace turbo
#endif  // TURBO_PLATFORM_LINUX