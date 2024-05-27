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

#[[carbin_cc_test(
        NAME custom_comparator_test
        SOURCES custom_comparator_test.cc
        DEPS turbo::turbo ${CARBIN_DEPS_LINK} ${GTEST_LIB} ${GTEST_MAIN_LIB}
        COPTS ${USER_CXX_FLAGS}
)]]
#[[
carbin_cc_test(
        NAME foo_test
        SOURCES foo_test.cc
        DEPS changeme::foo ${CARBIN_DEPS_LINK} gtest gtest_main
        COPTS ${USER_CXX_FLAGS}
)

carbin_cc_test(
        NAME foo_doctest
        SOURCES foo_doctest.cc
        DEPS changeme::foo ${CARBIN_DEPS_LINK} doctest
        COPTS ${USER_CXX_FLAGS}
)
]]
#[[
carbin_cc_test(
        NAME raw_test
        MODULE base
        SOURCES raw_test.cc
)

carbin_cc_test(
        NAME pass_test
        MODULE base
        SOURCES pass_test.cc
        EXT
)

# base_pass_test_1
carbin_cc_test_ext(
        NAME pass_test
        MODULE base
        ALIAS 1
        PASS_EXP "pass;Passed"
)

# base_pass_test_2
carbin_cc_test_ext(
        NAME pass_test
        MODULE base
        ALIAS 2
        PASS_EXP "pass;Passed"
)
# base_pass_test_foo
carbin_cc_test_ext(
        NAME pass_test
        MODULE base
        ALIAS foo
        PASS_EXP "pass;Passed"
)

# base_pass_test_bar not run
carbin_cc_test_ext(
        NAME pass_test
        MODULE base
        ALIAS bar
        PASS_EXP "pass;Passed"
        DISABLED
)

carbin_cc_test(
        NAME args_test
        MODULE base
        SOURCES args_test.cc
        EXT
)

# base_args_test_fail fail
carbin_cc_test_ext(
        NAME args_test
        MODULE base
        ALIAS fail
        FAIL_EXP "[^a-z]Error;ERROR;Failed"
)

# base_args_test_fail fail
carbin_cc_test_ext(
        NAME args_test
        MODULE base
        ALIAS fail_args
        ARGS "Failed"
        FAIL_EXP "[^a-z]Error;ERROR;Failed"
)

# base_args_test_skip fail
carbin_cc_test_ext(
        NAME args_test
        MODULE base
        ALIAS skip_fail
        PASS_EXP "pass;Passed"
        FAIL_EXP "[^a-z]Error;ERROR;Failed"
        SKIP_EXP "[^a-z]Skip" "SKIP" "Skipped"
)

# base_args_test_skip fail
carbin_cc_test_ext(
        NAME args_test
        MODULE base
        ALIAS skip
        ARGS "SKIP"
        PASS_EXP "pass;Passed"
        FAIL_EXP "[^a-z]Error;ERROR;Failed"
        SKIP_EXP "[^a-z]Skip" "SKIP" "Skipped"
)

# base_args_test_skip fail
carbin_cc_test_ext(
        NAME args_test
        MODULE base
        ALIAS skip_pass
        ARGS "Passed"
        PASS_EXP "pass;Passed"
        FAIL_EXP "[^a-z]Error;ERROR;Failed"
        SKIP_EXP "[^a-z]Skip" "SKIP" "Skipped"
)

# base_args_test_skip fail
carbin_cc_test_ext(
        NAME args_test
        MODULE base
        ALIAS skip_diabled
        ARGS "Passed"
        PASS_EXP "pass;Passed"
        FAIL_EXP "[^a-z]Error;ERROR;Failed"
        SKIP_EXP "[^a-z]Skip" "SKIP" "Skipped"
        DISABLED
)

carbin_cc_test(
        NAME raw_test
        MODULE norun
        SOURCES raw_test.cc
)
]]

list(APPEND CMAKE_PREFIX_PATH "/opt/EA/inf")

find_package(GTest REQUIRED)
include_directories(${GTEST_INCLUDE_DIR})
set(GTEST_LIB GTest::gtest)
set(GTEST_MAIN_LIB GTest::gtest_main)
set(GMOCK_LIB GTest::gmock)
set(GMOCK_MAIN_LIB GTest::gmock_main)

set(GTM_LIB GTest::gtest GTest::gmock GTest::gtest_main)

add_subdirectory(algorithm)
add_subdirectory(base)
add_subdirectory(cleanup)
add_subdirectory(container)
add_subdirectory(crc)
add_subdirectory(debugging)
add_subdirectory(flags)
add_subdirectory(functional)
add_subdirectory(hash)
add_subdirectory(log)
add_subdirectory(memory)
add_subdirectory(meta)
add_subdirectory(numeric)
add_subdirectory(profile)
add_subdirectory(random)
add_subdirectory(status)
add_subdirectory(strings)
add_subdirectory(synchronization)
add_subdirectory(times)
add_subdirectory(types)
add_subdirectory(utility)