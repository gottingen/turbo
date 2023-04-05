#
# Copyright 2019 The Turbo Authors.
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




############################################################################
# Unit tests in alpahabetical order.

turbo_cc_test(
  NAME
    flags_commandlineflag_test
  SRCS
    "commandlineflag_test.cc"
  COPTS
    ${TURBO_TEST_COPTS}
  DEPS
    turbo::turbo
    GTest::gtest_main
)

turbo_cc_test(
  NAME
    flags_config_test
  SRCS
    "config_test.cc"
  COPTS
    ${TURBO_TEST_COPTS}
  DEPS
    turbo::turbo
    GTest::gtest_main
)

turbo_cc_test(
  NAME
    flags_flag_test
  SRCS
    "flag_test.cc"
    "flag_test_defs.cc"
  COPTS
    ${TURBO_TEST_COPTS}
  DEPS
    turbo::turbo
    GTest::gtest_main
)

turbo_cc_test(
  NAME
    flags_marshalling_test
  SRCS
    "marshalling_test.cc"
  COPTS
    ${TURBO_TEST_COPTS}
  DEPS
    turbo::turbo
    GTest::gtest_main
)

turbo_cc_test(
  NAME
    flags_parse_test
  SRCS
    "parse_test.cc"
  COPTS
    ${TURBO_TEST_COPTS}
  DEPS
    turbo::turbo
    turbo::sync
    GTest::gmock_main
)

turbo_cc_test(
  NAME
    flags_path_util_test
  SRCS
    "internal/path_util_test.cc"
  COPTS
    ${TURBO_TEST_COPTS}
  DEPS
    turbo::turbo
    GTest::gtest_main
)

turbo_cc_test(
  NAME
    flags_program_name_test
  SRCS
    "internal/program_name_test.cc"
  COPTS
    ${TURBO_TEST_COPTS}
  DEPS
    turbo::turbo
    GTest::gtest_main
)

turbo_cc_test(
  NAME
    flags_reflection_test
  SRCS
    "reflection_test.cc"
  COPTS
    ${TURBO_TEST_COPTS}
  DEPS
    turbo::turbo
    GTest::gmock_main
)

turbo_cc_test(
  NAME
    flags_sequence_lock_test
  SRCS
    "internal/sequence_lock_test.cc"
  COPTS
    ${TURBO_TEST_COPTS}
  DEPS
    turbo::turbo
    GTest::gmock_main
)

turbo_cc_test(
  NAME
    flags_usage_config_test
  SRCS
    "usage_config_test.cc"
  COPTS
    ${TURBO_TEST_COPTS}
  DEPS
    turbo::turbo
    GTest::gtest_main
)

turbo_cc_test(
  NAME
    flags_usage_test
  SRCS
    "internal/usage_test.cc"
  COPTS
    ${TURBO_TEST_COPTS}
  DEPS
    turbo::turbo
    turbo::sync
    GTest::gmock
)
