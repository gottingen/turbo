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

#include <execinfo.h>
#include "turbo/times/time.h"
#include "turbo/log/logging.h"
#include "turbo/fiber/fiber.h"
#include "turbo/fiber/runtime.h"
#include "turbo/fiber/internal/fiber_entity.h"
#include "turbo/times/stop_watcher.h"

namespace turbo::fiber_internal {
    class FiberTest {
    protected:

        FiberTest() {
            const int kNumCores = sysconf(_SC_NPROCESSORS_ONLN);
            if (kNumCores > 0) {
                TURBO_UNUSED(fiber_set_concurrency(kNumCores));
            }
        };

        virtual ~FiberTest() {};

    };

    TEST_CASE_FIXTURE(FiberTest, "sizeof_task_meta") {
        TLOG_INFO("sizeof(fiber_entity)={}", sizeof(turbo::fiber_internal::FiberEntity));
    }

    void *unrelated_pthread(void *) {
        TLOG_INFO("I did not call any fiber function, "
                  "I should begin and end without any problem");
        return (void *) (intptr_t) 1;
    }

    TEST_CASE_FIXTURE(FiberTest, "unrelated_pthread") {
        pthread_t th;
        REQUIRE_EQ(0, pthread_create(&th, nullptr, unrelated_pthread, nullptr));
        void *ret = nullptr;
        REQUIRE_EQ(0, pthread_join(th, &ret));
        REQUIRE_EQ(1, (intptr_t) ret);
    }

    TEST_CASE_FIXTURE(FiberTest, "attr_init_and_destroy") {
        FiberAttribute attr;
        REQUIRE_EQ(0, fiber_attr_init(&attr));
        REQUIRE_EQ(0, fiber_attr_destroy(&attr));
    }

    fiber_fcontext_t fcm;
    fiber_fcontext_t fc;
    typedef std::pair<int, int> pair_t;

    static void ff(intptr_t param) {
        pair_t *p = (pair_t *) param;
        p = (pair_t *) fiber_jump_fcontext(&fc, fcm, (intptr_t) (p->first + p->second));
        fiber_jump_fcontext(&fc, fcm, (intptr_t) (p->first + p->second));
    }

    TEST_CASE_FIXTURE(FiberTest, "context_sanity") {
        fcm = nullptr;
        std::size_t size(8192);
        void *sp = malloc(size);

        pair_t p(std::make_pair(2, 7));
        fc = fiber_make_fcontext((char *) sp + size, size, ff);

        int res = (int) fiber_jump_fcontext(&fcm, fc, (intptr_t) &p);
        std::cout << p.first << " + " << p.second << " == " << res << std::endl;

        p = std::make_pair(5, 6);
        res = (int) fiber_jump_fcontext(&fcm, fc, (intptr_t) &p);
        std::cout << p.first << " + " << p.second << " == " << res << std::endl;
    }

    TEST_CASE_FIXTURE(FiberTest, "call_fiber_functions_before_tls_created") {
        REQUIRE_EQ(0, turbo::Fiber::sleep_for(turbo::Duration::milliseconds(1)).code());
        REQUIRE(turbo::is_invalid_argument(fiber_join(0, nullptr)));
        REQUIRE_EQ(0UL, turbo::Fiber::fiber_self());
    }

    std::atomic<bool> stop(false);

    void *sleep_for_awhile(void *arg) {
        TLOG_INFO("sleep_for_awhile({}) main thread {}", arg, pthread_self());
        TURBO_UNUSED(turbo::Fiber::sleep_for(turbo::Duration::microseconds(100000L)));
        TLOG_INFO("sleep_for_awhile({}) main thread {}", arg, pthread_self());
        return nullptr;
    }

    void *just_exit(void *arg) {
        TLOG_INFO("just_exit({}) main thread {}", arg, pthread_self());
        fiber_exit(nullptr);
        REQUIRE(false); //<< "just_exit(" << arg << ") should never be here";
        return nullptr;
    }

    void *repeated_sleep(void *arg) {
        for (size_t i = 0; !stop; ++i) {
            TLOG_INFO("repeated_sleep({}) i={}", arg, i);
            TURBO_UNUSED(turbo::Fiber::sleep_for(turbo::Duration::microseconds(100000L)));
        }
        return nullptr;
    }

    class every_duration {
    public:
        explicit every_duration(turbo::Duration d)
                : _last_time(turbo::time_now()), _interval(d) {}

        operator bool() {
            const auto now = turbo::time_now();
            if (now < _last_time + _interval) {
                return false;
            }
            _last_time = now;
            return true;
        }

