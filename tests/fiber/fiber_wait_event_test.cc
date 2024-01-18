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

#include "turbo/times/time.h"
#include "turbo/times/stop_watcher.h"
#include "turbo/log/logging.h"
#include "turbo/fiber/wait_event.h"
#include "turbo/fiber/internal/schedule_group.h"
#include "turbo/fiber/internal/fiber_worker.h"
#include "turbo/fiber/fiber.h"
#include "turbo/system/threading.h"


namespace turbo {
    /*
    extern std::atomic<ScheduleGroup *> g_task_control;

    inline ScheduleGroup *get_task_control() {
        return g_task_control.load(std::memory_order_consume);
    }*/

    TEST_CASE("WaitableEventTest, wait_on_already_timedout_event") {
        WaitEvent<uint32_t> event;
        event.initialize();
        REQUIRE(event);
        auto now = turbo::Time::time_now();
        event = 1;
        auto rs = event.wait_until(now, 1);
        turbo::println("{}", rs.to_string());
        REQUIRE_EQ(rs.code(), kETIMEDOUT);
    }

    void *sleeper(void *arg) {
        TURBO_UNUSED(turbo::fiber_sleep_for(turbo::Duration::microseconds((uint64_t) arg)));
        return nullptr;
    }

    void *joiner(void *arg) {
        const long t1 = turbo::get_current_time_micros();
        auto fs = (std::vector<Fiber> *) arg;
        for (size_t i = 0; i < fs->size(); ++i) {
            auto fid = (*fs)[i].self();
            auto rs = (*fs)[i].join(nullptr);
            if (!rs.ok()) {
                TLOG_CRITICAL("fail to join thread_{} reason: {}", i, rs.to_string());
            }
            long elp = turbo::get_current_time_micros() - t1;
            turbo::println("{}", i);
            REQUIRE_LE(labs(elp - (i + 1) * 100000L), 15000L);
            TLOG_INFO("Joined thread {0} at {1}us [{2}]",
                      fid, elp, fiber_self());
        }
        for (size_t i = 0; i < fs->size(); ++i) {
            REQUIRE((*fs)[i].join(nullptr).ok());
        }
        return nullptr;
    }

    struct A {
        uint64_t a;
        char dummy[0];
    };

    struct B {
        uint64_t a;
    };


    TEST_CASE("WaitableEventTest, with_or_without_array_zero") {
        REQUIRE_EQ(sizeof(B), sizeof(A));
    }


    TEST_CASE("WaitableEventTest, join") {
        const size_t N = 6;
        const size_t M = 6;
        std::vector<Fiber> th(N);
        std::vector<Fiber> jth(M);
        pthread_t pth[M];
        for (size_t i = 0; i < N; ++i) {
            FiberAttribute attr = (i == 0 ? FIBER_ATTR_PTHREAD : FIBER_ATTR_NORMAL);
            REQUIRE_EQ(turbo::ok_status(), th[i].start(attr, sleeper, (void *) (100000L * (i + 1))));
        }

        for (size_t i = 0; i < M; ++i) {
            REQUIRE_EQ(turbo::ok_status(), jth[i].start(joiner, &th));
        }
        for (size_t i = 0; i < M; ++i) {
            REQUIRE_EQ(0, pthread_create(&pth[i], nullptr, joiner, &th));
        }

        for (size_t i = 0; i < M; ++i) {
            REQUIRE(jth[i].join(nullptr).ok());
        }
        for (size_t i = 0; i < M; ++i) {
            REQUIRE_EQ(0, pthread_join(pth[i], nullptr));
        }
    }


    struct WaiterArg {
        turbo::StatusCode expected_result;
        int expected_value;
        WaitEvent<std::atomic<int>> *event;
        turbo::Time timeout;
    };

    void *waiter(void *arg) {
        WaiterArg *wa = (WaiterArg *) arg;
        const long t1 = turbo::get_current_time_micros();
        turbo::Status rc;
        if (wa->timeout != turbo::Time::infinite_future()) {
            TLOG_INFO("before wait_until, time={}", (wa->timeout - turbo::Time::time_now()).to_string());
            rc = wa->event->wait_until(wa->timeout, wa->expected_value);
            TLOG_INFO("before wait_until, time={}", (wa->timeout - turbo::Time::time_now()).to_string());
        } else {
            rc = wa->event->wait(wa->expected_value);
        }

        const long t2 = turbo::get_current_time_micros();
        if (rc.ok()) {
            REQUIRE_EQ(wa->expected_result, 0);
        } else {
            REQUIRE_EQ(wa->expected_result, rc.code());
        }

        TLOG_INFO("after wait, time={0}us", (t2 - t1));
        return nullptr;
    }

