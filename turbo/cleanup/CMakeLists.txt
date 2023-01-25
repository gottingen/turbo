# Copyright 2021 The Turbo Authors.
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

# Internal-only target, do not depend on directly.
turbo_cc_library(
  NAME
    cleanup_internal
  HDRS
    "internal/cleanup.h"
  COPTS
    ${TURBO_DEFAULT_COPTS}
  DEPS
    turbo::base_internal
    turbo::core_headers
    turbo::utility
  PUBLIC
)

turbo_cc_library(
  NAME
    cleanup
  HDRS
    "cleanup.h"
  COPTS
    ${TURBO_DEFAULT_COPTS}
  DEPS
    turbo::cleanup_internal
    turbo::config
    turbo::core_headers
  PUBLIC
)

turbo_cc_test(
  NAME
    cleanup_test
  SRCS
    "cleanup_test.cc"
  COPTS
    ${TURBO_TEST_COPTS}
  DEPS
    turbo::cleanup
    turbo::config
    turbo::utility
    GTest::gmock_main
)
