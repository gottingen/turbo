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


#ifndef  TURBO_FIBER_INTERNAL_FIBER_COND_H_
#define  TURBO_FIBER_INTERNAL_FIBER_COND_H_

#include "turbo/times/time.h"
#include "turbo/fiber/internal/mutex.h"
#include "turbo/platform/port.h"

namespace turbo::fiber_internal {

    struct fiber_cond_t{
        fiber_mutex_t *m;
        int *seq;
    };

    struct fiber_condattr_t{
    };

    turbo::Status fiber_cond_init(fiber_cond_t *TURBO_RESTRICT cond,
                        const fiber_condattr_t *TURBO_RESTRICT cond_attr);

    void fiber_cond_destroy(fiber_cond_t *cond);

    void fiber_cond_signal(fiber_cond_t *cond);

    void fiber_cond_broadcast(fiber_cond_t *cond);

    turbo::Status fiber_cond_wait(fiber_cond_t *TURBO_RESTRICT cond,
                        fiber_mutex_t *TURBO_RESTRICT mutex);

    turbo::Status fiber_cond_timedwait(
            fiber_cond_t *TURBO_RESTRICT cond,
            fiber_mutex_t *TURBO_RESTRICT mutex,
            const struct timespec *TURBO_RESTRICT abstime);
}  // namespace turbo::fiber_internal

#endif  // TURBO_FIBER_INTERNAL_FIBER_COND_H_
