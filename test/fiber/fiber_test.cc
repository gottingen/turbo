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

#include <execinfo.h>
#include "testing/gtest_wrap.h"
#include "flare/times/time.h"
#include "flare/log/logging.h"
#include "flare/base/gperftools_profiler.h"
#include "flare/fiber/internal/fiber.h"
#include "flare/fiber/internal/unstable.h"
#include "flare/fiber/internal/fiber_entity.h"
#include "flare/fiber/this_fiber.h"

namespace {
    class FiberTest : public ::testing::Test {
    protected:

        FiberTest() {
            const int kNumCores = sysconf(_SC_NPROCESSORS_ONLN);
            if (kNumCores > 0) {
                fiber_setconcurrency(kNumCores);
            }
        };

        virtual ~FiberTest() {};

        virtual void SetUp() {
        };

        virtual void TearDown() {
        };
    };

    TEST_F(FiberTest, sizeof_task_meta) {
        FLARE_LOG(INFO) << "sizeof(fiber_entity)=" << sizeof(flare::fiber_internal::fiber_entity);
    }

    void *unrelated_pthread(void *) {
        FLARE_LOG(INFO) << "I did not call any fiber function, "
                     "I should begin and end without any problem";
        return (void *) (intptr_t) 1;
    }

    TEST_F(FiberTest, unrelated_pthread) {
        pthread_t th;
        ASSERT_EQ(0, pthread_create(&th, nullptr, unrelated_pthread, nullptr));
        void *ret = nullptr;
        ASSERT_EQ(0, pthread_join(th, &ret));
        ASSERT_EQ(1, (intptr_t) ret);
    }

    TEST_F(FiberTest, attr_init_and_destroy) {
        fiber_attribute attr;
        ASSERT_EQ(0, fiber_attr_init(&attr));
        ASSERT_EQ(0, fiber_attr_destroy(&attr));
    }

    fiber_context_type fcm;
    fiber_context_type fc;
    typedef std::pair<int, int> pair_t;

    static void f(intptr_t param) {
        pair_t *p = (pair_t *) param;
        p = (pair_t *) flare_fiber_jump_context(&fc, fcm, (intptr_t) (p->first + p->second));
        flare_fiber_jump_context(&fc, fcm, (intptr_t) (p->first + p->second));
    }

    TEST_F(FiberTest, context_sanity) {
        fcm = nullptr;
        std::size_t size(8192);
        void *sp = malloc(size);

        pair_t p(std::make_pair(2, 7));
        fc = flare_fiber_make_context((char *) sp + size, size, f);

        int res = (int) flare_fiber_jump_context(&fcm, fc, (intptr_t) &p);
        std::cout << p.first << " + " << p.second << " == " << res << std::endl;

        p = std::make_pair(5, 6);
        res = (int) flare_fiber_jump_context(&fcm, fc, (intptr_t) &p);
        std::cout << p.first << " + " << p.second << " == " << res << std::endl;
    }

    TEST_F(FiberTest, call_fiber_functions_before_tls_created) {
        ASSERT_EQ(0, flare::fiber_sleep_for(1000));
        ASSERT_EQ(EINVAL, fiber_join(0, nullptr));
        ASSERT_EQ(0UL, fiber_self());
    }

    std::atomic<bool> stop(false);

    void *sleep_for_awhile(void *arg) {
        FLARE_LOG(INFO) << "sleep_for_awhile(" << arg << ")";
        flare::fiber_sleep_for(100000L);
        FLARE_LOG(INFO) << "sleep_for_awhile(" << arg << ") wakes up";
        return nullptr;
    }

    void *just_exit(void *arg) {
        FLARE_LOG(INFO) << "just_exit(" << arg << ")";
        fiber_exit(nullptr);
        EXPECT_TRUE(false) << "just_exit(" << arg << ") should never be here";
        return nullptr;
    }

    void *repeated_sleep(void *arg) {
        for (size_t i = 0; !stop; ++i) {
            FLARE_LOG(INFO) << "repeated_sleep(" << arg << ") i=" << i;
            flare::fiber_sleep_for(1000000L);
        }
        return nullptr;
    }

