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

#include <iostream>
#include "testing/gtest_wrap.h"
#include "flare/times/time.h"
#include "flare/fiber/internal/fiber.h"
#include "flare/fiber/internal/fiber_worker.h"
#include "flare/fiber/internal/waitable_event.h"
#include "flare/fiber/this_fiber.h"

namespace flare::fiber_internal {
    void token_status(fiber_token_t, std::ostream &);

    uint32_t token_value(fiber_token_t id);
}

namespace {
    inline uint32_t get_version(fiber_token_t id) {
        return (uint32_t) (id.value & 0xFFFFFFFFul);
    }

    struct SignalArg {
        fiber_token_t id;
        long sleep_us_before_fight;
        long sleep_us_before_signal;
    };

    void *signaller(void *void_arg) {
        SignalArg arg = *(SignalArg *) void_arg;
        flare::fiber_sleep_for(arg.sleep_us_before_fight);
        void *data = nullptr;
        int rc = fiber_token_trylock(arg.id, &data);
        if (rc == 0) {
            EXPECT_EQ(0xdead, *(int *) data);
            ++*(int *) data;
            //EXPECT_EQ(EBUSY, fiber_token_destroy(arg.id, ECANCELED));
            flare::fiber_sleep_for(arg.sleep_us_before_signal);
            EXPECT_EQ(0, fiber_token_unlock_and_destroy(arg.id));
            return void_arg;
        } else {
            EXPECT_TRUE(EBUSY == rc || EINVAL == rc);
            return nullptr;
        }
    }

    TEST(FiberTokenTest, join_after_destroy) {
        fiber_token_t id1;
        int x = 0xdead;
        ASSERT_EQ(0, fiber_token_create_ranged(&id1, &x, nullptr, 2));
        fiber_token_t id2 = {id1.value + 1};
        ASSERT_EQ(get_version(id1), flare::fiber_internal::token_value(id1));
        ASSERT_EQ(get_version(id1), flare::fiber_internal::token_value(id2));
        pthread_t th[8];
        SignalArg args[FLARE_ARRAY_SIZE(th)];
        for (size_t i = 0; i < FLARE_ARRAY_SIZE(th); ++i) {
            args[i].sleep_us_before_fight = 0;
            args[i].sleep_us_before_signal = 0;
            args[i].id = (i == 0 ? id1 : id2);
            ASSERT_EQ(0, pthread_create(&th[i], nullptr, signaller, &args[i]));
        }
        void *ret[FLARE_ARRAY_SIZE(th)];
        size_t non_null_ret = 0;
        for (size_t i = 0; i < FLARE_ARRAY_SIZE(th); ++i) {
            ASSERT_EQ(0, pthread_join(th[i], &ret[i]));
            non_null_ret += (ret[i] != nullptr);
        }
        ASSERT_EQ(1UL, non_null_ret);
        ASSERT_EQ(0, fiber_token_join(id1));
        ASSERT_EQ(0, fiber_token_join(id2));
        ASSERT_EQ(0xdead + 1, x);
        ASSERT_EQ(get_version(id1) + 5, flare::fiber_internal::token_value(id1));
        ASSERT_EQ(get_version(id1) + 5, flare::fiber_internal::token_value(id2));
    }

    TEST(FiberTokenTest, join_before_destroy) {
        fiber_token_t id1;
        int x = 0xdead;
        ASSERT_EQ(0, fiber_token_create(&id1, &x, nullptr));
        ASSERT_EQ(get_version(id1), flare::fiber_internal::token_value(id1));
        pthread_t th[8];
        SignalArg args[FLARE_ARRAY_SIZE(th)];
        for (size_t i = 0; i < FLARE_ARRAY_SIZE(th); ++i) {
            args[i].sleep_us_before_fight = 10000;
            args[i].sleep_us_before_signal = 0;
            args[i].id = id1;
            ASSERT_EQ(0, pthread_create(&th[i], nullptr, signaller, &args[i]));
        }
        ASSERT_EQ(0, fiber_token_join(id1));
        ASSERT_EQ(0xdead + 1, x);
        ASSERT_EQ(get_version(id1) + 4, flare::fiber_internal::token_value(id1));

        void *ret[FLARE_ARRAY_SIZE(th)];
        size_t non_null_ret = 0;
        for (size_t i = 0; i < FLARE_ARRAY_SIZE(th); ++i) {
            ASSERT_EQ(0, pthread_join(th[i], &ret[i]));
            non_null_ret += (ret[i] != nullptr);
        }
        ASSERT_EQ(1UL, non_null_ret);
    }

