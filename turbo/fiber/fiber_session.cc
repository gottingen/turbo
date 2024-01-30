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
// Created by jeff on 24-1-11.
//

#include "turbo/fiber/fiber_session.h"

namespace turbo {

    turbo::Status FiberSession::initialize() {
        auto r = turbo::fiber_internal::fiber_session_create(&session_, nullptr, nullptr);
        if (r != 0) {
            return turbo::errno_to_status(errno,"");
        }
        return turbo::ok_status();
    }

    turbo::Status FiberSession::initialize(void *data, const session_on_error &callback) {

        auto r = turbo::fiber_internal::fiber_session_create(&session_, data, callback);
        if (r != 0) {
            return turbo::errno_to_status(errno,"");
        }
        return turbo::ok_status();

    }

}  // namespace turbo