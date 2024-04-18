#
# Copyright 2023 The titan-search Authors.
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

# Internal-only target, do not depend on directly.
carbin_cc_library(
        NAMESPACE
        turbo
        MODULE debugging
        NAME
        stack_consumption
        HEADERS
        "stack_consumption.h"
        SOURCES
        "stack_consumption.cc"
        CXXOPTS
        ${TURBO_DEFAULT_CXXOPTS}
        LINKS
        turbo::turbo
        ${GTEST_LIB}
)

carbin_cc_test(
        NAME
        stacktrace_test
        MODULE debugging
        SOURCES
        "stacktrace_test.cc"
        CXXOPTS
        ${CARBIN_CXX_OPTIONS}
        LINKS
        turbo::turbo
        ${GTEST_LIB}
        ${GTEST_MAIN_LIB}
)

carbin_cc_test(
        NAME
        symbolize_test
        MODULE debugging
        SOURCES
        "symbolize_test.cc"
        CXXOPTS
        ${CARBIN_CXX_OPTIONS}
        LINKS
        turbo::turbo
        turbo::stack_consumption
        ${GTEST_LIB}
        ${GMOCK_LIB}
)


carbin_cc_test(
        NAME
        failure_signal_handler_test
        MODULE debugging
        SOURCES
        "failure_signal_handler_test.cc"
        CXXOPTS
        ${CARBIN_CXX_OPTIONS}
        LINKS
        turbo::turbo
        Threads::Threads
        ${GTEST_LIB}
        ${GMOCK_LIB}
)


carbin_cc_test(
        NAME
        demangle_test
        MODULE debugging
        SOURCES
        "demangle_test.cc"
        CXXOPTS
        ${CARBIN_CXX_OPTIONS}
        LINKS
        turbo::turbo
        turbo::stack_consumption
        ${GTEST_LIB}
        ${GTEST_MAIN_LIB}
)


carbin_cc_test(
        NAME
        leak_check_test
        MODULE debugging
        SOURCES
        "leak_check_test.cc"
        CXXOPTS
        ${CARBIN_CXX_OPTIONS}
        LINKS
        turbo::turbo
        ${GTEST_LIB}
        ${GTEST_MAIN_LIB}
)

carbin_cc_test(
        NAME
        stack_consumption_test
        MODULE debugging
        SOURCES
        "stack_consumption_test.cc"
        CXXOPTS
        ${CARBIN_CXX_OPTIONS}
        LINKS
        turbo::stack_consumption
        turbo::turbo
        ${GTEST_LIB}
        ${GTEST_MAIN_LIB}
)
