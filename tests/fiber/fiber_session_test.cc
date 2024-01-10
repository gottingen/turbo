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
// Created by jeff on 24-1-11.
//

#define DOCTEST_CONFIG_IMPLEMENT_WITH_MAIN
#include "turbo/testing/test.h"

#include <iostream>
#include "turbo/times/stop_watcher.h"
#include "turbo/fiber/fiber.h"
#include "turbo/fiber/internal/fiber_worker.h"
#include "turbo/fiber/internal/waitable_event.h"
#include "turbo/fiber/fiber_session.h"

namespace turbo::fiber_internal {
    void session_status(turbo::fiber_session_t, std::ostream &);
    uint32_t session_value(turbo::fiber_session_t id);
}

namespace {
    inline uint32_t get_version(turbo::fiber_session_t id) {
        return (uint32_t)(id.value & 0xFFFFFFFFul);
    }

    struct SignalArg {
        turbo::fiber_session_t id;
        long sleep_us_before_fight;
        long sleep_us_before_signal;
    };

    void* signaller(void* void_arg) {
        SignalArg arg = *(SignalArg*)void_arg;
        turbo::fiber_usleep(arg.sleep_us_before_fight);
        void* data = nullptr;
        int rc = turbo::fiber_session_trylock(arg.id, &data);
        if (rc == 0) {
            REQUIRE_EQ(0xdead, *(int*)data);
            ++*(int*)data;
            //REQUIRE_EQ(EBUSY, bthread_id_destroy(arg.id, ECANCELED));
            turbo::fiber_usleep(arg.sleep_us_before_signal);
            REQUIRE_EQ(0, turbo::fiber_session_unlock_and_destroy(arg.id));
            return void_arg;
        } else {
            REQUIRE((EBUSY == rc || EINVAL == rc));
            return nullptr;
        }
    }

    TEST_CASE("BthreadIdTest, join_after_destroy") {
        turbo::fiber_session_t id1;
        int x = 0xdead;
        REQUIRE_EQ(0, turbo::fiber_session_create_ranged(&id1, 2, &x, nullptr));
        turbo::fiber_session_t id2 = { id1.value + 1 };
        REQUIRE_EQ(get_version(id1), turbo::fiber_internal::session_value(id1));
        REQUIRE_EQ(get_version(id1), turbo::fiber_internal::session_value(id2));
        pthread_t th[8];
        SignalArg args[TURBO_ARRAY_SIZE(th)];
        for (size_t i = 0; i < TURBO_ARRAY_SIZE(th); ++i) {
            args[i].sleep_us_before_fight = 0;
            args[i].sleep_us_before_signal = 0;
            args[i].id = (i == 0 ? id1 : id2);
            REQUIRE_EQ(0, pthread_create(&th[i], nullptr, signaller, &args[i]));
        }
        void* ret[TURBO_ARRAY_SIZE(th)];
        size_t non_null_ret = 0;
        for (size_t i = 0; i < TURBO_ARRAY_SIZE(th); ++i) {
            REQUIRE_EQ(0, pthread_join(th[i], &ret[i]));
            non_null_ret += (ret[i] != nullptr);
        }
        REQUIRE_EQ(1UL, non_null_ret);
        REQUIRE_EQ(0, turbo::fiber_session_join(id1));
        REQUIRE_EQ(0, turbo::fiber_session_join(id2));
        REQUIRE_EQ(0xdead + 1, x);
        REQUIRE_EQ(get_version(id1) + 5, turbo::fiber_internal::session_value(id1));
        REQUIRE_EQ(get_version(id1) + 5, turbo::fiber_internal::session_value(id2));
    }

