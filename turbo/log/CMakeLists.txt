#
# Copyright 2022 The Turbo Authors.
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
    log_internal_test_actions
  SRCS
    "internal/test_actions.cc"
  HDRS
    "internal/test_actions.h"
  COPTS
    ${TURBO_DEFAULT_COPTS}
  LINKOPTS
    ${TURBO_DEFAULT_LINKOPTS}
  DEPS
    turbo::turbo
  TESTONLY
)

turbo_cc_library(
  NAME
    log_internal_test_helpers
  SRCS
    "internal/test_helpers.cc"
  HDRS
    "internal/test_helpers.h"
  COPTS
    ${TURBO_DEFAULT_COPTS}
  LINKOPTS
    ${TURBO_DEFAULT_LINKOPTS}
  DEPS
    turbo::turbo
    GTest::gtest
  TESTONLY
)

turbo_cc_library(
  NAME
    log_internal_test_matchers
  SRCS
    "internal/test_matchers.cc"
  HDRS
    "internal/test_matchers.h"
  COPTS
    ${TURBO_DEFAULT_COPTS}
  LINKOPTS
    ${TURBO_DEFAULT_LINKOPTS}
  DEPS
    turbo::turbo
    GTest::gtest
    GTest::gmock
  TESTONLY
)

turbo_cc_library(
  NAME
    scoped_mock_log
  SRCS
    "scoped_mock_log.cc"
  HDRS
    "scoped_mock_log.h"
  COPTS
    ${TURBO_DEFAULT_COPTS}
  LINKOPTS
    ${TURBO_DEFAULT_LINKOPTS}
  DEPS
    turbo::turbo
    GTest::gmock
    GTest::gtest
  PUBLIC
  TESTONLY
)


# Test targets

turbo_cc_test(
  NAME
    die_if_null_test
  SRCS
    "die_if_null_test.cc"
  COPTS
    ${TURBO_TEST_COPTS}
  LINKOPTS
    ${TURBO_DEFAULT_LINKOPTS}
  DEPS
    turbo::turbo
    turbo::log_internal_test_helpers
    GTest::gtest_main
)


turbo_cc_test(
  NAME
    log_entry_test
  SRCS
    "log_entry_test.cc"
  COPTS
    ${TURBO_TEST_COPTS}
  LINKOPTS
    ${TURBO_DEFAULT_LINKOPTS}
  DEPS
    turbo::log_internal_test_helpers
    turbo::turbo
    GTest::gmock
    GTest::gtest_main
)

turbo_cc_test(
  NAME
    log_flags_test
  SRCS
    "flags_test.cc"
  COPTS
    ${TURBO_TEST_COPTS}
  LINKOPTS
    ${TURBO_DEFAULT_LINKOPTS}
  DEPS
    turbo::log_internal_test_helpers
    turbo::log_internal_test_matchers
    turbo::scoped_mock_log
    turbo::turbo
    GTest::gmock
    GTest::gtest_main
)

turbo_cc_test(
  NAME
    log_globals_test
  SRCS
    "globals_test.cc"
  COPTS
    ${TURBO_TEST_COPTS}
  LINKOPTS
    ${TURBO_DEFAULT_LINKOPTS}
  DEPS
    turbo::turbo
    turbo::log_internal_test_helpers
    turbo::scoped_mock_log
    GTest::gtest_main
)

turbo_cc_test(
  NAME
    log_format_test
  SRCS
    "log_format_test.cc"
  COPTS
    ${TURBO_TEST_COPTS}
  LINKOPTS
    ${TURBO_DEFAULT_LINKOPTS}
  DEPS
    turbo::turbo
    turbo::log_internal_test_matchers
    turbo::log_internal_test_helpers
    turbo::scoped_mock_log
    GTest::gmock
    GTest::gtest_main
)

turbo_cc_test(
  NAME
    log_macro_hygiene_test
  SRCS
    "log_macro_hygiene_test.cc"
  COPTS
    ${TURBO_TEST_COPTS}
  LINKOPTS
    ${TURBO_DEFAULT_LINKOPTS}
  DEPS
    turbo::turbo
    turbo::scoped_mock_log
    GTest::gmock
    GTest::gtest_main
)

turbo_cc_test(
  NAME
    log_sink_test
  SRCS
    "log_sink_test.cc"
  COPTS
    ${TURBO_TEST_COPTS}
  LINKOPTS
    ${TURBO_DEFAULT_LINKOPTS}
  DEPS
    turbo::turbo
    turbo::log_internal_test_actions
    turbo::log_internal_test_helpers
    turbo::log_internal_test_matchers
    turbo::scoped_mock_log
    GTest::gtest_main
)

turbo_cc_test(
  NAME
    log_streamer_test
  SRCS
    "log_streamer_test.cc"
  COPTS
    ${TURBO_TEST_COPTS}
  LINKOPTS
    ${TURBO_DEFAULT_LINKOPTS}
  DEPS
    turbo::turbo
    turbo::log_internal_test_actions
    turbo::log_internal_test_helpers
    turbo::log_internal_test_matchers
    turbo::scoped_mock_log
    GTest::gtest_main
)

turbo_cc_test(
  NAME
    log_modifier_methods_test
  SRCS
    "log_modifier_methods_test.cc"
  COPTS
    ${TURBO_TEST_COPTS}
  LINKOPTS
    ${TURBO_DEFAULT_LINKOPTS}
  DEPS
    turbo::turbo
    turbo::log_internal_test_actions
    turbo::log_internal_test_helpers
    turbo::log_internal_test_matchers
    turbo::scoped_mock_log
    GTest::gmock
    GTest::gtest_main
)

turbo_cc_test(
  NAME
    scoped_mock_log_test
  SRCS
    "scoped_mock_log_test.cc"
  COPTS
    ${TURBO_TEST_COPTS}
  LINKOPTS
    ${TURBO_DEFAULT_LINKOPTS}
  DEPS
    turbo::turbo
    turbo::log_internal_test_helpers
    turbo::log_internal_test_matchers
    turbo::scoped_mock_log
    GTest::gtest_main
)

turbo_cc_test(
  NAME
    log_internal_stderr_log_sink_test
  SRCS
    "internal/stderr_log_sink_test.cc"
  COPTS
    ${TURBO_TEST_COPTS}
  LINKOPTS
    ${TURBO_DEFAULT_LINKOPTS}
  DEPS
    turbo::turbo
    turbo::log_internal_test_helpers
    GTest::gmock
    GTest::gtest_main
)

turbo_cc_test(
  NAME
    log_stripping_test
  SRCS
    "stripping_test.cc"
  COPTS
    ${TURBO_TEST_COPTS}
  LINKOPTS
    ${TURBO_DEFAULT_LINKOPTS}
  DEPS
    turbo::turbo
    turbo::log_internal_test_helpers
    GTest::gmock
    GTest::gtest_main
)

turbo_cc_test(
  NAME
    log_structured_test
  SRCS
    "structured_test.cc"
  COPTS
    ${TURBO_TEST_COPTS}
  LINKOPTS
    ${TURBO_DEFAULT_LINKOPTS}
  DEPS
    turbo::turbo
    turbo::log_internal_test_helpers
    turbo::log_internal_test_matchers
    turbo::scoped_mock_log
    GTest::gmock
    GTest::gtest_main
)
