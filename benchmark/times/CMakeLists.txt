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


carbin_cc_bm(
        NAME
        clock_benchmark.cc
        MODULE bmtimes
        SOURCES
        "clock_benchmark.cc"
        CXXOPTS
        ${CARBIN_CXX_OPTIONS}
        LINKS
        turbo::turbo
        benchmark::benchmark
        benchmark::benchmark_main
)

carbin_cc_bm(
        NAME
        civil_time_benchmark.cc
        MODULE bmtimes
        SOURCES
        "civil_time_benchmark.cc"
        CXXOPTS
        ${CARBIN_CXX_OPTIONS}
        LINKS
        turbo::turbo
        benchmark::benchmark
        benchmark::benchmark_main
)

carbin_cc_bm(
        NAME
        duration_benchmark.cc
        MODULE bmtimes
        SOURCES
        "duration_benchmark.cc"
        CXXOPTS
        ${CARBIN_CXX_OPTIONS}
        LINKS
        turbo::turbo
        benchmark::benchmark
        benchmark::benchmark_main
)

carbin_cc_bm(
        NAME
        time_benchmark.cc
        MODULE bmtimes
        SOURCES
        "time_benchmark.cc"
        CXXOPTS
        ${CARBIN_CXX_OPTIONS}
        LINKS
        turbo::turbo
        benchmark::benchmark
        benchmark::benchmark_main
)

carbin_cc_bm(
        NAME
        format_benchmark
        MODULE bmtimes
        SOURCES
        "format_benchmark.cc"
        CXXOPTS
        ${CARBIN_CXX_OPTIONS}
        LINKS
        turbo::turbo
        benchmark::benchmark
        benchmark::benchmark_main
)