    TEST_CASE("BthreadIdTest, join_before_destroy") {
        turbo::fiber_session_t id1;
        int x = 0xdead;
        REQUIRE_EQ(0, turbo::fiber_session_create(&id1, &x, nullptr));
        REQUIRE_EQ(get_version(id1), turbo::fiber_internal::session_value(id1));
        pthread_t th[8];
        SignalArg args[TURBO_ARRAY_SIZE(th)];
        for (size_t i = 0; i < TURBO_ARRAY_SIZE(th); ++i) {
            args[i].sleep_us_before_fight = 10000;
            args[i].sleep_us_before_signal = 0;
            args[i].id = id1;
            REQUIRE_EQ(0, pthread_create(&th[i], nullptr, signaller, &args[i]));
        }
        REQUIRE_EQ(0, turbo::fiber_session_join(id1));
        REQUIRE_EQ(0xdead + 1, x);
        REQUIRE_EQ(get_version(id1) + 4, turbo::fiber_internal::session_value(id1));

        void* ret[TURBO_ARRAY_SIZE(th)];
        size_t non_null_ret = 0;
        for (size_t i = 0; i < TURBO_ARRAY_SIZE(th); ++i) {
            REQUIRE_EQ(0, pthread_join(th[i], &ret[i]));
            non_null_ret += (ret[i] != nullptr);
        }
        REQUIRE_EQ(1UL, non_null_ret);
    }

    struct OnResetArg {
        turbo::fiber_session_t id;
        int error_code;
    };

    int on_reset(turbo::fiber_session_t id, void* data, int error_code) {
        OnResetArg* arg = static_cast<OnResetArg*>(data);
        arg->id = id;
        arg->error_code = error_code;
        return turbo::fiber_session_unlock_and_destroy(id);
    }

    TEST_CASE("BthreadIdTest, error_is_destroy") {
        turbo::fiber_session_t id1;
        OnResetArg arg = { { 0 }, 0 };
        REQUIRE_EQ(0, turbo::fiber_session_create(&id1, &arg, on_reset));
        REQUIRE_EQ(get_version(id1), turbo::fiber_internal::session_value(id1));
        REQUIRE_EQ(0, fiber_session_error(id1, EBADF));
        REQUIRE_EQ(EBADF, arg.error_code);
        REQUIRE_EQ(id1.value, arg.id.value);
        REQUIRE_EQ(get_version(id1) + 4, turbo::fiber_internal::session_value(id1));
    }

    TEST_CASE("BthreadIdTest, error_is_destroy_ranged") {
        turbo::fiber_session_t id1;
        OnResetArg arg = { { 0 }, 0 };
        REQUIRE_EQ(0, turbo::fiber_session_create_ranged(&id1, 2, &arg, on_reset));
        turbo::fiber_session_t id2 = { id1.value + 1 };
        REQUIRE_EQ(get_version(id1), turbo::fiber_internal::session_value(id2));
        REQUIRE_EQ(0, fiber_session_error(id2, EBADF));
        REQUIRE_EQ(EBADF, arg.error_code);
        REQUIRE_EQ(id2.value, arg.id.value);
        REQUIRE_EQ(get_version(id1) + 5, turbo::fiber_internal::session_value(id2));
    }

    TEST_CASE("BthreadIdTest, default_error_is_destroy") {
        turbo::fiber_session_t id1;
        REQUIRE_EQ(0, turbo::fiber_session_create(&id1, nullptr, nullptr));
        REQUIRE_EQ(get_version(id1), turbo::fiber_internal::session_value(id1));
        REQUIRE_EQ(0, fiber_session_error(id1, EBADF));
        REQUIRE_EQ(get_version(id1) + 4, turbo::fiber_internal::session_value(id1));
    }

    TEST_CASE("BthreadIdTest, doubly_destroy") {
        turbo::fiber_session_t id1;
        REQUIRE_EQ(0, turbo::fiber_session_create_ranged(&id1, 2, nullptr, nullptr));
        turbo::fiber_session_t id2 = { id1.value + 1 };
        REQUIRE_EQ(get_version(id1), turbo::fiber_internal::session_value(id1));
        REQUIRE_EQ(get_version(id1), turbo::fiber_internal::session_value(id2));
        REQUIRE_EQ(0, fiber_session_error(id1, EBADF));
        REQUIRE_EQ(get_version(id1) + 5, turbo::fiber_internal::session_value(id1));
        REQUIRE_EQ(get_version(id1) + 5, turbo::fiber_internal::session_value(id2));
        REQUIRE_EQ(EINVAL, fiber_session_error(id1, EBADF));
        REQUIRE_EQ(EINVAL, fiber_session_error(id2, EBADF));
    }

    static int on_numeric_error(turbo::fiber_session_t id, void* data, int error_code) {
        std::vector<int>* result = static_cast<std::vector<int>*>(data);
        result->push_back(error_code);
        REQUIRE_EQ(0, fiber_session_unlock(id));
        return 0;
    }