    TEST_CASE("WaitableEventTest, sanity") {
        const size_t N = 5;
        WaiterArg args[N * 4];
        pthread_t t1, t2;
        WaitEvent<std::atomic<int>> b1;
        b1.initialize();
        REQUIRE(b1);
        b1.destroy();

        b1.initialize();
        b1 = 1;
        REQUIRE_EQ(0, b1.notify_one());

        WaiterArg *unmatched_arg = new WaiterArg;
        unmatched_arg->expected_value = b1 + 1;
        unmatched_arg->expected_result = EWOULDBLOCK;;
        unmatched_arg->event = &b1;
        unmatched_arg->timeout = turbo::Time::infinite_future();
        pthread_create(&t2, nullptr, waiter, unmatched_arg);
        Fiber th;
        REQUIRE(th.start(waiter, unmatched_arg).ok());
        th.detach();

        const Time abstime = turbo::seconds_from_now(1);
        for (size_t i = 0; i < 4 * N; ++i) {
            args[i].expected_value = b1;
            args[i].event = &b1;
            if ((i % 2) == 0) {
                args[i].expected_result = 0;
                args[i].timeout = turbo::Time::infinite_future();
            } else {
                args[i].expected_result = turbo::kETIMEDOUT;
                args[i].timeout = abstime;
            }
            if (i < 2 * N) {
                pthread_create(&t1, nullptr, waiter, &args[i]);
            } else {
                Fiber th1;
                REQUIRE(th1.start(waiter, &args[i]).ok());
                th1.detach();
            }
        }

        sleep(2);
        for (size_t i = 0; i < 2 * N; ++i) {
            REQUIRE_EQ(1, b1.notify_one());
        }
        REQUIRE_EQ(0, b1.notify_all());
        sleep(1);
        b1.destroy();
    }


    struct event_wait_arg {
        WaitEvent<int> *event;
        int expected_val;
        long wait_msec;
        int error_code;
    };

    void *wait_event(void *void_arg) {
        event_wait_arg *arg = static_cast<event_wait_arg *>(void_arg);
        const auto ts = turbo::milliseconds_from_now(arg->wait_msec);
        TLOG_WARN("expected_val={}" , static_cast<int>(arg->expected_val));
        auto rc = arg->event->wait_until(ts, arg->expected_val);
        if (arg->error_code != 0) {
            REQUIRE(!rc.ok());
            REQUIRE_EQ(arg->error_code, rc.code());
        } else {
            REQUIRE(rc.ok());
        }
        return nullptr;
    }

    TEST_CASE("WaitableEventTest, wait_without_stop") {
        WaitEvent<int> event;
        event.initialize();
        event = 7;
        turbo::StopWatcher tm;
        const long WAIT_MSEC = 500;
        for (int i = 0; i < 2; ++i) {
            const FiberAttribute attr =
                    (i == 0 ? FIBER_ATTR_PTHREAD : FIBER_ATTR_NORMAL);
            event_wait_arg arg = {&event, event.load(), WAIT_MSEC, turbo::kETIMEDOUT};
            Fiber th;

            tm.reset();
            REQUIRE_EQ(turbo::ok_status(), th.start(attr, wait_event, &arg));
            REQUIRE(th.join(nullptr).ok());
            tm.stop();

            REQUIRE_LT(labs(tm.elapsed_mill() - WAIT_MSEC), 250);
        }
        event.destroy();
    }

    TEST_CASE("WaitableEventTest, stop_after_running") {
        WaitEvent<int> event;
        event.initialize();
        event = 7;
        turbo::StopWatcher tm;
        const long WAIT_MSEC = 500;
        turbo::Duration SleepDuration = turbo::Duration::milliseconds(10);
        for (int i = 0; i < 2; ++i) {
            const FiberAttribute attr =
                    (i == 0 ? FIBER_ATTR_PTHREAD : FIBER_ATTR_NORMAL);
            Fiber th;
            TLOG_INFO("make args event={0}, expected={1}, wait_msec={2}, error_code={3}",
                      turbo::ptr(&event), event.load(), WAIT_MSEC, turbo::kEINTR);
            event_wait_arg arg = {&event, event.load(), WAIT_MSEC, turbo::kEINTR};

            tm.reset();
            REQUIRE_EQ(turbo::ok_status(), th.start(attr, wait_event, &arg));
            REQUIRE_EQ(turbo::ok_status(), turbo::fiber_sleep_for(SleepDuration));
            REQUIRE_EQ(turbo::ok_status(), fiber_stop(th.self()));
            REQUIRE_EQ(turbo::ok_status(), th.stop());
            th.join(nullptr);
            tm.stop();

            REQUIRE_LT((tm.elapsed() - SleepDuration).abs().to_milliseconds(), 25);
            // REQUIRE(turbo::fiber_internal::get_task_control()->
            //             timer_thread()._idset.empty());
            REQUIRE_EQ(true, th.stop().ok());
        }
        event.destroy();
    }