    struct OnResetArg {
        fiber_token_t id;
        int error_code;
    };

    int on_reset(fiber_token_t id, void *data, int error_code) {
        OnResetArg *arg = static_cast<OnResetArg *>(data);
        arg->id = id;
        arg->error_code = error_code;
        return fiber_token_unlock_and_destroy(id);
    }

    TEST(FiberTokenTest, error_is_destroy) {
        fiber_token_t id1;
        OnResetArg arg = {{0}, 0};
        ASSERT_EQ(0, fiber_token_create(&id1, &arg, on_reset));
        ASSERT_EQ(get_version(id1), flare::fiber_internal::token_value(id1));
        ASSERT_EQ(0, fiber_token_error(id1, EBADF));
        ASSERT_EQ(EBADF, arg.error_code);
        ASSERT_EQ(id1.value, arg.id.value);
        ASSERT_EQ(get_version(id1) + 4, flare::fiber_internal::token_value(id1));
    }

    TEST(FiberTokenTest, error_is_destroy_ranged) {
        fiber_token_t id1;
        OnResetArg arg = {{0}, 0};
        ASSERT_EQ(0, fiber_token_create_ranged(&id1, &arg, on_reset, 2));
        fiber_token_t id2 = {id1.value + 1};
        ASSERT_EQ(get_version(id1), flare::fiber_internal::token_value(id2));
        ASSERT_EQ(0, fiber_token_error(id2, EBADF));
        ASSERT_EQ(EBADF, arg.error_code);
        ASSERT_EQ(id2.value, arg.id.value);
        ASSERT_EQ(get_version(id1) + 5, flare::fiber_internal::token_value(id2));
    }

    TEST(FiberTokenTest, default_error_is_destroy) {
        fiber_token_t id1;
        ASSERT_EQ(0, fiber_token_create(&id1, nullptr, nullptr));
        ASSERT_EQ(get_version(id1), flare::fiber_internal::token_value(id1));
        ASSERT_EQ(0, fiber_token_error(id1, EBADF));
        ASSERT_EQ(get_version(id1) + 4, flare::fiber_internal::token_value(id1));
    }

    TEST(FiberTokenTest, doubly_destroy) {
        fiber_token_t id1;
        ASSERT_EQ(0, fiber_token_create_ranged(&id1, nullptr, nullptr, 2));
        fiber_token_t id2 = {id1.value + 1};
        ASSERT_EQ(get_version(id1), flare::fiber_internal::token_value(id1));
        ASSERT_EQ(get_version(id1), flare::fiber_internal::token_value(id2));
        ASSERT_EQ(0, fiber_token_error(id1, EBADF));
        ASSERT_EQ(get_version(id1) + 5, flare::fiber_internal::token_value(id1));
        ASSERT_EQ(get_version(id1) + 5, flare::fiber_internal::token_value(id2));
        ASSERT_EQ(EINVAL, fiber_token_error(id1, EBADF));
        ASSERT_EQ(EINVAL, fiber_token_error(id2, EBADF));
    }

    static int on_numeric_error(fiber_token_t id, void *data, int error_code) {
        std::vector<int> *result = static_cast<std::vector<int> *>(data);
        result->push_back(error_code);
        EXPECT_EQ(0, fiber_token_unlock(id));
        return 0;
    }

