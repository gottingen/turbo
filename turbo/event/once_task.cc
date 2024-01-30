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

#include "turbo/event/once_task.h"
#include "turbo/log/logging.h"

namespace turbo {

    bool OnceTask::run_and_delete() {
        const uint32_t id_version = version_of_task_id(task_id);
        uint32_t expected_version = id_version;
        // This CAS is rarely contended, should be fast.
        if (version.compare_exchange_strong(
                expected_version, id_version + 1, std::memory_order_relaxed)) {
            fn(arg);
            // The release fence is paired with acquire fence in
            // TimerCore::unschedule to make changes of fn(arg) visible.
            version.store(id_version + 2, std::memory_order_release);
            turbo::return_resource(slot_of_task_id(task_id));
            return true;
        } else if (expected_version == id_version + 2) {
            // already unscheduled.
            turbo::return_resource(slot_of_task_id(task_id));
            return false;
        } else {
            // Impossible.
            TLOG_ERROR("Invalid version={}, expecting {}", expected_version, id_version + 2);
            return false;
        }
    }

    turbo::Status OnceTask::cancel() {
        const turbo::ResourceId<OnceTask> slot_id = slot_of_task_id(task_id);
        OnceTask *const task = turbo::address_resource(slot_id);
        if (task == nullptr) {
            TLOG_ERROR("Invalid task id={}", task_id);
            return turbo::make_status(kEINVAL);
        }
        const uint32_t id_version = version_of_task_id(task_id);
        uint32_t expected_version = id_version;
        if (task->version.compare_exchange_strong(expected_version, id_version + 2,std::memory_order_acquire)) {
            return turbo::ok_status();
        }
        return (expected_version == id_version + 1) ? turbo::make_status(kEBUSY) : turbo::make_status(kESTOP);
    }

    bool OnceTask::try_delete() {
        const uint32_t id_version = version_of_task_id(task_id);
        if (version.load(std::memory_order_relaxed) != id_version) {
            TLOG_CHECK_EQ(version.load(std::memory_order_relaxed), id_version + 2);
            turbo::return_resource(slot_of_task_id(task_id));
            return true;
        }
        return false;
    }

}