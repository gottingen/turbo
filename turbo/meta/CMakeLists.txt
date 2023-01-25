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

turbo_cc_library(
  NAME
    type_traits
  HDRS
    "type_traits.h"
  COPTS
    ${TURBO_DEFAULT_COPTS}
  DEPS
    turbo::config
  PUBLIC
)

turbo_cc_test(
  NAME
    type_traits_test
  SRCS
    "type_traits_test.cc"
  COPTS
    ${TURBO_TEST_COPTS}
  DEPS
    turbo::config
    turbo::time
    turbo::core_headers
    turbo::type_traits
    GTest::gmock_main
)

# component target
turbo_cc_library(
  NAME
    meta
  COPTS
    ${TURBO_DEFAULT_COPTS}
  DEPS
    turbo::type_traits
  PUBLIC
)