    TEST(FiberTokenTest, many_error) {
        fiber_token_t id1;
        std::vector<int> result;
        ASSERT_EQ(0, fiber_token_create(&id1, &result, on_numeric_error));
        ASSERT_EQ(get_version(id1), flare::fiber_internal::token_value(id1));
        int err = 0;
        const int N = 100;
        for (int i = 0; i < N; ++i) {
            ASSERT_EQ(0, fiber_token_error(id1, err++));
        }
        ASSERT_EQ((size_t) N, result.size());
        for (int i = 0; i < N; ++i) {
            ASSERT_EQ(i, result[i]);
        }
        ASSERT_EQ(0, fiber_token_trylock(id1, nullptr));
        ASSERT_EQ(get_version(id1) + 1, flare::fiber_internal::token_value(id1));
        for (int i = 0; i < N; ++i) {
            ASSERT_EQ(0, fiber_token_error(id1, err++));
        }
        ASSERT_EQ((size_t) N, result.size());
        ASSERT_EQ(0, fiber_token_unlock(id1));
        ASSERT_EQ(get_version(id1), flare::fiber_internal::token_value(id1));
        ASSERT_EQ((size_t) 2 * N, result.size());
        for (int i = 0; i < 2 * N; ++i) {
            EXPECT_EQ(i, result[i]);
        }
        result.clear();

        ASSERT_EQ(0, fiber_token_trylock(id1, nullptr));
        ASSERT_EQ(get_version(id1) + 1, flare::fiber_internal::token_value(id1));
        for (int i = 0; i < N; ++i) {
            ASSERT_EQ(0, fiber_token_error(id1, err++));
        }
        ASSERT_EQ(0, fiber_token_unlock_and_destroy(id1));
        ASSERT_TRUE(result.empty());
    }

    static void *locker(void *arg) {
        fiber_token_t id = {(uintptr_t) arg};
        flare::stop_watcher tm;
        tm.start();
        EXPECT_EQ(0, fiber_token_lock(id, nullptr));
        flare::fiber_sleep_for(2000);
        EXPECT_EQ(0, fiber_token_unlock(id));
        tm.stop();
        FLARE_LOG(INFO) << "Unlocked, tm=" << tm.u_elapsed();
        return nullptr;
    }

    TEST(FiberTokenTest, id_lock) {
        fiber_token_t id1;
        ASSERT_EQ(0, fiber_token_create(&id1, nullptr, nullptr));
        ASSERT_EQ(get_version(id1), flare::fiber_internal::token_value(id1));
        pthread_t th[8];
        for (size_t i = 0; i < FLARE_ARRAY_SIZE(th); ++i) {
            ASSERT_EQ(0, pthread_create(&th[i], nullptr, locker,
                                        (void *) id1.value));
        }
        for (size_t i = 0; i < FLARE_ARRAY_SIZE(th); ++i) {
            ASSERT_EQ(0, pthread_join(th[i], nullptr));
        }
    }

    static void *failed_locker(void *arg) {
        fiber_token_t id = {(uintptr_t) arg};
        int rc = fiber_token_lock(id, nullptr);
        if (rc == 0) {
            flare::fiber_sleep_for(2000);
            EXPECT_EQ(0, fiber_token_unlock_and_destroy(id));
            return (void *) 1;
        } else {
            EXPECT_EQ(EINVAL, rc);
            return nullptr;
        }
    }

    TEST(FiberTokenTest, id_lock_and_destroy) {
        fiber_token_t id1;
        ASSERT_EQ(0, fiber_token_create(&id1, nullptr, nullptr));
        ASSERT_EQ(get_version(id1), flare::fiber_internal::token_value(id1));
        pthread_t th[8];
        for (size_t i = 0; i < FLARE_ARRAY_SIZE(th); ++i) {
            ASSERT_EQ(0, pthread_create(&th[i], nullptr, failed_locker,
                                        (void *) id1.value));
        }
        int non_null = 0;
        for (size_t i = 0; i < FLARE_ARRAY_SIZE(th); ++i) {
            void *ret = nullptr;
            ASSERT_EQ(0, pthread_join(th[i], &ret));
            non_null += (ret != nullptr);
        }
        ASSERT_EQ(1, non_null);
    }