    TEST_CASE("BthreadIdTest, many_error") {
        turbo::fiber_session_t id1;
        std::vector<int> result;
        REQUIRE_EQ(0, turbo::fiber_session_create(&id1, &result, on_numeric_error));
        REQUIRE_EQ(get_version(id1), turbo::fiber_internal::session_value(id1));
        int err = 0;
        const int N = 100;
        for (int i = 0; i < N; ++i) {
            REQUIRE_EQ(0, fiber_session_error(id1, err++));
        }
        REQUIRE_EQ((size_t)N, result.size());
        for (int i = 0; i < N; ++i) {
            REQUIRE_EQ(i, result[i]);
        }
        REQUIRE_EQ(0, turbo::fiber_session_trylock(id1, nullptr));
        REQUIRE_EQ(get_version(id1) + 1, turbo::fiber_internal::session_value(id1));
        for (int i = 0; i < N; ++i) {
            REQUIRE_EQ(0, fiber_session_error(id1, err++));
        }
        REQUIRE_EQ((size_t)N, result.size());
        REQUIRE_EQ(0, fiber_session_unlock(id1));
        REQUIRE_EQ(get_version(id1), turbo::fiber_internal::session_value(id1));
        REQUIRE_EQ((size_t)2*N, result.size());
        for (int i = 0; i < 2*N; ++i) {
            REQUIRE_EQ(i, result[i]);
        }
        result.clear();

        REQUIRE_EQ(0, turbo::fiber_session_trylock(id1, nullptr));
        REQUIRE_EQ(get_version(id1) + 1, turbo::fiber_internal::session_value(id1));
        for (int i = 0; i < N; ++i) {
            REQUIRE_EQ(0, fiber_session_error(id1, err++));
        }
        REQUIRE_EQ(0, turbo::fiber_session_unlock_and_destroy(id1));
        REQUIRE(result.empty());
    }

    static void* locker(void* arg) {
        turbo::fiber_session_t id = { (uintptr_t)arg };
        turbo::StopWatcher tm;
        tm.reset();
        REQUIRE_EQ(0, fiber_session_lock(id, nullptr));
        turbo::fiber_usleep(2000);
        REQUIRE_EQ(0, fiber_session_unlock(id));
        tm.stop();
        TLOG_INFO("Unlocked, tm={}", tm.elapsed_micro());
        return nullptr;
    }

    TEST_CASE("BthreadIdTest, id_lock") {
        turbo::fiber_session_t id1;
        REQUIRE_EQ(0, turbo::fiber_session_create(&id1, nullptr, nullptr));
        REQUIRE_EQ(get_version(id1), turbo::fiber_internal::session_value(id1));
        pthread_t th[8];
        for (size_t i = 0; i < TURBO_ARRAY_SIZE(th); ++i) {
            REQUIRE_EQ(0, pthread_create(&th[i], nullptr, locker,
                                        (void*)id1.value));
        }
        for (size_t i = 0; i < TURBO_ARRAY_SIZE(th); ++i) {
            REQUIRE_EQ(0, pthread_join(th[i], nullptr));
        }
    }

    static void* failed_locker(void* arg) {
        turbo::fiber_session_t id = { (uintptr_t)arg };
        int rc = fiber_session_lock(id, nullptr);
        if (rc == 0) {
            turbo::fiber_usleep(2000);
            REQUIRE_EQ(0, turbo::fiber_session_unlock_and_destroy(id));
            return (void*)1;
        } else {
            REQUIRE_EQ(EINVAL, rc);
            return nullptr;
        }
    }

    TEST_CASE("BthreadIdTest, id_lock_and_destroy") {
        turbo::fiber_session_t id1;
        REQUIRE_EQ(0, turbo::fiber_session_create(&id1, nullptr, nullptr));
        REQUIRE_EQ(get_version(id1), turbo::fiber_internal::session_value(id1));
        pthread_t th[8];
        for (size_t i = 0; i < TURBO_ARRAY_SIZE(th); ++i) {
            REQUIRE_EQ(0, pthread_create(&th[i], nullptr, failed_locker,
                                        (void*)id1.value));
        }
        int non_null = 0;
        for (size_t i = 0; i < TURBO_ARRAY_SIZE(th); ++i) {
            void* ret = nullptr;
            REQUIRE_EQ(0, pthread_join(th[i], &ret));
            non_null += (ret != nullptr);
        }
        REQUIRE_EQ(1, non_null);
    }

