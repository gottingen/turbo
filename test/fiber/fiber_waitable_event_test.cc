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
#include "flare/base/static_atomic.h"
#include "flare/times/time.h"
#include "flare/log/logging.h"
#include "flare/fiber/internal/waitable_event.h"
#include "flare/fiber/internal/schedule_group.h"
#include "flare/fiber/internal/fiber_worker.h"
#include "flare/fiber/internal/fiber.h"
#include "flare/fiber/internal/unstable.h"
#include "flare/fiber/this_fiber.h"

namespace flare::fiber_internal {
    extern std::atomic<schedule_group *> g_task_control;

    inline schedule_group *get_task_control() {
        return g_task_control.load(std::memory_order_consume);
    }
} // namespace flare::fiber_internal

namespace {
    TEST(WaitableEventTest, wait_on_already_timedout_event) {
        uint32_t *event = flare::fiber_internal::waitable_event_create_checked<uint32_t>();
        ASSERT_TRUE(event);
        timespec now;
        ASSERT_EQ(0, clock_gettime(CLOCK_REALTIME, &now));
        *event = 1;
        ASSERT_EQ(-1, flare::fiber_internal::waitable_event_wait(event, 1, &now));
        ASSERT_EQ(ETIMEDOUT, errno);
    }

    void *sleeper(void *arg) {
        flare::fiber_sleep_for((uint64_t) arg);
        return nullptr;
    }

