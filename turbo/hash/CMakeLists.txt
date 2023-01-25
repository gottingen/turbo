#
# Copyright 2018 The Abseil Authors.
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
    hash
  HDRS
    "hash.h"
  SRCS
    "internal/hash.cc"
    "internal/hash.h"
  COPTS
    ${ABSL_DEFAULT_COPTS}
  DEPS
    turbo::bits
    turbo::city
    turbo::config
    turbo::core_headers
    turbo::endian
    turbo::fixed_array
    turbo::function_ref
    turbo::meta
    turbo::int128
    turbo::strings
    turbo::optional
    turbo::variant
    turbo::utility
    turbo::low_level_hash
  PUBLIC
)

absl_cc_library(
  NAME
    hash_testing
  HDRS
    "hash_testing.h"
  COPTS
    ${ABSL_TEST_COPTS}
  DEPS
    turbo::spy_hash_state
    turbo::meta
    turbo::strings
    turbo::variant
    GTest::gmock
  TESTONLY
  PUBLIC
)

absl_cc_test(
  NAME
    hash_test
  SRCS
    "hash_test.cc"
  COPTS
    ${ABSL_TEST_COPTS}
  DEPS
    turbo::cord_test_helpers
    turbo::hash
    turbo::hash_testing
    turbo::core_headers
    turbo::btree
    turbo::flat_hash_map
    turbo::flat_hash_set
    turbo::node_hash_map
    turbo::node_hash_set
    turbo::spy_hash_state
    turbo::meta
    turbo::int128
    GTest::gmock_main
)

# Internal-only target, do not depend on directly.
#
# Note: Even though external code should not depend on this target
# directly, it must be marked PUBLIC since it is a dependency of
# hash_testing.
absl_cc_library(
  NAME
    spy_hash_state
  HDRS
    "internal/spy_hash_state.h"
  COPTS
    ${ABSL_DEFAULT_COPTS}
  DEPS
    turbo::hash
    turbo::strings
    turbo::str_format
  TESTONLY
  PUBLIC
)

# Internal-only target, do not depend on directly.
absl_cc_library(
  NAME
    city
  HDRS
    "internal/city.h"
  SRCS
    "internal/city.cc"
  COPTS
    ${ABSL_DEFAULT_COPTS}
  DEPS
    turbo::config
    turbo::core_headers
    turbo::endian
)

absl_cc_test(
  NAME
    city_test
  SRCS
    "internal/city_test.cc"
  COPTS
    ${ABSL_TEST_COPTS}
  DEPS
    turbo::city
    GTest::gmock_main
)

# Internal-only target, do not depend on directly.
absl_cc_library(
  NAME
    low_level_hash
  HDRS
    "internal/low_level_hash.h"
  SRCS
    "internal/low_level_hash.cc"
  COPTS
    ${ABSL_DEFAULT_COPTS}
  DEPS
    turbo::config
    turbo::endian
    turbo::int128
)

absl_cc_test(
  NAME
    low_level_hash_test
  SRCS
    "internal/low_level_hash_test.cc"
  COPTS
    ${ABSL_TEST_COPTS}
  DEPS
    turbo::low_level_hash
    turbo::strings
    GTest::gmock_main
)
