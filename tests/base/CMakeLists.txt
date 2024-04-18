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
        throw_delegate_test
        MODULE base
        SOURCES
        "throw_delegate_test.cc"
        CXXOPTS
        ${CARBIN_CXX_OPTIONS}
        LINKS
        ${CARBIN_DEPS_LINK}
        turbo::turbo
)


carbin_cc_test(
        NAME
        bits_test
        MODULE base
        SOURCES
        "bits_test.cc"
        CXXOPTS
        ${CARBIN_CXX_OPTIONS}
        LINKS
        turbo::turbo
        ${CARBIN_DEPS_LINK}
)

carbin_cc_test(
        NAME
        endian_test
        MODULE base
        SOURCES
        "endian_test.cc"
        CXXOPTS
        ${CARBIN_CXX_OPTIONS}
        LINKS
        turbo::turbo
        ${CARBIN_DEPS_LINK}
)

carbin_cc_test(
        NAME
        int128_test
        MODULE base
        SOURCES
        "int128_stream_test.cc"
        "int128_test.cc"
        CXXOPTS
        ${CARBIN_CXX_OPTIONS}
        LINKS
        turbo::turbo
)


# Internal-only target, do not depend on directly.
carbin_cc_interface(
        NAMESPACE
        turbo
        NAME
        exception_testing
        HEADERS
        "exception_testing.h"
        CXXOPTS
        ${TURBO_DEFAULT_CXXOPTS}
        LINKS
        turbo::turbo
        ${GTEST_LIB}
)


# Internal-only target, do not depend on directly.
carbin_cc_test_library(
        NAMESPACE
        turbo
        NAME
        exception_safety_testing
        HEADERS
        "exception_safety_testing.h"
        SOURCES
        "exception_safety_testing.cc"
        CXXOPTS
        ${CARBIN_CXX_OPTIONS}
        LINKS
        ${GTEST_LIB}
        turbo::turbo
)


carbin_cc_test(
        NAME
        exception_safety_testing_test
        MODULE base
        SOURCES
        "exception_safety_testing_test.cc"
        CXXOPTS
        ${CARBIN_CXX_OPTIONS}
        LINKS
        turbo::turbo
        ${GTEST_LIB}
        ${GMOCK_LIB}
        turbo::exception_safety_testing_static
        ${GTEST_MAIN_LIB}
        ${CARBIN_DEPS_LINK}
)

carbin_cc_test(
        NAME
        bit_cast_test
        MODULE base
        SOURCES
        "bit_cast_test.cc"
        CXXOPTS
        ${CARBIN_CXX_OPTIONS}
        LINKS
        turbo::turbo
        ${CARBIN_DEPS_LINK}
)

carbin_cc_test(
        NAME
        inline_variable_test
        MODULE base
        SOURCES
        "inline_variable_testing.h"
        "inline_variable_test.cc"
        "inline_variable_test_a.cc"
        "inline_variable_test_b.cc"
        CXXOPTS
        ${CARBIN_CXX_OPTIONS}
        LINKS
        ${CARBIN_DEPS_LINK}
        turbo::turbo
)



carbin_cc_test(
        NAME
        raw_logging_test
        MODULE base
        SOURCES
        "raw_logging_test.cc"
        CXXOPTS
        ${CARBIN_CXX_OPTIONS}
        LINKS
        turbo::turbo
        ${GTEST_LIB}
        ${GTEST_MAIN_LIB}
)


carbin_cc_test(
        NAME
        fast_type_id_test
        MODULE base
        SOURCES
        "fast_type_id_test.cc"
        CXXOPTS
        ${CARBIN_CXX_OPTIONS}
        LINKS
        turbo::turbo
        ${GTEST_LIB}
        ${GTEST_MAIN_LIB}
)


carbin_cc_test(
        NAME
        strerror_test
        MODULE base
        SOURCES
        "strerror_test.cc"
        CXXOPTS
        ${CARBIN_CXX_OPTIONS}
        LINKS
        turbo::turbo
)


carbin_cc_test(
        NAME
        math_test
        MODULE base
        SOURCES
        "math_test.cc"
        CXXOPTS
        ${CARBIN_CXX_OPTIONS}
        LINKS
        ${CARBIN_DEPS_LINK}
        turbo::turbo
)

