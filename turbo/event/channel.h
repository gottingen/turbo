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
#include "turbo/fiber/wait_event.h"

namespace turbo {

    /// if no channel id is specified, use this default channel id
    static constexpr resource_id DEFAULT_EVENT_CHANNEL_ID = -1;

    struct EventChannel {
        EventChannel() = default;
        ~EventChannel();
        EventChannel(const EventChannel &) = delete;
        EventChannel &operator=(const EventChannel &) = delete;
        /// this handle_read
        void handle_read(int fd, int events);

        void handle_write(int fd, int events);

        ReadCallback                 read_callback{nullptr};
        WriteCallback                write_callback{nullptr};
        resource_id                  rid{DEFAULT_EVENT_CHANNEL_ID};
        int32_t                      version{0};
        WaitEvent<std::atomic<int>>  wait_event;
        turbo::Status initialize(resource_id rid, int32_t version);

        void destroy();
    };

    typedef uint64_t EventChannelId;

    inline EventChannel *get_event_channel(EventChannelId *cid, int version = 0) {
        ResourceId<EventChannel> temp;
        auto ptr = get_resource(&temp);
        *cid = make_resource_id(version, temp);
        return ptr;
    }

    inline EventChannel *get_event_channel(EventChannelId cid) {
        ResourceId<EventChannel> temp = get_resource_id<EventChannel>(cid);
        return address_resource(temp);
    }

    inline void return_event_channel(EventChannelId cid) {
        ResourceId<EventChannel> temp = get_resource_id<EventChannel>(cid);
        auto ptr = address_resource(temp);
        if(ptr == nullptr) {
            return;
        }
        ptr->destroy();
        return_resource(temp);
    }

}  // namespace turbo

#endif // TURBO_EVENT_CHANNEL_H_
