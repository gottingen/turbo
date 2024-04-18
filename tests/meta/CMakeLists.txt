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


carbin_cc_test(
        NAME
        type_traits_test
        MODULE meta
        SOURCES
        "type_traits_test.cc"
        CXXOPTS
        ${CARBIN_CXX_OPTIONS}
        LINKS
        turbo::turbo
)


carbin_cc_test(
        NAME
        utility_test
        MODULE meta
        SOURCES
        "utility_test.cc"
        CXXOPTS
        ${CARBIN_CXX_OPTIONS}
        LINKS
        turbo::turbo
)

#[[
carbin_cc_test(
        NAME
        span_test
        MODULE meta
        SOURCES
        "span_test.cc"
        CXXOPTS
        ${CARBIN_CXX_OPTIONS}
        LINKS
        turbo::exception_testing
        turbo::turbo
)
]]
carbin_cc_test(
        NAME
        compare_test
        MODULE meta
        SOURCES
        "compare_test.cc"
        CXXOPTS
        ${CARBIN_CXX_OPTIONS}
        LINKS
        turbo::turbo
)



carbin_cc_test(
        NAME
        algorithm_test
        MODULE meta
        SOURCES
        "algorithm_test.cc"
        CXXOPTS
        ${CARBIN_CXX_OPTIONS}
        LINKS
        turbo::turbo
)


carbin_cc_test(
        NAME
        container_test
        MODULE meta
        SOURCES
        "container_test.cc"
        CXXOPTS
        ${CARBIN_CXX_OPTIONS}
        LINKS
        turbo::turbo
)


carbin_cc_test(
        NAME
        bind_front_test
        MODULE meta
        SOURCES
        "bind_front_test.cc"
        CXXOPTS
        ${TURBO_DEFAULT_COPTS}
        LINKS
        turbo::turbo
)

carbin_cc_test(
        NAME
        function_ref_test
        MODULE meta
        SOURCES
        "function_ref_test.cc"
        CXXOPTS
        ${CARBIN_CXX_OPTIONS}
        LINKS
        ${CARBIN_LINKS_LINK}
        turbo::turbo
        turbo::test_instance_tracker_static
)
