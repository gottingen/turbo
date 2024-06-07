#
# Copyright (C) 2024 EA group inc.
# Author: Jeff.li lijippy@163.com
# All rights reserved.
# This program is free software: you can redistribute it and/or modify
# it under the terms of the GNU Affero General Public License as published
# by the Free Software Foundation, either version 3 of the License, or
# (at your option) any later version.
#
# This program is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
# GNU Affero General Public License for more details.
#
# You should have received a copy of the GNU Affero General Public License
# along with this program.  If not, see <https://www.gnu.org/licenses/>.
#

carbin_cc_test_library(
        NAMESPACE turbo
        NAME hash_generator_testing
        SOURCES hash_generator_testing.cc
        LINKS
        turbo::turbo
        GTest::gtest
        GTest::gmock
        INCLUDES "/opt/EA/inf/include"
        CXXOPTS ${CARBIN_CXX_OPTIONS}
)


set(CONTAINER_SRC_LIST
        common_policy_traits_test
        fixed_array_test
        hash_function_defaults_test
        hash_policy_testing_test
        hash_policy_traits_test
        hashtablez_sampler_test
        layout_test
        node_hash_map_test
        node_slot_policy_test
        raw_hash_set_allocator_test
        raw_hash_set_test
        sample_element_size_test
        unordered_map_test
        unordered_set_test
        span_test
)

foreach (SRC ${CONTAINER_SRC_LIST})
    carbin_cc_test(
            NAME ${SRC}
            MODULE container
            SOURCES ${SRC}.cc
            LINKS
            turbo::turbo
            GTest::gtest
            GTest::gmock
            hash_generator_testing_static
            GTest::gmock_main
            ${CARBIN_DEPS_LINK}
            CXXOPTS ${CARBIN_CXX_OPTIONS}
    )
endforeach ()

carbin_cc_test_library(
        NAMESPACE turbo
        NAME btree_test_lib
        SOURCES btree_test.cc
        LINKS
        turbo::turbo
        GTest::gtest
        GTest::gmock
        INCLUDES "/opt/EA/inf/include"
        CXXOPTS ${CARBIN_CXX_OPTIONS}
)

carbin_cc_test_library(
        NAMESPACE turbo
        NAME test_instance_tracker
        SOURCES test_instance_tracker.cc
        LINKS
        turbo::turbo
        GTest::gtest
        GTest::gmock
        INCLUDES "/opt/EA/inf/include"
        CXXOPTS ${CARBIN_CXX_OPTIONS}
)


carbin_cc_test(
        NAME node_hash_set_test
        MODULE container
        SOURCES node_hash_set_test.cc
        LINKS
        turbo::turbo
        GTest::gtest
        GTest::gmock
        GTest::gmock_main
        hash_generator_testing_static
        ${CARBIN_DEPS_LINK}
        CXXOPTS ${CARBIN_CXX_OPTIONS}
)

carbin_cc_test(
        NAME inlined_vector_test
        MODULE container
        SOURCES inlined_vector_test.cc
        DEPS
        exception_safety_testing_static
        LINKS
        turbo::turbo
        GTest::gtest
        GTest::gmock
        GTest::gmock_main
        test_instance_tracker_static
        exception_safety_testing_static
        ${CARBIN_DEPS_LINK}
        CXXOPTS ${CARBIN_CXX_OPTIONS}
)

carbin_cc_test(
        NAME fixed_array_exception_safety_test
        MODULE container
        SOURCES fixed_array_exception_safety_test.cc
        DEPS
        exception_safety_testing_static
        LINKS
        turbo::turbo
        GTest::gtest
        GTest::gmock
        GTest::gmock_main
        test_instance_tracker_static
        exception_safety_testing_static
        ${CARBIN_DEPS_LINK}
        CXXOPTS ${CARBIN_CXX_OPTIONS}
)

carbin_cc_test(
        NAME inlined_vector_exception_safety_test
        MODULE container
        SOURCES inlined_vector_exception_safety_test.cc
        DEPS
        exception_safety_testing_static
        LINKS
        turbo::turbo
        GTest::gtest
        GTest::gmock
        GTest::gmock_main
        test_instance_tracker_static
        exception_safety_testing_static
        ${CARBIN_DEPS_LINK}
        CXXOPTS ${CARBIN_CXX_OPTIONS}
)

carbin_cc_test(
        NAME compressed_tuple_test
        MODULE container
        SOURCES compressed_tuple_test.cc
        LINKS
        turbo::turbo
        GTest::gtest
        GTest::gmock
        GTest::gmock_main
        test_instance_tracker_static
        ${CARBIN_DEPS_LINK}
        CXXOPTS ${CARBIN_CXX_OPTIONS}
)

carbin_cc_test(
        NAME container_memory_test
        MODULE container
        SOURCES container_memory_test.cc
        LINKS
        turbo::turbo
        GTest::gtest
        GTest::gmock
        test_instance_tracker_static
        GTest::gmock_main
        ${CARBIN_DEPS_LINK}
        CXXOPTS ${CARBIN_CXX_OPTIONS}
)


carbin_cc_test(
        NAME flat_hash_map_test
        MODULE container
        SOURCES flat_hash_map_test.cc
        LINKS
        turbo::turbo
        GTest::gtest
        GTest::gmock
        GTest::gmock_main
        hash_generator_testing_static
        ${CARBIN_DEPS_LINK}
        CXXOPTS ${CARBIN_CXX_OPTIONS}
)
carbin_cc_test(
        NAME test_instance_tracker_test
        MODULE container
        SOURCES test_instance_tracker_test.cc
        LINKS
        turbo::turbo
        GTest::gtest
        GTest::gmock
        GTest::gmock_main
        test_instance_tracker_static
        ${CARBIN_DEPS_LINK}
        CXXOPTS ${CARBIN_CXX_OPTIONS}
)

carbin_cc_test(
        NAME flat_hash_set_test
        MODULE container
        SOURCES flat_hash_set_test.cc
        LINKS
        turbo::turbo
        GTest::gtest
        GTest::gmock
        GTest::gmock_main
        hash_generator_testing_static
        ${CARBIN_DEPS_LINK}
        CXXOPTS ${CARBIN_CXX_OPTIONS} "-DUNORDERED_SET_CXX17"
)

carbin_cc_test(
        NAME small_vector_test
        MODULE container
        SOURCES small_vector_test.cc
        LINKS
        turbo::turbo
        GTest::gtest
        GTest::gmock
        GTest::gmock_main
        ${CARBIN_DEPS_LINK}
        CXXOPTS ${CARBIN_CXX_OPTIONS} "-DUNORDERED_SET_CXX17"
)