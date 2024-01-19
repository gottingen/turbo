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

#ifndef TURBO_EVENT_POLLER_H_
#define TURBO_EVENT_POLLER_H_

#include "turbo/event/channel.h"
#include "turbo/status/status.h"
#include <vector>

namespace turbo {

    struct PollResult {
        int fd;
        int events;
        EventChannelId event_channel_id;
    };

    class Poller {
    public:
        virtual ~Poller() = default;

        virtual Status initialize() = 0;

        virtual Status destroy() = 0;

        virtual bool valid() const = 0;

        virtual Status add_poll_in(EventChannelId socket_id, int fd) = 0;

        virtual Status add_poll_out(EventChannelId socket_id, int fd, bool pollin) = 0;

        virtual Status remove_poll_out(EventChannelId socket_id, int fd, bool keep_pollin) = 0;

        virtual Status remove_poll_in(int fd)  = 0;

        virtual Status poll(std::vector<PollResult> &poll_results, int timeout) = 0;
    };

    Poller * create_poller();
}  // namespace turbo

#endif  // TURBO_EVENT_POLLER_H_