    TEST_CASE("BthreadIdTest, join_after_destroy_before_unlock") {
        turbo::fiber_session_t id1;
        int x = 0xdead;
        REQUIRE_EQ(0, turbo::fiber_session_create(&id1, &x, nullptr));
        REQUIRE_EQ(get_version(id1), turbo::fiber_internal::session_value(id1));
        pthread_t th[8];
        SignalArg args[TURBO_ARRAY_SIZE(th)];
        for (size_t i = 0; i < TURBO_ARRAY_SIZE(th); ++i) {
            args[i].sleep_us_before_fight = 0;
            args[i].sleep_us_before_signal = 20000;
            args[i].id = id1;
            REQUIRE_EQ(0, pthread_create(&th[i], nullptr, signaller, &args[i]));
        }
        turbo::fiber_usleep(10000);
        // join() waits until destroy() is called.
        REQUIRE_EQ(0, turbo::fiber_session_join(id1));
        REQUIRE_EQ(0xdead + 1, x);
        REQUIRE_EQ(get_version(id1) + 4, turbo::fiber_internal::session_value(id1));

        void* ret[TURBO_ARRAY_SIZE(th)];
        size_t non_null_ret = 0;
        for (size_t i = 0; i < TURBO_ARRAY_SIZE(th); ++i) {
            REQUIRE_EQ(0, pthread_join(th[i], &ret[i]));
            non_null_ret += (ret[i] != nullptr);
        }
        REQUIRE_EQ(1UL, non_null_ret);
    }

    struct StoppedWaiterArgs {
        turbo::fiber_session_t id;
        bool thread_started;
    };

    void* stopped_waiter(void* void_arg) {
        StoppedWaiterArgs* args = (StoppedWaiterArgs*)void_arg;
        args->thread_started = true;
        REQUIRE_EQ(0, turbo::fiber_session_join(args->id));
        REQUIRE_EQ(get_version(args->id) + 4, turbo::fiber_internal::session_value(args->id));
        return nullptr;
    }

    TEST_CASE("BthreadIdTest, stop_a_wait_after_fight_before_signal") {
        turbo::fiber_session_t id1;
        int x = 0xdead;
        REQUIRE_EQ(0, turbo::fiber_session_create(&id1, &x, nullptr));
        REQUIRE_EQ(get_version(id1), turbo::fiber_internal::session_value(id1));
        void* data;
        REQUIRE_EQ(0, turbo::fiber_session_trylock(id1, &data));
        REQUIRE_EQ(&x, data);
        turbo::fiber_id_t th[8];
        StoppedWaiterArgs args[TURBO_ARRAY_SIZE(th)];
        for (size_t i = 0; i < TURBO_ARRAY_SIZE(th); ++i) {
            args[i].id = id1;
            args[i].thread_started = false;
            REQUIRE_EQ(turbo::ok_status(), turbo::fiber_internal::fiber_start_urgent(&th[i], nullptr, stopped_waiter, &args[i]));
        }
        // stop does not wake up turbo::fiber_session_join
        for (size_t i = 0; i < TURBO_ARRAY_SIZE(th); ++i) {
            turbo::fiber_internal::fiber_stop(th[i]);
        }
        turbo::fiber_usleep(10000);
        for (size_t i = 0; i < TURBO_ARRAY_SIZE(th); ++i) {
            REQUIRE(turbo::fiber_internal::FiberWorker::exists(th[i]));
        }
        // destroy the id to end the joinings.
        REQUIRE_EQ(0, turbo::fiber_session_unlock_and_destroy(id1));
        for (size_t i = 0; i < TURBO_ARRAY_SIZE(th); ++i) {
            REQUIRE_EQ(turbo::ok_status(), turbo::fiber_internal::fiber_join(th[i], nullptr));
        }
    }

    void* waiter(void* arg) {
        turbo::fiber_session_t id = { (uintptr_t)arg };
        REQUIRE_EQ(0, turbo::fiber_session_join(id));
        REQUIRE_EQ(get_version(id) + 4, turbo::fiber_internal::session_value(id));
        return nullptr;
    }

