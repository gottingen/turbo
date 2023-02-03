
# Copyright 2022 The Turbo Authors.
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


turbo_cc_test(
        NAME
        crypto_test
        SRCS
        "crypto_test.cc"
        COPTS
        ${TURBO_TEST_COPTS}
        DEPS
        turbo::turbo
        GTest::gtest
        GTest::gtest_main
)



turbo_cc_test(
        NAME
        crc32c_test
        SRCS
        "crc32c_test.cc"
        COPTS
        ${TURBO_DEFAULT_COPTS}
        DEPS
        turbo::turbo
        GTest::gtest_main
)


turbo_cc_test(
        NAME
        crc_memcpy_test
        SRCS
        "internal/crc_memcpy_test.cc"
        COPTS
        ${TURBO_DEFAULT_COPTS}
        DEPS
        turbo::turbo
        GTest::gtest_main
)

turbo_cc_test(
        NAME
        non_temporal_memcpy_test
        SRCS
        "internal/non_temporal_memcpy_test.cc"
        COPTS
        ${TURBO_DEFAULT_COPTS}
        DEPS
        turbo::turbo
        GTest::gtest_main
)

turbo_cc_test(
        NAME
        crc_cord_state_test
        SRCS
        "internal/crc_cord_state_test.cc"
        COPTS
        ${TURBO_DEFAULT_COPTS}
        DEPS
        turbo::turbo
        GTest::gtest_main
)

