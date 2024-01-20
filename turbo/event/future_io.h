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

#ifndef TURBO_EVENT_FUTURE_IO_H_
#define TURBO_EVENT_FUTURE_IO_H_

#include "turbo/event/channel.h"

namespace turbo {

    class FutureIO {
    public:
        FutureIO() = default;

        ~FutureIO();

        turbo::Status initialize(int fd);

        turbo::Status wait_readable();

        turbo::Status wait_writable();

        turbo::Status wait_readable_until(turbo::Time deadline);

        turbo::Status wait_writable_until(turbo::Time deadline);

        turbo::Status wait_readable_for(turbo::Duration duration);

        turbo::Status wait_writable_for(turbo::Duration duration);

        turbo::Status cancel();

        void destroy();

    private:
        static void handle_read(EventChannel *channel, int events);

        static void handle_write(EventChannel *channel, int events);

        EventChannelId _ecid{DEFAULT_EVENT_CHANNEL_ID};
        int _fd{-1};
    };
}  // namespace turbo
#endif // TURBO_EVENT_FUTURE_IO_H_
