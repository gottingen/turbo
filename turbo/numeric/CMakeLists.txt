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
    bits
  HDRS
    "bits.h"
    "internal/bits.h"
  COPTS
    ${TURBO_DEFAULT_COPTS}
  DEPS
    turbo::core_headers
  PUBLIC
)

turbo_cc_test(
  NAME
    bits_test
  SRCS
    "bits_test.cc"
  COPTS
    ${TURBO_TEST_COPTS}
  DEPS
    turbo::bits
    turbo::core_headers
    turbo::random_random
    GTest::gmock_main
)

turbo_cc_library(
  NAME
    int128
  HDRS
    "int128.h"
  SRCS
    "int128.cc"
    "int128_have_intrinsic.inc"
    "int128_no_intrinsic.inc"
  COPTS
    ${TURBO_DEFAULT_COPTS}
  DEPS
    turbo::config
    turbo::core_headers
    turbo::bits
  PUBLIC
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
    turbo::int128
    turbo::base
    turbo::hash_testing
    turbo::type_traits
    GTest::gmock_main
)

# component target
turbo_cc_library(
  NAME
    numeric
  COPTS
    ${TURBO_DEFAULT_COPTS}
  DEPS
    turbo::int128
  PUBLIC
)

turbo_cc_library(
  NAME
    numeric_representation
  HDRS
    "internal/representation.h"
  COPTS
    ${TURBO_DEFAULT_COPTS}
  DEPS
    turbo::config
  PUBLIC
)
