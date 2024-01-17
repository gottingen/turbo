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
#include "turbo/fiber/internal/waitable_event.h"
#include "turbo/fiber/internal/schedule_group.h"
#include "turbo/fiber/internal/fiber_worker.h"
#include "turbo/fiber/fiber.h"


namespace turbo::fiber_internal {
    extern std::atomic<ScheduleGroup *> g_task_control;

    inline ScheduleGroup *get_task_control() {
        return g_task_control.load(std::memory_order_consume);
    }

    TEST_CASE("WaitableEventTest, wait_on_already_timedout_event") {
        uint32_t *event = turbo::fiber_internal::waitable_event_create_checked<uint32_t>();
        REQUIRE(event);
        auto now = turbo::Time::time_now();
        *event = 1;
        REQUIRE(turbo::is_deadline_exceeded(turbo::fiber_internal::waitable_event_wait(event, 1, now)));
    }

    void *sleeper(void *arg) {
        TURBO_UNUSED(turbo::fiber_sleep_for(turbo::Duration::microseconds((uint64_t)arg)));
        return nullptr;
    }

    void *joiner(void *arg) {
        const long t1 = turbo::get_current_time_micros();
        for (fiber_id_t *th = (fiber_id_t *) arg; *th; ++th) {
            if (!fiber_join(*th, nullptr).ok()) {
                TLOG_CRITICAL("fail to join thread_{0}", th - (fiber_id_t *) arg);
            }
            long elp = turbo::get_current_time_micros() - t1;
            REQUIRE_LE(labs(elp - (th - (fiber_id_t *) arg + 1) * 100000L), 15000L);
            TLOG_INFO("Joined thread {0} at {1}us [{2}]",
                      *th, elp, fiber_self());
        }
        for (fiber_id_t *th = (fiber_id_t *) arg; *th; ++th) {
            REQUIRE(fiber_join(*th, nullptr).ok());
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
        fiber_id_t th[N + 1];
        fiber_id_t jth[M];
        pthread_t pth[M];
        for (size_t i = 0; i < N; ++i) {
            FiberAttribute attr = (i == 0 ? FIBER_ATTR_PTHREAD : FIBER_ATTR_NORMAL);
            REQUIRE_EQ(turbo::ok_status(), fiber_start_urgent(
                    &th[i], &attr, sleeper,
                    (void *) (100000L * (i + 1))));
        }
        th[N] = 0;  // joiner will join tids in `th' until seeing 0.
        for (size_t i = 0; i < M; ++i) {
            REQUIRE_EQ(turbo::ok_status(), fiber_start_urgent(&jth[i], nullptr, joiner, th));
        }
        for (size_t i = 0; i < M; ++i) {
            REQUIRE_EQ(0, pthread_create(&pth[i], nullptr, joiner, th));
        }

        for (size_t i = 0; i < M; ++i) {
            auto rc = fiber_join(jth[i], nullptr);
            TLOG_INFO("join {} {}", i, rc.to_string());
            REQUIRE(rc.ok());
        }
        for (size_t i = 0; i < M; ++i) {
            REQUIRE_EQ(0, pthread_join(pth[i], nullptr));
        }
    }


    struct WaiterArg {
        turbo::StatusCode expected_result;
        int expected_value;
        std::atomic<int> *event;
        turbo::Time ptimeout;
    };

    void *waiter(void *arg) {
        WaiterArg *wa = (WaiterArg *) arg;
        const long t1 = turbo::get_current_time_micros();
        const auto rc = turbo::fiber_internal::waitable_event_wait(
                wa->event, wa->expected_value, wa->ptimeout);
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
        std::atomic<int> *b1 =
                turbo::fiber_internal::waitable_event_create_checked<std::atomic<int> >();
        REQUIRE(b1);
        turbo::fiber_internal::waitable_event_destroy(b1);

        b1 = turbo::fiber_internal::waitable_event_create_checked<std::atomic<int> >();
        *b1 = 1;
        REQUIRE_EQ(0, turbo::fiber_internal::waitable_event_wake(b1));

        WaiterArg *unmatched_arg = new WaiterArg;
        unmatched_arg->expected_value = *b1 + 1;
        unmatched_arg->expected_result = turbo::kUnavailable;
        unmatched_arg->event = b1;
        unmatched_arg->ptimeout = turbo::Time::infinite_future();
        pthread_create(&t2, nullptr, waiter, unmatched_arg);
        fiber_id_t th;
        REQUIRE(fiber_start_urgent(&th, nullptr, waiter, unmatched_arg).ok());

        const auto abstime = turbo::seconds_from_now(1);
        for (size_t i = 0; i < 4 * N; ++i) {
            args[i].expected_value = *b1;
            args[i].event = b1;
            if ((i % 2) == 0) {
                args[i].expected_result = 0;
                args[i].ptimeout = turbo::Time::infinite_future();
            } else {
                args[i].expected_result = turbo::kETIMEDOUT;
                args[i].ptimeout = abstime;
            }
            if (i < 2 * N) {
                pthread_create(&t1, nullptr, waiter, &args[i]);
            } else {
                REQUIRE(fiber_start_urgent(&th, nullptr, waiter, &args[i]).ok());
            }
        }

        sleep(2);
        for (size_t i = 0; i < 2 * N; ++i) {
            REQUIRE_EQ(1, turbo::fiber_internal::waitable_event_wake(b1));
        }
        REQUIRE_EQ(0, turbo::fiber_internal::waitable_event_wake_all(b1));
        sleep(1);
        turbo::fiber_internal::waitable_event_destroy(b1);
    }


    struct event_wait_arg {
        int *event;
        int expected_val;
        long wait_msec;
        int error_code;
    };

    void *wait_event(void *void_arg) {
        event_wait_arg *arg = static_cast<event_wait_arg *>(void_arg);
        const auto ts = turbo::milliseconds_from_now(arg->wait_msec);
        auto rc = turbo::fiber_internal::waitable_event_wait(arg->event, arg->expected_val, ts);
        if (arg->error_code != 0) {
            REQUIRE(!rc.ok());
            REQUIRE_EQ(arg->error_code, rc.code());
        } else {
            REQUIRE(rc.ok());
        }
        return nullptr;
    }

    TEST_CASE("WaitableEventTest, wait_without_stop") {
        int *event = turbo::fiber_internal::waitable_event_create_checked<int>();
        *event = 7;
        turbo::StopWatcher tm;
        const long WAIT_MSEC = 500;
        for (int i = 0; i < 2; ++i) {
            const FiberAttribute attr =
                    (i == 0 ? FIBER_ATTR_PTHREAD : FIBER_ATTR_NORMAL);
            event_wait_arg arg = {event, *event, WAIT_MSEC, turbo::kETIMEDOUT};
            fiber_id_t th;

            tm.reset();
            REQUIRE_EQ(turbo::ok_status(), fiber_start_urgent(&th, &attr, wait_event, &arg));
            REQUIRE(fiber_join(th, nullptr).ok());
            tm.stop();

            REQUIRE_LT(labs(tm.elapsed_mill() - WAIT_MSEC), 250);
        }
        turbo::fiber_internal::waitable_event_destroy(event);
    }

    TEST_CASE("WaitableEventTest, stop_after_running") {
        int *event = turbo::fiber_internal::waitable_event_create_checked<int>();
        *event = 7;
        turbo::StopWatcher tm;
        const long WAIT_MSEC = 500;
        turbo::Duration SleepDuration = turbo::Duration::milliseconds(10);
        for (int i = 0; i < 2; ++i) {
            const FiberAttribute attr = (i == 0 ? FIBER_ATTR_PTHREAD : FIBER_ATTR_NORMAL);
            fiber_id_t th;
            event_wait_arg arg = {event, *event, WAIT_MSEC, turbo::kEINTR};

            tm.reset();
            REQUIRE_EQ(turbo::ok_status(), fiber_start_urgent(&th, &attr, wait_event, &arg));
            REQUIRE_EQ(turbo::ok_status(), turbo::fiber_sleep_for(SleepDuration));
            REQUIRE_EQ(turbo::ok_status(), fiber_stop(th));
            fiber_join(th, nullptr);
            tm.stop();

            REQUIRE_LT((tm.elapsed() - SleepDuration).abs().to_milliseconds(), 25);
            // REQUIRE(turbo::fiber_internal::get_task_control()->
            //             timer_thread()._idset.empty());
            REQUIRE_EQ(true, turbo::is_invalid_argument(fiber_stop(th)));
        }
        turbo::fiber_internal::waitable_event_destroy(event);
    }

    TEST_CASE("WaitableEventTest, stop_before_running") {
        int *event = turbo::fiber_internal::waitable_event_create_checked<int>();
        *event = 7;
        turbo::StopWatcher tm;
        const long WAIT_MSEC = 500;

        for (int i = 0; i < 2; ++i) {
            const FiberAttribute attr =
                    (i == 0 ? FIBER_ATTR_PTHREAD : FIBER_ATTR_NORMAL) | AttributeFlag::FLAG_NOSIGNAL;
            fiber_id_t th;
            event_wait_arg arg = {event, *event, WAIT_MSEC, turbo::kEINTR};

            tm.reset();
            REQUIRE_EQ(turbo::ok_status(), fiber_start_background(&th, &attr, wait_event, &arg));
            REQUIRE_EQ(turbo::ok_status(), fiber_stop(th));
            fiber_flush();
            REQUIRE(fiber_join(th, nullptr).ok());
            tm.stop();

            REQUIRE_LT(tm.elapsed_mill(), 5);
            // REQUIRE(turbo::fiber_internal::get_task_control()->
            //             timer_thread()._idset.empty());
            REQUIRE(turbo::is_invalid_argument(fiber_stop(th)));
        }
        turbo::fiber_internal::waitable_event_destroy(event);
    }

    void *join_the_waiter(void *arg) {
        auto rc = fiber_join((fiber_id_t) arg, nullptr);
        TLOG_INFO("join {} {}", arg, rc.to_string());
        REQUIRE(rc.ok());
        return nullptr;
    }

    TEST_CASE("WaitableEventTest, join_cant_be_wakeup") {
        const long WAIT_MSEC = 100;
        turbo::Duration WaitDuration = turbo::Duration::milliseconds(WAIT_MSEC);
        int *event = turbo::fiber_internal::waitable_event_create_checked<int>();
        *event = 7;
        turbo::StopWatcher tm;
        event_wait_arg arg = {event, *event, 1000, turbo::kEINTR};

        for (int i = 0; i < 2; ++i) {
            const FiberAttribute attr =
                    (i == 0 ? FIBER_ATTR_PTHREAD : FIBER_ATTR_NORMAL);
            tm.reset();
            fiber_id_t th, th2;
            REQUIRE_EQ(turbo::ok_status(), fiber_start_urgent(&th, nullptr, wait_event, &arg));
            REQUIRE_EQ(turbo::ok_status(), fiber_start_urgent(&th2, &attr, join_the_waiter, (void *) th));
            REQUIRE_EQ(turbo::ok_status(), fiber_stop(th2));
            REQUIRE_EQ(turbo::ok_status(), turbo::fiber_sleep_for(WaitDuration / 2));
            REQUIRE(turbo::fiber_internal::FiberWorker::exists(th));
            REQUIRE(turbo::fiber_internal::FiberWorker::exists(th2));
            REQUIRE_EQ(turbo::ok_status(), turbo::fiber_sleep_for(WaitDuration / 2));
            REQUIRE_EQ(turbo::ok_status(), fiber_stop(th));
            REQUIRE(fiber_join(th2, nullptr).ok());
            REQUIRE(fiber_join(th, nullptr).ok());
            tm.stop();
            REQUIRE_LT(tm.elapsed_mill(), WAIT_MSEC + 15);
            REQUIRE(turbo::is_invalid_argument(fiber_stop(th)));
            REQUIRE(turbo::is_invalid_argument(fiber_stop(th2)));
        }
        turbo::fiber_internal::waitable_event_destroy(event);
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
            fiber_id_t th;
            REQUIRE_EQ(turbo::ok_status(), fiber_start_urgent(
                    &th, &attr, sleeper, (void *) (SLEEP_MSEC * 1000L)));
            REQUIRE_EQ(turbo::ok_status(), turbo::fiber_sleep_for(WaitDuration));
            REQUIRE_EQ(turbo::ok_status(), fiber_stop(th));
            REQUIRE( fiber_join(th, nullptr).ok());
            tm.stop();
            if (is_pthread_stack(attr)) {
                REQUIRE_LT(labs(tm.elapsed_mill() - SLEEP_MSEC), 15);
            } else {
                REQUIRE_LT(labs(tm.elapsed_mill() - WAIT_MSEC), 15);
            }
            // REQUIRE(turbo::fiber_internal::get_task_control()->
            //             timer_thread()._idset.empty());
            REQUIRE(turbo::is_invalid_argument(fiber_stop(th)));
        }
    }

