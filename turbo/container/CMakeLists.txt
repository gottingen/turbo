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

# Internal-only target, do not depend on directly.
turbo_cc_library(
  NAME
    btree_test_common
  hdrs
    "btree_test.h"
  COPTS
    ${TURBO_TEST_COPTS}
  LINKOPTS
    ${TURBO_DEFAULT_LINKOPTS}
  DEPS
    turbo::turbo
  TESTONLY
)

turbo_cc_test(
  NAME
    btree_test
  SRCS
    "btree_test.cc"
  COPTS
    ${TURBO_TEST_COPTS}
  LINKOPTS
    ${TURBO_DEFAULT_LINKOPTS}
  DEPS
    turbo::turbo
    turbo::btree_test_common
    turbo::hash_testing
    turbo::test_instance_tracker
    GTest::gmock_main
)


turbo_cc_test(
  NAME
    compressed_tuple_test
  SRCS
    "internal/compressed_tuple_test.cc"
  COPTS
    ${TURBO_TEST_COPTS}
  DEPS
    turbo::turbo
    turbo::test_instance_tracker
    GTest::gmock_main
)

turbo_cc_test(
  NAME
    fixed_array_test
  SRCS
    "fixed_array_test.cc"
  COPTS
    ${TURBO_TEST_COPTS}
  DEPS
    turbo::exception_testing
    turbo::hash_testing
    GTest::gmock_main
)

turbo_cc_test(
  NAME
    fixed_array_exception_safety_test
  SRCS
    "fixed_array_exception_safety_test.cc"
  COPTS
    ${TURBO_TEST_COPTS}
  DEPS
    turbo::exception_safety_testing
    turbo::turbo
    GTest::gmock_main
)



turbo_cc_test(
  NAME
    inlined_vector_test
  SRCS
    "inlined_vector_test.cc"
  COPTS
    ${TURBO_TEST_COPTS}
  DEPS
    turbo::test_instance_tracker
    turbo::exception_testing
    turbo::hash_testing
    turbo::turbo
    GTest::gmock_main
)

turbo_cc_test(
  NAME
    inlined_vector_exception_safety_test
  SRCS
    "inlined_vector_exception_safety_test.cc"
  COPTS
    ${TURBO_TEST_COPTS}
  DEPS
    turbo::turbo
    turbo::exception_safety_testing
    GTest::gmock_main
)

# Internal-only target, do not depend on directly.
turbo_cc_library(
  NAME
    test_instance_tracker
  HDRS
    "internal/test_instance_tracker.h"
  SRCS
    "internal/test_instance_tracker.cc"
  COPTS
    ${TURBO_DEFAULT_COPTS}
  DEPS
    turbo::turbo
  TESTONLY
)

turbo_cc_test(
  NAME
    test_instance_tracker_test
  SRCS
    "internal/test_instance_tracker_test.cc"
  COPTS
    ${TURBO_TEST_COPTS}
  DEPS
    turbo::test_instance_tracker
    GTest::gmock_main
)

turbo_cc_test(
  NAME
    flat_hash_map_test
  SRCS
    "flat_hash_map_test.cc"
  COPTS
    ${TURBO_TEST_COPTS}
  DEPS
    turbo::hash_generator_testing
    turbo::unordered_map_constructor_test
    turbo::unordered_map_lookup_test
    turbo::unordered_map_members_test
    turbo::unordered_map_modifiers_test
    turbo::turbo
    GTest::gmock_main
)

turbo_cc_test(
  NAME
    flat_hash_set_test
  SRCS
    "flat_hash_set_test.cc"
  COPTS
    ${TURBO_TEST_COPTS}
    "-DUNORDERED_SET_CXX17"
  DEPS
    turbo::hash_generator_testing
    turbo::unordered_set_constructor_test
    turbo::unordered_set_lookup_test
    turbo::unordered_set_members_test
    turbo::unordered_set_modifiers_test
    turbo::turbo
    GTest::gmock_main
)

