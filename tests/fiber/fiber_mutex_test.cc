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

#include <inttypes.h>
#include "turbo/times/clock.h"
#include "turbo/log/logging.h"
#include "turbo/fiber/internal/fiber.h"
#include "turbo/fiber/internal/waitable_event.h"
#include "turbo/fiber/internal/schedule_group.h"
#include "turbo/fiber/fiber_mutex.h"
#include "turbo/fiber/fiber_cond.h"
#include "turbo/fiber/fiber.h"
#include "turbo/system/sysinfo.h"
#include "turbo/format/print.h"
#include "turbo/times/stop_watcher.h"

namespace turbo::fiber_internal {
    inline unsigned *get_futex(fiber_mutex_t &m) {
        return m.event;
    }

    turbo::Time start_time = turbo::time_now();
    int c = 0;

    void *locker(void *arg) {
        fiber_mutex_t *m = (fiber_mutex_t *) arg;
        TURBO_UNUSED(fiber_mutex_lock(m));
        turbo::println("{} I'm here, {}, {}ms",
                       turbo::thread_numeric_id(), ++c, (turbo::time_now() - start_time).to_milliseconds());
        turbo::fiber_sleep_for(turbo::microseconds(10000));
        fiber_mutex_unlock(m);
        return nullptr;
    }

    TEST_CASE("MutexTest, sanity") {
        fiber_mutex_t m;
        REQUIRE(fiber_mutex_init(&m, nullptr).ok());
        REQUIRE_EQ(0u, *get_futex(m));
        REQUIRE(fiber_mutex_lock(&m).ok());
        REQUIRE_EQ(1u, *get_futex(m));
        fiber_id_t th1;
        REQUIRE_EQ(turbo::ok_status(), fiber_start_urgent(&th1, nullptr, locker, &m));
        TLOG_INFO("1");
        usleep(5000); // wait for locker to run.
        REQUIRE_EQ(257u, *get_futex(m)); // contention
        fiber_mutex_unlock(&m);
        TLOG_INFO("2");
        REQUIRE(fiber_join(th1, nullptr).ok());
        TLOG_INFO("3");
        REQUIRE_EQ(0u, *get_futex(m));
        fiber_mutex_destroy(&m);
    }

    TEST_CASE("MutexTest, used_in_pthread") {
        fiber_mutex_t m;
        REQUIRE(fiber_mutex_init(&m, nullptr).ok());
        pthread_t th[8];
        for (size_t i = 0; i < TURBO_ARRAY_SIZE(th); ++i) {
            REQUIRE_EQ(0, pthread_create(&th[i], nullptr, locker, &m));
        }
        for (size_t i = 0; i < TURBO_ARRAY_SIZE(th); ++i) {
            pthread_join(th[i], nullptr);
        }
        REQUIRE_EQ(0u, *get_futex(m));
        fiber_mutex_destroy(&m);
    }

    void *do_locks(void *arg) {
        struct timespec t = {-2, 0};
        REQUIRE(turbo::is_deadline_exceeded(fiber_mutex_timedlock((fiber_mutex_t *) arg, &t)));
        return nullptr;
    }

    TEST_CASE("MutexTest, timedlock") {
        fiber_cond_t cond;
        fiber_mutex_t m1;
        fiber_mutex_t m2;
        REQUIRE(fiber_cond_init(&cond, nullptr).ok());
        REQUIRE(fiber_mutex_init(&m1, nullptr).ok());
        REQUIRE(fiber_mutex_init(&m2, nullptr).ok());

        struct timespec t = {-2, 0};

        TURBO_UNUSED(fiber_mutex_lock(&m1));
        TURBO_UNUSED(fiber_mutex_lock(&m2));
        fiber_id_t pth;
        REQUIRE(fiber_start_urgent(&pth, nullptr, do_locks, &m1).ok());
        REQUIRE(turbo::is_deadline_exceeded(fiber_cond_timedwait(&cond, &m2, &t)));
        REQUIRE(fiber_join(pth, nullptr).ok());
        fiber_mutex_unlock(&m1);
        fiber_mutex_unlock(&m2);
        fiber_mutex_destroy(&m1);
        fiber_mutex_destroy(&m2);
    }

    TEST_CASE("MutexTest, cpp_wrapper") {
        turbo::FiberMutex mutex;
        REQUIRE(mutex.try_lock());
        mutex.unlock();
        mutex.lock();
        mutex.unlock();
        {
            std::unique_lock<turbo::FiberMutex> lck(mutex);
        }
        {
            std::unique_lock<turbo::FiberMutex> lck1;
            std::unique_lock<turbo::FiberMutex> lck2(mutex);
            lck1.swap(lck2);
            lck1.unlock();
            lck1.lock();
        }
        REQUIRE(mutex.try_lock());
        mutex.unlock();
        {
            std::unique_lock<fiber_mutex_t> lck(*mutex.native_handler());
        }
        {
            std::unique_lock<fiber_mutex_t> lck1;
            std::unique_lock<fiber_mutex_t> lck2(*mutex.native_handler());
            lck1.swap(lck2);
            lck1.unlock();
            lck1.lock();
        }
        REQUIRE(mutex.try_lock());
        mutex.unlock();
    }

