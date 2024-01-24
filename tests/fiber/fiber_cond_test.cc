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

#include <cinttypes>
#include <map>
#include <atomic>
#include <mutex>
#include "turbo/times/time.h"
#include "turbo/fiber/fiber.h"
#include "turbo/fiber/fiber_cond.h"
#include "turbo/fiber/internal/stack.h"
#include "turbo/fiber/runtime.h"
#include "turbo/times/stop_watcher.h"

namespace turbo::fiber_internal {
    struct Arg {
        fiber_mutex_t m;
        fiber_cond_t c;
    };

    std::mutex wake_mutex;
    long signal_start_time = 0;
    std::vector<fiber_id_t> wake_tid;
    std::vector<long> wake_time;
    volatile bool stop = false;
    const long SIGNAL_INTERVAL_US = 10000;

    void *signaler(void *void_arg) {
        Arg *a = (Arg *) void_arg;
        signal_start_time = turbo::get_current_time_micros();
        while (!stop) {
            turbo::Fiber::sleep_for(turbo::Duration::microseconds(SIGNAL_INTERVAL_US));
            fiber_cond_signal(&a->c);
        }
        return nullptr;
    }

    void *waiter(void *void_arg) {
        Arg *a = (Arg *) void_arg;
        TURBO_UNUSED(fiber_mutex_lock(&a->m));
        while (!stop) {
            TURBO_UNUSED(fiber_cond_wait(&a->c, &a->m));

            std::unique_lock l(wake_mutex);
            wake_tid.push_back(Fiber::fiber_self());
            wake_time.push_back(turbo::get_current_time_micros());
        }
        fiber_mutex_unlock(&a->m);
        return nullptr;
    }

    TEST_CASE("CondTest, sanity") {
        Arg a;
        REQUIRE(fiber_mutex_init(&a.m, nullptr).ok());
        REQUIRE(fiber_cond_init(&a.c, nullptr).ok());
        // has no effect
        fiber_cond_signal(&a.c);

        stop = false;
        wake_tid.resize(1024);
        wake_tid.clear();
        wake_time.resize(1024);
        wake_time.clear();

        fiber_id_t wth[8];
        const size_t NW = TURBO_ARRAY_SIZE(wth);
        for (size_t i = 0; i < NW; ++i) {
            REQUIRE_EQ(turbo::ok_status(), fiber_start(&wth[i], nullptr, waiter, &a));
        }

        fiber_id_t sth;
        REQUIRE_EQ(turbo::ok_status(), fiber_start(&sth, nullptr, signaler, &a));

        turbo::Fiber::sleep_for(turbo::Duration::microseconds(SIGNAL_INTERVAL_US * 200));

        wake_mutex.lock();
        const size_t nbeforestop = wake_time.size();
        wake_mutex.unlock();

        stop = true;
        for (size_t i = 0; i < NW; ++i) {
            fiber_cond_signal(&a.c);
        }

        TURBO_UNUSED(fiber_join(sth, nullptr));
        for (size_t i = 0; i < NW; ++i) {
            TURBO_UNUSED(fiber_join(wth[i], nullptr));
        }

        printf("wake up for %lu times\n", wake_tid.size());

        // Check timing
        long square_sum = 0;
        for (size_t i = 0; i < nbeforestop; ++i) {
            long last_time = (i ? wake_time[i - 1] : signal_start_time);
            long delta = wake_time[i] - last_time - SIGNAL_INTERVAL_US;
            REQUIRE_GT(wake_time[i], last_time);
            square_sum += delta * delta;
            REQUIRE_LT(labs(delta), 10000L);// << "error[" << i << "]=" << delta << "="
                                            // << wake_time[i] << " - " << last_time;
        }
        printf("Average error is %fus\n", sqrt(square_sum / std::max(nbeforestop, 1UL)));

        // Check fairness
        std::map<fiber_id_t, int> count;
        for (size_t i = 0; i < wake_tid.size(); ++i) {
            ++count[wake_tid[i]];
        }
        REQUIRE_EQ(NW, count.size());
        int avg_count = (int) (wake_tid.size() / count.size());
        for (std::map<fiber_id_t, int>::iterator
                     it = count.begin(); it != count.end(); ++it) {
            REQUIRE_LE(abs(it->second - avg_count), 1);
            printf("%" PRId64 " wakes up %d times\n", it->first, it->second);
        }

        fiber_cond_destroy(&a.c);
        fiber_mutex_destroy(&a.m);
    }

    struct WrapperArg1 {
        WrapperArg1() {
            REQUIRE_EQ(turbo::ok_status(), fiber_mutex_init(&mutex, nullptr));
            REQUIRE_EQ(turbo::ok_status(), fiber_cond_init(&cond, nullptr));
        }

