#
# Copyright 2017 The Abseil Authors.
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
    btree
  HDRS
    "btree_map.h"
    "btree_set.h"
    "internal/btree.h"
    "internal/btree_container.h"
  COPTS
    ${ABSL_DEFAULT_COPTS}
  LINKOPTS
    ${ABSL_DEFAULT_LINKOPTS}
  DEPS
    turbo::container_common
    turbo::common_policy_traits
    turbo::compare
    turbo::compressed_tuple
    turbo::container_memory
    turbo::cord
    turbo::core_headers
    turbo::layout
    turbo::memory
    turbo::raw_logging_internal
    turbo::strings
    turbo::throw_delegate
    turbo::type_traits
    turbo::utility
)

# Internal-only target, do not depend on directly.
absl_cc_library(
  NAME
    btree_test_common
  hdrs
    "btree_test.h"
  COPTS
    ${ABSL_TEST_COPTS}
  LINKOPTS
    ${ABSL_DEFAULT_LINKOPTS}
  DEPS
    turbo::btree
    turbo::cord
    turbo::flat_hash_set
    turbo::strings
    turbo::time
  TESTONLY
)

absl_cc_test(
  NAME
    btree_test
  SRCS
    "btree_test.cc"
  COPTS
    ${ABSL_TEST_COPTS}
  LINKOPTS
    ${ABSL_DEFAULT_LINKOPTS}
  DEPS
    turbo::algorithm_container
    turbo::btree
    turbo::btree_test_common
    turbo::compare
    turbo::core_headers
    turbo::counting_allocator
    turbo::flags
    turbo::hash_testing
    turbo::random_random
    turbo::raw_logging_internal
    turbo::strings
    turbo::test_instance_tracker
    GTest::gmock_main
)

# Internal-only target, do not depend on directly.
absl_cc_library(
  NAME
    compressed_tuple
  HDRS
    "internal/compressed_tuple.h"
  COPTS
    ${ABSL_DEFAULT_COPTS}
  DEPS
    turbo::utility
  PUBLIC
)

absl_cc_test(
  NAME
    compressed_tuple_test
  SRCS
    "internal/compressed_tuple_test.cc"
  COPTS
    ${ABSL_TEST_COPTS}
  DEPS
    turbo::any
    turbo::compressed_tuple
    turbo::memory
    turbo::optional
    turbo::test_instance_tracker
    turbo::utility
    GTest::gmock_main
)

absl_cc_library(
  NAME
    fixed_array
  HDRS
   "fixed_array.h"
  COPTS
    ${ABSL_DEFAULT_COPTS}
  DEPS
    turbo::compressed_tuple
    turbo::algorithm
    turbo::config
    turbo::core_headers
    turbo::dynamic_annotations
    turbo::throw_delegate
    turbo::memory
  PUBLIC
)

absl_cc_test(
  NAME
    fixed_array_test
  SRCS
    "fixed_array_test.cc"
  COPTS
    ${ABSL_TEST_COPTS}
  DEPS
    turbo::fixed_array
    turbo::counting_allocator
    turbo::config
    turbo::exception_testing
    turbo::hash_testing
    turbo::memory
    GTest::gmock_main
)

absl_cc_test(
  NAME
    fixed_array_exception_safety_test
  SRCS
    "fixed_array_exception_safety_test.cc"
  COPTS
    ${ABSL_TEST_COPTS}
  DEPS
    turbo::fixed_array
    turbo::config
    turbo::exception_safety_testing
    GTest::gmock_main
)

# Internal-only target, do not depend on directly.
absl_cc_library(
  NAME
    inlined_vector_internal
  HDRS
   "internal/inlined_vector.h"
  COPTS
    ${ABSL_DEFAULT_COPTS}
  DEPS
    turbo::compressed_tuple
    turbo::core_headers
    turbo::memory
    turbo::span
    turbo::type_traits
  PUBLIC
)

absl_cc_library(
  NAME
    inlined_vector
  HDRS
   "inlined_vector.h"
  COPTS
    ${ABSL_DEFAULT_COPTS}
  DEPS
    turbo::algorithm
    turbo::core_headers
    turbo::inlined_vector_internal
    turbo::throw_delegate
    turbo::memory
    turbo::type_traits
  PUBLIC
)

# Internal-only target, do not depend on directly.
absl_cc_library(
  NAME
    counting_allocator
  HDRS
    "internal/counting_allocator.h"
  COPTS
    ${ABSL_DEFAULT_COPTS}
  DEPS
    turbo::config
)

