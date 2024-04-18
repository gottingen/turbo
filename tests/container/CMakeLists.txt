#
# Copyright 2023 The titan-search Authors.
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
carbin_cc_test_library(
        NAMESPACE
        turbo
        NAME
        test_instance_tracker
        HEADERS
        "test_instance_tracker.h"
        SOURCES
        "test_instance_tracker.cc"
        CXXOPTS
        ${CARBIN_CXX_OPTIONS}
        LINKS
        turbo::turbo
        ${GMOCK_LIB}
        ${GTEST_LIB}
)

carbin_cc_test(
        NAME
        btree_test
        MODULE container
        SOURCES
        "btree_test.cc"
        CXXOPTS
        ${CARBIN_CXX_OPTIONS}
        LINKS
        ${CARBIN_DEPS_LINK}
        turbo::turbo
        turbo::test_instance_tracker_static
        ${GTEST_LIB}
        ${GTEST_MAIN_LIB}
)


carbin_cc_test(
        NAME  compressed_tuple_test
        MODULE container
        SOURCES
        "compressed_tuple_test.cc"
        CXXOPTS
        ${CARBIN_CXX_OPTIONS}
        LINKS
        turbo::turbo
        turbo::test_instance_tracker_static
        ${GTEST_LIB}
        ${GMOCK_LIB}
        ${GTEST_MAIN_LIB}
)

carbin_cc_test(
        NAME  fixed_array_test
        MODULE container
        SOURCES
        "fixed_array_test.cc"
        CXXOPTS
        ${CARBIN_CXX_OPTIONS}
        LINKS
        turbo::turbo
        ${GTEST_LIB}
        ${GMOCK_LIB}
        ${GTEST_MAIN_LIB}
)

carbin_cc_test(
        NAME  inlined_vector_test
        MODULE container
        SOURCES
        "inlined_vector_test.cc"
        CXXOPTS
        ${CARBIN_CXX_OPTIONS}
        LINKS
        turbo::turbo
        turbo::test_instance_tracker_static
        ${GTEST_LIB}
        ${GMOCK_LIB}
        ${GTEST_MAIN_LIB}
)


carbin_cc_test(
        NAME
        fixed_array_exception_safety_test
        MODULE container
        LINKS
        turbo::exception_safety_testing_static
        turbo::turbo
        ${GTEST_LIB}
        ${GMOCK_LIB}
        ${GTEST_MAIN_LIB}
        CXXOPTS
        ${CARBIN_CXX_OPTIONS}
        SOURCES
        "fixed_array_exception_safety_test.cc"
)


carbin_cc_test(
        NAME
        inlined_vector_exception_safety_test
        MODULE container
        SOURCES
        "inlined_vector_exception_safety_test.cc"
        CXXOPTS
        ${CARBIN_CXX_OPTIONS}
        LINKS
        turbo::turbo
        turbo::exception_safety_testing_static
        ${GTEST_LIB}
        ${GMOCK_LIB}
        ${GTEST_MAIN_LIB}
)


carbin_cc_test(
        NAME
        test_instance_tracker_test
        MODULE container
        SOURCES
        "test_instance_tracker_test.cc"
        CXXOPTS
        ${CARBIN_CXX_OPTIONS}
        LINKS
        turbo::test_instance_tracker_static
        turbo::turbo
        ${GTEST_LIB}
        ${GMOCK_LIB}
        ${GTEST_MAIN_LIB}
)

carbin_cc_test(
        NAME
        flat_hash_map_test
        MODULE container
        SOURCES
        "flat_hash_map_test.cc"
        CXXOPTS
        ${CARBIN_CXX_OPTIONS}
        LINKS
        ${CARBIN_DEPS_LINK}
        turbo::hash_generator_testing
        turbo::turbo
        ${GTEST_LIB}
        ${GMOCK_LIB}
        ${GTEST_MAIN_LIB}
)
#TODO disabled now(jeff)

