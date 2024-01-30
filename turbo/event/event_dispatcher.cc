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
// Created by jeff on 24-1-18.
//

#include "turbo/event/event_dispatcher.h"
#include "turbo/event/internal/epoll_poller.h"
#include "turbo/hash/hash.h"
#include "turbo/concurrent/call_once.h"

namespace turbo {
    EventDispatcher::~EventDispatcher() {
        stop();
        join();
        // explicitly destroy poller
        _poller->remove_poll_in(_wakeup_fds[0]);
        _poller->destroy();
        _poller.reset();
        if (_wakeup_fds[0] != -1) {
            ::close(_wakeup_fds[0]);
        }
        if (_wakeup_fds[1] != -1) {
            ::close(_wakeup_fds[1]);
        }
        return_event_channel(_wakeup_channel);
    }

    turbo::Status EventDispatcher::start(const FiberAttribute *consumer_thread_attr, bool run_in_pthread) {
        if (running()) {
            return turbo::ok_status();
        }
        _run_in_pthread = run_in_pthread;
        _poller.reset(create_poller());
        if (!_poller) {
            return turbo::make_status(kENOMEM, "new EpollPoller failed");
        }

        turbo::Status status = _poller->initialize();
        if (!status.ok()) {
            return status;
        }
        // add wake up
        // initialize wakeup channel
        auto wakeup_channel = get_event_channel(&_wakeup_channel);
        if (!wakeup_channel) {
            return make_status(kENOMEM, "Fail to get wakeup channel");
        }

        auto rs = wakeup_channel->initialize(0, 0);
        if (!rs.ok()) {
            TLOG_CRITICAL("Fail to initialize wakeup channel: {}", rs.to_string());
            return rs;
        }
        wakeup_channel->read_callback = handle_wakeup;
        // create pipe
        _wakeup_fds[0] = -1;
        _wakeup_fds[1] = -1;
        if (pipe(_wakeup_fds) != 0) {
            TLOG_CRITICAL("Fail to create pipe: {}", strerror(errno));
            return make_status(errno, "Fail to create pipe");
        }
        wakeup_channel->fd = _wakeup_fds[0];
        make_non_blocking(_wakeup_fds[0]);
        make_non_blocking(_wakeup_fds[1]);
        rs = _poller->add_poll_in(_wakeup_channel, _wakeup_fds[0]);
        if (!rs.ok()) {
            TLOG_CRITICAL("Fail to add poll in: {}", rs.to_string());
            return rs;
        }

        _consumer_thread_attr = (consumer_thread_attr ?
                                 *consumer_thread_attr : FIBER_ATTR_NORMAL);

        if (run_in_pthread) {
            if (pthread_create(&_thread_id, nullptr, fiber_func, this) != 0) {
                return make_status(errno, "Fail to create epoll thread");
            }
        } else {
            FiberAttribute epoll_thread_attr = _consumer_thread_attr | AttributeFlag::FLAG_NEVER_QUIT;
            rs = _fiber.start(epoll_thread_attr, fiber_func, this);
            if (!rs.ok()) {
                TLOG_CRITICAL("Fail to create epoll fiber: {}", rs.to_string());
                return rs;
            }
        }
        //rs = _fiber.start(epoll_thread_attr, fiber_func, this);
        if (!rs.ok()) {
            TLOG_CRITICAL("Fail to create epoll thread: {}", rs.to_string());
            return rs;
        }
        return turbo::ok_status();
    }

    bool EventDispatcher::running() const {
        if(!_poller || !_poller->valid()) {
            return false;
        }
        if (_run_in_pthread) {
            return _thread_id != 0;
        } else {
            return _fiber.running();
        }
    }

    void EventDispatcher::stop() {
        if (_stop) {
            return;
        }
        _stop = true;
        wakeup();
        if(_run_in_pthread) {

        } else {
            _fiber.stop();
        }
    }

    void EventDispatcher::wakeup() {
        uint64_t buf = 1;
        auto r = ::write(_wakeup_fds[1], &buf, sizeof(buf));
        TURBO_UNUSED(r);
    }

    void EventDispatcher::join() {
        wakeup();
        if (_run_in_pthread) {
            if (_thread_id != 0) {
                pthread_join(_thread_id, nullptr);
                _thread_id = 0;
            }
        } else {
            _fiber.join();
        }
    }

    void *EventDispatcher::fiber_func(void *arg) {
        EventDispatcher *dispatcher = (EventDispatcher *) arg;
        dispatcher->loop();
        return nullptr;
    }

    void EventDispatcher::loop() {
        std::vector<PollResult> poll_results;
        poll_results.reserve(1024);
        while (!_stop) {
            auto rs = _poller->poll(poll_results, 1000);
            _num_iterators++;
            if (_stop) {
                break;
            }

            if (!rs.ok()) {
                if (rs.code() == kEINTR) {
                    continue;
                }
                TLOG_CRITICAL("Fail to poll, {}", rs.to_string());
                break;
            }

            for (auto &poll_result: poll_results) {
                if (poll_result.fd == _wakeup_fds[0]) {
                    // wake up
                    TLOG_INFO("Wake up");
                }
                ResourceId<EventChannel> channel_id{poll_result.event_channel_id};
                auto *event_channel = turbo::address_resource(channel_id);
                if (!event_channel) {
                    /// let it leak, user do not close the fd and remove the channel, let user
                    /// to fix they code
                    TLOG_CRITICAL("Fail to get event channel: {}", poll_result.event_channel_id);
                    continue;
                }
                if (poll_result.events & EPOLLIN) {
                    event_channel->handle_read(poll_result.events);
                }
                if (poll_result.events & EPOLLOUT) {
                    event_channel->handle_write(poll_result.events);
                }
            }
            poll_results.clear();
        }
    }