absl_cc_test(
  NAME
    inlined_vector_test
  SRCS
    "inlined_vector_test.cc"
  COPTS
    ${ABSL_TEST_COPTS}
  DEPS
    turbo::counting_allocator
    turbo::inlined_vector
    turbo::test_instance_tracker
    turbo::config
    turbo::core_headers
    turbo::exception_testing
    turbo::hash_testing
    turbo::memory
    turbo::raw_logging_internal
    turbo::strings
    GTest::gmock_main
)

absl_cc_test(
  NAME
    inlined_vector_exception_safety_test
  SRCS
    "inlined_vector_exception_safety_test.cc"
  COPTS
    ${ABSL_TEST_COPTS}
  DEPS
    turbo::inlined_vector
    turbo::config
    turbo::exception_safety_testing
    GTest::gmock_main
)

# Internal-only target, do not depend on directly.
absl_cc_library(
  NAME
    test_instance_tracker
  HDRS
    "internal/test_instance_tracker.h"
  SRCS
    "internal/test_instance_tracker.cc"
  COPTS
    ${ABSL_DEFAULT_COPTS}
  DEPS
    turbo::compare
  TESTONLY
)

absl_cc_test(
  NAME
    test_instance_tracker_test
  SRCS
    "internal/test_instance_tracker_test.cc"
  COPTS
    ${ABSL_TEST_COPTS}
  DEPS
    turbo::test_instance_tracker
    GTest::gmock_main
)

absl_cc_library(
  NAME
    flat_hash_map
  HDRS
    "flat_hash_map.h"
  COPTS
    ${ABSL_DEFAULT_COPTS}
  DEPS
    turbo::container_memory
    turbo::core_headers
    turbo::hash_function_defaults
    turbo::raw_hash_map
    turbo::algorithm_container
    turbo::memory
  PUBLIC
)

absl_cc_test(
  NAME
    flat_hash_map_test
  SRCS
    "flat_hash_map_test.cc"
  COPTS
    ${ABSL_TEST_COPTS}
  DEPS
    turbo::flat_hash_map
    turbo::hash_generator_testing
    turbo::unordered_map_constructor_test
    turbo::unordered_map_lookup_test
    turbo::unordered_map_members_test
    turbo::unordered_map_modifiers_test
    turbo::any
    turbo::raw_logging_internal
    GTest::gmock_main
)

absl_cc_library(
  NAME
    flat_hash_set
  HDRS
    "flat_hash_set.h"
  COPTS
    ${ABSL_DEFAULT_COPTS}
  DEPS
    turbo::container_memory
    turbo::hash_function_defaults
    turbo::raw_hash_set
    turbo::algorithm_container
    turbo::core_headers
    turbo::memory
  PUBLIC
)

absl_cc_test(
  NAME
    flat_hash_set_test
  SRCS
    "flat_hash_set_test.cc"
  COPTS
    ${ABSL_TEST_COPTS}
    "-DUNORDERED_SET_CXX17"
  DEPS
    turbo::flat_hash_set
    turbo::hash_generator_testing
    turbo::unordered_set_constructor_test
    turbo::unordered_set_lookup_test
    turbo::unordered_set_members_test
    turbo::unordered_set_modifiers_test
    turbo::memory
    turbo::raw_logging_internal
    turbo::strings
    GTest::gmock_main
)

absl_cc_library(
  NAME
    node_hash_map
  HDRS
    "node_hash_map.h"
  COPTS
    ${ABSL_DEFAULT_COPTS}
  DEPS
    turbo::container_memory
    turbo::core_headers
    turbo::hash_function_defaults
    turbo::node_slot_policy
    turbo::raw_hash_map
    turbo::algorithm_container
    turbo::memory
  PUBLIC
)

absl_cc_test(
  NAME
    node_hash_map_test
  SRCS
    "node_hash_map_test.cc"
  COPTS
    ${ABSL_TEST_COPTS}
  DEPS
    turbo::hash_generator_testing
    turbo::node_hash_map
    turbo::tracked
    turbo::unordered_map_constructor_test
    turbo::unordered_map_lookup_test
    turbo::unordered_map_members_test
    turbo::unordered_map_modifiers_test
    GTest::gmock_main
)

absl_cc_library(
  NAME
    node_hash_set
  HDRS
    "node_hash_set.h"
  COPTS
    ${ABSL_DEFAULT_COPTS}
  DEPS
    turbo::core_headers
    turbo::hash_function_defaults
    turbo::node_slot_policy
    turbo::raw_hash_set
    turbo::algorithm_container
    turbo::memory
  PUBLIC
)