    TEST_CASE("WaitableEventTest, stop_just_when_sleeping") {
        turbo::StopWatcher tm;
        const long SLEEP_MSEC = 100;

        for (int i = 0; i < 2; ++i) {
            const FiberAttribute attr =
                    (i == 0 ? FIBER_ATTR_PTHREAD : FIBER_ATTR_NORMAL);
            tm.reset();
            fiber_id_t th;
            REQUIRE_EQ(turbo::ok_status(), fiber_start_urgent(
                    &th, &attr, sleeper, (void *) (SLEEP_MSEC * 1000L)));
            REQUIRE_EQ(turbo::ok_status(), fiber_stop(th));
            REQUIRE(fiber_join(th, nullptr).ok());
            tm.stop();
            if (is_pthread_stack(attr)) {
                REQUIRE_LT(labs(tm.elapsed_mill() - SLEEP_MSEC), 15);
            } else {
                REQUIRE_LT(tm.elapsed_mill(), 15);
            }
            // REQUIRE(turbo::fiber_internal::get_task_control()->
            //             timer_thread()._idset.empty());
            REQUIRE_EQ(turbo::is_invalid_argument(fiber_stop(th)), true);
        }
    }

    TEST_CASE("WaitableEventTest, stop_before_sleeping") {
        turbo::StopWatcher tm;
        const long SLEEP_MSEC = 100;

        for (int i = 0; i < 2; ++i) {
            fiber_id_t th;
            const FiberAttribute attr =
                    (i == 0 ? FIBER_ATTR_PTHREAD : FIBER_ATTR_NORMAL) | AttributeFlag::FLAG_NOSIGNAL;

            tm.reset();
            REQUIRE_EQ(turbo::ok_status(), fiber_start_background(&th, &attr, sleeper,
                                                 (void *) (SLEEP_MSEC * 1000L)));
            REQUIRE_EQ(turbo::ok_status(), fiber_stop(th));
            fiber_flush();
            REQUIRE(fiber_join(th, nullptr).ok());
            tm.stop();

            if (is_pthread_stack(attr)) {
                REQUIRE_LT(labs(tm.elapsed_mill() - SLEEP_MSEC), 10);
            } else {
                REQUIRE_LT(tm.elapsed_mill(), 10);
            }
            // REQUIRE(turbo::fiber_internal::get_task_control()->
            //             timer_thread()._idset.empty());
            REQUIRE_EQ(turbo::is_invalid_argument(fiber_stop(th)), true);
        }
    }

}  // namespace turbo::fiber_internal
