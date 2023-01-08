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

#include <inttypes.h>
#include <map>
#include "testing/gtest_wrap.h"
#include "flare/base/static_atomic.h"
#include "flare/times/time.h"
#include "flare/base/scoped_lock.h"
#include "flare/base/gperftools_profiler.h"
#include "flare/fiber/internal/fiber.h"
#include "flare/fiber/fiber_cond.h"
#include "flare/fiber/internal/stack.h"
#include "flare/fiber/this_fiber.h"

namespace {
    struct Arg {
        fiber_mutex_t m;
        fiber_cond_t c;
    };

    pthread_mutex_t wake_mutex = PTHREAD_MUTEX_INITIALIZER;
    long signal_start_time = 0;
    std::vector<fiber_id_t> wake_tid;
    std::vector<long> wake_time;
    volatile bool stop = false;
    const long SIGNAL_INTERVAL_US = 10000;

    void *signaler(void *void_arg) {
        Arg *a = (Arg *) void_arg;
        signal_start_time = flare::get_current_time_micros();
        while (!stop) {
            flare::fiber_sleep_for(SIGNAL_INTERVAL_US);
            fiber_cond_signal(&a->c);
        }
        return nullptr;
    }

    void *waiter(void *void_arg) {
        Arg *a = (Arg *) void_arg;
        fiber_mutex_lock(&a->m);
        while (!stop) {
            fiber_cond_wait(&a->c, &a->m);

            FLARE_SCOPED_LOCK(wake_mutex);
            wake_tid.push_back(fiber_self());
            wake_time.push_back(flare::get_current_time_micros());
        }
        fiber_mutex_unlock(&a->m);
        return nullptr;
    }

    TEST(CondTest, sanity) {
        Arg a;
        ASSERT_EQ(0, fiber_mutex_init(&a.m, nullptr));
        ASSERT_EQ(0, fiber_cond_init(&a.c, nullptr));
        // has no effect
        ASSERT_EQ(0, fiber_cond_signal(&a.c));

        stop = false;
        wake_tid.resize(1024);
        wake_tid.clear();
        wake_time.resize(1024);
        wake_time.clear();

        fiber_id_t wth[8];
        const size_t NW = FLARE_ARRAY_SIZE(wth);
        for (size_t i = 0; i < NW; ++i) {
            ASSERT_EQ(0, fiber_start_urgent(&wth[i], nullptr, waiter, &a));
        }

        fiber_id_t sth;
        ASSERT_EQ(0, fiber_start_urgent(&sth, nullptr, signaler, &a));

        flare::fiber_sleep_for(SIGNAL_INTERVAL_US * 200);

        pthread_mutex_lock(&wake_mutex);
        const size_t nbeforestop = wake_time.size();
        pthread_mutex_unlock(&wake_mutex);

        stop = true;
        for (size_t i = 0; i < NW; ++i) {
            fiber_cond_signal(&a.c);
        }

        fiber_join(sth, nullptr);
        for (size_t i = 0; i < NW; ++i) {
            fiber_join(wth[i], nullptr);
        }

        printf("wake up for %lu times\n", wake_tid.size());

        // Check timing
        long square_sum = 0;
        for (size_t i = 0; i < nbeforestop; ++i) {
            long last_time = (i ? wake_time[i - 1] : signal_start_time);
            long delta = wake_time[i] - last_time - SIGNAL_INTERVAL_US;
            EXPECT_GT(wake_time[i], last_time);
            square_sum += delta * delta;
            EXPECT_LT(labs(delta), 10000L) << "error[" << i << "]=" << delta << "="
                                           << wake_time[i] << " - " << last_time;
        }
        printf("Average error is %fus\n", sqrt(square_sum / std::max(nbeforestop, 1UL)));

        // Check fairness
        std::map<fiber_id_t, int> count;
        for (size_t i = 0; i < wake_tid.size(); ++i) {
            ++count[wake_tid[i]];
        }
        EXPECT_EQ(NW, count.size());
        int avg_count = (int) (wake_tid.size() / count.size());
        for (std::map<fiber_id_t, int>::iterator
                     it = count.begin(); it != count.end(); ++it) {
            ASSERT_LE(abs(it->second - avg_count), 1)
                                        << "fiber=" << it->first
                                        << " count=" << it->second
                                        << " avg=" << avg_count;
            printf("%" PRId64 " wakes up %d times\n", it->first, it->second);
        }

        fiber_cond_destroy(&a.c);
        fiber_mutex_destroy(&a.m);
    }

    struct WrapperArg {
        flare::fiber_mutex mutex;
        flare::fiber_cond cond;
    };

