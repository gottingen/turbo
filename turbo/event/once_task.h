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

#ifndef TURBO_ONCE_TASK_H_
#define TURBO_ONCE_TASK_H_

#include "turbo/platform/port.h"
#include <atomic>
#include <functional>
#include "turbo/times/time.h"
#include "turbo/status/status.h"
#include "turbo/memory/resource_pool.h"

namespace turbo {

    struct TURBO_CACHE_LINE_ALIGNED OnceTask {
    public:
        using task_fn_t = std::function<void(void *)>;
        using TaskId = uint64_t;

        constexpr static TaskId INVALID_TASK_ID = 0;

        turbo::Time run_time;
        task_fn_t fn;          // the fn(arg) to run
        void *arg;
        // Current TaskId, checked against version in TimerCore::run to test
        // if this task is unscheduled.
        TaskId task_id;
        // initial_version:     not run yet
        // initial_version + 1: running
        // initial_version + 2: removed (also the version of next Task reused
        //                      this struct)
        std::atomic<uint32_t> version;

        uint64_t               sequence{0};

        OnceTask() : version(2/*skip 0*/) {}

        // Run this task and delete this struct.
        // Returns true if fn(arg) did run.
        bool run_and_delete();

        // Delete this struct if this task was unscheduled.
        // Returns true on deletion.
        bool try_delete();

        turbo::Status cancel();

        constexpr uint32_t version_of_task_id(TaskId id) {
            return (uint32_t) (id >> 32);
        }

        constexpr turbo::ResourceId<OnceTask> slot_of_task_id(TaskId id) {
            turbo::ResourceId<OnceTask> slot = {(id & 0xFFFFFFFFul)};
            return slot;
        }
    };

}  // namespace turbo

#endif  // TURBO_ONCE_TASK_H_
