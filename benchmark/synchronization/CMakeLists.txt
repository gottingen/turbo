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

carbin_cc_bm(
        NAME blocking_counter_benchmark
        MODULE synchronization
        SOURCES blocking_counter_benchmark.cc
        LINKS turbo::turbo benchmark::benchmark benchmark::benchmark_main ${CARBIN_DEPS_LINK}
        CXXOPTS ${USER_CXX_FLAGS}
)

carbin_cc_bm(
        NAME graphcycles_benchmark
        MODULE synchronization
        SOURCES graphcycles_benchmark.cc
        LINKS turbo::turbo benchmark::benchmark benchmark::benchmark_main ${CARBIN_DEPS_LINK}
        CXXOPTS ${USER_CXX_FLAGS}
)

carbin_cc_bm(
        NAME mutex_benchmark
        MODULE synchronization
        SOURCES mutex_benchmark.cc
        LINKS turbo::turbo benchmark::benchmark benchmark::benchmark_main ${CARBIN_DEPS_LINK}
        CXXOPTS ${USER_CXX_FLAGS}
)