    private:
        turbo::Time _last_time;
        const turbo::Duration _interval;
    };


    void *spin_and_log(void *arg) {
        // This thread never yields CPU.
        every_duration every_1s(turbo::Duration::seconds(1));
        size_t i = 0;
        while (!stop) {
            if (every_1s) {
                TLOG_INFO("spin_and_log({}) i={}", arg, i);
            }
        }
        return nullptr;
    }

    void *do_nothing(void *arg) {
        TLOG_INFO("do_nothing({}) main thread {}", arg, pthread_self());
        return nullptr;
    }

    void *launcher(void *arg) {
        TLOG_INFO("launcher({}) main thread {}", arg, pthread_self());
        for (size_t i = 0; !stop; ++i) {
            fiber_id_t th;
            REQUIRE(fiber_start(&th, nullptr, do_nothing, (void *) i).ok());
            TURBO_UNUSED(turbo::Fiber::sleep_for(turbo::Duration::microseconds(100000L)));
        }
        return nullptr;
    }

    void *stopper(void *) {
        // Need this thread to set `stop' to true. Reason: If spin_and_log (which
        // never yields CPU) is scheduled to main thread, main thread cannot get
        // to run again.
        TURBO_UNUSED(turbo::Fiber::sleep_for(turbo::Duration::microseconds(5 * 1000000L)));
        TLOG_INFO("about to stop");
        stop = true;
        return nullptr;
    }

    void *misc(void *arg) {
        TLOG_INFO("misc({}) main thread {}", arg, pthread_self());
        fiber_id_t th[8];
        REQUIRE_EQ(turbo::ok_status(), fiber_start(&th[0], nullptr, sleep_for_awhile, (void *) 2));
        REQUIRE_EQ(turbo::ok_status(), fiber_start(&th[1], nullptr, just_exit, (void *) 3));
        REQUIRE_EQ(turbo::ok_status(), fiber_start(&th[2], nullptr, repeated_sleep, (void *) 4));
        REQUIRE_EQ(turbo::ok_status(), fiber_start(&th[3], nullptr, repeated_sleep, (void *) 68));
        REQUIRE_EQ(turbo::ok_status(), fiber_start(&th[4], nullptr, spin_and_log, (void *) 5));
        REQUIRE_EQ(turbo::ok_status(), fiber_start(&th[5], nullptr, spin_and_log, (void *) 85));
        REQUIRE_EQ(turbo::ok_status(), fiber_start(&th[6], nullptr, launcher, (void *) 6));
        REQUIRE_EQ(turbo::ok_status(), fiber_start(&th[7], nullptr, stopper, nullptr));
        for (size_t i = 0; i < TURBO_ARRAY_SIZE(th); ++i) {
            REQUIRE(fiber_join(th[i], nullptr).ok());
        }
        return nullptr;
    }