    bool g_started = false;
    bool g_stopped = false;

    template<typename FiberMutex>
    struct TURBO_CACHE_LINE_ALIGNED PerfArgs {
        FiberMutex *mutex;
        int64_t counter;
        int64_t elapse_ns;
        bool ready;

        PerfArgs() : mutex(nullptr), counter(0), elapse_ns(0), ready(false) {}
    };

    template<typename FiberMutex>
    void *add_with_mutex(void *void_arg) {
        PerfArgs<FiberMutex> *args = (PerfArgs<FiberMutex> *) void_arg;
        args->ready = true;
        turbo::StopWatcher t;
        while (!g_stopped) {
            if (g_started) {
                break;
            }
            turbo::fiber_sleep_for(turbo::microseconds(1000));
        }
        t.reset();
        while (!g_stopped) {
            std::unique_lock l(*args->mutex);
            ++args->counter;
        }
        t.stop();
        args->elapse_ns = t.elapsed_nano();
        return nullptr;
    }

    int g_prof_name_counter = 0;

    template<typename FiberMutex, typename ThreadId,
            typename ThreadCreateFn, typename ThreadJoinFn>
    void PerfTest(FiberMutex *mutex,
                  ThreadId * ,
                  int thread_num,
                  const ThreadCreateFn &create_fn,
                  const ThreadJoinFn &join_fn) {
        g_started = false;
        g_stopped = false;
        ThreadId threads[thread_num];
        std::vector<PerfArgs<FiberMutex> > args(thread_num);
        for (int i = 0; i < thread_num; ++i) {
            args[i].mutex = mutex;
            TURBO_UNUSED(create_fn(&threads[i], nullptr, add_with_mutex<FiberMutex>, &args[i]));
        }
        while (true) {
            bool all_ready = true;
            for (int i = 0; i < thread_num; ++i) {
                if (!args[i].ready) {
                    all_ready = false;
                    break;
                }
            }
            if (all_ready) {
                break;
            }
            usleep(1000);
        }
        g_started = true;
        char prof_name[32];
        snprintf(prof_name, sizeof(prof_name), "mutex_perf_%d.prof", ++g_prof_name_counter);
#ifdef ENABLE_PROFILE
        ProfilerStart(prof_name);
#endif
        usleep(500 * 1000);
#ifdef ENABLE_PROFILE
        ProfilerStop();
#endif
        g_stopped = true;
        int64_t wait_time = 0;
        int64_t count = 0;
        for (int i = 0; i < thread_num; ++i) {
            TURBO_UNUSED(join_fn(threads[i], nullptr));
            wait_time += args[i].elapse_ns;
            count += args[i].counter;
        }
    }


    TEST_CASE("MutexTest, performance") {
        const int thread_num = 12;
        std::mutex base_mutex;
        TLOG_INFO("perf test pthread");
        PerfTest(&base_mutex, (pthread_t *) nullptr, thread_num, pthread_create, pthread_join);
        TLOG_INFO("perf test fiber");
        PerfTest(&base_mutex, (fiber_id_t *) nullptr, thread_num, fiber_start_background, fiber_join);
        turbo::FiberMutex fbr_mutex;
        TLOG_INFO("perf test pthread--------------------------------------------------------------------------");
        PerfTest(&fbr_mutex, (pthread_t *) nullptr, thread_num, pthread_create, pthread_join);
        TLOG_INFO("perf test fiber");
        PerfTest(&fbr_mutex, (fiber_id_t *) nullptr, thread_num, fiber_start_background, fiber_join);
    }

    void *loop_until_stopped(void *arg) {
        turbo::FiberMutex *m = (turbo::FiberMutex *) arg;
        while (!g_stopped) {
            std::unique_lock l(*m);
            turbo::fiber_sleep_for(turbo::microseconds(20));
        }
        return nullptr;
    }

    TEST_CASE("MutexTest, mix_thread_types") {
        g_stopped = false;
        const int N = 16;
        const int M = N * 2;
        turbo::FiberMutex m;
        pthread_t pthreads[N];
        fiber_id_t fibers[M];
        // reserve enough workers for test. This is a must since we have
        // FIBER_ATTR_PTHREAD fibers which may cause deadlocks (the
        // fiber_usleep below can't be scheduled and g_stopped is never
        // true, thus loop_until_stopped spins forever)
        TURBO_UNUSED(fiber_set_concurrency(M));
        for (int i = 0; i < N; ++i) {
            REQUIRE_EQ(0, pthread_create(&pthreads[i], nullptr, loop_until_stopped, &m));
        }
        for (int i = 0; i < M; ++i) {
            const FiberAttribute *attr = i % 2 ? nullptr : &FIBER_ATTR_PTHREAD;
            REQUIRE_EQ(turbo::ok_status(), fiber_start_urgent(&fibers[i], attr, loop_until_stopped, &m));
        }
        turbo::fiber_sleep_for(turbo::microseconds(1000L * 1000));
        g_stopped = true;
        for (int i = 0; i < M; ++i) {
            TURBO_UNUSED(fiber_join(fibers[i], nullptr));
        }
        for (int i = 0; i < N; ++i) {
            pthread_join(pthreads[i], nullptr);
        }
    }
} // namespace turbo::fiber_internal
