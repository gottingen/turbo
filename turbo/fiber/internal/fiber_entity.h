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


#ifndef TURBO_FIBER_INTERNAL_FIBER_ENTITY_H_
#define TURBO_FIBER_INTERNAL_FIBER_ENTITY_H_

#include <pthread.h>                 // pthread_spin_init
#include <functional>
#include <atomic>
#include "turbo/fiber/internal/waitable_event.h"
#include "turbo/fiber/internal/types.h"
#include "turbo/fiber/internal/stack.h"           // ContextualStack

namespace turbo::fiber_internal {

    struct FiberStatistics {
        int64_t cputime_ns{0};
        int64_t nswitch{0};
    };

    class KeyTable;

    struct EventWaiterNode;

    struct fiber_local_storage {
        KeyTable *keytable;
        void *assigned_data;
        void *rpcz_parent_span;
    };

#define FIBER_LOCAL_STORAGE_INITIALIZER { nullptr, nullptr, nullptr }

    const static fiber_local_storage LOCAL_STORAGE_INIT = FIBER_LOCAL_STORAGE_INITIALIZER;

    struct FiberEntity {
        // [Not Reset]
        std::atomic<EventWaiterNode *> current_waiter;
        uint64_t current_sleep;

        // A builtin flag to mark if the thread is stopping.
        bool stop;

        // The thread is interrupted and should wake up from some blocking ops.
        bool interrupted;

        // Scheduling of the thread can be delayed.
        bool about_to_quit;

        // [Not Reset] guarantee visibility of version_futex.
        turbo::SpinLock version_lock;

        // [Not Reset] only modified by one fiber at any time, no need to be atomic
        uint32_t *version_futex;

        // The identifier. It does not have to be here, however many code is
        // simplified if they can get tid from FiberEntity.
        fiber_id_t tid;

        // User function and argument
        //void *(*fn)(void *);
        std::function<void*(void*)> fn;
        void *arg;

        // Stack of this task.
        ContextualStack *stack;

        // Attributes creating this task
        FiberAttribute attr;

        // Statistics
        int64_t cpuwide_start_ns;
        FiberStatistics stat;

        // fiber local storage, sync with tls_bls (defined in task_group.cpp)
        // when the fiber is created or destroyed.
        // DO NOT use this field directly, use tls_bls instead.
        fiber_local_storage local_storage;

    public:
        // Only initialize [Not Reset] fields, other fields will be reset in
        // fiber_start* functions
        FiberEntity()
                : current_waiter(nullptr), current_sleep(0), stack(nullptr) {
            version_futex = waitable_event_create_checked<uint32_t>();
            *version_futex = 1;
        }

        ~FiberEntity() {
            waitable_event_destroy(version_futex);
            version_futex = nullptr;
        }

        void set_stack(ContextualStack *s) {
            stack = s;
        }

        ContextualStack *release_stack() {
            ContextualStack *tmp = stack;
            stack = nullptr;
            return tmp;
        }

        StackType stack_type() const {
            return static_cast<StackType>(attr.stack_type);
        }
    };

}  // namespace turbo::fiber_internal

#endif  // TURBO_FIBER_INTERNAL_FIBER_ENTITY_H_