    TEST_CASE("WaitableEventTest, stop_before_running") {
        WaitEvent<int> event;
        event.initialize();
        event = 7;
        turbo::StopWatcher tm;
        const long WAIT_MSEC = 500;

        for (int i = 0; i < 2; ++i) {
            const FiberAttribute attr =
                    (i == 0 ? FIBER_ATTR_PTHREAD : FIBER_ATTR_NORMAL) | AttributeFlag::FLAG_NOSIGNAL;
            Fiber th;
            event_wait_arg arg = {&event, event.load(), WAIT_MSEC, turbo::kEINTR};

            tm.reset();
            REQUIRE_EQ(turbo::ok_status(), th.start(LaunchPolicy::eLazy, attr, wait_event, &arg));
            REQUIRE_EQ(turbo::ok_status(), th.stop());
            turbo::Fiber::fiber_flush();
            REQUIRE(th.join().ok());
            tm.stop();

            REQUIRE_LT(tm.elapsed_mill(), 5);
            // REQUIRE(turbo::fiber_internal::get_task_control()->
            //             timer_thread()._idset.empty());
            REQUIRE_EQ(turbo::ok_status(), th.stop());
        }
        event.destroy();
    }

    void *join_the_waiter(void *arg) {
        REQUIRE(((Fiber *) arg)->join().ok());
        return nullptr;
    }

    TEST_CASE("WaitableEventTest, join_cant_be_wakeup") {
        const long WAIT_MSEC = 100;
        turbo::Duration WaitDuration = turbo::Duration::milliseconds(WAIT_MSEC);
        WaitEvent<int> event;
        event.initialize();
        event = 7;
        turbo::StopWatcher tm;
        event_wait_arg arg = {&event, event.load(), 1000, turbo::kEINTR};

        for (int i = 0; i < 2; ++i) {
            const FiberAttribute attr =
                    (i == 0 ? FIBER_ATTR_PTHREAD : FIBER_ATTR_NORMAL);
            tm.reset();
            Fiber th, th2;
            fiber_id_t fth2;
            fiber_id_t fth;
            REQUIRE_EQ(turbo::ok_status(), th.start(wait_event, &arg));
            fth = th.self();
            REQUIRE(turbo::Fiber::exists(fth));
            REQUIRE_EQ(turbo::ok_status(), th2.start(attr, join_the_waiter, &th));
            fth2 = th2.self();
            REQUIRE_EQ(turbo::ok_status(), th2.stop());
            REQUIRE_EQ(turbo::ok_status(), turbo::fiber_sleep_for(WaitDuration / 2));
            turbo::println("{}, {}", fth, fth2);
            REQUIRE(turbo::Fiber::exists(fth));
            REQUIRE(turbo::Fiber::exists(fth2));
            REQUIRE_EQ(turbo::ok_status(), turbo::fiber_sleep_for(WaitDuration / 2));
            REQUIRE_EQ(turbo::ok_status(), th.stop());
            REQUIRE(th2.join().ok());
            REQUIRE(th.join().ok());
            tm.stop();
            REQUIRE_LT(tm.elapsed_mill(), WAIT_MSEC + 15);
            REQUIRE(th.stop().ok());
            REQUIRE(th2.stop().ok());
        }
        event.destroy();
    }

    TEST_CASE("WaitableEventTest, stop_after_slept") {
        turbo::StopWatcher tm;
        const long SLEEP_MSEC = 100;
        const long WAIT_MSEC = 10;
        turbo::Duration WaitDuration = turbo::Duration::milliseconds(WAIT_MSEC);

        for (int i = 0; i < 2; ++i) {
            const FiberAttribute attr =
                    (i == 0 ? FIBER_ATTR_PTHREAD : FIBER_ATTR_NORMAL);
            tm.reset();
            Fiber th;
            REQUIRE_EQ(turbo::ok_status(), th.start(attr, sleeper, (void *) (SLEEP_MSEC * 1000L)));
            REQUIRE_EQ(turbo::ok_status(), turbo::fiber_sleep_for(WaitDuration));
            REQUIRE_EQ(turbo::ok_status(), th.stop());
            REQUIRE(th.join().ok());
            tm.stop();
            if (is_pthread_stack(attr)) {
                REQUIRE_LT(labs(tm.elapsed_mill() - SLEEP_MSEC), 15);
            } else {
                REQUIRE_LT(labs(tm.elapsed_mill() - WAIT_MSEC), 15);
            }
            REQUIRE(th.stop().ok());
        }
    }