    void *spin_and_log(void *arg) {
        // This thread never yields CPU.
        flare::every_duration every_1s(flare::duration::seconds(1));
        size_t i = 0;
        while (!stop) {
            if (every_1s) {
                FLARE_LOG(INFO) << "spin_and_log(" << arg << ")=" << i++;
            }
        }
        return nullptr;
    }

    void *do_nothing(void *arg) {
        FLARE_LOG(INFO) << "do_nothing(" << arg << ")";
        return nullptr;
    }

    void *launcher(void *arg) {
        FLARE_LOG(INFO) << "launcher(" << arg << ")";
        for (size_t i = 0; !stop; ++i) {
            fiber_id_t th;
            fiber_start_urgent(&th, nullptr, do_nothing, (void *) i);
            flare::fiber_sleep_for(1000000L);
        }
        return nullptr;
    }

    void *stopper(void *) {
        // Need this thread to set `stop' to true. Reason: If spin_and_log (which
        // never yields CPU) is scheduled to main thread, main thread cannot get
        // to run again.
        flare::fiber_sleep_for(5 * 1000000L);
        FLARE_LOG(INFO) << "about to stop";
        stop = true;
        return nullptr;
    }

    void *misc(void *arg) {
        FLARE_LOG(INFO) << "misc(" << arg << ")";
        fiber_id_t th[8];
        EXPECT_EQ(0, fiber_start_urgent(&th[0], nullptr, sleep_for_awhile, (void *) 2));
        EXPECT_EQ(0, fiber_start_urgent(&th[1], nullptr, just_exit, (void *) 3));
        EXPECT_EQ(0, fiber_start_urgent(&th[2], nullptr, repeated_sleep, (void *) 4));
        EXPECT_EQ(0, fiber_start_urgent(&th[3], nullptr, repeated_sleep, (void *) 68));
        EXPECT_EQ(0, fiber_start_urgent(&th[4], nullptr, spin_and_log, (void *) 5));
        EXPECT_EQ(0, fiber_start_urgent(&th[5], nullptr, spin_and_log, (void *) 85));
        EXPECT_EQ(0, fiber_start_urgent(&th[6], nullptr, launcher, (void *) 6));
        EXPECT_EQ(0, fiber_start_urgent(&th[7], nullptr, stopper, nullptr));
        for (size_t i = 0; i < FLARE_ARRAY_SIZE(th); ++i) {
            EXPECT_EQ(0, fiber_join(th[i], nullptr));
        }
        return nullptr;
    }

    TEST_F(FiberTest, sanity) {
        FLARE_LOG(INFO) << "main thread " << pthread_self();
        fiber_id_t th1;
        ASSERT_EQ(0, fiber_start_urgent(&th1, nullptr, misc, (void *) 1));
        FLARE_LOG(INFO) << "back to main thread " << th1 << " " << pthread_self();
        ASSERT_EQ(0, fiber_join(th1, nullptr));
    }

    const size_t BT_SIZE = 64;
    void *bt_array[BT_SIZE];
    int bt_cnt;

    int do_bt(void) {
        bt_cnt = backtrace(bt_array, BT_SIZE);
        return 56;
    }

    int call_do_bt(void) {
        return do_bt() + 1;
    }

    void *tf(void *) {
        if (call_do_bt() != 57) {
            return (void *) 1L;
        }
        return nullptr;
    }

    TEST_F(FiberTest, backtrace) {
        fiber_id_t th;
        ASSERT_EQ(0, fiber_start_urgent(&th, nullptr, tf, nullptr));
        ASSERT_EQ(0, fiber_join(th, nullptr));

        char **text = backtrace_symbols(bt_array, bt_cnt);
        ASSERT_TRUE(text);
        for (int i = 0; i < bt_cnt; ++i) {
            puts(text[i]);
        }
    }

    TEST_F(FiberTest, lambda_backtrace) {
        fiber_id_t th;
        ASSERT_EQ(0, fiber_start_urgent(&th, nullptr, [](void*)->void*{
            if (call_do_bt() != 57) {
                return (void *) 1L;
            }
            return nullptr;
            }, nullptr));
        ASSERT_EQ(0, fiber_join(th, nullptr));

        char **text = backtrace_symbols(bt_array, bt_cnt);
        ASSERT_TRUE(text);
        for (int i = 0; i < bt_cnt; ++i) {
            puts(text[i]);
        }
    }

