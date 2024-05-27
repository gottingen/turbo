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
        NAME ascii_benchmark
        MODULE strings
        SOURCES ascii_benchmark.cc
        LINKS turbo::turbo benchmark::benchmark benchmark::benchmark_main ${CARBIN_DEPS_LINK}
        CXXOPTS ${USER_CXX_FLAGS}
)

carbin_cc_bm(
        NAME charconv_benchmark
        MODULE strings
        SOURCES charconv_benchmark.cc
        LINKS turbo::turbo benchmark::benchmark benchmark::benchmark_main ${CARBIN_DEPS_LINK}
        CXXOPTS ${USER_CXX_FLAGS}
)

carbin_cc_bm(
        NAME charset_benchmark
        MODULE strings
        SOURCES charset_benchmark.cc
        LINKS turbo::turbo benchmark::benchmark benchmark::benchmark_main ${CARBIN_DEPS_LINK}
        CXXOPTS ${USER_CXX_FLAGS}
)

carbin_cc_bm(
        NAME escaping_benchmark
        MODULE strings
        SOURCES escaping_benchmark.cc
        LINKS turbo::turbo benchmark::benchmark benchmark::benchmark_main ${CARBIN_DEPS_LINK}
        CXXOPTS ${USER_CXX_FLAGS}
)

carbin_cc_bm(
        NAME memutil_benchmark
        MODULE strings
        SOURCES memutil_benchmark.cc
        LINKS turbo::turbo benchmark::benchmark benchmark::benchmark_main ${CARBIN_DEPS_LINK}
        CXXOPTS ${USER_CXX_FLAGS}
)

carbin_cc_bm(
        NAME numbers_benchmark
        MODULE strings
        SOURCES numbers_benchmark.cc
        LINKS turbo::turbo benchmark::benchmark benchmark::benchmark_main ${CARBIN_DEPS_LINK}
        CXXOPTS ${USER_CXX_FLAGS}
)

carbin_cc_bm(
        NAME ostringstream_benchmark
        MODULE strings
        SOURCES ostringstream_benchmark.cc
        LINKS turbo::turbo benchmark::benchmark benchmark::benchmark_main ${CARBIN_DEPS_LINK}
        CXXOPTS ${USER_CXX_FLAGS}
)

carbin_cc_bm(
        NAME str_cat_benchmark
        MODULE strings
        SOURCES str_cat_benchmark.cc
        LINKS turbo::turbo benchmark::benchmark benchmark::benchmark_main ${CARBIN_DEPS_LINK}
        CXXOPTS ${USER_CXX_FLAGS}
)

carbin_cc_bm(
        NAME str_join_benchmark
        MODULE strings
        SOURCES str_join_benchmark.cc
        LINKS turbo::turbo benchmark::benchmark benchmark::benchmark_main ${CARBIN_DEPS_LINK}
        CXXOPTS ${USER_CXX_FLAGS}
)

carbin_cc_bm(
        NAME str_replace_benchmark
        MODULE strings
        SOURCES str_replace_benchmark.cc
        LINKS turbo::turbo benchmark::benchmark benchmark::benchmark_main ${CARBIN_DEPS_LINK}
        CXXOPTS ${USER_CXX_FLAGS}
)

carbin_cc_bm(
        NAME str_split_benchmark
        MODULE strings
        SOURCES str_split_benchmark.cc
        LINKS turbo::turbo benchmark::benchmark benchmark::benchmark_main ${CARBIN_DEPS_LINK}
        CXXOPTS ${USER_CXX_FLAGS}
)

carbin_cc_bm(
        NAME string_view_benchmark
        MODULE strings
        SOURCES string_view_benchmark.cc
        LINKS turbo::turbo benchmark::benchmark benchmark::benchmark_main ${CARBIN_DEPS_LINK}
        CXXOPTS ${USER_CXX_FLAGS}
)