    TEST_CASE_FIXTURE(FiberTest, "sanity") {
        TLOG_INFO("main thread {}", pthread_self());
        fiber_id_t th1;
        REQUIRE_EQ(turbo::ok_status(), fiber_start(&th1, nullptr, misc, (void *) 1));
        TLOG_INFO("main thread {}", pthread_self());
        REQUIRE(fiber_join(th1, nullptr).ok());
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

    TEST_CASE_FIXTURE(FiberTest, "backtrace") {
        fiber_id_t th;
        REQUIRE_EQ(turbo::ok_status(), fiber_start(&th, nullptr, tf, nullptr));
        REQUIRE(fiber_join(th, nullptr).ok());

        char **text = backtrace_symbols(bt_array, bt_cnt);
        REQUIRE(text);
        for (int i = 0; i < bt_cnt; ++i) {
            puts(text[i]);
        }
    }

    TEST_CASE_FIXTURE(FiberTest, "lambda_backtrace") {
        fiber_id_t th;
        REQUIRE_EQ(turbo::ok_status(), fiber_start(&th, nullptr, [](void *) -> void * {
            if (call_do_bt() != 57) {
                return (void *) 1L;
            }
            return nullptr;
        }, nullptr));
        REQUIRE(fiber_join(th, nullptr).ok());

        char **text = backtrace_symbols(bt_array, bt_cnt);
        REQUIRE(text);
        for (int i = 0; i < bt_cnt; ++i) {
            puts(text[i]);
        }
    }

    void *show_self(void *) {
        REQUIRE_NE(0ul, turbo::Fiber::fiber_self());
        TLOG_INFO("fiber_self={}", turbo::Fiber::fiber_self());
        return nullptr;
    }

    TEST_CASE_FIXTURE(FiberTest, "fiber_self") {
        REQUIRE_EQ(0ul, turbo::Fiber::fiber_self());
        fiber_id_t bth;
        REQUIRE_EQ(turbo::ok_status(), fiber_start(&bth, nullptr, show_self, nullptr));
        REQUIRE(fiber_join(bth, nullptr).ok());
    }

    void *join_self(void *) {
        REQUIRE(turbo::is_invalid_argument(fiber_join(turbo::Fiber::fiber_self(), nullptr)));
        return nullptr;
    }

    TEST_CASE_FIXTURE(FiberTest, "fiber_join") {
        // Invalid tid
        REQUIRE(turbo::is_invalid_argument(fiber_join(0, nullptr)));

        // Unexisting tid
        REQUIRE(turbo::is_invalid_argument(fiber_join((fiber_id_t) -1, nullptr)));

        // Joining self
        fiber_id_t th;
        REQUIRE_EQ(turbo::ok_status(), fiber_start(&th, nullptr, join_self, nullptr));
    }

    void *change_errno(void *arg) {
        errno = (intptr_t) arg;
        return nullptr;
    }

    TEST_CASE_FIXTURE(FiberTest, "errno_not_changed") {
        fiber_id_t th;
        errno = 1;
        REQUIRE(fiber_start(&th, nullptr, change_errno, (void *) (intptr_t) 2).ok());
        REQUIRE_EQ(1, errno);
    }

    static long sleep_in_adding_func = 0;

    void *adding_func(void *arg) {
        std::atomic<size_t> *s = (std::atomic<size_t> *) arg;
        if (sleep_in_adding_func > 0) {
            long t1 = 0;
            if (10000 == s->fetch_add(1)) {
                t1 = turbo::get_current_time_micros();
            }
            turbo::Fiber::sleep_for(turbo::Duration::microseconds(sleep_in_adding_func));
            if (t1) {
                TLOG_INFO("elapse is {}ns", turbo::get_current_time_micros() - t1);
            }
        } else {
            s->fetch_add(1);
        }
        return nullptr;
    }


    TEST_CASE_FIXTURE(FiberTest, "small_threads") {
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
            turbo::StopWatcher tm;
            for (size_t j = 0; j < 3; ++j) {
                th.clear();
#ifdef ENABLE_PROFILE
                if (j == 1) {
                    ProfilerStart(prof_name);
                }
#endif
                tm.reset();
                for (size_t i = 0; i < N; ++i) {
                    fiber_id_t t1;
                    REQUIRE(fiber_start(
                            &t1, &FIBER_ATTR_SMALL, adding_func, &s).ok());
                    th.push_back(t1);
                }
                tm.stop();
#ifdef ENABLE_PROFILE
                if (j == 1) {
                    ProfilerStop();
                }
#endif
                for (size_t i = 0; i < N; ++i) {
                    TURBO_UNUSED(fiber_join(th[i], nullptr));
                }

                TLOG_INFO("[Round {}] fiber_start takes {}ns, sum={}", j + 1,
                          tm.elapsed_nano() / N, s.load());
                REQUIRE_EQ(N * (j + 1), (size_t) s);

                // Check uniqueness of th
                std::sort(th.begin(), th.end());
                REQUIRE_EQ(th.end(), std::unique(th.begin(), th.end()));
            }
        }
    }

    void *fiber_starter(void *void_counter) {
        while (!stop.load(std::memory_order_relaxed)) {
            fiber_id_t th;
            REQUIRE_EQ(turbo::ok_status(), fiber_start(&th, nullptr, adding_func, void_counter));
        }
        return nullptr;
    }

    struct TURBO_CACHE_LINE_ALIGNED AlignedCounter {
        AlignedCounter() : value(0) {}

        std::atomic<size_t> value;
    };

    TEST_CASE_FIXTURE(FiberTest, "start_fibers_frequently") {
        sleep_in_adding_func = 0;
        char prof_name[32];
        snprintf(prof_name, sizeof(prof_name), "start_fibers_frequently.prof");
        const int con = fiber_get_concurrency();
        REQUIRE_GT(con, 0);
        AlignedCounter *counters = new AlignedCounter[con];
        fiber_id_t th[con];

        std::cout << "Perf with different parameters..." << std::endl;
        //ProfilerStart(prof_name);
        for (int cur_con = 1; cur_con <= con; ++cur_con) {
            stop = false;
            for (int i = 0; i < cur_con; ++i) {
                counters[i].value = 0;
                REQUIRE_EQ(turbo::ok_status(), fiber_start(
                        &th[i], nullptr, fiber_starter, &counters[i].value));
            }
            turbo::StopWatcher tm;
            tm.reset();
            turbo::Fiber::sleep_for(turbo::Duration::microseconds(200000L));
            stop = true;
            for (int i = 0; i < cur_con; ++i) {
                TURBO_UNUSED(fiber_join(th[i], nullptr));
            }
            tm.stop();
            size_t sum = 0;
            for (int i = 0; i < cur_con; ++i) {
                sum += counters[i].value * 1000 / tm.elapsed_mill();
            }
            std::cout << sum << ",";
        }
        std::cout << std::endl;
        //ProfilerStop();
        delete[] counters;
    }