absl_cc_test(
  NAME
    node_hash_set_test
  SRCS
    "node_hash_set_test.cc"
  COPTS
    ${ABSL_TEST_COPTS}
    "-DUNORDERED_SET_CXX17"
  DEPS
    turbo::hash_generator_testing
    turbo::node_hash_set
    turbo::unordered_set_constructor_test
    turbo::unordered_set_lookup_test
    turbo::unordered_set_members_test
    turbo::unordered_set_modifiers_test
    GTest::gmock_main
)

# Internal-only target, do not depend on directly.
absl_cc_library(
  NAME
    container_memory
  HDRS
    "internal/container_memory.h"
  COPTS
    ${ABSL_DEFAULT_COPTS}
  DEPS
    turbo::config
    turbo::memory
    turbo::type_traits
    turbo::utility
  PUBLIC
)

absl_cc_test(
  NAME
    container_memory_test
  SRCS
    "internal/container_memory_test.cc"
  COPTS
    ${ABSL_TEST_COPTS}
  DEPS
    turbo::container_memory
    turbo::strings
    turbo::test_instance_tracker
    GTest::gmock_main
)

# Internal-only target, do not depend on directly.
absl_cc_library(
  NAME
    hash_function_defaults
  HDRS
    "internal/hash_function_defaults.h"
  COPTS
    ${ABSL_DEFAULT_COPTS}
  DEPS
    turbo::config
    turbo::cord
    turbo::hash
    turbo::strings
  PUBLIC
)

absl_cc_test(
  NAME
    hash_function_defaults_test
  SRCS
    "internal/hash_function_defaults_test.cc"
  COPTS
    ${ABSL_TEST_COPTS}
  DEPS
    turbo::cord
    turbo::cord_test_helpers
    turbo::hash_function_defaults
    turbo::hash
    turbo::random_random
    turbo::strings
    GTest::gmock_main
)

# Internal-only target, do not depend on directly.
absl_cc_library(
  NAME
    hash_generator_testing
  HDRS
    "internal/hash_generator_testing.h"
  SRCS
    "internal/hash_generator_testing.cc"
  COPTS
    ${ABSL_TEST_COPTS}
  DEPS
    turbo::hash_policy_testing
    turbo::memory
    turbo::meta
    turbo::strings
  TESTONLY
)

# Internal-only target, do not depend on directly.
absl_cc_library(
  NAME
    hash_policy_testing
  HDRS
    "internal/hash_policy_testing.h"
  COPTS
    ${ABSL_TEST_COPTS}
  DEPS
    turbo::hash
    turbo::strings
  TESTONLY
)

absl_cc_test(
  NAME
    hash_policy_testing_test
  SRCS
    "internal/hash_policy_testing_test.cc"
  COPTS
    ${ABSL_TEST_COPTS}
  DEPS
    turbo::hash_policy_testing
    GTest::gmock_main
)

# Internal-only target, do not depend on directly.
absl_cc_library(
  NAME
    hash_policy_traits
  HDRS
    "internal/hash_policy_traits.h"
  COPTS
    ${ABSL_DEFAULT_COPTS}
  DEPS
    turbo::common_policy_traits
    turbo::meta
  PUBLIC
)

absl_cc_test(
  NAME
    hash_policy_traits_test
  SRCS
    "internal/hash_policy_traits_test.cc"
  COPTS
    ${ABSL_TEST_COPTS}
  DEPS
    turbo::hash_policy_traits
    GTest::gmock_main
)

# Internal-only target, do not depend on directly.
absl_cc_library(
  NAME
    common_policy_traits
  HDRS
    "internal/common_policy_traits.h"
  COPTS
    ${ABSL_DEFAULT_COPTS}
  DEPS
    turbo::meta
  PUBLIC
)

absl_cc_test(
  NAME
    common_policy_traits_test
  SRCS
    "internal/common_policy_traits_test.cc"
  COPTS
    ${ABSL_TEST_COPTS}
  DEPS
    turbo::common_policy_traits
    GTest::gmock_main
)

# Internal-only target, do not depend on directly.
absl_cc_library(
  NAME
    hashtablez_sampler
  HDRS
    "internal/hashtablez_sampler.h"
  SRCS
    "internal/hashtablez_sampler.cc"
    "internal/hashtablez_sampler_force_weak_definition.cc"
  COPTS
    ${ABSL_DEFAULT_COPTS}
  DEPS
    turbo::base
    turbo::config
    turbo::exponential_biased
    turbo::sample_recorder
    turbo::synchronization
)