    void *cv_signaler(void *void_arg) {
        WrapperArg *a = (WrapperArg *) void_arg;
        signal_start_time = flare::get_current_time_micros();
        while (!stop) {
            flare::fiber_sleep_for(SIGNAL_INTERVAL_US);
            a->cond.notify_one();
        }
        return nullptr;
    }

    void *cv_bmutex_waiter(void *void_arg) {
        WrapperArg *a = (WrapperArg *) void_arg;
        std::unique_lock<fiber_mutex_t> lck(*a->mutex.native_handler());
        while (!stop) {
            a->cond.wait(lck);
        }
        return nullptr;
    }

    void *cv_mutex_waiter(void *void_arg) {
        WrapperArg *a = (WrapperArg *) void_arg;
        std::unique_lock<flare::fiber_mutex> lck(a->mutex);
        while (!stop) {
            a->cond.wait(lck);
        }
        return nullptr;
    }

#define COND_IN_PTHREAD

#ifndef COND_IN_PTHREAD
#define pthread_join fiber_join
#define pthread_create fiber_start_urgent
#endif

    TEST(CondTest, cpp_wrapper) {
        stop = false;
        flare::fiber_cond cond;
        pthread_t bmutex_waiter_threads[8];
        pthread_t mutex_waiter_threads[8];
        pthread_t signal_thread;
        WrapperArg a;
        for (size_t i = 0; i < FLARE_ARRAY_SIZE(bmutex_waiter_threads); ++i) {
            ASSERT_EQ(0, pthread_create(&bmutex_waiter_threads[i], nullptr,
                                        cv_bmutex_waiter, &a));
            ASSERT_EQ(0, pthread_create(&mutex_waiter_threads[i], nullptr,
                                        cv_mutex_waiter, &a));
        }
        ASSERT_EQ(0, pthread_create(&signal_thread, nullptr, cv_signaler, &a));
        flare::fiber_sleep_for(100L * 1000);
        {
            FLARE_SCOPED_LOCK(a.mutex);
            stop = true;
        }
        pthread_join(signal_thread, nullptr);
        a.cond.notify_all();
        for (size_t i = 0; i < FLARE_ARRAY_SIZE(bmutex_waiter_threads); ++i) {
            pthread_join(bmutex_waiter_threads[i], nullptr);
            pthread_join(mutex_waiter_threads[i], nullptr);
        }
    }

#ifndef COND_IN_PTHREAD
#undef pthread_join
#undef pthread_create
#endif

    class Signal {
    protected:

        Signal() : _signal(0) {}

        void notify() {
            FLARE_SCOPED_LOCK(_m);
            ++_signal;
            _c.notify_one();
        }

        int wait(int old_signal) {
            std::unique_lock<flare::fiber_mutex> lck(_m);
            while (_signal == old_signal) {
                _c.wait(lck);
            }
            return _signal;
        }

    private:
        flare::fiber_mutex _m;
        flare::fiber_cond _c;
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

    TEST(CondTest, ping_pong) {
        PingPongArg arg;
        arg.stopped = false;
        arg.nthread = 0;
        fiber_id_t threads[2];
        ProfilerStart("cond.prof");
        for (int i = 0; i < 2; ++i) {
            ASSERT_EQ(0, fiber_start_urgent(&threads[i], nullptr, ping_pong_thread, &arg));
        }
        usleep(1000 * 1000);
        arg.stopped = true;
        arg.sig1.notify();
        arg.sig2.notify();
        for (int i = 0; i < 2; ++i) {
            ASSERT_EQ(0, fiber_join(threads[i], nullptr));
        }
        ProfilerStop();
        FLARE_LOG(INFO) << "total_count=" << arg.total_count.load();
    }

    struct BroadcastArg {
        flare::fiber_cond wait_cond;
        flare::fiber_cond broadcast_cond;
        flare::fiber_mutex mutex;
        int nwaiter;
        int cur_waiter;
        int rounds;
        int sig;
    };

    void *wait_thread(void *arg) {
        BroadcastArg *ba = (BroadcastArg *) arg;
        std::unique_lock<flare::fiber_mutex> lck(ba->mutex);
        while (ba->rounds > 0) {
            const int saved_round = ba->rounds;
            ++ba->cur_waiter;
            while (saved_round == ba->rounds) {
                if (ba->cur_waiter >= ba->nwaiter) {
                    ba->broadcast_cond.notify_one();
                }
                ba->wait_cond.wait(lck);
            }
        }
        return nullptr;
    }