    int handle_data(turbo::fiber_session_t id, void* data, int error_code) {
        REQUIRE_EQ(EBADF, error_code);
        ++*(int*)data;
        REQUIRE_EQ(0, turbo::fiber_session_unlock_and_destroy(id));
        return 0;
    }

    TEST_CASE("BthreadIdTest, list_signal") {
        turbo::fiber_session_list_t list;
        REQUIRE_EQ(0, turbo::fiber_session_list_init(&list, 32, 32));
        turbo::fiber_session_t id[16];
        int data[TURBO_ARRAY_SIZE(id)];
        for (size_t i = 0; i < TURBO_ARRAY_SIZE(id); ++i) {
            data[i] = i;
            REQUIRE_EQ(0, turbo::fiber_session_create(&id[i], &data[i], handle_data));
            REQUIRE_EQ(get_version(id[i]), turbo::fiber_internal::session_value(id[i]));
            REQUIRE_EQ(0, turbo::fiber_session_list_add(&list, id[i]));
        }
        pthread_t th[TURBO_ARRAY_SIZE(id)];
        for (size_t i = 0; i < TURBO_ARRAY_SIZE(th); ++i) {
            REQUIRE_EQ(0, pthread_create(&th[i], nullptr, waiter, (void*)(intptr_t)id[i].value));
        }
        turbo::fiber_usleep(10000);
        REQUIRE_EQ(0, turbo::fiber_session_list_reset(&list, EBADF));

        for (size_t i = 0; i < TURBO_ARRAY_SIZE(th); ++i) {
            REQUIRE_EQ((int)(i + 1), data[i]);
            REQUIRE_EQ(0, pthread_join(th[i], nullptr));
            // already reset.
            REQUIRE_EQ((int)(i + 1), data[i]);
        }

        turbo::fiber_session_list_destroy(&list);
    }

    int error_without_unlock(turbo::fiber_session_t, void *, int) {
        return 0;
    }

    TEST_CASE("BthreadIdTest, status") {
        turbo::fiber_session_t id;
        turbo::fiber_session_create(&id, nullptr, nullptr);
        turbo::fiber_internal::session_status(id, std::cout);
        fiber_session_lock(id, nullptr);
        fiber_session_error(id, 123);
        fiber_session_error(id, 256);
        fiber_session_error(id, 1256);
        turbo::fiber_internal::session_status(id, std::cout);
        turbo::fiber_session_unlock_and_destroy(id);
        turbo::fiber_session_create(&id, nullptr, error_without_unlock);
        fiber_session_lock(id, nullptr);
        turbo::fiber_internal::session_status(id, std::cout);
        fiber_session_error(id, 12);
        turbo::fiber_internal::session_status(id, std::cout);
        fiber_session_unlock(id);
        turbo::fiber_internal::session_status(id, std::cout);
        turbo::fiber_session_unlock_and_destroy(id);
    }

    TEST_CASE("BthreadIdTest, reset_range") {
        turbo::fiber_session_t id;
        REQUIRE_EQ(0, turbo::fiber_session_create(&id, nullptr, nullptr));
        REQUIRE_EQ(0, fiber_session_lock_and_reset_range(id, nullptr, 1000));
        turbo::fiber_internal::session_status(id, std::cout);
        fiber_session_unlock(id);
        REQUIRE_EQ(0, fiber_session_lock_and_reset_range(id, nullptr, 300));
        turbo::fiber_internal::session_status(id, std::cout);
        turbo::fiber_session_unlock_and_destroy(id);
    }

    static bool any_thread_quit = false;

    struct FailToLockIdArgs {
        turbo::fiber_session_t id;
        int expected_return;
    };

    static void* fail_to_lock_id(void* args_in) {
        FailToLockIdArgs* args = (FailToLockIdArgs*)args_in;
        turbo::StopWatcher tm;
        REQUIRE_EQ(args->expected_return, fiber_session_lock(args->id, nullptr));
        any_thread_quit = true;
        return nullptr;
    }