turbo_cc_test(
  NAME
    node_hash_map_test
  SRCS
    "node_hash_map_test.cc"
  COPTS
    ${TURBO_TEST_COPTS}
  DEPS
    turbo::hash_generator_testing
    turbo::turbo
    turbo::unordered_map_constructor_test
    turbo::unordered_map_lookup_test
    turbo::unordered_map_members_test
    turbo::unordered_map_modifiers_test
    GTest::gmock_main
)


turbo_cc_test(
  NAME
    node_hash_set_test
  SRCS
    "node_hash_set_test.cc"
  COPTS
    ${TURBO_TEST_COPTS}
    "-DUNORDERED_SET_CXX17"
  DEPS
    turbo::hash_generator_testing
    turbo::turbo
    turbo::unordered_set_constructor_test
    turbo::unordered_set_lookup_test
    turbo::unordered_set_members_test
    turbo::unordered_set_modifiers_test
    GTest::gmock_main
)

turbo_cc_test(
  NAME
    container_memory_test
  SRCS
    "internal/container_memory_test.cc"
  COPTS
    ${TURBO_TEST_COPTS}
  DEPS
    turbo::turbo
    turbo::test_instance_tracker
    GTest::gmock_main
)

turbo_cc_test(
  NAME
    hash_function_defaults_test
  SRCS
    "internal/hash_function_defaults_test.cc"
  COPTS
    ${TURBO_TEST_COPTS}
  DEPS
    turbo::turbo
    GTest::gmock_main
)

# Internal-only target, do not depend on directly.
turbo_cc_library(
  NAME
    hash_generator_testing
  HDRS
    "internal/hash_generator_testing.h"
  SRCS
    "internal/hash_generator_testing.cc"
  COPTS
    ${TURBO_TEST_COPTS}
  DEPS
    turbo::hash_policy_testing
    turbo::turbo
  TESTONLY
)

# Internal-only target, do not depend on directly.
turbo_cc_library(
  NAME
    hash_policy_testing
  HDRS
    "internal/hash_policy_testing.h"
  COPTS
    ${TURBO_TEST_COPTS}
  DEPS
    turbo::turbo
  TESTONLY
)

turbo_cc_test(
  NAME
    hash_policy_testing_test
  SRCS
    "internal/hash_policy_testing_test.cc"
  COPTS
    ${TURBO_TEST_COPTS}
  DEPS
    turbo::turbo
    GTest::gmock_main
)



turbo_cc_test(
  NAME
    hash_policy_traits_test
  SRCS
    "internal/hash_policy_traits_test.cc"
  COPTS
    ${TURBO_TEST_COPTS}
  DEPS
    turbo::turbo
    GTest::gmock_main
)


turbo_cc_test(
  NAME
    common_policy_traits_test
  SRCS
    "internal/common_policy_traits_test.cc"
  COPTS
    ${TURBO_TEST_COPTS}
  DEPS
    turbo::turbo
    GTest::gmock_main
)


turbo_cc_test(
  NAME
    hashtablez_sampler_test
  SRCS
    "internal/hashtablez_sampler_test.cc"
  COPTS
    ${TURBO_TEST_COPTS}
  DEPS
    turbo::turbo
    GTest::gmock_main
)



turbo_cc_test(
  NAME
    node_slot_policy_test
  SRCS
    "internal/node_slot_policy_test.cc"
  COPTS
    ${TURBO_TEST_COPTS}
  DEPS
    turbo::turbo
    GTest::gmock_main
)

turbo_cc_test(
  NAME
    raw_hash_set_test
  SRCS
    "internal/raw_hash_set_test.cc"
  COPTS
    ${TURBO_TEST_COPTS}
  DEPS
    turbo::hash_policy_testing
    turbo::turbo
    GTest::gmock_main
)

turbo_cc_test(
  NAME
    raw_hash_set_allocator_test
  SRCS
    "internal/raw_hash_set_allocator_test.cc"
  COPTS
    ${TURBO_TEST_COPTS}
  DEPS
    turbo::turbo
    GTest::gmock_main
)


turbo_cc_test(
  NAME
    layout_test
  SRCS
    "internal/layout_test.cc"
  COPTS
    ${TURBO_TEST_COPTS}
  DEPS
    turbo::turbo
    GTest::gmock_main
)