    void *show_self(void *) {
        EXPECT_NE(0ul, fiber_self());
        FLARE_LOG(INFO) << "fiber_self=" << fiber_self();
        return nullptr;
    }

    TEST_F(FiberTest, fiber_self) {
        ASSERT_EQ(0ul, fiber_self());
        fiber_id_t bth;
        ASSERT_EQ(0, fiber_start_urgent(&bth, nullptr, show_self, nullptr));
        ASSERT_EQ(0, fiber_join(bth, nullptr));
    }

    void *join_self(void *) {
        EXPECT_EQ(EINVAL, fiber_join(fiber_self(), nullptr));
        return nullptr;
    }

    TEST_F(FiberTest, fiber_join) {
        // Invalid tid
        ASSERT_EQ(EINVAL, fiber_join(0, nullptr));

        // Unexisting tid
        ASSERT_EQ(EINVAL, fiber_join((fiber_id_t) -1, nullptr));

        // Joining self
        fiber_id_t th;
        ASSERT_EQ(0, fiber_start_urgent(&th, nullptr, join_self, nullptr));
    }

    void *change_errno(void *arg) {
        errno = (intptr_t) arg;
        return nullptr;
    }

    TEST_F(FiberTest, errno_not_changed) {
        fiber_id_t th;
        errno = 1;
        fiber_start_urgent(&th, nullptr, change_errno, (void *) (intptr_t) 2);
        ASSERT_EQ(1, errno);
    }

    static long sleep_in_adding_func = 0;

    void *adding_func(void *arg) {
        std::atomic<size_t> *s = (std::atomic<size_t> *) arg;
        if (sleep_in_adding_func > 0) {
            long t1 = 0;
            if (10000 == s->fetch_add(1)) {
                t1 = flare::get_current_time_micros();
            }
            flare::fiber_sleep_for(sleep_in_adding_func);
            if (t1) {
                FLARE_LOG(INFO) << "elapse is " << flare::get_current_time_micros() - t1 << "ns";
            }
        } else {
            s->fetch_add(1);
        }
        return nullptr;
    }

    TEST_F(FiberTest, small_threads) {
        for (size_t z = 0; z < 2; ++z) {
            sleep_in_adding_func = (z ? 1 : 0);
            char prof_name[32];
            if (sleep_in_adding_func) {
                snprintf(prof_name, sizeof(prof_name), "smallthread.prof");
            } else {
                snprintf(prof_name, sizeof(prof_name), "smallthread_nosleep.prof");
            }

            std::atomic<size_t> s(0);
            size_t N = (sleep_in_adding_func ? 40000 : 100000);
            std::vector<fiber_id_t> th;
            th.reserve(N);
            flare::stop_watcher tm;
            for (size_t j = 0; j < 3; ++j) {
                th.clear();
                if (j == 1) {
                    ProfilerStart(prof_name);
                }
                tm.start();
                for (size_t i = 0; i < N; ++i) {
                    fiber_id_t t1;
                    ASSERT_EQ(0, fiber_start_urgent(
                            &t1, &FIBER_ATTR_SMALL, adding_func, &s));
                    th.push_back(t1);
                }
                tm.stop();
                if (j == 1) {
                    ProfilerStop();
                }
                for (size_t i = 0; i < N; ++i) {
                    fiber_join(th[i], nullptr);
                }
                FLARE_LOG(INFO) << "[Round " << j + 1 << "] fiber_start_urgent takes "
                          << tm.n_elapsed() / N << "ns, sum=" << s;
                ASSERT_EQ(N * (j + 1), (size_t) s);

                // Check uniqueness of th
                std::sort(th.begin(), th.end());
                ASSERT_EQ(th.end(), std::unique(th.begin(), th.end()));
            }
        }
    }

    void *fiber_starter(void *void_counter) {
        while (!stop.load(std::memory_order_relaxed)) {
            fiber_id_t th;
            EXPECT_EQ(0, fiber_start_urgent(&th, nullptr, adding_func, void_counter));
        }
        return nullptr;
    }

