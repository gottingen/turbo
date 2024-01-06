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

#ifndef TURBO_FIBER_INTERNAL_WAITABLE_EVENT_H_
#define TURBO_FIBER_INTERNAL_WAITABLE_EVENT_H_

#include <cerrno>                               // users need to check errno
#include <ctime>                                // timespec
#include "turbo/fiber/internal/types.h"
#include "turbo/base/status.h"

namespace turbo::fiber_internal {

    // Create a waitable_event which is a futex-like 32-bit primitive for synchronizing
    // fiber/pthreads.
    // Returns a pointer to 32-bit data, nullptr on failure.
    // NOTE: all waitable_events are private(not inter-process).
    void *waitable_event_create();

    // Check width of user type before casting.
    template<typename T>
    T *waitable_event_create_checked() {
        static_assert(sizeof(T) == sizeof(int), "sizeof_T_must_equal_int");
        return static_cast<T *>(waitable_event_create());
    }

    // Destroy the event.
    void waitable_event_destroy(void *event);

    // Wake up at most 1 thread waiting on |event|.
    // Returns # of threads woken up.
    int waitable_event_wake(void *event);

    // Wake up all threads waiting on |event|.
    // Returns # of threads woken up.
    int waitable_event_wake_all(void *event);

    // Wake up all threads waiting on |event| except a fiber whose identifier
    // is |excluded_fiber|. This function does not yield.
    // Returns # of threads woken up.
    int waitable_event_wake_except(void *event, fiber_id_t excluded_fiber);

    // Wake up at most 1 thread waiting on |futex1|, let all other threads wait
    // on |futex2| instead.
    // Returns # of threads woken up.
    int waitable_event_requeue(void *event1, void *event2);

    // Atomically wait on |event| if *event equals |expected_value|, until the
    // event is woken up by waitable_event_wake*, or CLOCK_REALTIME reached |abstime| if
    // abstime is not nullptr.
    // About |abstime|:
    //   Different from FUTEX_WAIT, waitable_event_wait uses absolute time.
    // Returns 0 on success, -1 otherwise and errno is set.
    turbo::Status waitable_event_wait(void *event, int expected_value, const timespec *abstime);

}  // namespace turbo::fiber_internal

#endif  // TURBO_FIBER_INTERNAL_WAITABLE_EVENT_H_
