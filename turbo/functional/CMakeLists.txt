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

turbo_cc_library(
  NAME
    any_invocable
  SRCS
    "internal/any_invocable.h"
  HDRS
    "any_invocable.h"
  COPTS
    ${TURBO_DEFAULT_COPTS}
  DEPS
    turbo::base_internal
    turbo::config
    turbo::core_headers
    turbo::type_traits
    turbo::utility
  PUBLIC
)

turbo_cc_test(
  NAME
    any_invocable_test
  SRCS
    "any_invocable_test.cc"
    "internal/any_invocable.h"
  COPTS
    ${TURBO_DEFAULT_COPTS}
  DEPS
    turbo::any_invocable
    turbo::base_internal
    turbo::config
    turbo::core_headers
    turbo::type_traits
    turbo::utility
    GTest::gmock_main
)

turbo_cc_library(
  NAME
    bind_front
  SRCS
    "internal/front_binder.h"
  HDRS
    "bind_front.h"
  COPTS
    ${TURBO_DEFAULT_COPTS}
  DEPS
    turbo::base_internal
    turbo::compressed_tuple
  PUBLIC
)

turbo_cc_test(
  NAME
    bind_front_test
  SRCS
    "bind_front_test.cc"
  COPTS
    ${TURBO_DEFAULT_COPTS}
  DEPS
    turbo::bind_front
    turbo::memory
    GTest::gmock_main
)

turbo_cc_library(
  NAME
    function_ref
  SRCS
    "internal/function_ref.h"
  HDRS
    "function_ref.h"
  COPTS
    ${TURBO_DEFAULT_COPTS}
  DEPS
    turbo::base_internal
    turbo::core_headers
    turbo::meta
  PUBLIC
)

turbo_cc_test(
  NAME
    function_ref_test
  SRCS
    "function_ref_test.cc"
  COPTS
    ${TURBO_TEST_COPTS}
  DEPS
    turbo::function_ref
    turbo::memory
    turbo::test_instance_tracker
    GTest::gmock_main
)
