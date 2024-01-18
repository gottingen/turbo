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

#ifndef TURBO_EVENT_EVENT_DISPATCHER_H_
#define TURBO_EVENT_EVENT_DISPATCHER_H_

#include <memory>
#include "turbo/event/channel.h"
#include "turbo/fiber/fiber.h"
#include "turbo/event/poller.h"

namespace turbo {

    class EventDispatcher {
    public:
        EventDispatcher() = default;

        ~EventDispatcher();

        Status start(const FiberAttribute *consumer_thread_attr);

        bool running() const;

        void stop();

        void join();

        int add_consumer(EventChannelId channel_id, int fd);

        int add_poll_out(EventChannelId channel_id, int fd, bool pollin);

        int remove_poll_out(EventChannelId channel_id, int fd, bool pollin);

        int64_t num_iterators() const { return _num_iterators; }

    private:
        TURBO_NON_COPYABLE(EventDispatcher);

        static void* fiber_func(void* arg);

        void loop();

        int RemoveConsumer(int fd);

        Status handle_wakeup();

        volatile bool _stop{false};

        int _wakeup_fds[2];

        int64_t _num_iterators{0};

        Fiber _fiber;

        FiberAttribute _consumer_thread_attr;

        std::unique_ptr<Poller> _poller{nullptr};
    };
}  // namespace turbo

#endif  // TURBO_EVENT_EVENT_DISPATCHER_H_
