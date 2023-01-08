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

#include "testing/gtest_wrap.h"
#include <gflags/gflags.h>
#include "flare/base/static_atomic.h"
#include "flare/times/time.h"
#include "flare/log/logging.h"
#include "flare/thread/thread.h"
#include <flare/fiber/internal/waitable_event.h>
#include "flare/log/logging.h"
#include "flare/fiber/runtime.h"
#include "flare/fiber/internal/fiber.h"
#include "flare/fiber/internal/schedule_group.h"

namespace flare::fiber_internal {
    extern schedule_group *g_task_control;
}

namespace {
    void *dummy(void *) {
        return nullptr;
    }

    TEST(FiberTest, setconcurrency) {
        ASSERT_EQ(8 + FIBER_EPOLL_THREAD_NUM, (size_t) flare::fiber_getconcurrency());
        ASSERT_EQ(EINVAL, flare::fiber_setconcurrency(FIBER_MIN_CONCURRENCY - 1));
        ASSERT_EQ(EINVAL, flare::fiber_setconcurrency(0));
        ASSERT_EQ(EINVAL, flare::fiber_setconcurrency(-1));
        ASSERT_EQ(EINVAL, flare::fiber_setconcurrency(FIBER_MAX_CONCURRENCY + 1));
        ASSERT_EQ(0, flare::fiber_setconcurrency(FIBER_MIN_CONCURRENCY));
        ASSERT_EQ(FIBER_MIN_CONCURRENCY, flare::fiber_getconcurrency());
        ASSERT_EQ(0, flare::fiber_setconcurrency(FIBER_MIN_CONCURRENCY + 1));
        ASSERT_EQ(FIBER_MIN_CONCURRENCY + 1, flare::fiber_getconcurrency());
        ASSERT_EQ(0, flare::fiber_setconcurrency(FIBER_MIN_CONCURRENCY));  // smaller value
        fiber_id_t th;
        ASSERT_EQ(0, fiber_start_urgent(&th, nullptr, dummy, nullptr));
        ASSERT_EQ(FIBER_MIN_CONCURRENCY + 1, flare::fiber_getconcurrency());
        ASSERT_EQ(0, flare::fiber_setconcurrency(FIBER_MIN_CONCURRENCY + 5));
        ASSERT_EQ(FIBER_MIN_CONCURRENCY + 5, flare::fiber_getconcurrency());
        ASSERT_EQ(EPERM, flare::fiber_setconcurrency(FIBER_MIN_CONCURRENCY + 1));
        ASSERT_EQ(FIBER_MIN_CONCURRENCY + 5, flare::fiber_getconcurrency());
    }

    static std::atomic<int> *odd;
    static std::atomic<int> *even;

    static std::atomic<int> nfibers(0);
    static std::atomic<int> npthreads(0);
    static FLARE_THREAD_LOCAL bool counted = false;
    static std::atomic<bool> stop(false);

    static void *odd_thread(void *) {
        nfibers.fetch_add(1);
        while (!stop) {
            if (!counted) {
                counted = true;
                npthreads.fetch_add(1);
            }
            flare::fiber_internal::waitable_event_wake_all(even);
            flare::fiber_internal::waitable_event_wait(odd, 0, nullptr);
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
            flare::fiber_internal::waitable_event_wake_all(odd);
            flare::fiber_internal::waitable_event_wait(even, 0, nullptr);
        }
        return nullptr;
    }