carbin_cc_test(
        NAME
        flat_hash_set_test
        MODULE container
        SOURCES
        "flat_hash_set_test.cc"
        CXXOPTS
        ${CARBIN_CXX_OPTIONS}
        "-DUNORDERED_SET_CXX17"
        LINKS
        ${CARBIN_DEPS_LINK}
        turbo::turbo
        turbo::hash_generator_testing
        ${GTEST_LIB}
        ${GMOCK_LIB}
        ${GTEST_MAIN_LIB}
)

carbin_cc_test(
        NAME
        node_hash_map_test
        MODULE container
        SOURCES
        "node_hash_map_test.cc"
        CXXOPTS
        ${CARBIN_CXX_OPTIONS}
        LINKS
        turbo::hash_generator_testing
        turbo::turbo
        ${GTEST_LIB}
        ${GMOCK_LIB}
        ${GTEST_MAIN_LIB}
)

carbin_cc_test(
        NAME
        node_hash_set_test
        MODULE container
        SOURCES
        "node_hash_set_test.cc"
        CXXOPTS
        ${CARBIN_CXX_OPTIONS}
        "-DUNORDERED_SET_CXX17"
        LINKS
        turbo::hash_generator_testing
        turbo::turbo
        ${GTEST_LIB}
        ${GMOCK_LIB}
        ${GTEST_MAIN_LIB}
)

carbin_cc_test(
        NAME
        container_memory_test
        MODULE container
        SOURCES
        "container_memory_test.cc"
        CXXOPTS
        ${CARBIN_CXX_OPTIONS}
        LINKS
        ${CARBIN_DEPS_LINK}
        turbo::turbo
        turbo::test_instance_tracker_static
        ${GTEST_LIB}
        ${GTEST_MAIN_LIB}
)

carbin_cc_test(
        NAME
        hash_function_defaults_test
        MODULE container
        SOURCES
        "hash_function_defaults_test.cc"
        CXXOPTS
        ${CARBIN_CXX_OPTIONS}
        LINKS
        turbo::turbo
        ${GTEST_LIB}
        ${GMOCK_LIB}
        ${GTEST_MAIN_LIB}
)

# Internal-only target, do not depend on directly.
carbin_cc_library(
        NAMESPACE
        turbo
        NAME
        hash_generator_testing
        HEADERS
        "hash_generator_testing.h"
        SOURCES
        "hash_generator_testing.cc"
        CXXOPTS
        ${CARBIN_CXX_OPTIONS}
        LINKS
        ${GTEST_LIB}
        turbo::turbo
)


carbin_cc_test(
        NAME
        hash_policy_testing_test
        MODULE container
        SOURCES
        "hash_policy_testing_test.cc"
        CXXOPTS
        ${CARBIN_CXX_OPTIONS}
        LINKS
        ${GTEST_LIB}
        turbo::turbo
        ${GMOCK_LIB}
        ${GTEST_MAIN_LIB}
)


carbin_cc_test(
        NAME
        hash_policy_traits_test
        MODULE container
        SOURCES
        "hash_policy_traits_test.cc"
        CXXOPTS
        ${CARBIN_CXX_OPTIONS}
        LINKS
        ${GTEST_LIB}
        ${GMOCK_LIB}
        turbo::turbo
        ${GTEST_MAIN_LIB}
)


carbin_cc_test(
        NAME
        common_policy_traits_test
        MODULE container
        SOURCES
        "common_policy_traits_test.cc"
        CXXOPTS
        ${CARBIN_CXX_OPTIONS}
        LINKS
        ${GTEST_LIB}
        ${GMOCK_LIB}
        turbo::turbo
        ${GTEST_MAIN_LIB}
)