    void *joiner(void *arg) {
        const long t1 = flare::get_current_time_micros();
        for (fiber_id_t *th = (fiber_id_t *) arg; *th; ++th) {
            if (0 != fiber_join(*th, nullptr)) {
                FLARE_LOG(FATAL) << "fail to join thread_" << th - (fiber_id_t *) arg;
            }
            long elp = flare::get_current_time_micros() - t1;
            EXPECT_LE(labs(elp - (th - (fiber_id_t *) arg + 1) * 100000L), 15000L)
                                << "timeout when joining thread_" << th - (fiber_id_t *) arg;
            FLARE_LOG(INFO) << "Joined thread " << *th << " at " << elp << "us ["
                      << fiber_self() << "]";
        }
        for (fiber_id_t *th = (fiber_id_t *) arg; *th; ++th) {
            EXPECT_EQ(0, fiber_join(*th, nullptr));
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


    TEST(WaitableEventTest, with_or_without_array_zero) {
        ASSERT_EQ(sizeof(B), sizeof(A));
    }


    TEST(WaitableEventTest, join) {
        const size_t N = 6;
        const size_t M = 6;
        fiber_id_t th[N + 1];
        fiber_id_t jth[M];
        pthread_t pth[M];
        for (size_t i = 0; i < N; ++i) {
            fiber_attribute attr = (i == 0 ? FIBER_ATTR_PTHREAD : FIBER_ATTR_NORMAL);
            ASSERT_EQ(0, fiber_start_urgent(
                    &th[i], &attr, sleeper,
                    (void *) (100000L/*100ms*/ * (i + 1))));
        }
        th[N] = 0;  // joiner will join tids in `th' until seeing 0.
        for (size_t i = 0; i < M; ++i) {
            ASSERT_EQ(0, fiber_start_urgent(&jth[i], nullptr, joiner, th));
        }
        for (size_t i = 0; i < M; ++i) {
            ASSERT_EQ(0, pthread_create(&pth[i], nullptr, joiner, th));
        }

        for (size_t i = 0; i < M; ++i) {
            ASSERT_EQ(0, fiber_join(jth[i], nullptr))
                                        << "i=" << i << " error=" << flare_error();
        }
        for (size_t i = 0; i < M; ++i) {
            ASSERT_EQ(0, pthread_join(pth[i], nullptr));
        }
    }


    struct WaiterArg {
        int expected_result;
        int expected_value;
        std::atomic<int> *event;
        const timespec *ptimeout;
    };

    void *waiter(void *arg) {
        WaiterArg *wa = (WaiterArg *) arg;
        const long t1 = flare::get_current_time_micros();
        const int rc = flare::fiber_internal::waitable_event_wait(
                wa->event, wa->expected_value, wa->ptimeout);
        const long t2 = flare::get_current_time_micros();
        if (rc == 0) {
            EXPECT_EQ(wa->expected_result, 0) << fiber_self();
        } else {
            EXPECT_EQ(wa->expected_result, errno) << fiber_self();
        }
        FLARE_LOG(INFO) << "after wait, time=" << (t2 - t1) << "us";
        return nullptr;
    }

    TEST(WaitableEventTest, sanity) {
        const size_t N = 5;
        WaiterArg args[N * 4];
        pthread_t t1, t2;
        std::atomic<int> *b1 =
                flare::fiber_internal::waitable_event_create_checked<std::atomic<int> >();
        ASSERT_TRUE(b1);
        flare::fiber_internal::waitable_event_destroy(b1);

        b1 = flare::fiber_internal::waitable_event_create_checked<std::atomic<int> >();
        *b1 = 1;
        ASSERT_EQ(0, flare::fiber_internal::waitable_event_wake(b1));

        WaiterArg *unmatched_arg = new WaiterArg;
        unmatched_arg->expected_value = *b1 + 1;
        unmatched_arg->expected_result = EWOULDBLOCK;
        unmatched_arg->event = b1;
        unmatched_arg->ptimeout = nullptr;
        pthread_create(&t2, nullptr, waiter, unmatched_arg);
        fiber_id_t th;
        ASSERT_EQ(0, fiber_start_urgent(&th, nullptr, waiter, unmatched_arg));

        const timespec abstime = flare::time_point::future_unix_seconds(1).to_timespec();
        for (size_t i = 0; i < 4 * N; ++i) {
            args[i].expected_value = *b1;
            args[i].event = b1;
            if ((i % 2) == 0) {
                args[i].expected_result = 0;
                args[i].ptimeout = nullptr;
            } else {
                args[i].expected_result = ETIMEDOUT;
                args[i].ptimeout = &abstime;
            }
            if (i < 2 * N) {
                pthread_create(&t1, nullptr, waiter, &args[i]);
            } else {
                ASSERT_EQ(0, fiber_start_urgent(&th, nullptr, waiter, &args[i]));
            }
        }

        sleep(2);
        for (size_t i = 0; i < 2 * N; ++i) {
            ASSERT_EQ(1, flare::fiber_internal::waitable_event_wake(b1));
        }
        ASSERT_EQ(0, flare::fiber_internal::waitable_event_wake(b1));
        sleep(1);
        flare::fiber_internal::waitable_event_destroy(b1);
    }


    struct event_wait_arg {
        int *event;
        int expected_val;
        long wait_msec;
        int error_code;
    };

    void *wait_event(void *void_arg) {
        event_wait_arg *arg = static_cast<event_wait_arg *>(void_arg);
        const timespec ts = flare::time_point::future_unix_millis(arg->wait_msec).to_timespec();
        int rc = flare::fiber_internal::waitable_event_wait(arg->event, arg->expected_val, &ts);
        int saved_errno = errno;
        if (arg->error_code) {
            EXPECT_EQ(-1, rc);
            EXPECT_EQ(arg->error_code, saved_errno);
        } else {
            EXPECT_EQ(0, rc);
        }
        return nullptr;
    }

    TEST(WaitableEventTest, wait_without_stop) {
        int *event = flare::fiber_internal::waitable_event_create_checked<int>();
        *event = 7;
        flare::stop_watcher tm;
        const long WAIT_MSEC = 500;
        for (int i = 0; i < 2; ++i) {
            const fiber_attribute attr =
                    (i == 0 ? FIBER_ATTR_PTHREAD : FIBER_ATTR_NORMAL);
            event_wait_arg arg = {event, *event, WAIT_MSEC, ETIMEDOUT};
            fiber_id_t th;

            tm.start();
            ASSERT_EQ(0, fiber_start_urgent(&th, &attr, wait_event, &arg));
            ASSERT_EQ(0, fiber_join(th, nullptr));
            tm.stop();

            ASSERT_LT(labs(tm.m_elapsed() - WAIT_MSEC), 250);
        }
        flare::fiber_internal::waitable_event_destroy(event);
    }

    TEST(WaitableEventTest, stop_after_running) {
        int *event = flare::fiber_internal::waitable_event_create_checked<int>();
        *event = 7;
        flare::stop_watcher tm;
        const long WAIT_MSEC = 500;
        const long SLEEP_MSEC = 10;
        for (int i = 0; i < 2; ++i) {
            const fiber_attribute attr =
                    (i == 0 ? FIBER_ATTR_PTHREAD : FIBER_ATTR_NORMAL);
            fiber_id_t th;
            event_wait_arg arg = {event, *event, WAIT_MSEC, EINTR};

            tm.start();
            ASSERT_EQ(0, fiber_start_urgent(&th, &attr, wait_event, &arg));
            ASSERT_EQ(0, flare::fiber_sleep_for(SLEEP_MSEC * 1000L));
            ASSERT_EQ(0, fiber_stop(th));
            ASSERT_EQ(0, fiber_join(th, nullptr));
            tm.stop();

            ASSERT_LT(labs(tm.m_elapsed() - SLEEP_MSEC), 25);
            // ASSERT_TRUE(flare::fiber_internal::get_task_control()->
            //             timer_thread()._idset.empty());
            ASSERT_EQ(EINVAL, fiber_stop(th));
        }
        flare::fiber_internal::waitable_event_destroy(event);
    }

    TEST(WaitableEventTest, stop_before_running) {
        int *event = flare::fiber_internal::waitable_event_create_checked<int>();
        *event = 7;
        flare::stop_watcher tm;
        const long WAIT_MSEC = 500;

        for (int i = 0; i < 2; ++i) {
            const fiber_attribute attr =
                    (i == 0 ? FIBER_ATTR_PTHREAD : FIBER_ATTR_NORMAL) | FIBER_NOSIGNAL;
            fiber_id_t th;
            event_wait_arg arg = {event, *event, WAIT_MSEC, EINTR};

            tm.start();
            ASSERT_EQ(0, fiber_start_background(&th, &attr, wait_event, &arg));
            ASSERT_EQ(0, fiber_stop(th));
            fiber_flush();
            ASSERT_EQ(0, fiber_join(th, nullptr));
            tm.stop();

            ASSERT_LT(tm.m_elapsed(), 5);
            // ASSERT_TRUE(flare::fiber_internal::get_task_control()->
            //             timer_thread()._idset.empty());
            ASSERT_EQ(EINVAL, fiber_stop(th));
        }
        flare::fiber_internal::waitable_event_destroy(event);
    }

    void *join_the_waiter(void *arg) {
        EXPECT_EQ(0, fiber_join((fiber_id_t) arg, nullptr));
        return nullptr;
    }

    TEST(WaitableEventTest, join_cant_be_wakeup) {
        const long WAIT_MSEC = 100;
        int *event = flare::fiber_internal::waitable_event_create_checked<int>();
        *event = 7;
        flare::stop_watcher tm;
        event_wait_arg arg = {event, *event, 1000, EINTR};

        for (int i = 0; i < 2; ++i) {
            const fiber_attribute attr =
                    (i == 0 ? FIBER_ATTR_PTHREAD : FIBER_ATTR_NORMAL);
            tm.start();
            fiber_id_t th, th2;
            ASSERT_EQ(0, fiber_start_urgent(&th, nullptr, wait_event, &arg));
            ASSERT_EQ(0, fiber_start_urgent(&th2, &attr, join_the_waiter, (void *) th));
            ASSERT_EQ(0, fiber_stop(th2));
            ASSERT_EQ(0, flare::fiber_sleep_for(WAIT_MSEC / 2 * 1000L));
            ASSERT_TRUE(flare::fiber_internal::fiber_worker::exists(th));
            ASSERT_TRUE(flare::fiber_internal::fiber_worker::exists(th2));
            ASSERT_EQ(0, flare::fiber_sleep_for(WAIT_MSEC / 2 * 1000L));
            ASSERT_EQ(0, fiber_stop(th));
            ASSERT_EQ(0, fiber_join(th2, nullptr));
            ASSERT_EQ(0, fiber_join(th, nullptr));
            tm.stop();
            ASSERT_LT(tm.m_elapsed(), WAIT_MSEC + 15);
            ASSERT_EQ(EINVAL, fiber_stop(th));
            ASSERT_EQ(EINVAL, fiber_stop(th2));
        }
        flare::fiber_internal::waitable_event_destroy(event);
    }

    TEST(WaitableEventTest, stop_after_slept) {
        flare::stop_watcher tm;
        const long SLEEP_MSEC = 100;
        const long WAIT_MSEC = 10;

        for (int i = 0; i < 2; ++i) {
            const fiber_attribute attr =
                    (i == 0 ? FIBER_ATTR_PTHREAD : FIBER_ATTR_NORMAL);
            tm.start();
            fiber_id_t th;
            ASSERT_EQ(0, fiber_start_urgent(
                    &th, &attr, sleeper, (void *) (SLEEP_MSEC * 1000L)));
            ASSERT_EQ(0, flare::fiber_sleep_for(WAIT_MSEC * 1000L));
            ASSERT_EQ(0, fiber_stop(th));
            ASSERT_EQ(0, fiber_join(th, nullptr));
            tm.stop();
            if (attr.stack_type == FIBER_STACKTYPE_PTHREAD) {
                ASSERT_LT(labs(tm.m_elapsed() - SLEEP_MSEC), 15);
            } else {
                ASSERT_LT(labs(tm.m_elapsed() - WAIT_MSEC), 15);
            }
            // ASSERT_TRUE(flare::fiber_internal::get_task_control()->
            //             timer_thread()._idset.empty());
            ASSERT_EQ(EINVAL, fiber_stop(th));
        }
    }

    TEST(WaitableEventTest, stop_just_when_sleeping) {
        flare::stop_watcher tm;
        const long SLEEP_MSEC = 100;

        for (int i = 0; i < 2; ++i) {
            const fiber_attribute attr =
                    (i == 0 ? FIBER_ATTR_PTHREAD : FIBER_ATTR_NORMAL);
            tm.start();
            fiber_id_t th;
            ASSERT_EQ(0, fiber_start_urgent(
                    &th, &attr, sleeper, (void *) (SLEEP_MSEC * 1000L)));
            ASSERT_EQ(0, fiber_stop(th));
            ASSERT_EQ(0, fiber_join(th, nullptr));
            tm.stop();
            if (attr.stack_type == FIBER_STACKTYPE_PTHREAD) {
                ASSERT_LT(labs(tm.m_elapsed() - SLEEP_MSEC), 15);
            } else {
                ASSERT_LT(tm.m_elapsed(), 15);
            }
            // ASSERT_TRUE(flare::fiber_internal::get_task_control()->
            //             timer_thread()._idset.empty());
            ASSERT_EQ(EINVAL, fiber_stop(th));
        }
    }

    TEST(WaitableEventTest, stop_before_sleeping) {
        flare::stop_watcher tm;
        const long SLEEP_MSEC = 100;

        for (int i = 0; i < 2; ++i) {
            fiber_id_t th;
            const fiber_attribute attr =
                    (i == 0 ? FIBER_ATTR_PTHREAD : FIBER_ATTR_NORMAL) | FIBER_NOSIGNAL;

            tm.start();
            ASSERT_EQ(0, fiber_start_background(&th, &attr, sleeper,
                                                (void *) (SLEEP_MSEC * 1000L)));
            ASSERT_EQ(0, fiber_stop(th));
            fiber_flush();
            ASSERT_EQ(0, fiber_join(th, nullptr));
            tm.stop();

            if (attr.stack_type == FIBER_STACKTYPE_PTHREAD) {
                ASSERT_LT(labs(tm.m_elapsed() - SLEEP_MSEC), 10);
            } else {
                ASSERT_LT(tm.m_elapsed(), 10);
            }
            // ASSERT_TRUE(flare::fiber_internal::get_task_control()->
            //             timer_thread()._idset.empty());
            ASSERT_EQ(EINVAL, fiber_stop(th));
        }
    }
} // namespace