        ~WrapperArg1() {
            fiber_cond_destroy(&cond);
            fiber_mutex_destroy(&mutex);
        }
        turbo::fiber_internal::fiber_mutex_t mutex;
        turbo::fiber_internal::fiber_cond_t cond;
    };

    void *cv_signaler1(void *void_arg) {
        WrapperArg1 *a = (WrapperArg1 *) void_arg;
        signal_start_time = turbo::get_current_time_micros();
        while (!stop) {
            turbo::Fiber::sleep_for(turbo::Duration::microseconds(SIGNAL_INTERVAL_US));
            turbo::fiber_internal::fiber_cond_signal(&a->cond);
        }
        return nullptr;
    }

    void *cv_fmutex_waiter1(void *void_arg) {
        WrapperArg1 *a = (WrapperArg1 *) void_arg;
        std::unique_lock<fiber_mutex_t> lck(a->mutex);
        while (!stop) {
            TURBO_UNUSED(turbo::fiber_internal::fiber_cond_wait(&a->cond, lck.mutex()));
        }
        return nullptr;
    }

    void *cv_pmutex_waiter1(void *void_arg) {
        WrapperArg1 *a = (WrapperArg1 *) void_arg;
        std::unique_lock<fiber_mutex_t> lck(a->mutex);
        while (!stop) {
            TURBO_UNUSED(turbo::fiber_internal::fiber_cond_wait(&a->cond, lck.mutex()));
        }
        return nullptr;
    }


    TEST_CASE("CondTest, no_wrapper") {
        stop = false;
        pthread_t fmutex_waiter_threads[8];
        Fiber      pmutex_waiter_threads[8];
        pthread_t signal_thread;
        WrapperArg1 a;
        TLOG_INFO("start");
        for (size_t i = 0; i < TURBO_ARRAY_SIZE(fmutex_waiter_threads); ++i) {
            // TLOG_INFO("start {}", i);
            REQUIRE_EQ(0, pthread_create(&fmutex_waiter_threads[i], nullptr,
                                         cv_fmutex_waiter1, &a));
            REQUIRE(pmutex_waiter_threads[i].start(cv_fmutex_waiter1, &a).ok());
        }
        TLOG_INFO("start 2");
        REQUIRE_EQ(0, pthread_create(&signal_thread, nullptr, cv_signaler1, &a));
        turbo::Fiber::sleep_for(turbo::Duration::microseconds(100L * 1000));
        {
            std::unique_lock l(a.mutex);
            stop = true;
        }
        TLOG_INFO("stop");
        pthread_join(signal_thread, nullptr);
        turbo::fiber_internal::fiber_cond_broadcast(&a.cond);
        for (size_t i = 0; i < TURBO_ARRAY_SIZE(fmutex_waiter_threads); ++i) {
            pthread_join(fmutex_waiter_threads[i], nullptr);
            pmutex_waiter_threads[i].join();
        }
        TLOG_INFO("stop 2");
    }



    struct WrapperArg {
        turbo::FiberMutex mutex;
        turbo::FiberCond cond;
    };

    void *cv_signaler(void *void_arg) {
        WrapperArg *a = (WrapperArg *) void_arg;
        signal_start_time = turbo::get_current_time_micros();
        while (!stop) {
            turbo::Fiber::sleep_for(turbo::Duration::microseconds(SIGNAL_INTERVAL_US));
            a->cond.notify_one();
        }
        return nullptr;
    }

    void *cv_fmutex_waiter(void *void_arg) {
        WrapperArg *a = (WrapperArg *) void_arg;
        std::unique_lock<fiber_mutex_t> lck(*a->mutex.native_handler());
        while (!stop) {
            TURBO_UNUSED(a->cond.wait(lck));
        }
        return nullptr;
    }

    void *cv_mutex_waiter(void *void_arg) {
        WrapperArg *a = (WrapperArg *) void_arg;
        std::unique_lock<turbo::FiberMutex> lck(a->mutex);
        while (!stop) {
            TURBO_UNUSED(a->cond.wait(lck));
        }
        return nullptr;
    }