absl_cc_test(
  NAME
    hashtablez_sampler_test
  SRCS
    "internal/hashtablez_sampler_test.cc"
  COPTS
    ${ABSL_TEST_COPTS}
  DEPS
    turbo::config
    turbo::hashtablez_sampler
    GTest::gmock_main
)

# Internal-only target, do not depend on directly.
absl_cc_library(
  NAME
    hashtable_debug
  HDRS
    "internal/hashtable_debug.h"
  COPTS
    ${ABSL_DEFAULT_COPTS}
  DEPS
    turbo::hashtable_debug_hooks
)

# Internal-only target, do not depend on directly.
absl_cc_library(
  NAME
    hashtable_debug_hooks
  HDRS
    "internal/hashtable_debug_hooks.h"
  COPTS
    ${ABSL_DEFAULT_COPTS}
  DEPS
    turbo::config
  PUBLIC
)

# Internal-only target, do not depend on directly.
absl_cc_library(
  NAME
    node_slot_policy
  HDRS
    "internal/node_slot_policy.h"
  COPTS
    ${ABSL_DEFAULT_COPTS}
  DEPS
    turbo::config
  PUBLIC
)

absl_cc_test(
  NAME
    node_slot_policy_test
  SRCS
    "internal/node_slot_policy_test.cc"
  COPTS
    ${ABSL_TEST_COPTS}
  DEPS
    turbo::hash_policy_traits
    turbo::node_slot_policy
    GTest::gmock_main
)

# Internal-only target, do not depend on directly.
absl_cc_library(
  NAME
    raw_hash_map
  HDRS
    "internal/raw_hash_map.h"
  COPTS
    ${ABSL_DEFAULT_COPTS}
  DEPS
    turbo::container_memory
    turbo::raw_hash_set
    turbo::throw_delegate
  PUBLIC
)

# Internal-only target, do not depend on directly.
absl_cc_library(
  NAME
    container_common
  HDRS
    "internal/common.h"
  COPTS
    ${ABSL_DEFAULT_COPTS}
  DEPS
    turbo::type_traits
)

# Internal-only target, do not depend on directly.
absl_cc_library(
  NAME
    raw_hash_set
  HDRS
    "internal/raw_hash_set.h"
  SRCS
    "internal/raw_hash_set.cc"
  COPTS
    ${ABSL_DEFAULT_COPTS}
  DEPS
    turbo::bits
    turbo::compressed_tuple
    turbo::config
    turbo::container_common
    turbo::container_memory
    turbo::core_headers
    turbo::endian
    turbo::hash_policy_traits
    turbo::hashtable_debug_hooks
    turbo::memory
    turbo::meta
    turbo::optional
    turbo::prefetch
    turbo::raw_logging_internal
    turbo::utility
    turbo::hashtablez_sampler
  PUBLIC
)

absl_cc_test(
  NAME
    raw_hash_set_test
  SRCS
    "internal/raw_hash_set_test.cc"
  COPTS
    ${ABSL_TEST_COPTS}
  DEPS
    turbo::container_memory
    turbo::flat_hash_map
    turbo::flat_hash_set
    turbo::hash_function_defaults
    turbo::hash_policy_testing
    turbo::hashtable_debug
    turbo::raw_hash_set
    turbo::base
    turbo::config
    turbo::log
    turbo::core_headers
    turbo::prefetch
    turbo::raw_logging_internal
    turbo::strings
    GTest::gmock_main
)

absl_cc_test(
  NAME
    raw_hash_set_allocator_test
  SRCS
    "internal/raw_hash_set_allocator_test.cc"
  COPTS
    ${ABSL_TEST_COPTS}
  DEPS
    turbo::raw_hash_set
    turbo::tracked
    turbo::core_headers
    GTest::gmock_main
)

# Internal-only target, do not depend on directly.
absl_cc_library(
  NAME
    layout
  HDRS
    "internal/layout.h"
  COPTS
    ${ABSL_DEFAULT_COPTS}
  DEPS
    turbo::config
    turbo::core_headers
    turbo::meta
    turbo::strings
    turbo::span
    turbo::utility
  PUBLIC
)