    TEST(FiberTokenTest, join_after_destroy_before_unlock) {
        fiber_token_t id1;
        int x = 0xdead;
        ASSERT_EQ(0, fiber_token_create(&id1, &x, nullptr));
        ASSERT_EQ(get_version(id1), flare::fiber_internal::token_value(id1));
        pthread_t th[8];
        SignalArg args[FLARE_ARRAY_SIZE(th)];
        for (size_t i = 0; i < FLARE_ARRAY_SIZE(th); ++i) {
            args[i].sleep_us_before_fight = 0;
            args[i].sleep_us_before_signal = 20000;
            args[i].id = id1;
            ASSERT_EQ(0, pthread_create(&th[i], nullptr, signaller, &args[i]));
        }
        flare::fiber_sleep_for(10000);
        // join() waits until destroy() is called.
        ASSERT_EQ(0, fiber_token_join(id1));
        ASSERT_EQ(0xdead + 1, x);
        ASSERT_EQ(get_version(id1) + 4, flare::fiber_internal::token_value(id1));

        void *ret[FLARE_ARRAY_SIZE(th)];
        size_t non_null_ret = 0;
        for (size_t i = 0; i < FLARE_ARRAY_SIZE(th); ++i) {
            ASSERT_EQ(0, pthread_join(th[i], &ret[i]));
            non_null_ret += (ret[i] != nullptr);
        }
        ASSERT_EQ(1UL, non_null_ret);
    }

    struct StoppedWaiterArgs {
        fiber_token_t id;
        bool thread_started;
    };

    void *stopped_waiter(void *void_arg) {
        StoppedWaiterArgs *args = (StoppedWaiterArgs *) void_arg;
        args->thread_started = true;
        EXPECT_EQ(0, fiber_token_join(args->id));
        EXPECT_EQ(get_version(args->id) + 4, flare::fiber_internal::token_value(args->id));
        return nullptr;
    }

    TEST(FiberTokenTest, stop_a_wait_after_fight_before_signal) {
        fiber_token_t id1;
        int x = 0xdead;
        ASSERT_EQ(0, fiber_token_create(&id1, &x, nullptr));
        ASSERT_EQ(get_version(id1), flare::fiber_internal::token_value(id1));
        void *data;
        ASSERT_EQ(0, fiber_token_trylock(id1, &data));
        ASSERT_EQ(&x, data);
        fiber_id_t th[8];
        StoppedWaiterArgs args[FLARE_ARRAY_SIZE(th)];
        for (size_t i = 0; i < FLARE_ARRAY_SIZE(th); ++i) {
            args[i].id = id1;
            args[i].thread_started = false;
            ASSERT_EQ(0, fiber_start_urgent(&th[i], nullptr, stopped_waiter, &args[i]));
        }
        // stop does not wake up fiber_token_join
        for (size_t i = 0; i < FLARE_ARRAY_SIZE(th); ++i) {
            fiber_stop(th[i]);
        }
        flare::fiber_sleep_for(10000);
        for (size_t i = 0; i < FLARE_ARRAY_SIZE(th); ++i) {
            ASSERT_TRUE(flare::fiber_internal::fiber_worker::exists(th[i]));
        }
        // destroy the id to end the joinings.
        ASSERT_EQ(0, fiber_token_unlock_and_destroy(id1));
        for (size_t i = 0; i < FLARE_ARRAY_SIZE(th); ++i) {
            ASSERT_EQ(0, fiber_join(th[i], nullptr));
        }
    }

