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
// Created by jeff on 23-12-16.
//

#ifndef TURBO_FIBER_INTERNAL_REMOTE_TASK_QUEUE_H_
#define TURBO_FIBER_INTERNAL_REMOTE_TASK_QUEUE_H_

#include "turbo/container/bounded_queue.h"
#include "turbo/fiber/internal/types.h"

namespace turbo::fiber_internal {

    class FiberWorker;

    class RemoteTaskQueue {
    public:
        RemoteTaskQueue() {}

        int init(size_t cap) {
            const size_t memsize = sizeof(fiber_id_t) * cap;
            void *q_mem = malloc(memsize);
            if (q_mem == nullptr) {
                return -1;
            }
            turbo::bounded_queue<fiber_id_t> q(q_mem, memsize, turbo::OWNS_STORAGE);
            _tasks.swap(q);
            return 0;
        }

        bool pop(fiber_id_t *task) {
            if (_tasks.empty()) {
                return false;
            }
            _mutex.lock();
            const bool result = _tasks.pop(task);
            _mutex.unlock();
            return result;
        }

        bool push(fiber_id_t task) {
            _mutex.lock();
            const bool res = push_locked(task);
            _mutex.unlock();
            return res;
        }

        bool push_locked(fiber_id_t task) {
            return _tasks.push(task);
        }

        size_t capacity() const { return _tasks.capacity(); }

    private:
        friend class FiberWorker;
        // nolint
        TURBO_NON_COPYABLE(RemoteTaskQueue);
        turbo::bounded_queue<fiber_id_t> _tasks;
        std::mutex _mutex;
    };

}  // namespace turbo::fiber_internal

#endif  // TURBO_FIBER_INTERNAL_REMOTE_TASK_QUEUE_H_