    struct FLARE_CACHELINE_ALIGNMENT AlignedCounter {
        AlignedCounter() : value(0) {}

        std::atomic<size_t> value;
    };

    TEST_F(FiberTest, start_fibers_frequently) {
        sleep_in_adding_func = 0;
        char prof_name[32];
        snprintf(prof_name, sizeof(prof_name), "start_fibers_frequently.prof");
        const int con = fiber_getconcurrency();
        ASSERT_GT(con, 0);
        AlignedCounter *counters = new AlignedCounter[con];
        fiber_id_t th[con];

        std::cout << "Perf with different parameters..." << std::endl;
        //ProfilerStart(prof_name);
        for (int cur_con = 1; cur_con <= con; ++cur_con) {
            stop = false;
            for (int i = 0; i < cur_con; ++i) {
                counters[i].value = 0;
                ASSERT_EQ(0, fiber_start_urgent(
                        &th[i], nullptr, fiber_starter, &counters[i].value));
            }
            flare::stop_watcher tm;
            tm.start();
            flare::fiber_sleep_for(200000L);
            stop = true;
            for (int i = 0; i < cur_con; ++i) {
                fiber_join(th[i], nullptr);
            }
            tm.stop();
            size_t sum = 0;
            for (int i = 0; i < cur_con; ++i) {
                sum += counters[i].value * 1000 / tm.m_elapsed();
            }
            std::cout << sum << ",";
        }
        std::cout << std::endl;
        //ProfilerStop();
        delete[] counters;
    }

    void *log_start_latency(void *void_arg) {
        flare::stop_watcher *tm = static_cast<flare::stop_watcher *>(void_arg);
        tm->stop();
        return nullptr;
    }

    TEST_F(FiberTest, start_latency_when_high_idle) {
        bool warmup = true;
        long elp1 = 0;
        long elp2 = 0;
        int REP = 0;
        for (int i = 0; i < 10000; ++i) {
            flare::stop_watcher tm;
            tm.start();
            fiber_id_t th;
            fiber_start_urgent(&th, nullptr, log_start_latency, &tm);
            fiber_join(th, nullptr);
            fiber_id_t th2;
            flare::stop_watcher tm2;
            tm2.start();
            fiber_start_background(&th2, nullptr, log_start_latency, &tm2);
            fiber_join(th2, nullptr);
            if (!warmup) {
                ++REP;
                elp1 += tm.n_elapsed();
                elp2 += tm2.n_elapsed();
            } else if (i == 100) {
                warmup = false;
            }
        }
        FLARE_LOG(INFO) << "start_urgent=" << elp1 / REP << "ns start_background="
                  << elp2 / REP << "ns";
    }

    void *sleep_for_awhile_with_sleep(void *arg) {
        flare::fiber_sleep_for((intptr_t) arg);
        return nullptr;
    }

    TEST_F(FiberTest, stop_sleep) {
        fiber_id_t th;
        ASSERT_EQ(0, fiber_start_urgent(
                &th, nullptr, sleep_for_awhile_with_sleep, (void *) 1000000L));
        flare::stop_watcher tm;
        tm.start();
        flare::fiber_sleep_for(10000);
        ASSERT_EQ(0, fiber_stop(th));
        ASSERT_EQ(0, fiber_join(th, nullptr));
        tm.stop();
        ASSERT_LE(labs(tm.m_elapsed() - 10), 10);
    }

    TEST_F(FiberTest, fiber_exit) {
        fiber_id_t th1;
        fiber_id_t th2;
        pthread_t th3;
        fiber_id_t th4;
        fiber_id_t th5;
        const fiber_attribute attr = FIBER_ATTR_PTHREAD;

        ASSERT_EQ(0, fiber_start_urgent(&th1, nullptr, just_exit, nullptr));
        ASSERT_EQ(0, fiber_start_background(&th2, nullptr, just_exit, nullptr));
        ASSERT_EQ(0, pthread_create(&th3, nullptr, just_exit, nullptr));
        EXPECT_EQ(0, fiber_start_urgent(&th4, &attr, just_exit, nullptr));
        EXPECT_EQ(0, fiber_start_background(&th5, &attr, just_exit, nullptr));

        ASSERT_EQ(0, fiber_join(th1, nullptr));
        ASSERT_EQ(0, fiber_join(th2, nullptr));
        ASSERT_EQ(0, pthread_join(th3, nullptr));
        ASSERT_EQ(0, fiber_join(th4, nullptr));
        ASSERT_EQ(0, fiber_join(th5, nullptr));
    }