    void *waiter(void *arg) {
        fiber_token_t id = {(uintptr_t) arg};
        EXPECT_EQ(0, fiber_token_join(id));
        EXPECT_EQ(get_version(id) + 4, flare::fiber_internal::token_value(id));
        return nullptr;
    }

    int handle_data(fiber_token_t id, void *data, int error_code) {
        EXPECT_EQ(EBADF, error_code);
        ++*(int *) data;
        EXPECT_EQ(0, fiber_token_unlock_and_destroy(id));
        return 0;
    }

    TEST(FiberTokenTest, list_signal) {
        fiber_token_list_t list;
        ASSERT_EQ(0, fiber_token_list_init(&list, 32, 32));
        fiber_token_t id[16];
        int data[FLARE_ARRAY_SIZE(id)];
        for (size_t i = 0; i < FLARE_ARRAY_SIZE(id); ++i) {
            data[i] = i;
            ASSERT_EQ(0, fiber_token_create(&id[i], &data[i], handle_data));
            ASSERT_EQ(get_version(id[i]), flare::fiber_internal::token_value(id[i]));
            ASSERT_EQ(0, fiber_token_list_add(&list, id[i]));
        }
        pthread_t th[FLARE_ARRAY_SIZE(id)];
        for (size_t i = 0; i < FLARE_ARRAY_SIZE(th); ++i) {
            ASSERT_EQ(0, pthread_create(&th[i], nullptr, waiter, (void *) (intptr_t) id[i].value));
        }
        flare::fiber_sleep_for(10000);
        ASSERT_EQ(0, fiber_token_list_reset(&list, EBADF));

        for (size_t i = 0; i < FLARE_ARRAY_SIZE(th); ++i) {
            ASSERT_EQ((int) (i + 1), data[i]);
            ASSERT_EQ(0, pthread_join(th[i], nullptr));
            // already reset.
            ASSERT_EQ((int) (i + 1), data[i]);
        }

        fiber_token_list_destroy(&list);
    }

    int error_without_unlock(fiber_token_t, void *, int) {
        return 0;
    }

    TEST(FiberTokenTest, status) {
        fiber_token_t id;
        fiber_token_create(&id, nullptr, nullptr);
        flare::fiber_internal::token_status(id, std::cout);
        fiber_token_lock(id, nullptr);
        fiber_token_error(id, 123);
        fiber_token_error(id, 256);
        fiber_token_error(id, 1256);
        flare::fiber_internal::token_status(id, std::cout);
        fiber_token_unlock_and_destroy(id);
        fiber_token_create(&id, nullptr, error_without_unlock);
        fiber_token_lock(id, nullptr);
        flare::fiber_internal::token_status(id, std::cout);
        fiber_token_error(id, 12);
        flare::fiber_internal::token_status(id, std::cout);
        fiber_token_unlock(id);
        flare::fiber_internal::token_status(id, std::cout);
        fiber_token_unlock_and_destroy(id);
    }

    TEST(FiberTokenTest, reset_range) {
        fiber_token_t id;
        ASSERT_EQ(0, fiber_token_create(&id, nullptr, nullptr));
        ASSERT_EQ(0, fiber_token_lock_and_reset_range(id, nullptr, 1000));
        flare::fiber_internal::token_status(id, std::cout);
        fiber_token_unlock(id);
        ASSERT_EQ(0, fiber_token_lock_and_reset_range(id, nullptr, 300));
        flare::fiber_internal::token_status(id, std::cout);
        fiber_token_unlock_and_destroy(id);
    }

    static bool any_thread_quit = false;

    struct FailToLockIdArgs {
        fiber_token_t id;
        int expected_return;
    };

    static void *fail_to_lock_id(void *args_in) {
        FailToLockIdArgs *args = (FailToLockIdArgs *) args_in;
        flare::stop_watcher tm;
        EXPECT_EQ(args->expected_return, fiber_token_lock(args->id, nullptr));
        any_thread_quit = true;
        return nullptr;
    }

