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

find_library(EXECINFO_LIBRARY execinfo)


turbo_cc_test(
  NAME
    stacktrace_test
  SRCS
    "stacktrace_test.cc"
  COPTS
    ${TURBO_TEST_COPTS}
  DEPS
    turbo::turbo
    GTest::gmock_main
)

turbo_cc_test(
  NAME
    symbolize_test
  SRCS
    "symbolize_test.cc"
  COPTS
    ${TURBO_TEST_COPTS}
    $<$<BOOL:${MSVC}>:-Z7>
  LINKOPTS
    $<$<BOOL:${MSVC}>:-DEBUG>
  DEPS
    turbo::turbo
    GTest::gmock
)


turbo_cc_test(
  NAME
    failure_signal_handler_test
  SRCS
    "failure_signal_handler_test.cc"
  COPTS
    ${TURBO_TEST_COPTS}
  DEPS
    turbo::turbo
    Threads::Threads
    GTest::gmock
)


turbo_cc_test(
  NAME
    demangle_test
  SRCS
    "internal/demangle_test.cc"
  COPTS
    ${TURBO_TEST_COPTS}
  DEPS
    turbo::turbo
    GTest::gmock_main
)


turbo_cc_test(
  NAME
    leak_check_test
  SRCS
    "leak_check_test.cc"
  COPTS
    ${TURBO_TEST_COPTS}
  LINKOPTS
    ${TURBO_DEFAULT_LINKOPTS}
  DEPS
    turbo::turbo
    GTest::gmock_main
)

# Internal-only target, do not depend on directly.
turbo_cc_library(
  NAME
    stack_consumption
  HDRS
    "internal/stack_consumption.h"
  SRCS
    "internal/stack_consumption.cc"
  COPTS
    ${TURBO_DEFAULT_COPTS}
  DEPS
    turbo::turbo
  TESTONLY
)

turbo_cc_test(
  NAME
    stack_consumption_test
  SRCS
    "internal/stack_consumption_test.cc"
  COPTS
    ${TURBO_TEST_COPTS}
  DEPS
    turbo::stack_consumption
    turbo::turbo
    GTest::gmock_main
)
