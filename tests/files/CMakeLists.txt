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
        multi1_test
        SOURCES
        "multi1_test.cc"
        COPTS
        ${CARBIN_TEST_COPTS}
        DEPS
        turbo::turbo
        ${GTEST_LIB}
        ${GTEST_MAIN_LIB}
)

carbin_cc_test(
        NAME
        filess_system_test
        SOURCES
        "filess_system_test.cc"
        COPTS
        ${CARBIN_TEST_COPTS}
        DEPS
        turbo::turbo
        ${GTEST_LIB}
        ${GTEST_MAIN_LIB}
)

carbin_cc_test(
        NAME
        multi2_test
        SOURCES
        "multi2_test.cc"
        COPTS
        ${CARBIN_TEST_COPTS}
        DEPS
        turbo::turbo
        ${GTEST_LIB}
        ${GTEST_MAIN_LIB}
)

carbin_cc_test(
        NAME
        impl_test
        SOURCES
        "impl_test.cc"
        COPTS
        ${CARBIN_TEST_COPTS}
        DEPS
        turbo::turbo
        ${GTEST_LIB}
        ${GTEST_MAIN_LIB}
)


carbin_cc_test(
        NAME
        fs_exception_test
        SOURCES
        "fs_exception_test.cc"
        COPTS
        ${CARBIN_TEST_COPTS}
        DEPS
        turbo::turbo
        ${GTEST_LIB}
        ${GTEST_MAIN_LIB}
)

carbin_cc_test(
        NAME
        filesystem_test
        SOURCES
        "filesystem_test.cc"
        COPTS
        ${CARBIN_TEST_COPTS}
        DEPS
        turbo::turbo
        ${GTEST_LIB}
        ${GTEST_MAIN_LIB}
)