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

turbo_cc_test(
  NAME
    status_test
  SRCS
   "status_test.cc"
  COPTS
    ${TURBO_TEST_COPTS}
  DEPS
    turbo::turbo
    GTest::gmock_main
)

turbo_cc_test(
        NAME
        throw_delegate_test
        SRCS
        "internal/throw_delegate_test.cc"
        COPTS
        ${TURBO_TEST_COPTS}
        DEPS
        turbo::turbo
        GTest::gtest_main
)

turbo_cc_test(
        NAME
        call_once_test
        SRCS
        "call_once_test.cc"
        COPTS
        ${TURBO_TEST_COPTS}
        DEPS
        turbo::turbo
        GTest::gtest_main
)

turbo_cc_test(
  NAME
    statusor_test
  SRCS
   "statusor_test.cc"
  COPTS
    ${TURBO_TEST_COPTS}
  DEPS
    turbo::turbo
    GTest::gmock_main
)


turbo_cc_test(
        NAME
        bits_test
        SRCS
        "bits_test.cc"
        COPTS
        ${TURBO_TEST_COPTS}
        DEPS
        turbo::turbo
        GTest::gmock_main
)

turbo_cc_test(
        NAME
        int128_test
        SRCS
        "int128_stream_test.cc"
        "int128_test.cc"
        COPTS
        ${TURBO_TEST_COPTS}
        DEPS
        turbo::turbo
        GTest::gmock_main
)




# Internal-only target, do not depend on directly.
turbo_cc_library(
        NAME
        exception_testing
        HDRS
        "internal/exception_testing.h"
        COPTS
        ${TURBO_DEFAULT_COPTS}
        DEPS
        turbo::turbo
        GTest::gtest
        TESTONLY
)


# Internal-only target, do not depend on directly.
turbo_cc_library(
        NAME
        exception_safety_testing
        HDRS
        "internal/exception_safety_testing.h"
        SRCS
        "internal/exception_safety_testing.cc"
        COPTS
        ${TURBO_TEST_COPTS}
        DEPS
        GTest::gtest
        TESTONLY
)


turbo_cc_test(
        NAME
        turbo_exception_safety_testing_test
        SRCS
        "exception_safety_testing_test.cc"
        COPTS
        ${TURBO_TEST_COPTS}
        DEPS
        turbo::turbo
        turbo::exception_safety_testing
        GTest::gtest_main
)

turbo_cc_test(
        NAME
        bit_cast_test
        SRCS
        "bit_cast_test.cc"
        COPTS
        ${TURBO_TEST_COPTS}
        DEPS
        turbo::turbo
        GTest::gtest_main
)

turbo_cc_test(
        NAME
        inline_variable_test
        SRCS
        "internal/inline_variable_testing.h"
        "inline_variable_test.cc"
        "inline_variable_test_a.cc"
        "inline_variable_test_b.cc"
        COPTS
        ${TURBO_TEST_COPTS}
        DEPS
        turbo::turbo
        GTest::gtest_main
)


turbo_cc_test(
        NAME
        invoke_test
        SRCS
        "invoke_test.cc"
        COPTS
        ${TURBO_TEST_COPTS}
        DEPS
        turbo::turbo
        GTest::gmock
        GTest::gtest_main
)


turbo_cc_test(
        NAME
        raw_logging_test
        SRCS
        "raw_logging_test.cc"
        COPTS
        ${TURBO_TEST_COPTS}
        DEPS
        turbo::turbo
        GTest::gtest_main
)

turbo_cc_test(
        NAME
        log_severity_test
        SRCS
        "log_severity_test.cc"
        DEPS
        turbo::turbo
        GTest::gmock
        GTest::gtest_main
)


turbo_cc_test(
        NAME
        fast_type_id_test
        SRCS
        "internal/fast_type_id_test.cc"
        COPTS
        ${TURBO_TEST_COPTS}
        DEPS
        turbo::turbo
        GTest::gtest_main
)


turbo_cc_test(
        NAME
        strerror_test
        SRCS
        "internal/strerror_test.cc"
        COPTS
        ${TURBO_TEST_COPTS}
        DEPS
        turbo::turbo
        GTest::gmock
        GTest::gtest_main
)

