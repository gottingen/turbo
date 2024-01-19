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

#ifndef TURBO_EVENT_TYPES_H_
#define TURBO_EVENT_TYPES_H_

#include <functional>

namespace turbo {

    typedef uint64_t resource_id;

    struct EventChannel;

    typedef std::function<void(EventChannel *channel, int event)> ReadCallback;

    typedef std::function<void(EventChannel *channel, int event)> WriteCallback;

    /// do not  define and using on error callback here, user should handle error by the above callbacks.

    using TimerId = uint64_t;

    constexpr static TimerId INVALID_TIMER_ID = 0;

    typedef std::function<void(void *)> timer_task_fn_t;

    struct TimerOptions {
        size_t   num_buckets{13};
        uint64_t nano_delta{100};
        TimerOptions() = default;
    };

}  // namespace turbo

#endif  // TURBO_EVENT_TYPES_H_
