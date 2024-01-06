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

#include "turbo/fiber/this_fiber.h"
#include "turbo/fiber/internal/fiber_worker.h"
#include "turbo/times/clock.h"

namespace turbo::fiber_internal {
    extern TURBO_THREAD_LOCAL FiberWorker *tls_task_group;
}  // namespace turbo::fiber_internal


namespace turbo {
    int fiber_yield() {
        turbo::fiber_internal::FiberWorker *g = turbo::fiber_internal::tls_task_group;
        if (nullptr != g && !g->is_current_pthread_task()) {
            turbo::fiber_internal::FiberWorker::yield(&g);
            return 0;
        }
        // pthread_yield is not available on MAC
        return sched_yield();
    }

    turbo::Status fiber_sleep_until(const turbo::Time& deadline) {
        turbo::fiber_internal::FiberWorker *g = turbo::fiber_internal::tls_task_group;
        if (nullptr != g && !g->is_current_pthread_task()) {
            return turbo::fiber_internal::FiberWorker::sleep(&g, deadline);
        }
        turbo::sleep_until(deadline);
        return turbo::ok_status();
    }

    turbo::Status fiber_sleep_for(const turbo::Duration& span) {
        turbo::fiber_internal::FiberWorker *g = turbo::fiber_internal::tls_task_group;
        if (nullptr != g && !g->is_current_pthread_task()) {
            return turbo::fiber_internal::FiberWorker::sleep(&g, span);
        }
        turbo::sleep_for(span);
        return turbo::ok_status();
    }

    fiber_id_t get_fiber_id() {
        turbo::fiber_internal::FiberWorker *g = turbo::fiber_internal::tls_task_group;
        if (nullptr != g && !g->is_current_pthread_task()) {
            return static_cast<fiber_id_t>(g->current_fid());
        }
        return static_cast<fiber_id_t>(turbo::fiber_internal::INVALID_FIBER_ID);
    }
}  // namespace turbo