# Internal-only target, do not depend on directly.
turbo_cc_library(
  NAME
    tracked
  HDRS
    "internal/tracked.h"
  COPTS
    ${TURBO_TEST_COPTS}
  DEPS
    turbo::turbo
  TESTONLY
)

# Internal-only target, do not depend on directly.
turbo_cc_library(
  NAME
    unordered_map_constructor_test
  HDRS
    "internal/unordered_map_constructor_test.h"
  COPTS
    ${TURBO_TEST_COPTS}
  DEPS
    turbo::hash_generator_testing
    turbo::hash_policy_testing
    GTest::gmock
  TESTONLY
)

# Internal-only target, do not depend on directly.
turbo_cc_library(
  NAME
    unordered_map_lookup_test
  HDRS
    "internal/unordered_map_lookup_test.h"
  COPTS
    ${TURBO_TEST_COPTS}
  DEPS
    turbo::hash_generator_testing
    turbo::hash_policy_testing
    GTest::gmock
  TESTONLY
)

# Internal-only target, do not depend on directly.
turbo_cc_library(
  NAME
    unordered_map_members_test
  HDRS
    "internal/unordered_map_members_test.h"
  COPTS
    ${TURBO_TEST_COPTS}
  DEPS
    turbo::turbo
    GTest::gmock
  TESTONLY
)

# Internal-only target, do not depend on directly.
turbo_cc_library(
  NAME
    unordered_map_modifiers_test
  HDRS
    "internal/unordered_map_modifiers_test.h"
  COPTS
    ${TURBO_TEST_COPTS}
  DEPS
    turbo::hash_generator_testing
    turbo::hash_policy_testing
    GTest::gmock
  TESTONLY
)

# Internal-only target, do not depend on directly.
turbo_cc_library(
  NAME
    unordered_set_constructor_test
  HDRS
    "internal/unordered_set_constructor_test.h"
  COPTS
    ${TURBO_TEST_COPTS}
  DEPS
    turbo::hash_generator_testing
    turbo::hash_policy_testing
    GTest::gmock
  TESTONLY
)

# Internal-only target, do not depend on directly.
turbo_cc_library(
  NAME
    unordered_set_lookup_test
  HDRS
    "internal/unordered_set_lookup_test.h"
  COPTS
    ${TURBO_TEST_COPTS}
  DEPS
    turbo::hash_generator_testing
    turbo::hash_policy_testing
    GTest::gmock
  TESTONLY
)

# Internal-only target, do not depend on directly.
turbo_cc_library(
  NAME
    unordered_set_members_test
  HDRS
    "internal/unordered_set_members_test.h"
  COPTS
    ${TURBO_TEST_COPTS}
  DEPS
    turbo::turbo
    GTest::gmock
  TESTONLY
)

# Internal-only target, do not depend on directly.
turbo_cc_library(
  NAME
    unordered_set_modifiers_test
  HDRS
    "internal/unordered_set_modifiers_test.h"
  COPTS
    ${TURBO_TEST_COPTS}
  DEPS
    turbo::hash_generator_testing
    turbo::hash_policy_testing
    GTest::gmock
  TESTONLY
)

turbo_cc_test(
  NAME
    unordered_set_test
  SRCS
    "internal/unordered_set_test.cc"
  COPTS
    ${TURBO_TEST_COPTS}
  DEPS
    turbo::unordered_set_constructor_test
    turbo::unordered_set_lookup_test
    turbo::unordered_set_members_test
    turbo::unordered_set_modifiers_test
    GTest::gmock_main
)

turbo_cc_test(
  NAME
    unordered_map_test
  SRCS
    "internal/unordered_map_test.cc"
  COPTS
    ${TURBO_TEST_COPTS}
  DEPS
    turbo::unordered_map_constructor_test
    turbo::unordered_map_lookup_test
    turbo::unordered_map_members_test
    turbo::unordered_map_modifiers_test
    GTest::gmock_main
)

turbo_cc_test(
  NAME
    sample_element_size_test
  SRCS
    "sample_element_size_test.cc"
  COPTS
    ${TURBO_TEST_COPTS}
  DEPS
    turbo::turbo
    GTest::gmock_main
)
