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
#include "turbo/event/future_io.h"
#include "turbo/event/event_dispatcher.h"

namespace turbo {

    EventDispatcher * get_event_dispatcher();

    FutureIO::~FutureIO() {
        if(_ecid != DEFAULT_EVENT_CHANNEL_ID) {
            auto channel = get_event_channel(_ecid);
            if(channel) {
                channel->destroy();
            }
        }
    }
    turbo::Status FutureIO::initialize(int fd) {
        if(fd < 0) {
            return make_status(turbo::kEINVAL, "invalid fd");
        }
        _fd = fd;
        auto channel = get_event_channel(&_ecid);
        if(!channel) {
            return make_status(turbo::kENOMEM, "Fail to get event channel");
        }
        auto status = channel->initialize(fd, 0);
        if(!status.ok()) {
            return status;
        }
        return ok_status();
    }

    turbo::Status FutureIO::wait_readable_until(turbo::Time deadline) {
        auto channel = get_event_channel(_ecid);
        if(!channel) {
            return make_status(turbo::kENOMEM, "Fail to get event channel");
        }
        channel->read_callback = handle_read;
        channel->write_callback = nullptr;
        channel->user_data = this;
        channel->wait_event.store(0);
        auto dispatcher = get_event_dispatcher();
        if(!dispatcher) {
            return make_status(turbo::kENOMEM, "Fail to get event dispatcher");
        }
        dispatcher->add_poll_in(channel->rid, _fd);
        auto status = channel->wait_event.wait_until(deadline);
        return ok_status();
    }

    void FutureIO::destroy() {
        if(_ecid != DEFAULT_EVENT_CHANNEL_ID) {
            auto channel = get_event_channel(_ecid);
            if (channel) {
                channel->destroy();
            }
            return_event_channel(_ecid);
            _ecid = DEFAULT_EVENT_CHANNEL_ID;
        }
    }
    void FutureIO::handle_read(EventChannel *channel, int events) {
        channel->wait_event.notify_all();
    }
    void FutureIO::handle_write(EventChannel *channel, int events) {
        channel->wait_event.notify_all();
    }

}  // namespace turbo