    TEST_F(FiberTest, fiber_equal) {
        fiber_id_t th1;
        ASSERT_EQ(0, fiber_start_urgent(&th1, nullptr, do_nothing, nullptr));
        fiber_id_t th2;
        ASSERT_EQ(0, fiber_start_urgent(&th2, nullptr, do_nothing, nullptr));
        ASSERT_EQ(0, fiber_equal(th1, th2));
        fiber_id_t th3 = th2;
        ASSERT_EQ(1, fiber_equal(th3, th2));
        ASSERT_EQ(0, fiber_join(th1, nullptr));
        ASSERT_EQ(0, fiber_join(th2, nullptr));
    }

    void *mark_run(void *run) {
        *static_cast<pthread_t *>(run) = pthread_self();
        return nullptr;
    }

    void *check_sleep(void *pthread_task) {
        EXPECT_TRUE(fiber_self() != 0);
        // Create a no-signal task that other worker will not steal. The task will be
        // run if current fiber does context switch.
        fiber_attribute attr = FIBER_ATTR_NORMAL | FIBER_NOSIGNAL;
        fiber_id_t th1;
        pthread_t run = 0;
        const pthread_t pid = pthread_self();
        EXPECT_EQ(0, fiber_start_urgent(&th1, &attr, mark_run, &run));
        if (pthread_task) {
            flare::fiber_sleep_for(100000L);
            // due to NOSIGNAL, mark_run did not run.
            // FIXME: actually runs. someone is still stealing.
            // EXPECT_EQ((pthread_t)0, run);
            // flare::fiber_sleep_for = usleep for FIBER_ATTR_PTHREAD
            EXPECT_EQ(pid, pthread_self());
            // schedule mark_run
            fiber_flush();
        } else {
            // start_urgent should jump to the new thread first, then back to
            // current thread.
            EXPECT_EQ(pid, run);             // should run in the same pthread
        }
        EXPECT_EQ(0, fiber_join(th1, nullptr));
        if (pthread_task) {
            EXPECT_EQ(pid, pthread_self());
            EXPECT_NE((pthread_t) 0, run); // the mark_run should run.
        }
        return nullptr;
    }

    TEST_F(FiberTest, fiber_sleep_for) {
        // NOTE: May fail because worker threads may still be stealing tasks
        // after previous cases.
        usleep(10000);

        fiber_id_t th1;
        ASSERT_EQ(0, fiber_start_urgent(&th1, &FIBER_ATTR_PTHREAD,
                                        check_sleep, (void *) 1));
        ASSERT_EQ(0, fiber_join(th1, nullptr));

        fiber_id_t th2;
        ASSERT_EQ(0, fiber_start_urgent(&th2, nullptr,
                                        check_sleep, (void *) 0));
        ASSERT_EQ(0, fiber_join(th2, nullptr));
    }

    void *dummy_thread(void *) {
        return nullptr;
    }

    TEST_F(FiberTest, too_many_nosignal_threads) {
        for (size_t i = 0; i < 100000; ++i) {
            fiber_attribute attr = FIBER_ATTR_NORMAL | FIBER_NOSIGNAL;
            fiber_id_t tid;
            ASSERT_EQ(0, fiber_start_urgent(&tid, &attr, dummy_thread, nullptr));
        }
    }

    static void *yield_thread(void *) {
        flare::fiber_yield();
        return nullptr;
    }

    TEST_F(FiberTest, yield_single_thread) {
        fiber_id_t tid;
        ASSERT_EQ(0, fiber_start_background(&tid, nullptr, yield_thread, nullptr));
        ASSERT_EQ(0, fiber_join(tid, nullptr));
    }

} // namespace