    TEST_CASE("BthreadIdTest, about_to_destroy_before_locking") {
        turbo::fiber_session_t id;
        REQUIRE_EQ(0, turbo::fiber_session_create(&id, nullptr, nullptr));
        REQUIRE_EQ(0, fiber_session_lock(id, nullptr));
        REQUIRE_EQ(0, turbo::fiber_session_about_to_destroy(id));
        pthread_t pth;
        turbo::fiber_id_t bth;
        FailToLockIdArgs args = { id, EPERM };
        REQUIRE_EQ(0, pthread_create(&pth, nullptr, fail_to_lock_id, &args));
        REQUIRE_EQ(turbo::ok_status(), turbo::fiber_internal::fiber_start_background(&bth, nullptr, fail_to_lock_id, &args));
        // The threads should quit soon.
        pthread_join(pth, nullptr);
        turbo::fiber_internal::fiber_join(bth, nullptr);
        turbo::fiber_internal::session_status(id, std::cout);
        REQUIRE_EQ(0, turbo::fiber_session_unlock_and_destroy(id));
    }

    static void* succeed_to_lock_id(void* arg) {
        turbo::fiber_session_t id = *(turbo::fiber_session_t*)arg;
        REQUIRE_EQ(0, fiber_session_lock(id, nullptr));
        REQUIRE_EQ(0, fiber_session_unlock(id));
        return nullptr;
    }

    TEST_CASE("BthreadIdTest, about_to_destroy_cancelled") {
        turbo::fiber_session_t id;
        REQUIRE_EQ(0, turbo::fiber_session_create(&id, nullptr, nullptr));
        REQUIRE_EQ(0, fiber_session_lock(id, nullptr));
        REQUIRE_EQ(0, turbo::fiber_session_about_to_destroy(id));
        REQUIRE_EQ(0, fiber_session_unlock(id));
        pthread_t pth;
        turbo::fiber_id_t bth;
        REQUIRE_EQ(0, pthread_create(&pth, nullptr, succeed_to_lock_id, &id));
        REQUIRE_EQ(turbo::ok_status(), turbo::fiber_internal::fiber_start_background(&bth, nullptr, succeed_to_lock_id, &id));
        // The threads should quit soon.
        pthread_join(pth, nullptr);
        turbo::fiber_internal::fiber_join(bth, nullptr);
        turbo::fiber_internal::session_status(id, std::cout);
        REQUIRE_EQ(0, fiber_session_lock(id, nullptr));
        REQUIRE_EQ(0, turbo::fiber_session_unlock_and_destroy(id));
    }

    TEST_CASE("BthreadIdTest, about_to_destroy_during_locking") {
        turbo::fiber_session_t id;
        REQUIRE_EQ(0, turbo::fiber_session_create(&id, nullptr, nullptr));
        REQUIRE_EQ(0, fiber_session_lock(id, nullptr));
        any_thread_quit = false;
        pthread_t pth;
        turbo::fiber_id_t bth;
        FailToLockIdArgs args = { id, EPERM };
        REQUIRE_EQ(0, pthread_create(&pth, nullptr, fail_to_lock_id, &args));
        REQUIRE_EQ(turbo::ok_status(), turbo::fiber_internal::fiber_start_background(&bth, nullptr, fail_to_lock_id, &args));

        usleep(100000);
        REQUIRE_FALSE(any_thread_quit);
        REQUIRE_EQ(0, turbo::fiber_session_about_to_destroy(id));

        // The threads should quit soon.
        pthread_join(pth, nullptr);
        turbo::fiber_internal::fiber_join(bth, nullptr);
        turbo::fiber_internal::session_status(id, std::cout);
        REQUIRE_EQ(0, turbo::fiber_session_unlock_and_destroy(id));
    }

    void* const DUMMY_DATA1 = (void*)1;
    void* const DUMMY_DATA2 = (void*)2;
    int branch_counter = 0;
    int branch_tags[4] = {};
    int expected_code = 0;
    const char* expected_desc = "";
    int handler_without_desc(turbo::fiber_session_t id, void* data, int error_code) {
        REQUIRE_EQ(DUMMY_DATA1, data);
        REQUIRE_EQ(expected_code, error_code);
        if (error_code == -20) {
            branch_tags[0] = branch_counter;
            return turbo::fiber_session_unlock_and_destroy(id);
        } else {
            branch_tags[1] = branch_counter;
            return fiber_session_unlock(id);
        }
    }
    int handler_with_desc(turbo::fiber_session_t id, void* data, int error_code,
                          const std::string& error_text) {
        REQUIRE_EQ(DUMMY_DATA2, data);
        REQUIRE_EQ(expected_code, error_code);
        REQUIRE_EQ(expected_desc, error_text);
        if (error_code == -20) {
            branch_tags[2] = branch_counter;
            return turbo::fiber_session_unlock_and_destroy(id);
        } else {
            branch_tags[3] = branch_counter;
            return fiber_session_unlock(id);
        }
    }