    TEST_CASE("CondTest, cpp_wrapper") {
        stop = false;
        fiber_id_t fmutex_waiter_threads[8];
        pthread_t mutex_waiter_threads[8];
        pthread_t signal_thread;
        WrapperArg a;
        TLOG_INFO("start");
        for (size_t i = 0; i < TURBO_ARRAY_SIZE(fmutex_waiter_threads); ++i) {
           // TLOG_INFO("start {}", i);
            REQUIRE(turbo::fiber_start(&fmutex_waiter_threads[i], nullptr,
                                        cv_fmutex_waiter, &a).ok());
            REQUIRE_EQ(0, pthread_create(&mutex_waiter_threads[i], nullptr,
                                        cv_mutex_waiter, &a));
        }
        TLOG_INFO("start 2");
        REQUIRE_EQ(0, pthread_create(&signal_thread, nullptr, cv_signaler, &a));
        turbo::Fiber::sleep_for(turbo::Duration::microseconds(100L * 1000));
        {
            std::unique_lock l(a.mutex);
            stop = true;
        }
        TLOG_INFO("stop");
        pthread_join(signal_thread, nullptr);
        a.cond.notify_all();
        for (size_t i = 0; i < TURBO_ARRAY_SIZE(fmutex_waiter_threads); ++i) {
            turbo::fiber_join(fmutex_waiter_threads[i], nullptr);
            pthread_join(mutex_waiter_threads[i], nullptr);
        }
        TLOG_INFO("stop 2");
    }


    class Signal {
    public:

        Signal() : _signal(0) {}

        void notify() {
            std::unique_lock l(_m);
            ++_signal;
            _c.notify_one();
        }

        int wait(int old_signal) {
            std::unique_lock<turbo::FiberMutex> lck(_m);
            while (_signal == old_signal) {
                TURBO_UNUSED(_c.wait(lck));
            }
            return _signal;
        }

    private:
        turbo::FiberMutex _m;
        turbo::FiberCond _c;
        int _signal;
    };

    struct PingPongArg {
        bool stopped;
        Signal sig1;
        Signal sig2;
        std::atomic<int> nthread;
        std::atomic<long> total_count;
    };

    void *ping_pong_thread(void *arg) {
        PingPongArg *a = (PingPongArg *) arg;
        long local_count = 0;
        bool odd = (a->nthread.fetch_add(1)) % 2;
        int old_signal = 0;
        while (!a->stopped) {
            if (odd) {
                a->sig1.notify();
                old_signal = a->sig2.wait(old_signal);
            } else {
                old_signal = a->sig1.wait(old_signal);
                a->sig2.notify();
            }
            ++local_count;
        }
        a->total_count.fetch_add(local_count);
        return nullptr;
    }

    TEST_CASE("CondTest, ping_pong") {
        PingPongArg arg;
        arg.stopped = false;
        arg.nthread = 0;
        fiber_id_t threads[2];
#ifdef ENABLE_PROFILE
        ProfilerStart("cond.prof");
#endif
        for (int i = 0; i < 2; ++i) {
            REQUIRE(fiber_start(&threads[i], nullptr, ping_pong_thread, &arg).ok());
        }
        usleep(1000 * 1000);
        arg.stopped = true;
        arg.sig1.notify();
        arg.sig2.notify();
        for (int i = 0; i < 2; ++i) {
            REQUIRE(fiber_join(threads[i], nullptr).ok());
        }
#ifdef ENABLE_PROFILE
        ProfilerStop();
#endif
        TLOG_INFO("total_count={}", arg.total_count.load());
    }

    struct BroadcastArg {
        turbo::FiberCond wait_cond;
        turbo::FiberCond broadcast_cond;
        turbo::FiberMutex mutex;
        int nwaiter;
        int cur_waiter;
        int rounds;
        int sig;
    };

    void *wait_thread(void *arg) {
        BroadcastArg *ba = (BroadcastArg *) arg;
        std::unique_lock<turbo::FiberMutex> lck(ba->mutex);
        while (ba->rounds > 0) {
            const int saved_round = ba->rounds;
            ++ba->cur_waiter;
            while (saved_round == ba->rounds) {
                if (ba->cur_waiter >= ba->nwaiter) {
                    ba->broadcast_cond.notify_one();
                }
                TURBO_UNUSED(ba->wait_cond.wait(lck));
            }
        }
        return nullptr;
    }

    void *broadcast_thread(void *arg) {
        BroadcastArg *ba = (BroadcastArg *) arg;
        //int local_round = 0;
        while (ba->rounds > 0) {
            std::unique_lock<turbo::FiberMutex> lck(ba->mutex);
            while (ba->cur_waiter < ba->nwaiter) {
                TURBO_UNUSED(ba->broadcast_cond.wait(lck));
            }
            ba->cur_waiter = 0;
            --ba->rounds;
            ba->wait_cond.notify_all();
        }
        return nullptr;
    }

    void *disturb_thread(void *arg) {
        BroadcastArg *ba = (BroadcastArg *) arg;
        std::unique_lock<turbo::FiberMutex> lck(ba->mutex);
        while (ba->rounds > 0) {
            lck.unlock();
            lck.lock();
        }
        return nullptr;
    }