    TEST(FiberTest, setconcurrency_with_running_fiber) {
        odd = flare::fiber_internal::waitable_event_create_checked<std::atomic<int> >();
        even = flare::fiber_internal::waitable_event_create_checked<std::atomic<int> >();
        ASSERT_TRUE(odd != nullptr && even != nullptr);
        *odd = 0;
        *even = 0;
        std::vector<fiber_id_t> tids;
        const int N = 500;
        for (int i = 0; i < N; ++i) {
            fiber_id_t tid;
            fiber_start_background(&tid, &FIBER_ATTR_SMALL, odd_thread, nullptr);
            tids.push_back(tid);
            fiber_start_background(&tid, &FIBER_ATTR_SMALL, even_thread, nullptr);
            tids.push_back(tid);
        }
        for (int i = 100; i <= N; ++i) {
            ASSERT_EQ(0, flare::fiber_setconcurrency(i));
            ASSERT_EQ(i, flare::fiber_getconcurrency());
        }
        usleep(1000 * N);
        *odd = 1;
        *even = 1;
        stop = true;
        flare::fiber_internal::waitable_event_wake_all(odd);
        flare::fiber_internal::waitable_event_wake_all(even);
        for (size_t i = 0; i < tids.size(); ++i) {
            fiber_join(tids[i], nullptr);
        }
        FLARE_LOG(INFO) << "All fibers has quit";
        ASSERT_EQ(2 * N, nfibers);
        // This is not necessarily true, not all workers need to run sth.
        //ASSERT_EQ(N, npthreads);
        FLARE_LOG(INFO) << "Touched pthreads=" << npthreads;
    }

    void *sleep_proc(void *) {
        usleep(100000);
        return nullptr;
    }

    void *add_concurrency_proc(void *) {
        fiber_id_t tid;
        fiber_start_background(&tid, &FIBER_ATTR_SMALL, sleep_proc, nullptr);
        fiber_join(tid, nullptr);
        return nullptr;
    }

    bool set_min_concurrency(int num) {
        std::stringstream ss;
        ss << num;
        std::string ret = google::SetCommandLineOption("fiber_min_concurrency", ss.str().c_str());
        return !ret.empty();
    }

    int get_min_concurrency() {
        std::string ret;
        google::GetCommandLineOption("fiber_min_concurrency", &ret);
        return atoi(ret.c_str());
    }

    TEST(FiberTest, min_concurrency) {
        ASSERT_EQ(1, set_min_concurrency(-1)); // set min success
        ASSERT_EQ(1, set_min_concurrency(0)); // set min success
        ASSERT_EQ(0, get_min_concurrency());
        int conn = flare::fiber_getconcurrency();
        int add_conn = 100;

        ASSERT_EQ(0, set_min_concurrency(conn + 1)); // set min failed
        ASSERT_EQ(0, get_min_concurrency());

        ASSERT_EQ(1, set_min_concurrency(conn - 1)); // set min success
        ASSERT_EQ(conn - 1, get_min_concurrency());

        ASSERT_EQ(EINVAL, flare::fiber_setconcurrency(conn - 2)); // set max failed
        ASSERT_EQ(0, flare::fiber_setconcurrency(conn + add_conn + 1)); // set max success
        ASSERT_EQ(0, flare::fiber_setconcurrency(conn + add_conn)); // set max success
        ASSERT_EQ(conn + add_conn, flare::fiber_getconcurrency());
        ASSERT_EQ(conn, flare::fiber_internal::g_task_control->concurrency());

        ASSERT_EQ(1, set_min_concurrency(conn + 1)); // set min success
        ASSERT_EQ(conn + 1, get_min_concurrency());
        ASSERT_EQ(conn + 1, flare::fiber_internal::g_task_control->concurrency());

        std::vector<fiber_id_t> tids;
        for (int i = 0; i < conn; ++i) {
            fiber_id_t tid;
            fiber_start_background(&tid, &FIBER_ATTR_SMALL, sleep_proc, nullptr);
            tids.push_back(tid);
        }
        for (int i = 0; i < add_conn; ++i) {
            fiber_id_t tid;
            fiber_start_background(&tid, &FIBER_ATTR_SMALL, add_concurrency_proc, nullptr);
            tids.push_back(tid);
        }
        for (size_t i = 0; i < tids.size(); ++i) {
            fiber_join(tids[i], nullptr);
        }
        ASSERT_EQ(conn + add_conn, flare::fiber_getconcurrency());
        ASSERT_EQ(conn + add_conn, flare::fiber_internal::g_task_control->concurrency());
    }

} // namespace
