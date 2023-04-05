#
# Copyright 2020 The Turbo Authors.
#
# Licensed under the Apache License, Version 2.0 (the "License");
# you may not use this file except in compliance with the License.
# You may obtain a copy of the License at
#
#      https://www.apache.org/licenses/LICENSE-2.0
#
# Unless required by applicable law or agreed to in writing, software
# distributed under the License is distributed on an "AS IS" BASIS,
# WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
# See the License for the specific language governing permissions and
# limitations under the License.
#

turbo_cc_library(
        NAME
        sync
        SRCS
        "barrier.cc"
        "blocking_counter.cc"
        "mutex.cc"
        "notification.cc"
        "internal/create_thread_identity.cc"
        "internal/graphcycles.cc"
        "internal/per_thread_sem.cc"
        "internal/waiter.cc"
        COPTS
        ${TURBO_DEFAULT_COPTS}
        ${TURBO_RANDOM_RANDEN_COPTS}
        ${TURBO_SIMD_FLAGS}
        DEPS
        ${TURBO_DYLINK}
        turbo::turbo
        TESTONLY
)
#[[
turbo_cc_test(
  NAME
    barrier_test
  SRCS
    "barrier_test.cc"
  COPTS
    ${TURBO_TEST_COPTS}
  DEPS
    turbo::turbo
    GTest::gmock_main
)

turbo_cc_test(
  NAME
    blocking_counter_test
  SRCS
    "blocking_counter_test.cc"
  COPTS
    ${TURBO_TEST_COPTS}
  DEPS
    turbo::turbo
    GTest::gmock_main
)

turbo_cc_test(
  NAME
    graphcycles_test
  SRCS
    "internal/graphcycles_test.cc"
  COPTS
    ${TURBO_TEST_COPTS}
  DEPS
    turbo::turbo
    GTest::gmock_main
)


turbo_cc_test(
  NAME
    mutex_test
  SRCS
    "mutex_test.cc"
  COPTS
    ${TURBO_TEST_COPTS}
  DEPS
    turbo::turbo
    GTest::gmock_main
)

turbo_cc_test(
  NAME
    mutex_method_pointer_test
  SRCS
    "mutex_method_pointer_test.cc"
  COPTS
    ${TURBO_TEST_COPTS}
  DEPS
    turbo::turbo
    GTest::gmock_main
)

turbo_cc_test(
  NAME
    notification_test
  SRCS
    "notification_test.cc"
  COPTS
    ${TURBO_TEST_COPTS}
  DEPS
    turbo::turbo
    GTest::gmock_main
)

# Internal-only target, do not depend on directly.
turbo_cc_library(
  NAME
    per_thread_sem_test_common
  SRCS
    "internal/per_thread_sem_test.cc"
  COPTS
    ${TURBO_TEST_COPTS}
  DEPS
    turbo::turbo
    GTest::gmock
  TESTONLY
)

turbo_cc_test(
  NAME
    per_thread_sem_test
  SRCS
    "internal/per_thread_sem_test.cc"
  COPTS
    ${TURBO_TEST_COPTS}
  DEPS
    turbo::per_thread_sem_test_common
    turbo::turbo
    GTest::gmock_main
)

turbo_cc_test(
  NAME
    lifetime_test
  SRCS
    "lifetime_test.cc"
  COPTS
    ${TURBO_TEST_COPTS}
  DEPS
    turbo::turbo
)
]]