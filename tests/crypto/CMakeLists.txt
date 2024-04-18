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
        crypto_test
        MODULE crypto
        SOURCES
        "crypto_test.cc"
        CXXOPTS
        ${CARBIN_CXX_OPTIONS}
        LINKS
        turbo::turbo
)


carbin_cc_test(
        NAME
        crc32c_test
        MODULE crypto
        SOURCES
        "crc32c_test.cc"
        CXXOPTS
        ${CARBIN_CXX_OPTIONS}
        LINKS
        turbo::turbo
)


carbin_cc_test(
        NAME
        crc_memcpy_test
        MODULE crypto
        SOURCES
        "crc_memcpy_test.cc"
        CXXOPTS
        ${CARBIN_CXX_OPTIONS}
        LINKS
        turbo::turbo
        ${GTEST_LIB}
        ${GTEST_MAIN_LIB}
)

carbin_cc_test(
        NAME
        non_temporal_memcpy_test
        MODULE crypto
        SOURCES
        "non_temporal_memcpy_test.cc"
        CXXOPTS
        ${CARBIN_CXX_OPTIONS}
        LINKS
        turbo::turbo
        ${GTEST_LIB}
        ${GTEST_MAIN_LIB}
)

carbin_cc_test(
        NAME
        crc_cord_state_test
        MODULE crypto
        SOURCES
        "crc_cord_state_test.cc"
        CXXOPTS
        ${CARBIN_CXX_OPTIONS}
        LINKS
        turbo::turbo
)

