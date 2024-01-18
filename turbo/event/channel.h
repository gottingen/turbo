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

#ifndef TURBO_EVENT_CHANNEL_H_
#define TURBO_EVENT_CHANNEL_H_

#include "turbo/platform/port.h"
#include "turbo/event/types.h"
#include "turbo/memory/resource_pool.h"

#define TURBO_EVENT_CHANNEL_NBLOCK 262144
#define TURBO_EVENT_CHANNEL_BLOCK_SIZE 256

namespace turbo {

    /// if no channel id is specified, use this default channel id
    static constexpr resource_id DEFAULT_EVENT_CHANNEL_ID = -1;

    struct EventChannel {
        /// this handle_read
        void handle_read(int fd, int events);

        void handle_write(int fd, int events);

        ReadCallback  read_callback;
        WriteCallback write_callback;
        resource_id   rid{DEFAULT_EVENT_CHANNEL_ID};
        int32_t       version{0};
    };

    typedef uint64_t EventChannelId;

}  // namespace turbo

#endif // TURBO_EVENT_CHANNEL_H_