    void *broadcast_thread(void *arg) {
        BroadcastArg *ba = (BroadcastArg *) arg;
        //int local_round = 0;
        while (ba->rounds > 0) {
            std::unique_lock<flare::fiber_mutex> lck(ba->mutex);
            while (ba->cur_waiter < ba->nwaiter) {
                ba->broadcast_cond.wait(lck);
            }
            ba->cur_waiter = 0;
            --ba->rounds;
            ba->wait_cond.notify_all();
        }
        return nullptr;
    }

    void *disturb_thread(void *arg) {
        BroadcastArg *ba = (BroadcastArg *) arg;
        std::unique_lock<flare::fiber_mutex> lck(ba->mutex);
        while (ba->rounds > 0) {
            lck.unlock();
            lck.lock();
        }
        return nullptr;
    }

    TEST(CondTest, mixed_usage) {
        BroadcastArg ba;
        ba.nwaiter = 0;
        ba.cur_waiter = 0;
        ba.rounds = 30000;
        const int NTHREADS = 10;
        ba.nwaiter = NTHREADS * 2;

        fiber_id_t normal_threads[NTHREADS];
        for (int i = 0; i < NTHREADS; ++i) {
            ASSERT_EQ(0, fiber_start_urgent(&normal_threads[i], nullptr, wait_thread, &ba));
        }
        pthread_t pthreads[NTHREADS];
        for (int i = 0; i < NTHREADS; ++i) {
            ASSERT_EQ(0, pthread_create(&pthreads[i], nullptr,
                                        wait_thread, &ba));
        }
        pthread_t broadcast;
        pthread_t disturb;
        ASSERT_EQ(0, pthread_create(&broadcast, nullptr, broadcast_thread, &ba));
        ASSERT_EQ(0, pthread_create(&disturb, nullptr, disturb_thread, &ba));
        for (int i = 0; i < NTHREADS; ++i) {
            fiber_join(normal_threads[i], nullptr);
            pthread_join(pthreads[i], nullptr);
        }
        pthread_join(broadcast, nullptr);
        pthread_join(disturb, nullptr);
    }

    class FiberCond {
    public:
        FiberCond() {
            fiber_cond_init(&_cond, nullptr);
            fiber_mutex_init(&_mutex, nullptr);
            _count = 1;
        }

        ~FiberCond() {
            fiber_mutex_destroy(&_mutex);
            fiber_cond_destroy(&_cond);
        }

        void Init(int count = 1) {
            _count = count;
        }

        int Signal() {
            int ret = 0;
            fiber_mutex_lock(&_mutex);
            _count--;
            fiber_cond_signal(&_cond);
            fiber_mutex_unlock(&_mutex);
            return ret;
        }

        int Wait() {
            int ret = 0;
            fiber_mutex_lock(&_mutex);
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
            flare::fiber_sleep_for(1000L * 1000L);
        }
        return nullptr;
    }

    void *wait_cond_thread(void *arg) {
        FiberCond *c = (FiberCond *) arg;
        started_wait = true;
        c->Wait();
        ended_wait = true;
        return nullptr;
    }

    static void launch_many_fibers() {
        g_stop = false;
        fiber_id_t tid;
        FiberCond c;
        c.Init();
        flare::stop_watcher tm;
        fiber_start_urgent(&tid, &FIBER_ATTR_PTHREAD, wait_cond_thread, &c);
        std::vector<fiber_id_t> tids;
        tids.reserve(32768);
        tm.start();
        for (size_t i = 0; i < 32768; ++i) {
            fiber_id_t t0;
            ASSERT_EQ(0, fiber_start_background(&t0, nullptr, usleep_thread, nullptr));
            tids.push_back(t0);
        }
        tm.stop();
        FLARE_LOG(INFO) << "Creating fibers took " << tm.u_elapsed() << " us";
        usleep(3 * 1000 * 1000L);
        c.Signal();
        g_stop = true;
        fiber_join(tid, nullptr);
        for (size_t i = 0; i < tids.size(); ++i) {
            FLARE_LOG_EVERY_SECOND(INFO) << "Joined " << i << " threads";
            fiber_join(tids[i], nullptr);
        }
        FLARE_LOG_EVERY_SECOND(INFO) << "Joined " << tids.size() << " threads";
    }

    TEST(CondTest, too_many_fibers_from_pthread) {
        launch_many_fibers();
    }

    static void *run_launch_many_fibers(void *) {
        launch_many_fibers();
        return nullptr;
    }

    TEST(CondTest, too_many_fibers_from_fiber) {
        fiber_id_t th;
        ASSERT_EQ(0, fiber_start_urgent(&th, nullptr, run_launch_many_fibers, nullptr));
        fiber_join(th, nullptr);
    }
} // namespace