absl_cc_test(
  NAME
    layout_test
  SRCS
    "internal/layout_test.cc"
  COPTS
    ${ABSL_TEST_COPTS}
  DEPS
    turbo::layout
    turbo::config
    turbo::core_headers
    turbo::raw_logging_internal
    turbo::span
    GTest::gmock_main
)

# Internal-only target, do not depend on directly.
absl_cc_library(
  NAME
    tracked
  HDRS
    "internal/tracked.h"
  COPTS
    ${ABSL_TEST_COPTS}
  DEPS
    turbo::config
  TESTONLY
)

# Internal-only target, do not depend on directly.
absl_cc_library(
  NAME
    unordered_map_constructor_test
  HDRS
    "internal/unordered_map_constructor_test.h"
  COPTS
    ${ABSL_TEST_COPTS}
  DEPS
    turbo::hash_generator_testing
    turbo::hash_policy_testing
    GTest::gmock
  TESTONLY
)

# Internal-only target, do not depend on directly.
absl_cc_library(
  NAME
    unordered_map_lookup_test
  HDRS
    "internal/unordered_map_lookup_test.h"
  COPTS
    ${ABSL_TEST_COPTS}
  DEPS
    turbo::hash_generator_testing
    turbo::hash_policy_testing
    GTest::gmock
  TESTONLY
)

# Internal-only target, do not depend on directly.
absl_cc_library(
  NAME
    unordered_map_members_test
  HDRS
    "internal/unordered_map_members_test.h"
  COPTS
    ${ABSL_TEST_COPTS}
  DEPS
    turbo::type_traits
    GTest::gmock
  TESTONLY
)

# Internal-only target, do not depend on directly.
absl_cc_library(
  NAME
    unordered_map_modifiers_test
  HDRS
    "internal/unordered_map_modifiers_test.h"
  COPTS
    ${ABSL_TEST_COPTS}
  DEPS
    turbo::hash_generator_testing
    turbo::hash_policy_testing
    GTest::gmock
  TESTONLY
)

# Internal-only target, do not depend on directly.
absl_cc_library(
  NAME
    unordered_set_constructor_test
  HDRS
    "internal/unordered_set_constructor_test.h"
  COPTS
    ${ABSL_TEST_COPTS}
  DEPS
    turbo::hash_generator_testing
    turbo::hash_policy_testing
    GTest::gmock
  TESTONLY
)

# Internal-only target, do not depend on directly.
absl_cc_library(
  NAME
    unordered_set_lookup_test
  HDRS
    "internal/unordered_set_lookup_test.h"
  COPTS
    ${ABSL_TEST_COPTS}
  DEPS
    turbo::hash_generator_testing
    turbo::hash_policy_testing
    GTest::gmock
  TESTONLY
)

# Internal-only target, do not depend on directly.
absl_cc_library(
  NAME
    unordered_set_members_test
  HDRS
    "internal/unordered_set_members_test.h"
  COPTS
    ${ABSL_TEST_COPTS}
  DEPS
    turbo::type_traits
    GTest::gmock
  TESTONLY
)

# Internal-only target, do not depend on directly.
absl_cc_library(
  NAME
    unordered_set_modifiers_test
  HDRS
    "internal/unordered_set_modifiers_test.h"
  COPTS
    ${ABSL_TEST_COPTS}
  DEPS
    turbo::hash_generator_testing
    turbo::hash_policy_testing
    GTest::gmock
  TESTONLY
)

absl_cc_test(
  NAME
    unordered_set_test
  SRCS
    "internal/unordered_set_test.cc"
  COPTS
    ${ABSL_TEST_COPTS}
  DEPS
    turbo::unordered_set_constructor_test
    turbo::unordered_set_lookup_test
    turbo::unordered_set_members_test
    turbo::unordered_set_modifiers_test
    GTest::gmock_main
)

absl_cc_test(
  NAME
    unordered_map_test
  SRCS
    "internal/unordered_map_test.cc"
  COPTS
    ${ABSL_TEST_COPTS}
  DEPS
    turbo::unordered_map_constructor_test
    turbo::unordered_map_lookup_test
    turbo::unordered_map_members_test
    turbo::unordered_map_modifiers_test
    GTest::gmock_main
)

absl_cc_test(
  NAME
    sample_element_size_test
  SRCS
    "sample_element_size_test.cc"
  COPTS
    ${ABSL_TEST_COPTS}
  DEPS
    turbo::flat_hash_map
    turbo::flat_hash_set
    turbo::node_hash_map
    turbo::node_hash_set
    GTest::gmock_main
)