    TEST_CASE("CondTest, mixed_usage") {
        BroadcastArg ba;
        ba.nwaiter = 0;
        ba.cur_waiter = 0;
        ba.rounds = 30000;
        const int NTHREADS = 10;
        ba.nwaiter = NTHREADS * 2;

        fiber_id_t normal_threads[NTHREADS];
        for (int i = 0; i < NTHREADS; ++i) {
            REQUIRE_EQ(turbo::ok_status(), fiber_start(&normal_threads[i], nullptr, wait_thread, &ba));
        }
        pthread_t pthreads[NTHREADS];
        for (int i = 0; i < NTHREADS; ++i) {
            REQUIRE_EQ(0, pthread_create(&pthreads[i], nullptr,
                                        wait_thread, &ba));
        }
        pthread_t broadcast;
        pthread_t disturb;
        REQUIRE_EQ(0, pthread_create(&broadcast, nullptr, broadcast_thread, &ba));
        REQUIRE_EQ(0, pthread_create(&disturb, nullptr, disturb_thread, &ba));
        for (int i = 0; i < NTHREADS; ++i) {
            TURBO_UNUSED(fiber_join(normal_threads[i], nullptr));
            pthread_join(pthreads[i], nullptr);
        }
        pthread_join(broadcast, nullptr);
        pthread_join(disturb, nullptr);
    }

    class FiberCond {
    public:
        FiberCond() {
            TURBO_UNUSED(fiber_cond_init(&_cond, nullptr));
            TURBO_UNUSED(fiber_mutex_init(&_mutex, nullptr));
            _count = 1;
        }

        ~FiberCond() {
            fiber_mutex_destroy(&_mutex);
            fiber_cond_destroy(&_cond);
        }

        void Init(int count = 1) {
            _count = count;
        }

        void Signal() {
            TURBO_UNUSED(fiber_mutex_lock(&_mutex));
            _count--;
            fiber_cond_signal(&_cond);
            fiber_mutex_unlock(&_mutex);
        }

        turbo::Status Wait() {
            turbo::Status ret;
            TURBO_UNUSED(fiber_mutex_lock(&_mutex));
            while (_count > 0) {
                ret = fiber_cond_wait(&_cond, &_mutex);
            }
            fiber_mutex_unlock(&_mutex);
            return ret;
        }

    private:
        int _count;
        fiber_cond_t _cond;
        fiber_mutex_t _mutex;
    };

    volatile bool g_stop = false;
    bool started_wait = false;
    bool ended_wait = false;

    void *usleep_thread(void *) {
        while (!g_stop) {
            turbo::Fiber::sleep_for(turbo::Duration::microseconds(1000L * 1000L));
        }
        return nullptr;
    }

    void *wait_cond_thread(void *arg) {
        FiberCond *c = (FiberCond *) arg;
        started_wait = true;
        TURBO_UNUSED(c->Wait());
        ended_wait = true;
        return nullptr;
    }

    static void launch_many_fibers() {
        g_stop = false;
        fiber_id_t tid;
        FiberCond c;
        c.Init();
        turbo::StopWatcher tm;
        TLOG_INFO("workers {}", turbo::fiber_get_concurrency());
        TURBO_UNUSED(fiber_start(&tid, &FIBER_ATTR_PTHREAD, wait_cond_thread, &c));
        std::vector<fiber_id_t> tids;
        tids.reserve(32768);
        tm.reset();
        for (size_t i = 0; i < 32768; ++i) {
            fiber_id_t t0;
            REQUIRE_EQ(turbo::ok_status(), fiber_start_background(&t0, nullptr, usleep_thread, nullptr));
            tids.push_back(t0);
        }
        tm.stop();
        TLOG_INFO("Creating fibers took {} us", tm.elapsed_micro());
        usleep(3 * 1000 * 1000L);
        c.Signal();
        g_stop = true;
        TURBO_UNUSED(fiber_join(tid, nullptr));
        for (size_t i = 0; i < tids.size(); ++i) {
            TLOG_INFO_EVERY_SEC("Joined {} threads", i);
            TURBO_UNUSED(fiber_join(tids[i], nullptr));
        }
        TLOG_INFO_EVERY_SEC("Joined {} threads", tids.size());
    }

    TEST_CASE("CondTest, too_many_fibers_from_pthread") {
        launch_many_fibers();
    }

    static void *run_launch_many_fibers(void *) {
        launch_many_fibers();
        return nullptr;
    }

    TEST_CASE("CondTest, too_many_fibers_from_fiber") {
        fiber_id_t th;
        REQUIRE_EQ(turbo::ok_status(), fiber_start(&th, nullptr, run_launch_many_fibers, nullptr));
        TURBO_UNUSED(fiber_join(th, nullptr));
    }
}  // namespace  turbo::fiber_internal
