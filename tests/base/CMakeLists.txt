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

carbin_cc_test(
        NAME atomic_hook_test
        MODULE base
        SOURCES atomic_hook_test.cc atomic_hook_test_helper.h atomic_hook_test_helper.cc
        LINKS
        turbo::turbo
        GTest::gtest
        GTest::gmock
        GTest::gmock_main
        ${CARBIN_DEPS_LINK}
        CXXOPTS
        ${CARBIN_CXX_OPTIONS}
)

carbin_cc_test(
        NAME bit_cast_test
        MODULE base
        SOURCES bit_cast_test.cc
        LINKS
        turbo::turbo
        GTest::gtest
        GTest::gmock
        GTest::gmock_main
        ${CARBIN_DEPS_LINK}
        CXXOPTS ${CARBIN_CXX_OPTIONS}
)

carbin_cc_test(
        NAME call_once_test
        MODULE base
        SOURCES call_once_test.cc
        LINKS
        turbo::turbo
        GTest::gtest
        GTest::gmock
        GTest::gmock_main
        ${CARBIN_DEPS_LINK}
        CXXOPTS ${CARBIN_CXX_OPTIONS}
)

carbin_cc_test(
        NAME cmake_thread_test
        MODULE base
        SOURCES cmake_thread_test.cc
        LINKS
        turbo::turbo
        ${CARBIN_DEPS_LINK}
        CXXOPTS ${CARBIN_CXX_OPTIONS}
)
set(BASE_SRC_LIST
        config_test
        endian_test
        errno_saver_test
        fast_type_id_test
        invoke_test
        log_severity_test
        low_level_alloc_test
        no_destructor_test
        nullability_test
        optimization_test
        prefetch_test
        raw_logging_test
        scoped_set_env_test
        spinlock_test_common
        strerror_test
        sysinfo_test
        thread_identity_test
        throw_delegate_test
        unique_small_name_test
)

foreach (SRC ${BASE_SRC_LIST})
    carbin_cc_test(
            NAME ${SRC}
            MODULE base
            SOURCES ${SRC}.cc
            LINKS
            turbo::turbo
            GTest::gtest
            GTest::gmock
            GTest::gmock_main
            ${CARBIN_DEPS_LINK}
            CXXOPTS ${CARBIN_CXX_OPTIONS}
    )
endforeach ()

carbin_cc_test_library(
        NAMESPACE turbo
        NAME exception_safety_testing
        SOURCES exception_safety_testing.cc
        LINKS
        turbo::turbo
        GTest::gtest
        GTest::gmock
        INCLUDES "/opt/EA/inf/include"
        CXXOPTS ${CARBIN_CXX_OPTIONS}
)

carbin_cc_test(
        NAME exception_safety_testing_test
        MODULE base
        SOURCES exception_safety_testing_test.cc
        LINKS
        turbo::turbo
        GTest::gtest
        GTest::gmock
        GTest::gmock_main
        exception_safety_testing_static
        ${CARBIN_DEPS_LINK}
        CXXOPTS ${CARBIN_CXX_OPTIONS}
)

carbin_cc_test_library(
        NAMESPACE turbo
        NAME inline_variable_test_a
        SOURCES inline_variable_test_a.cc
        LINKS
        turbo::turbo
        GTest::gtest
        GTest::gmock
        INCLUDES "/opt/EA/inf/include"
        CXXOPTS ${CARBIN_CXX_OPTIONS}
)

carbin_cc_test_library(
        NAMESPACE turbo
        NAME inline_variable_test_b
        SOURCES inline_variable_test_b.cc
        LINKS
        turbo::turbo
        GTest::gtest
        GTest::gmock
        INCLUDES "/opt/EA/inf/include"
        CXXOPTS ${CARBIN_CXX_OPTIONS}
)

carbin_cc_test(
        NAME inline_variable_test
        MODULE base
        SOURCES inline_variable_test.cc
        LINKS
        turbo::turbo
        inline_variable_test_a_static
        inline_variable_test_b_static
        GTest::gtest
        GTest::gmock
        GTest::gmock_main
        exception_safety_testing_static
        ${CARBIN_DEPS_LINK}
        CXXOPTS ${CARBIN_CXX_OPTIONS}
)