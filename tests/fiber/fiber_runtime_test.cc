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
// Created by jeff on 24-1-3.
//

#define DOCTEST_CONFIG_IMPLEMENT_WITH_MAIN

#include "turbo/testing/test.h"
#include "turbo/times/stop_watcher.h"
#include "turbo/log/logging.h"
#include <turbo/fiber/internal/waitable_event.h>
#include "turbo/log/logging.h"
#include "turbo/fiber/runtime.h"
#include "turbo/fiber/internal/fiber.h"
#include "turbo/fiber/internal/schedule_group.h"

namespace turbo::fiber_internal {
    extern ScheduleGroup *g_task_control;


    void *dummy(void *) {
        return nullptr;
    }

    TEST_CASE("FiberTest, setconcurrency") {
        REQUIRE_EQ(8 + turbo::FiberConfig::FIBER_EPOLL_THREAD_NUM, (size_t) turbo::fiber_get_concurrency());
        REQUIRE_EQ(turbo::invalid_argument_error(""), turbo::fiber_set_concurrency(turbo::FiberConfig::FIBER_MIN_CONCURRENCY - 1));
        REQUIRE_EQ(turbo::invalid_argument_error(""), turbo::fiber_set_concurrency(0));
        REQUIRE_EQ(turbo::invalid_argument_error(""), turbo::fiber_set_concurrency(-1));
        REQUIRE_EQ(turbo::invalid_argument_error(""), turbo::fiber_set_concurrency(turbo::FiberConfig::FIBER_MAX_CONCURRENCY + 1));
        REQUIRE_EQ(turbo::ok_status(), turbo::fiber_set_concurrency(turbo::FiberConfig::FIBER_MIN_CONCURRENCY));
        REQUIRE_EQ(turbo::FiberConfig::FIBER_MIN_CONCURRENCY, turbo::fiber_get_concurrency());
        REQUIRE_EQ(turbo::ok_status(), turbo::fiber_set_concurrency(turbo::FiberConfig::FIBER_MIN_CONCURRENCY + 1));
        REQUIRE_EQ(turbo::FiberConfig::FIBER_MIN_CONCURRENCY + 1, turbo::fiber_get_concurrency());
        REQUIRE_EQ(turbo::ok_status(), turbo::fiber_set_concurrency(turbo::FiberConfig::FIBER_MIN_CONCURRENCY));  // smaller value
        fiber_id_t th;
        REQUIRE_EQ(turbo::ok_status(), fiber_start_urgent(&th, nullptr, dummy, nullptr));
        REQUIRE_EQ(turbo::FiberConfig::FIBER_MIN_CONCURRENCY + 1, turbo::fiber_get_concurrency());
        REQUIRE_EQ(turbo::ok_status(), turbo::fiber_set_concurrency(turbo::FiberConfig::FIBER_MIN_CONCURRENCY + 5));
        REQUIRE_EQ(turbo::FiberConfig::FIBER_MIN_CONCURRENCY + 5, turbo::fiber_get_concurrency());
        REQUIRE_EQ(turbo::resource_exhausted_error(""), turbo::fiber_set_concurrency(turbo::FiberConfig::FIBER_MIN_CONCURRENCY + 1));
        REQUIRE_EQ(turbo::FiberConfig::FIBER_MIN_CONCURRENCY + 5, turbo::fiber_get_concurrency());
    }

    static std::atomic<int> *odd;
    static std::atomic<int> *even;

    static std::atomic<int> nfibers(0);
    static std::atomic<int> npthreads(0);
    static thread_local bool counted = false;
    static std::atomic<bool> stop(false);

    static void *odd_thread(void *) {
        nfibers.fetch_add(1);
        while (!stop) {
            if (!counted) {
                counted = true;
                npthreads.fetch_add(1);
            }
            turbo::fiber_internal::waitable_event_wake_all(even);
            TURBO_UNUSED(turbo::fiber_internal::waitable_event_wait(odd, 0, nullptr));
        }
        return nullptr;
    }

    static void *even_thread(void *) {
        nfibers.fetch_add(1);
        while (!stop) {
            if (!counted) {
                counted = true;
                npthreads.fetch_add(1);
            }
            turbo::fiber_internal::waitable_event_wake_all(odd);
            TURBO_UNUSED(turbo::fiber_internal::waitable_event_wait(even, 0, nullptr));
        }
        return nullptr;
    }

    TEST_CASE("FiberTest, setconcurrency_with_running_fiber") {
        odd = turbo::fiber_internal::waitable_event_create_checked<std::atomic<int> >();
        even = turbo::fiber_internal::waitable_event_create_checked<std::atomic<int> >();
        REQUIRE((odd != nullptr && even != nullptr));
        *odd = 0;
        *even = 0;
        std::vector<fiber_id_t> tids;
        const int N = 500;
        for (int i = 0; i < N; ++i) {
            fiber_id_t tid;
            TURBO_UNUSED(fiber_start_background(&tid, &FIBER_ATTR_SMALL, odd_thread, nullptr));
            tids.push_back(tid);
            TURBO_UNUSED(fiber_start_background(&tid, &FIBER_ATTR_SMALL, even_thread, nullptr));
            tids.push_back(tid);
        }
        for (int i = 100; i <= N; ++i) {
            REQUIRE_EQ(turbo::ok_status(), turbo::fiber_set_concurrency(i));
            REQUIRE_EQ(i, turbo::fiber_get_concurrency());
        }
        usleep(1000 * N);
        *odd = 1;
        *even = 1;
        stop = true;
        turbo::fiber_internal::waitable_event_wake_all(odd);
        turbo::fiber_internal::waitable_event_wake_all(even);
        for (size_t i = 0; i < tids.size(); ++i) {
            TURBO_UNUSED(fiber_join(tids[i], nullptr));
        }
        TLOG_INFO("All fibers has quit");
        REQUIRE_EQ(2 * N, nfibers);
        TLOG_INFO("Touched pthreads={}", npthreads.load());
    }

    void *sleep_proc(void *) {
        usleep(100000);
        return nullptr;
    }

    void *add_concurrency_proc(void *) {
        fiber_id_t tid;
        TURBO_UNUSED(fiber_start_background(&tid, &FIBER_ATTR_SMALL, sleep_proc, nullptr));
        TURBO_UNUSED(fiber_join(tid, nullptr));
        return nullptr;
    }

} // namespace turbo::fiber_internal
