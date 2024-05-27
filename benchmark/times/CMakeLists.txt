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
        NAME bm_test_util
        SOURCES ${PROJECT_SOURCE_DIR}/tests/times/test_util.cc ${PROJECT_SOURCE_DIR}/tests/times/test_util.h
        LINKS
        turbo::turbo
        GTest::gtest
        GTest::gmock
        INCLUDES "/opt/EA/inf/include"
        CXXOPTS ${CARBIN_CXX_OPTIONS}
)

carbin_cc_bm(
        NAME cctz_benchmark
        MODULE times
        SOURCES cctz_benchmark.cc
        LINKS turbo::turbo benchmark::benchmark benchmark::benchmark_main ${CARBIN_DEPS_LINK}
        CXXOPTS ${USER_CXX_FLAGS}
)

carbin_cc_bm(
        NAME civil_time_benchmark
        MODULE times
        SOURCES civil_time_benchmark.cc
        LINKS turbo::turbo benchmark::benchmark benchmark::benchmark_main ${CARBIN_DEPS_LINK}
        CXXOPTS ${USER_CXX_FLAGS}
)

carbin_cc_bm(
        NAME clock_benchmark
        MODULE times
        SOURCES clock_benchmark.cc
        LINKS turbo::turbo benchmark::benchmark benchmark::benchmark_main ${CARBIN_DEPS_LINK}
        CXXOPTS ${USER_CXX_FLAGS}
)
carbin_cc_bm(
        NAME duration_benchmark
        MODULE times
        SOURCES duration_benchmark.cc
        LINKS turbo::turbo benchmark::benchmark benchmark::benchmark_main ${CARBIN_DEPS_LINK}
        CXXOPTS ${USER_CXX_FLAGS}
)

carbin_cc_bm(
        NAME format_benchmark
        MODULE times
        SOURCES format_benchmark.cc
        LINKS
        turbo::turbo bm_test_util_static
        benchmark::benchmark benchmark::benchmark_main ${CARBIN_DEPS_LINK}
        CXXOPTS ${USER_CXX_FLAGS}
)

carbin_cc_bm(
        NAME time_benchmark
        MODULE times
        SOURCES time_benchmark.cc
        LINKS turbo::turbo benchmark::benchmark bm_test_util_static benchmark::benchmark_main ${CARBIN_DEPS_LINK}
        CXXOPTS ${USER_CXX_FLAGS}
)