    void *log_start_latency(void *void_arg) {
        turbo::StopWatcher *tm = static_cast<turbo::StopWatcher *>(void_arg);
        tm->stop();
        return nullptr;
    }

    TEST_CASE_FIXTURE(FiberTest, "start_latency_when_high_idle") {
        bool warmup = true;
        long elp1 = 0;
        long elp2 = 0;
        int REP = 0;
        for (int i = 0; i < 10000; ++i) {
            turbo::StopWatcher tm;
            tm.reset();
            fiber_id_t th;
            REQUIRE(fiber_start(&th, nullptr, log_start_latency, &tm).ok());
            auto status = fiber_join(th, nullptr);
            REQUIRE(status.ok());
            fiber_id_t th2;
            turbo::StopWatcher tm2;
            tm2.reset();
            REQUIRE(turbo::fiber_start_background(&th2, nullptr, log_start_latency, &tm2).ok());
            status = fiber_join(th2, nullptr);
            REQUIRE(status.ok());
            if (!warmup) {
                ++REP;
                elp1 += tm.elapsed_nano();
                elp2 += tm2.elapsed_nano();
            } else if (i == 100) {
                warmup = false;
            }
        }
        TLOG_INFO("start_urgent={}ns start_background={}ns", elp1 / REP, elp2 / REP);
    }

    void *sleep_for_awhile_with_sleep(void *arg) {
        turbo::Fiber::sleep_for(turbo::Duration::microseconds((intptr_t) arg));
        return nullptr;
    }

    TEST_CASE_FIXTURE(FiberTest, "stop_sleep") {
        fiber_id_t th;
        REQUIRE_EQ(turbo::ok_status(), fiber_start(
                &th, nullptr, sleep_for_awhile_with_sleep, (void *) 1000000L));
        turbo::StopWatcher tm;
        tm.reset();
        turbo::Fiber::sleep_for(turbo::Duration::microseconds(10000L));
        REQUIRE_EQ(turbo::ok_status(), fiber_stop(th));
        REQUIRE(fiber_join(th, nullptr).ok());
        tm.stop();
        REQUIRE_LE(labs(tm.elapsed_mill() - 10), 10);
    }

    TEST_CASE_FIXTURE(FiberTest, "fiber_exit") {
        fiber_id_t th1;
        fiber_id_t th2;
        pthread_t th3;
        fiber_id_t th4;
        fiber_id_t th5;
        const FiberAttribute attr = FIBER_ATTR_PTHREAD;

        REQUIRE_EQ(turbo::ok_status(), fiber_start(&th1, nullptr, just_exit, nullptr));
        REQUIRE_EQ(turbo::ok_status(), fiber_start_background(&th2, nullptr, just_exit, nullptr));
        REQUIRE_EQ(0, pthread_create(&th3, nullptr, just_exit, nullptr));
        REQUIRE_EQ(turbo::ok_status(), fiber_start(&th4, &attr, just_exit, nullptr));
        REQUIRE_EQ(turbo::ok_status(), fiber_start_background(&th5, &attr, just_exit, nullptr));

        REQUIRE(fiber_join(th1, nullptr).ok());
        REQUIRE(fiber_join(th2, nullptr).ok());
        REQUIRE_EQ(0, pthread_join(th3, nullptr));
        REQUIRE(fiber_join(th4, nullptr).ok());
        REQUIRE(fiber_join(th5, nullptr).ok());
    }

    TEST_CASE_FIXTURE(FiberTest, "fiber_equal") {
        fiber_id_t th1;
        REQUIRE_EQ(turbo::ok_status(), fiber_start(&th1, nullptr, do_nothing, nullptr));
        fiber_id_t th2;
        REQUIRE_EQ(turbo::ok_status(), fiber_start(&th2, nullptr, do_nothing, nullptr));
        REQUIRE_EQ(0, turbo::Fiber::equal(th1, th2));
        fiber_id_t th3 = th2;
        REQUIRE_EQ(1, turbo::Fiber::equal(th3, th2));
        REQUIRE(fiber_join(th1, nullptr).ok());
        REQUIRE(fiber_join(th2, nullptr).ok());
    }