carbin_cc_test(
        NAME
        hashtablez_sampler_test
        MODULE container
        SOURCES
        "hashtablez_sampler_test.cc"
        CXXOPTS
        ${CARBIN_CXX_OPTIONS}
        LINKS
        ${GTEST_LIB}
        ${GMOCK_LIB}
        turbo::turbo
        ${GTEST_MAIN_LIB}
)


carbin_cc_test(
        NAME
        node_slot_policy_test
        MODULE container
        SOURCES
        "node_slot_policy_test.cc"
        CXXOPTS
        ${CARBIN_CXX_OPTIONS}
        LINKS
        turbo::turbo
        ${GTEST_LIB}
        ${GMOCK_LIB}
        ${GTEST_MAIN_LIB}
)

carbin_cc_test(
        NAME
        raw_hash_set_test
        MODULE container
        SOURCES
        "raw_hash_set_test.cc"
        CXXOPTS
        ${CARBIN_CXX_OPTIONS}
        LINKS
        ${GTEST_LIB}
        ${GMOCK_LIB}
        turbo::turbo
        ${GTEST_MAIN_LIB}
)

carbin_cc_test(
        NAME
        raw_hash_set_allocator_test
        MODULE container
        SOURCES
        "raw_hash_set_allocator_test.cc"
        CXXOPTS
        ${CARBIN_CXX_OPTIONS}
        LINKS
        ${GTEST_LIB}
        turbo::turbo
        ${GMOCK_LIB}
        ${GTEST_MAIN_LIB}
)


carbin_cc_test(
        NAME
        layout_test
        MODULE container
        SOURCES
        "layout_test.cc"
        CXXOPTS
        ${CARBIN_CXX_OPTIONS}
        LINKS
        turbo::turbo
)


carbin_cc_test(
        NAME
        unordered_set_test
        MODULE container
        SOURCES
        "unordered_set_test.cc"
        CXXOPTS
        ${CARBIN_CXX_OPTIONS}
        LINKS
        turbo::hash_generator_testing_static
        ${GTEST_LIB}
        ${GMOCK_LIB}
        ${GTEST_MAIN_LIB}
)

carbin_cc_test(
        NAME
        unordered_map_test
        MODULE container
        SOURCES
        "unordered_map_test.cc"
        CXXOPTS
        ${CARBIN_CXX_OPTIONS}
        LINKS
        turbo::hash_generator_testing_static
        ${GTEST_LIB}
        ${GMOCK_LIB}
        ${GTEST_MAIN_LIB}
)

carbin_cc_test(
        NAME
        sample_element_size_test
        MODULE container
        SOURCES
        "sample_element_size_test.cc"
        CXXOPTS
        ${CARBIN_CXX_OPTIONS}
        LINKS
        turbo::turbo
        ${GTEST_LIB}
        ${GMOCK_LIB}
        ${GTEST_MAIN_LIB}
)

carbin_cc_test(
        NAME
        ring_buffer_test
        MODULE container
        SOURCES
        "ring_buffer_test.cc"
        CXXOPTS
        ${CARBIN_CXX_OPTIONS}
        LINKS
        turbo::turbo
        ${GTEST_LIB}
        ${GMOCK_LIB}
        ${GTEST_MAIN_LIB}
)


carbin_cc_test(
        NAME
        intrusive_list_test
        MODULE container
        SOURCES
        "intrusive_list_test.cc"
        CXXOPTS
        ${CARBIN_CXX_OPTIONS}
        LINKS
        turbo::turbo
)

carbin_cc_test(
        NAME
        bounded_queue_test
        MODULE container
        SOURCES
        "bounded_queue_test.cc"
        CXXOPTS
        ${CARBIN_CXX_OPTIONS}
        LINKS
        turbo::turbo
)

carbin_cc_test(
        NAME container_batch_test
        MODULE container
        SOURCES
        batch_test.cc
        CXXOPTS ${CARBIN_CXX_OPTIONS}
        LINKS
        ${CARBIN_DEPS_LINK}
        turbo::turbo
)