    int EventDispatcher::add_poll_in(EventChannelId channel_id, int fd) {
        TURBO_ASSERT(_poller);
        auto rs = _poller->add_poll_in(channel_id, fd);
        if (!rs.ok()) {
            TLOG_CRITICAL("Fail to add poll in: {}", rs.to_string());
            return -1;
        }
        return 0;
    }

    int EventDispatcher::add_poll_out(EventChannelId channel_id, int fd, bool pollin) {
        TURBO_ASSERT(_poller);
        auto rs = _poller->add_poll_out(channel_id, fd, pollin);
        if (!rs.ok()) {
            TLOG_CRITICAL("Fail to add poll out: {}", rs.to_string());
            return -1;
        }
        return 0;
    }

    int EventDispatcher::remove_poll_out(EventChannelId channel_id, int fd, bool pollin) {
        TURBO_ASSERT(_poller);
        auto rs = _poller->remove_poll_out(channel_id, fd, pollin);
        if (!rs.ok()) {
            TLOG_CRITICAL("Fail to remove poll out: {}", rs.to_string());
            return -1;
        }
        return 0;
    }

    turbo::Status EventDispatcher::remove_poll_in(int fd) {
        TURBO_ASSERT(_poller);
        auto rs = _poller->remove_poll_in(fd);
        if (!rs.ok()) {
            TLOG_CRITICAL("Fail to remove poll in: {}", rs.to_string());
            return rs;
        }
        return turbo::ok_status();
    }

    void EventDispatcher::handle_wakeup(EventChannel *channel, int event) {
        TURBO_UNUSED(channel);
        TURBO_UNUSED(event);
        uint64_t buf;
        auto r = ::read(channel->fd, &buf, sizeof(buf));
        if (r < 0 && errno != EAGAIN && errno != EINTR) {
            TLOG_CRITICAL("Fail to read wakeup fd: {}", strerror(errno));
        }
    }

    int io_dispatcher_concurrency = kDefaultIODispatcherConcurrent;
    bool g_user_fiber = kUserFiber;

    void setup_event_dispatcher(int concurrency, bool user_fiber) {
        TLOG_CHECK(concurrency > 0, "concurrency must be greater than 0");
        io_dispatcher_concurrency = concurrency;
        g_user_fiber = user_fiber;
    }

    std::vector<EventDispatcher *> g_io_dispatchers;
    EventDispatcher * listener_dispatcher = nullptr;

    void destroy_event_dispatchers() {
        for (int i = 0; i < io_dispatcher_concurrency; i++) {
            g_io_dispatchers[i]->stop();
            g_io_dispatchers[i]->join();
            delete g_io_dispatchers[i];
        }
        listener_dispatcher->stop();
        listener_dispatcher->join();
        delete listener_dispatcher;
    }

    void init_event_dispatchers() {
        FiberAttribute consumer_thread_attr = FIBER_ATTR_NORMAL;
        if(!g_user_fiber) {
            consumer_thread_attr = FIBER_ATTR_PTHREAD;
        }
        g_io_dispatchers.resize(io_dispatcher_concurrency);
        for (int i = 0; i < io_dispatcher_concurrency; i++) {
            g_io_dispatchers[i] = new EventDispatcher();
            if(!g_io_dispatchers[i]) {
                TLOG_CRITICAL("Fail to create io dispatcher");
                exit(-1);
            }
            if(g_io_dispatchers[i]->start(&consumer_thread_attr, g_user_fiber).ok()) {
                TLOG_INFO("Start io dispatcher {} success", i);
            } else {
                TLOG_CRITICAL("Fail to start io dispatcher {}", i);
                exit(-1);
            }
        }
        listener_dispatcher = new EventDispatcher();
        if(!listener_dispatcher) {
            TLOG_CRITICAL("Fail to create listener dispatcher");
            exit(-1);
        }
        if(listener_dispatcher->start(&consumer_thread_attr, g_user_fiber).ok()) {
            TLOG_INFO("Start listener dispatcher success");
        } else {
            TLOG_CRITICAL("Fail to start listener dispatcher");
            exit(-1);
        }
        TLOG_CHECK_EQ(0, atexit(destroy_event_dispatchers), "Fail to register destroy_event_dispatchers");
    }

    turbo::once_flag init_event_dispatchers_once;

    EventDispatcher *get_listener_dispatcher() {
        turbo::call_once(init_event_dispatchers_once, init_event_dispatchers);
        return listener_dispatcher;
    }

    EventDispatcher *get_io_dispatcher(int fd) {
        turbo::call_once(init_event_dispatchers_once, init_event_dispatchers);
        if(io_dispatcher_concurrency == 1) {
            return g_io_dispatchers[0];
        }
        auto index = turbo::hash_mixer4(fd);
        return g_io_dispatchers[index % io_dispatcher_concurrency];
    }
}  // namespace turbo
