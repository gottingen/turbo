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

find_library(EXECINFO_LIBRARY execinfo)


carbin_cc_test(
        NAME
        stacktrace_test
        SOURCES
        "stacktrace_test.cc"
        COPTS
        ${CARBIN_TEST_COPTS}
        DEPS
        turbo::turbo
        ${GTEST_LIB}
        ${GMOCK_MAIN_LIB}
)

carbin_cc_test(
        NAME
        symbolize_test
        SOURCES
        "symbolize_test.cc"
        COPTS
        ${CARBIN_TEST_COPTS}
        DEPS
        turbo::turbo
        ${GTEST_LIB}
        ${GMOCK_LIB}
)


carbin_cc_test(
        NAME
        failure_signal_handler_test
        SOURCES
        "failure_signal_handler_test.cc"
        COPTS
        ${CARBIN_TEST_COPTS}
        DEPS
        turbo::turbo
        Threads::Threads
        ${GTEST_LIB}
        ${GMOCK_LIB}
)


carbin_cc_test(
        NAME
        demangle_test
        SOURCES
        "demangle_test.cc"
        COPTS
        ${CARBIN_TEST_COPTS}
        DEPS
        turbo::turbo
        ${GTEST_LIB}
        ${GMOCK_MAIN_LIB}
)


carbin_cc_test(
        NAME
        leak_check_test
        SOURCES
        "leak_check_test.cc"
        COPTS
        ${CARBIN_TEST_COPTS}
        DEPS
        turbo::turbo
        ${GTEST_LIB}
        ${GMOCK_MAIN_LIB}
)

# Internal-only target, do not depend on directly.
carbin_cc_library(
        NAME
        stack_consumption
        HEADERS
        "stack_consumption.h"
        SOURCES
        "stack_consumption.cc"
        COPTS
        ${TURBO_DEFAULT_COPTS}
        DEPS
        turbo::turbo
        ${GTEST_LIB}
)

carbin_cc_test(
        NAME
        stack_consumption_test
        SOURCES
        "stack_consumption_test.cc"
        COPTS
        ${CARBIN_TEST_COPTS}
        DEPS
        turbo::stack_consumption
        turbo::turbo
        ${GTEST_LIB}
        ${GMOCK_MAIN_LIB}
)
