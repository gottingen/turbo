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



# Internal-only target, do not depend on directly.
file(GLOB REF_SRC "reference/*.cc")
carbin_cc_library(
        NAMESPACE
        turbo
        NAME
        test_reference
        SOURCES
        ${REF_SRC}
        COPTS
        ${TURBO_DEFAULT_COPTS}
        ${CARBIN_SIMD_FLAGS}
        DEPS
        turbo::turbo
)

# Internal-only target, do not depend on directly.
file(GLOB HELPER_SRC "helpers/*.cc")
carbin_cc_library(
        NAMESPACE
        turbo
        NAME
        test_helper
        SOURCES
        ${HELPER_SRC}
        COPTS
        ${TURBO_DEFAULT_COPTS}
        ${CARBIN_SIMD_FLAGS}
        DEPS
        turbo::turbo
        turbo::test_reference
)

file(GLOB UNICODE_TEST_SRCS "*.cc")
foreach (UNICODE_UT ${UNICODE_TEST_SRCS})
    get_filename_component(UNICODE_UT_WE ${UNICODE_UT} NAME_WE)
    carbin_cc_test(
            NAME
            ${UNICODE_UT_WE}
            SOURCES
            ${UNICODE_UT}
            COPTS
            ${CARBIN_TEST_COPTS}
            ${CARBIN_SIMD_FLAGS}
            DEPS
            turbo::test_reference
            turbo::test_helper
            turbo::turbo
            ${GMOCK_LIB}
            ${GTEST_LIB}
            ${GTEST_MAIN_LIB}
    )
endforeach ()