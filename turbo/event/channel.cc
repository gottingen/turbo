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

#include "turbo/event/channel.h"

namespace turbo {

    void EventChannel::handle_read(int fd, int events) {
        if (read_callback) {
            read_callback(rid, fd, events);
        }
    }

    void EventChannel::handle_write(int fd, int events) {
        if (write_callback) {
            write_callback(rid, fd);
        }
    }

}  // namespace turbo