    TEST(FiberTokenTest, about_to_destroy_before_locking) {
        fiber_token_t id;
        ASSERT_EQ(0, fiber_token_create(&id, nullptr, nullptr));
        ASSERT_EQ(0, fiber_token_lock(id, nullptr));
        ASSERT_EQ(0, fiber_token_about_to_destroy(id));
        pthread_t pth;
        fiber_id_t bth;
        FailToLockIdArgs args = {id, EPERM};
        ASSERT_EQ(0, pthread_create(&pth, nullptr, fail_to_lock_id, &args));
        ASSERT_EQ(0, fiber_start_background(&bth, nullptr, fail_to_lock_id, &args));
        // The threads should quit soon.
        pthread_join(pth, nullptr);
        fiber_join(bth, nullptr);
        flare::fiber_internal::token_status(id, std::cout);
        ASSERT_EQ(0, fiber_token_unlock_and_destroy(id));
    }

    static void *succeed_to_lock_id(void *arg) {
        fiber_token_t id = *(fiber_token_t *) arg;
        EXPECT_EQ(0, fiber_token_lock(id, nullptr));
        EXPECT_EQ(0, fiber_token_unlock(id));
        return nullptr;
    }

    TEST(FiberTokenTest, about_to_destroy_cancelled) {
        fiber_token_t id;
        ASSERT_EQ(0, fiber_token_create(&id, nullptr, nullptr));
        ASSERT_EQ(0, fiber_token_lock(id, nullptr));
        ASSERT_EQ(0, fiber_token_about_to_destroy(id));
        ASSERT_EQ(0, fiber_token_unlock(id));
        pthread_t pth;
        fiber_id_t bth;
        ASSERT_EQ(0, pthread_create(&pth, nullptr, succeed_to_lock_id, &id));
        ASSERT_EQ(0, fiber_start_background(&bth, nullptr, succeed_to_lock_id, &id));
        // The threads should quit soon.
        pthread_join(pth, nullptr);
        fiber_join(bth, nullptr);
        flare::fiber_internal::token_status(id, std::cout);
        ASSERT_EQ(0, fiber_token_lock(id, nullptr));
        ASSERT_EQ(0, fiber_token_unlock_and_destroy(id));
    }

    TEST(FiberTokenTest, about_to_destroy_during_locking) {
        fiber_token_t id;
        ASSERT_EQ(0, fiber_token_create(&id, nullptr, nullptr));
        ASSERT_EQ(0, fiber_token_lock(id, nullptr));
        any_thread_quit = false;
        pthread_t pth;
        fiber_id_t bth;
        FailToLockIdArgs args = {id, EPERM};
        ASSERT_EQ(0, pthread_create(&pth, nullptr, fail_to_lock_id, &args));
        ASSERT_EQ(0, fiber_start_background(&bth, nullptr, fail_to_lock_id, &args));

        usleep(100000);
        ASSERT_FALSE(any_thread_quit);
        ASSERT_EQ(0, fiber_token_about_to_destroy(id));

        // The threads should quit soon.
        pthread_join(pth, nullptr);
        fiber_join(bth, nullptr);
        flare::fiber_internal::token_status(id, std::cout);
        ASSERT_EQ(0, fiber_token_unlock_and_destroy(id));
    }

    void *const DUMMY_DATA1 = (void *) 1;
    void *const DUMMY_DATA2 = (void *) 2;
    int branch_counter = 0;
    int branch_tags[4] = {};
    int expected_code = 0;
    const char *expected_desc = "";

    int handler_without_desc(fiber_token_t id, void *data, int error_code) {
        EXPECT_EQ(DUMMY_DATA1, data);
        EXPECT_EQ(expected_code, error_code);
        if (error_code == ESTOP) {
            branch_tags[0] = branch_counter;
            return fiber_token_unlock_and_destroy(id);
        } else {
            branch_tags[1] = branch_counter;
            return fiber_token_unlock(id);
        }
    }