    TEST_CASE("BthreadIdTest, error_with_descriptions") {
        turbo::fiber_session_t id1;
        REQUIRE_EQ(0, turbo::fiber_session_create(&id1, DUMMY_DATA1, handler_without_desc));
        turbo::fiber_session_t id2;
        REQUIRE_EQ(0, turbo::fiber_session_create_msg(&id2, DUMMY_DATA2, handler_with_desc));

        // [ Matched in-place ]
        // Call fiber_session_error on an id created by turbo::fiber_session_create
        ++branch_counter;
        expected_code = EINVAL;
        REQUIRE_EQ(0, fiber_session_error(id1, expected_code));
        REQUIRE_EQ(branch_counter, branch_tags[1]);

        // Call turbo::fiber_session_error2 on an id created by turbo::fiber_session_create2
        ++branch_counter;
        expected_code = EPERM;
        expected_desc = "description1";
        REQUIRE_EQ(0, turbo::fiber_session_error_msg(id2, expected_code, expected_desc));
        REQUIRE_EQ(branch_counter, branch_tags[3]);

        // [ Mixed in-place ]
        // Call fiber_session_error on an id created by turbo::fiber_session_create2
        ++branch_counter;
        expected_code = ECONNREFUSED;
        expected_desc = "";
        REQUIRE_EQ(0, fiber_session_error(id2, expected_code));
        REQUIRE_EQ(branch_counter, branch_tags[3]);
        // Call turbo::fiber_session_error2 on an id created by turbo::fiber_session_create
        ++branch_counter;
        expected_code = EINTR;
        REQUIRE_EQ(0, turbo::fiber_session_error_msg(id1, expected_code, ""));
        REQUIRE_EQ(branch_counter, branch_tags[1]);

        // [ Matched pending ]
        // Call fiber_session_error on an id created by turbo::fiber_session_create
        ++branch_counter;
        expected_code = ECONNRESET;
        REQUIRE_EQ(0, fiber_session_lock(id1, nullptr));
        REQUIRE_EQ(0, fiber_session_error(id1, expected_code));
        REQUIRE_EQ(0, fiber_session_unlock(id1));
        REQUIRE_EQ(branch_counter, branch_tags[1]);

        // Call turbo::fiber_session_error2 on an id created by turbo::fiber_session_create2
        ++branch_counter;
        expected_code = ENOSPC;
        expected_desc = "description3";
        REQUIRE_EQ(0, fiber_session_lock(id2, nullptr));
        REQUIRE_EQ(0, turbo::fiber_session_error_msg(id2, expected_code, expected_desc));
        REQUIRE_EQ(0, fiber_session_unlock(id2));
        REQUIRE_EQ(branch_counter, branch_tags[3]);

        // [ Mixed pending ]
        // Call fiber_session_error on an id created by turbo::fiber_session_create2
        ++branch_counter;
        expected_code = -20;
        expected_desc = "";
        REQUIRE_EQ(0, fiber_session_lock(id2, nullptr));
        REQUIRE_EQ(0, fiber_session_error(id2, expected_code));
        REQUIRE_EQ(0, fiber_session_unlock(id2));
        REQUIRE_EQ(branch_counter, branch_tags[2]);
        // Call turbo::fiber_session_error2 on an id created by turbo::fiber_session_create
        ++branch_counter;
        REQUIRE_EQ(0, fiber_session_lock(id1, nullptr));
        REQUIRE_EQ(0, turbo::fiber_session_error_msg(id1, expected_code, ""));
        REQUIRE_EQ(0, fiber_session_unlock(id1));
        REQUIRE_EQ(branch_counter, branch_tags[0]);
    }
} // namespace
