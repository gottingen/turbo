#
# Copyright 2020 The Abseil Authors.
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
absl_cc_library(
  NAME
    status
  HDRS
    "status.h"
  SRCS
    "internal/status_internal.h"
    "status.cc"
    "status_payload_printer.h"
    "status_payload_printer.cc"
  COPTS
    ${ABSL_DEFAULT_COPTS}
  DEPS
    turbo::atomic_hook
    turbo::config
    turbo::cord
    turbo::core_headers
    turbo::function_ref
    turbo::inlined_vector
    turbo::optional
    turbo::raw_logging_internal
    turbo::stacktrace
    turbo::str_format
    turbo::strerror
    turbo::strings
    turbo::symbolize
  PUBLIC
)

absl_cc_test(
  NAME
    status_test
  SRCS
   "status_test.cc"
  COPTS
    ${ABSL_TEST_COPTS}
  DEPS
    turbo::status
    turbo::strings
    GTest::gmock_main
)

absl_cc_library(
  NAME
    statusor
  HDRS
    "statusor.h"
  SRCS
    "statusor.cc"
    "internal/statusor_internal.h"
  COPTS
    ${ABSL_DEFAULT_COPTS}
  DEPS
    turbo::base
    turbo::status
    turbo::core_headers
    turbo::raw_logging_internal
    turbo::type_traits
    turbo::strings
    turbo::utility
    turbo::variant
  PUBLIC
)

absl_cc_test(
  NAME
    statusor_test
  SRCS
   "statusor_test.cc"
  COPTS
    ${ABSL_TEST_COPTS}
  DEPS
    turbo::status
    turbo::statusor
    GTest::gmock_main
)