    TEST_CASE("WaitableEventTest, stop_just_when_sleeping") {
        turbo::StopWatcher tm;
        const long SLEEP_MSEC = 100;

        for (int i = 0; i < 2; ++i) {
            const FiberAttribute attr =
                    (i == 0 ? FIBER_ATTR_PTHREAD : FIBER_ATTR_NORMAL);
            tm.reset();
            Fiber th;
            REQUIRE_EQ(turbo::ok_status(), th.start(attr, sleeper, (void *) (SLEEP_MSEC * 1000L)));
            REQUIRE_EQ(turbo::ok_status(), th.stop());
            REQUIRE(th.join().ok());
            tm.stop();
            if (is_pthread_stack(attr)) {
                REQUIRE_LT(labs(tm.elapsed_mill() - SLEEP_MSEC), 15);
            } else {
                REQUIRE_LT(tm.elapsed_mill(), 15);
            }
            // REQUIRE(turbo::fiber_internal::get_task_control()->
            //             timer_thread()._idset.empty());
            REQUIRE_EQ(th.stop().ok(), true);
        }
    }

    TEST_CASE("WaitableEventTest, stop_before_sleeping") {
        turbo::StopWatcher tm;
        const long SLEEP_MSEC = 100;

        for (int i = 0; i < 2; ++i) {
            Fiber th;
            const FiberAttribute attr =
                    (i == 0 ? FIBER_ATTR_PTHREAD : FIBER_ATTR_NORMAL) | AttributeFlag::FLAG_NOSIGNAL;

            tm.reset();
            REQUIRE_EQ(turbo::ok_status(), th.start(attr, sleeper, (void *) (SLEEP_MSEC * 1000L)));
            REQUIRE_EQ(turbo::ok_status(), th.stop());
            Fiber::fiber_flush();
            REQUIRE(th.join().ok());
            tm.stop();

            if (is_pthread_stack(attr)) {
                REQUIRE_LT(labs(tm.elapsed_mill() - SLEEP_MSEC), 10);
            } else {
                REQUIRE_LT(tm.elapsed_mill(), 10);
            }
            REQUIRE_EQ(th.stop().ok(), true);
        }
    }


    void *trigger_signal(void *arg) {
        pthread_t *th = (pthread_t *) arg;
        const auto t1 = turbo::Time::time_now();
        for (size_t i = 0; i < 50; ++i) {
            turbo::sleep_for(turbo::Duration::milliseconds(10));
            if (turbo::PlatformThread::kill_thread(*th) == ESRCH) {
                TLOG_INFO("waiter thread end, trigger count={}", i);
                break;
            }
        }
        const auto t2 = turbo::Time::time_now();
        TLOG_INFO("trigger signal thread end, elapsed {} us", (t2 - t1).to_microseconds());
        return nullptr;
    }

    TEST_CASE("FiberTest, wait_with_signal_triggered") {
        turbo::StopWatcher tm;

        const int64_t WAIT_MSEC = 500;
        WaiterArg waiter_args;
        pthread_t waiter_th, tigger_th;
        WaitEvent<std::atomic<int>> event;
        event.initialize();
        REQUIRE(event);
        event = 1;
        REQUIRE_EQ(0, event.notify_one());

        const auto abstime = turbo::milliseconds_from_now(WAIT_MSEC);
        waiter_args.expected_value = event.load();
        waiter_args.event = &event;
        waiter_args.expected_result = turbo::kETIMEDOUT;
        waiter_args.timeout = abstime;
        tm.reset();
        pthread_create(&waiter_th, nullptr, waiter, &waiter_args);
        pthread_create(&tigger_th, nullptr, trigger_signal, &waiter_th);

        REQUIRE_EQ(0, pthread_join(waiter_th, nullptr));
        tm.stop();
        auto wait_elapsed_ms = tm.elapsed_mill();
        TLOG_INFO("waiter thread end, elapsed {} ms", wait_elapsed_ms);
        REQUIRE_LT(labs(wait_elapsed_ms - WAIT_MSEC), 250);

        REQUIRE_EQ(0, pthread_join(tigger_th, nullptr));
        event.destroy();
    }
}  // namespace turbo