    void *mark_run(void *run) {
        *static_cast<pthread_t *>(run) = pthread_self();
        return nullptr;
    }

    void *check_sleep(void *pthread_task) {
        REQUIRE_NE(turbo::Fiber::fiber_self(), 0);
        // Create a no-signal task that other worker will not steal. The task will be
        // run if current fiber does context switch.
        FiberAttribute attr = FIBER_ATTR_NORMAL | AttributeFlag::FLAG_NOSIGNAL;
        fiber_id_t th1;
        pthread_t run = 0;
        const pthread_t pid = pthread_self();
        REQUIRE_EQ(turbo::ok_status(), fiber_start(&th1, &attr, mark_run, &run));
        if (pthread_task) {
            turbo::Fiber::sleep_for(turbo::Duration::microseconds(100000L));
            // due to NOSIGNAL, mark_run did not run.
            // FIXME: actually runs. someone is still stealing.
            // REQUIRE_EQ((pthread_t)0, run);
            // turbo::Fiber::sleep_for = usleep for FIBER_ATTR_PTHREAD
            REQUIRE_EQ(pid, pthread_self());
            // schedule mark_run
            turbo::Fiber::fiber_flush();
        } else {
            // start_urgent should jump to the new thread first, then back to
            // current thread.
            REQUIRE_EQ(pid, run);             // should run in the same pthread
        }
        REQUIRE(fiber_join(th1, nullptr).ok());
        if (pthread_task) {
            REQUIRE_EQ(pid, pthread_self());
            REQUIRE_NE((pthread_t) 0, run); // the mark_run should run.
        }
        return nullptr;
    }

    TEST_CASE_FIXTURE(FiberTest, "fiber_sleep_for") {
        // NOTE: May fail because worker threads may still be stealing tasks
        // after previous cases.
        usleep(10000);

        fiber_id_t th1;
        REQUIRE_EQ(turbo::ok_status(), fiber_start(&th1, &FIBER_ATTR_PTHREAD,
                                                   check_sleep, (void *) 1));
        REQUIRE(fiber_join(th1, nullptr).ok());

        fiber_id_t th2;
        REQUIRE_EQ(turbo::ok_status(), fiber_start(&th2, nullptr,
                                                   check_sleep, (void *) 0));
        REQUIRE(fiber_join(th2, nullptr).ok());
    }


    void *test_parent_span(void *p) {
        uint64_t *q = (uint64_t *) p;
        *q = (uint64_t) (Fiber::get_span());
        TLOG_INFO("span id in thread is {}", *q);
        return NULL;
    }

    TEST_CASE_FIXTURE(FiberTest, "test_span") {
        uint64_t p1 = 0;
        uint64_t p2 = 0;

        uint64_t target = 0xBADBEAFUL;
        TLOG_INFO("Target span id is {}", target);

        turbo::Fiber::start_span((void *) target);
        turbo::Fiber fb1;
        auto rs = fb1.start(turbo::FIBER_ATTR_NORMAL_WITH_SPAN, test_parent_span, &p1);
        REQUIRE(rs.ok());
        rs = fb1.join();
        REQUIRE(rs.ok());

        turbo::Fiber fb2;
        rs = fb2.start_lazy(test_parent_span, &p2);
        REQUIRE(rs.ok());
        rs = fb2.join();
        REQUIRE(rs.ok());
        REQUIRE_EQ(p1, target);
        REQUIRE_NE(p2, target);
    }

    void *dummy_thread(void *) {
        return nullptr;
    }

    TEST_CASE_FIXTURE(FiberTest, "too_many_nosignal_threads") {
        for (size_t i = 0; i < 100000; ++i) {
            FiberAttribute attr = FIBER_ATTR_NORMAL | AttributeFlag::FLAG_NOSIGNAL;
            fiber_id_t tid;
            REQUIRE_EQ(turbo::ok_status(), fiber_start(&tid, &attr, dummy_thread, nullptr));
        }
    }

    static void *yield_thread(void *) {
        turbo::Fiber::yield();
        return nullptr;
    }

    TEST_CASE_FIXTURE(FiberTest, "yield_single_thread") {
        fiber_id_t tid;
        REQUIRE_EQ(turbo::ok_status(), fiber_start_background(&tid, nullptr, yield_thread, nullptr));
        REQUIRE(fiber_join(tid, nullptr).ok());
    }

} // namespace turbo::fiber_internal
