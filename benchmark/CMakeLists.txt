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
#[[

carbin_cc_benchmark(
        NAME tedis_bench
        SOURCES tedis_bench.cc
        DEPS tedis::tedis ${CARBIN_DEPS_LINK} ${BENCHMARK_LIB} ${BENCHMARK_MAIN_LIB}
        COPTS ${USER_CXX_FLAGS}
)
]]
list(APPEND CMAKE_PREFIX_PATH "/opt/EA/inf")

find_package(GTest REQUIRED)
set(GTEST_LIB GTest::gtest)
set(GTEST_MAIN_LIB GTest::gtest_main)
set(GMOCK_LIB GTest::gmock)
set(GMOCK_MAIN_LIB GTest::gmock_main)
find_package(benchmark REQUIRED PATHS /opt/EA/inf)
include_directories(/opt/EA/inf/include)

add_subdirectory(base)
add_subdirectory(container)
add_subdirectory(crc)
add_subdirectory(debugging)
add_subdirectory(flags)
add_subdirectory(functional)
add_subdirectory(hash)
add_subdirectory(log)
add_subdirectory(numeric)
add_subdirectory(profile)
add_subdirectory(random)
add_subdirectory(strings)
add_subdirectory(synchronization)
add_subdirectory(times)
