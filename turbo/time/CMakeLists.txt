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


# Internal-only target, do not depend on directly.
turbo_cc_library(
  NAME
    time_internal_test_util
  HDRS
    "internal/test_util.h"
  SRCS
    "internal/test_util.cc"
  COPTS
    ${TURBO_DEFAULT_COPTS}
  DEPS
    turbo::turbo
  TESTONLY
)

turbo_cc_test(
  NAME
    time_test
  SRCS
    "civil_time_test.cc"
    "clock_test.cc"
    "duration_test.cc"
    "format_test.cc"
    "time_test.cc"
    "time_zone_test.cc"
  COPTS
    ${TURBO_TEST_COPTS}
  DEPS
    turbo::time_internal_test_util
    turbo::turbo
    GTest::gmock_main
)

turbo_cc_test(
  NAME
    flag_test
  SRCS
    "flag_test.cc"
  COPTS
    ${TURBO_TEST_COPTS}
  DEPS
    turbo::turbo
    GTest::gmock_main
)
