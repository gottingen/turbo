// Licensed to the Apache Software Foundation (ASF) under one
// or more contributor license agreements.  See the NOTICE file
// distributed with this work for additional information
// regarding copyright ownership.  The ASF licenses this file
// to you under the Apache License, Version 2.0 (the
// "License"); you may not use this file except in compliance
// with the License.  You may obtain a copy of the License at
//
//   http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing,
// software distributed under the License is distributed on an
// "AS IS" BASIS, WITHOUT WARRANTIES OR CONDITIONS OF ANY
// KIND, either express or implied.  See the License for the
// specific language governing permissions and limitations
// under the License.

#ifndef FLARE_FIBER_INTERNAL_FIBER_ENTITY_H_
#define FLARE_FIBER_INTERNAL_FIBER_ENTITY_H_

#include <pthread.h>                 // pthread_spin_init
#include <functional>
#include "flare/fiber/internal/waitable_event.h"           // butex_construct/destruct
#include "flare/base/static_atomic.h"          // std::atomic
#include "flare/fiber/internal/types.h"           // fiber_attribute
#include "flare/fiber/internal/stack.h"           // fiber_contextual_stack

namespace flare::fiber_internal {

    struct fiber_statistics {
        int64_t cputime_ns;
        int64_t nswitch;
    };

    class KeyTable;

    struct fiber_mutex_waiter;

    struct fiber_local_storage {
        KeyTable *keytable;
        void *assigned_data;
        void *rpcz_parent_span;
    };

#define FIBER_LOCAL_STORAGE_INITIALIZER { nullptr, nullptr, nullptr }

    const static fiber_local_storage LOCAL_STORAGE_INIT = FIBER_LOCAL_STORAGE_INITIALIZER;

    struct fiber_entity {
        // [Not Reset]
        std::atomic<fiber_mutex_waiter *> current_waiter;
        uint64_t current_sleep;

        // A builtin flag to mark if the thread is stopping.
        bool stop;

        // The thread is interrupted and should wake up from some blocking ops.
        bool interrupted;

        // Scheduling of the thread can be delayed.
        bool about_to_quit;

        // [Not Reset] guarantee visibility of version_butex.
        pthread_spinlock_t version_lock;

        // [Not Reset] only modified by one fiber at any time, no need to be atomic
        uint32_t *version_butex;

        // The identifier. It does not have to be here, however many code is
        // simplified if they can get tid from fiber_entity.
        fiber_id_t tid;

        // User function and argument
        //void *(*fn)(void *);
        std::function<void*(void*)> fn;
        void *arg;

        // Stack of this task.
        fiber_contextual_stack *stack;

        // Attributes creating this task
        fiber_attribute attr;

        // Statistics
        int64_t cpuwide_start_ns;
        fiber_statistics stat;

        // fiber local storage, sync with tls_bls (defined in task_group.cpp)
        // when the fiber is created or destroyed.
        // DO NOT use this field directly, use tls_bls instead.
        fiber_local_storage local_storage;

    public:
        // Only initialize [Not Reset] fields, other fields will be reset in
        // fiber_start* functions
        fiber_entity()
                : current_waiter(nullptr), current_sleep(0), stack(nullptr) {
            pthread_spin_init(&version_lock, 0);
            version_butex = waitable_event_create_checked<uint32_t>();
            *version_butex = 1;
        }

        ~fiber_entity() {
            waitable_event_destroy(version_butex);
            version_butex = nullptr;
            pthread_spin_destroy(&version_lock);
        }

        void set_stack(fiber_contextual_stack *s) {
            stack = s;
        }

        fiber_contextual_stack *release_stack() {
            fiber_contextual_stack *tmp = stack;
            stack = nullptr;
            return tmp;
        }

        fiber_stack_type stack_type() const {
            return static_cast<fiber_stack_type>(attr.stack_type);
        }
    };

}  // namespace flare::fiber_internal

#endif  // FLARE_FIBER_INTERNAL_FIBER_ENTITY_H_
