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
        NAME btree_benchmark
        MODULE container
        SOURCES btree_benchmark.cc
        LINKS turbo::turbo benchmark::benchmark benchmark::benchmark_main ${CARBIN_DEPS_LINK}
        CXXOPTS ${USER_CXX_FLAGS}
)

carbin_cc_bm(
        NAME fixed_array_benchmark
        MODULE container
        SOURCES fixed_array_benchmark.cc
        LINKS turbo::turbo benchmark::benchmark benchmark::benchmark_main ${CARBIN_DEPS_LINK}
        CXXOPTS ${USER_CXX_FLAGS}
)

carbin_cc_bm(
        NAME inlined_vector_benchmark
        MODULE container
        SOURCES inlined_vector_benchmark.cc
        LINKS turbo::turbo benchmark::benchmark benchmark::benchmark_main ${CARBIN_DEPS_LINK}
        CXXOPTS ${USER_CXX_FLAGS}
)

carbin_cc_bm(
        NAME layout_benchmark
        MODULE container
        SOURCES layout_benchmark.cc
        LINKS turbo::turbo benchmark::benchmark benchmark::benchmark_main ${CARBIN_DEPS_LINK}
        CXXOPTS ${USER_CXX_FLAGS}
)

carbin_cc_bm(
        NAME raw_hash_set_benchmark
        MODULE container
        SOURCES raw_hash_set_benchmark.cc
        LINKS turbo::turbo benchmark::benchmark benchmark::benchmark_main ${CARBIN_DEPS_LINK}
        CXXOPTS ${USER_CXX_FLAGS}
)

carbin_cc_bm(
        NAME raw_hash_set_probe_benchmark
        MODULE container
        SOURCES raw_hash_set_probe_benchmark.cc
        LINKS turbo::turbo benchmark::benchmark benchmark::benchmark_main ${CARBIN_DEPS_LINK}
        CXXOPTS ${USER_CXX_FLAGS}
)
carbin_cc_bm(
        NAME small_vector_benchmark
        MODULE container
        SOURCES small_vector_benchmark.cc
        LINKS turbo::turbo benchmark::benchmark benchmark::benchmark_main ${CARBIN_DEPS_LINK}
        CXXOPTS ${USER_CXX_FLAGS}
)
