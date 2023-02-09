#
# Copyright 2017 The Turbo Authors.
#
# Licensed under the Apache License, Version 2.0 (the "License");
# you may not use this file except in compliance with the License.
# You may obtain a copy of the License at
#
#    https://www.apache.org/licenses/LICENSE-2.0
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
    atomic_hook_test_helper
  SRCS
    "internal/atomic_hook_test_helper.cc"
  COPTS
    ${TURBO_TEST_COPTS}
  DEPS
    turbo::turbo
  TESTONLY
)

turbo_cc_test(
  NAME
    atomic_hook_test
  SRCS
    "internal/atomic_hook_test.cc"
  COPTS
    ${TURBO_TEST_COPTS}
  DEPS
    turbo::atomic_hook_test_helper
    turbo::turbo
    GTest::gmock
    GTest::gtest_main
)


turbo_cc_test(
  NAME
    errno_saver_test
  SRCS
    "internal/errno_saver_test.cc"
  COPTS
    ${TURBO_TEST_COPTS}
  DEPS
    turbo::turbo
    GTest::gmock
    GTest::gtest_main
)




# Internal-only target, do not depend on directly.
turbo_cc_library(
  NAME
    spinlock_test_common
  SRCS
    "spinlock_test_common.cc"
  COPTS
    ${TURBO_TEST_COPTS}
  DEPS
    turbo::turbo
    GTest::gtest
  TESTONLY
)

# On bazel BUILD this target use "alwayslink = 1" which is not implemented here
turbo_cc_test(
  NAME
    spinlock_test
  SRCS
    "spinlock_test_common.cc"
  COPTS
    ${TURBO_TEST_COPTS}
  DEPS
    turbo::turbo
    GTest::gtest_main
)


turbo_cc_test(
  NAME
    endian_test
  SRCS
    "internal/endian_test.cc"
  COPTS
    ${TURBO_TEST_COPTS}
  DEPS
    turbo::turbo
    GTest::gtest_main
)

turbo_cc_test(
  NAME
    config_test
  SRCS
    "config/config_test.cc"
  COPTS
    ${TURBO_TEST_COPTS}
  DEPS
    turbo::turbo
    GTest::gtest_main
)




turbo_cc_test(
  NAME
    sysinfo_test
  SRCS
    "internal/sysinfo_test.cc"
  COPTS
    ${TURBO_TEST_COPTS}
  DEPS
    turbo::turbo
    GTest::gtest_main
)

turbo_cc_test(
  NAME
    low_level_alloc_test
  SRCS
    "internal/low_level_alloc_test.cc"
  COPTS
    ${TURBO_TEST_COPTS}
  DEPS
    turbo::turbo
    Threads::Threads
)

turbo_cc_test(
  NAME
    thread_identity_test
  SRCS
    "internal/thread_identity_test.cc"
  COPTS
    ${TURBO_TEST_COPTS}
  DEPS
    turbo::turbo
    Threads::Threads
    GTest::gtest_main
)

turbo_cc_test(
  NAME
    scoped_set_env_test
  SRCS
    "internal/scoped_set_env_test.cc"
  COPTS
    ${TURBO_TEST_COPTS}
  DEPS
    turbo::turbo
    GTest::gtest_main
)

turbo_cc_test(
  NAME
    cmake_thread_test
  SRCS
    "internal/cmake_thread_test.cc"
  COPTS
    ${TURBO_TEST_COPTS}
  DEPS
    turbo::turbo
)



turbo_cc_test(
  NAME
    prefetch_test
  SRCS
    "internal/prefetch_test.cc"
  COPTS
    ${TURBO_TEST_COPTS}
  DEPS
    turbo::turbo
    GTest::gtest_main
)

turbo_cc_test(
  NAME
    optimization_test
  SRCS
    "config/optimization_test.cc"
  COPTS
    ${TURBO_TEST_COPTS}
  DEPS
    turbo::turbo
    GTest::gtest_main
)
