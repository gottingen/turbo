#
# Copyright 2017 The Turbo Authors.
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

# Internal-only target, do not depend on directly.
turbo_cc_library(
  NAME
    graphcycles_internal
  HDRS
    "internal/graphcycles.h"
  SRCS
    "internal/graphcycles.cc"
  COPTS
    ${TURBO_DEFAULT_COPTS}
  DEPS
    turbo::base
    turbo::base_internal
    turbo::config
    turbo::core_headers
    turbo::malloc_internal
    turbo::raw_logging_internal
)

# Internal-only target, do not depend on directly.
turbo_cc_library(
  NAME
    kernel_timeout_internal
  HDRS
    "internal/kernel_timeout.h"
  COPTS
    ${TURBO_DEFAULT_COPTS}
  DEPS
    turbo::core_headers
    turbo::raw_logging_internal
    turbo::time
)

turbo_cc_library(
  NAME
    synchronization
  HDRS
    "barrier.h"
    "blocking_counter.h"
    "internal/create_thread_identity.h"
    "internal/futex.h"
    "internal/per_thread_sem.h"
    "internal/waiter.h"
    "mutex.h"
    "notification.h"
  SRCS
    "barrier.cc"
    "blocking_counter.cc"
    "internal/create_thread_identity.cc"
    "internal/per_thread_sem.cc"
    "internal/waiter.cc"
    "notification.cc"
    "mutex.cc"
  COPTS
    ${TURBO_DEFAULT_COPTS}
  DEPS
    turbo::graphcycles_internal
    turbo::kernel_timeout_internal
    turbo::atomic_hook
    turbo::base
    turbo::base_internal
    turbo::config
    turbo::core_headers
    turbo::dynamic_annotations
    turbo::malloc_internal
    turbo::raw_logging_internal
    turbo::stacktrace
    turbo::symbolize
    turbo::time
    Threads::Threads
  PUBLIC
)

turbo_cc_test(
  NAME
    barrier_test
  SRCS
    "barrier_test.cc"
  COPTS
    ${TURBO_TEST_COPTS}
  DEPS
    turbo::synchronization
    turbo::time
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
    turbo::synchronization
    turbo::time
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
    turbo::graphcycles_internal
    turbo::core_headers
    turbo::raw_logging_internal
    GTest::gmock_main
)

# Internal-only target, do not depend on directly.
turbo_cc_library(
  NAME
    thread_pool
  HDRS
    "internal/thread_pool.h"
  COPTS
    ${TURBO_DEFAULT_COPTS}
  DEPS
    turbo::any_invocable
    turbo::core_headers
    turbo::synchronization
  TESTONLY
)

turbo_cc_test(
  NAME
    mutex_test
  SRCS
    "mutex_test.cc"
  COPTS
    ${TURBO_TEST_COPTS}
  DEPS
    turbo::synchronization
    turbo::thread_pool
    turbo::base
    turbo::config
    turbo::core_headers
    turbo::memory
    turbo::raw_logging_internal
    turbo::time
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
    turbo::base
    turbo::config
    turbo::synchronization
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
    turbo::synchronization
    turbo::time
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
    turbo::synchronization
    turbo::base
    turbo::config
    turbo::strings
    turbo::time
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
    turbo::synchronization
    turbo::strings
    turbo::time
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
    turbo::synchronization
    turbo::core_headers
    turbo::raw_logging_internal
)