    int handler_with_desc(fiber_token_t id, void *data, int error_code,
                          const std::string &error_text) {
        EXPECT_EQ(DUMMY_DATA2, data);
        EXPECT_EQ(expected_code, error_code);
        EXPECT_STREQ(expected_desc, error_text.c_str());
        if (error_code == ESTOP) {
            branch_tags[2] = branch_counter;
            return fiber_token_unlock_and_destroy(id);
        } else {
            branch_tags[3] = branch_counter;
            return fiber_token_unlock(id);
        }
    }

    TEST(FiberTokenTest, error_with_descriptions) {
        fiber_token_t id1;
        ASSERT_EQ(0, fiber_token_create(&id1, DUMMY_DATA1, handler_without_desc));
        fiber_token_t id2;
        ASSERT_EQ(0, fiber_token_create2(&id2, DUMMY_DATA2, handler_with_desc));

        // [ Matched in-place ]
        // Call fiber_token_error on an id created by fiber_token_create
        ++branch_counter;
        expected_code = EINVAL;
        ASSERT_EQ(0, fiber_token_error(id1, expected_code));
        ASSERT_EQ(branch_counter, branch_tags[1]);

        // Call fiber_token_error2 on an id created by fiber_token_create2
        ++branch_counter;
        expected_code = EPERM;
        expected_desc = "description1";
        ASSERT_EQ(0, fiber_token_error2(id2, expected_code, expected_desc));
        ASSERT_EQ(branch_counter, branch_tags[3]);

        // [ Mixed in-place ]
        // Call fiber_token_error on an id created by fiber_token_create2
        ++branch_counter;
        expected_code = ECONNREFUSED;
        expected_desc = "";
        ASSERT_EQ(0, fiber_token_error(id2, expected_code));
        ASSERT_EQ(branch_counter, branch_tags[3]);
        // Call fiber_token_error2 on an id created by fiber_token_create
        ++branch_counter;
        expected_code = EINTR;
        ASSERT_EQ(0, fiber_token_error2(id1, expected_code, ""));
        ASSERT_EQ(branch_counter, branch_tags[1]);

        // [ Matched pending ]
        // Call fiber_token_error on an id created by fiber_token_create
        ++branch_counter;
        expected_code = ECONNRESET;
        ASSERT_EQ(0, fiber_token_lock(id1, nullptr));
        ASSERT_EQ(0, fiber_token_error(id1, expected_code));
        ASSERT_EQ(0, fiber_token_unlock(id1));
        ASSERT_EQ(branch_counter, branch_tags[1]);

        // Call fiber_token_error2 on an id created by fiber_token_create2
        ++branch_counter;
        expected_code = ENOSPC;
        expected_desc = "description3";
        ASSERT_EQ(0, fiber_token_lock(id2, nullptr));
        ASSERT_EQ(0, fiber_token_error2(id2, expected_code, expected_desc));
        ASSERT_EQ(0, fiber_token_unlock(id2));
        ASSERT_EQ(branch_counter, branch_tags[3]);

        // [ Mixed pending ]
        // Call fiber_token_error on an id created by fiber_token_create2
        ++branch_counter;
        expected_code = ESTOP;
        expected_desc = "";
        ASSERT_EQ(0, fiber_token_lock(id2, nullptr));
        ASSERT_EQ(0, fiber_token_error(id2, expected_code));
        ASSERT_EQ(0, fiber_token_unlock(id2));
        ASSERT_EQ(branch_counter, branch_tags[2]);
        // Call fiber_token_error2 on an id created by fiber_token_create
        ++branch_counter;
        ASSERT_EQ(0, fiber_token_lock(id1, nullptr));
        ASSERT_EQ(0, fiber_token_error2(id1, expected_code, ""));
        ASSERT_EQ(0, fiber_token_unlock(id1));
        ASSERT_EQ(branch_counter, branch_tags[0]);
    }
} // namespace
