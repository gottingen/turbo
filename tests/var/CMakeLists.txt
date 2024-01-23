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
        var_agent_group_unittest
        SOURCES
        "var_agent_group_test.cc"
        COPTS
        ${CARBIN_CXX_OPTIONS}
        DEPS
        ${CARBIN_DEPS_LINK}
        turbo::turbo
        ${GTEST_LIB}
        ${GMOCK_LIB}
        ${GTEST_MAIN_LIB}
)

carbin_cc_test(
        NAME
        var_file_dumper_test
        SOURCES
        "var_file_dumper_test.cc"
        COPTS
        ${CARBIN_CXX_OPTIONS}
        DEPS
        ${CARBIN_DEPS_LINK}
        turbo::turbo
        ${GTEST_LIB}
        ${GMOCK_LIB}
        ${GTEST_MAIN_LIB}
)
carbin_cc_test(
        NAME
        var_percentile_unittest
        SOURCES
        "var_percentile_test.cc"
        COPTS
        ${CARBIN_CXX_OPTIONS}
        "-fno-access-control"
        DEPS
        ${CARBIN_DEPS_LINK}
        turbo::turbo
        ${GTEST_LIB}
        ${GMOCK_LIB}
        ${GTEST_MAIN_LIB}
)

carbin_cc_test(
        NAME
        var_sampler_unittest
        SOURCES
        "var_sampler_test.cc"
        COPTS
        ${CARBIN_CXX_OPTIONS}
        "-fno-access-control"
        DEPS
        ${CARBIN_DEPS_LINK}
        turbo::turbo
        ${GTEST_LIB}
        ${GMOCK_LIB}
        ${GTEST_MAIN_LIB}
)

carbin_cc_test(
        NAME
        var_reducer_test
        SOURCES
        "var_reducer_test.cc"
        COPTS
        ${CARBIN_CXX_OPTIONS}
        "-fno-access-control"
        DEPS
        ${CARBIN_DEPS_LINK}
        turbo::turbo
        ${GTEST_LIB}
        ${GMOCK_LIB}
        ${GTEST_MAIN_LIB}
)

carbin_cc_test(
        NAME
        var_window_unittest
        SOURCES
        "var_window_test.cc"
        COPTS
        ${CARBIN_CXX_OPTIONS}
        "-fno-access-control"
        DEPS
        ${CARBIN_DEPS_LINK}
        turbo::turbo
        ${GTEST_LIB}
        ${GMOCK_LIB}
        ${GTEST_MAIN_LIB}
)

carbin_cc_test(
        NAME
        var_variable_test
        SOURCES
        "var_variable_test.cc"
        COPTS
        ${CARBIN_CXX_OPTIONS}
        "-fno-access-control"
        DEPS
        ${CARBIN_DEPS_LINK}
        turbo::turbo
        ${GTEST_LIB}
        ${GMOCK_LIB}
        ${GTEST_MAIN_LIB}
)

carbin_cc_test(
        NAME
        var_status_test
        SOURCES
        "var_status_test.cc"
        COPTS
        ${CARBIN_CXX_OPTIONS}
        "-fno-access-control"
        DEPS
        ${CARBIN_DEPS_LINK}
        turbo::turbo
        ${GTEST_LIB}
        ${GMOCK_LIB}
        ${GTEST_MAIN_LIB}
)

carbin_cc_test(
        NAME
        var_recorder_test
        SOURCES
        "var_recorder_test.cc"
        COPTS
        ${CARBIN_CXX_OPTIONS}
        "-fno-access-control"
        DEPS
        ${CARBIN_DEPS_LINK}
        turbo::turbo
        ${GTEST_LIB}
        ${GMOCK_LIB}
        ${GTEST_MAIN_LIB}
)

carbin_cc_test(
        NAME
        sample_recorder_test
        SOURCES
        "periodic_sampler_test.cc"
        COPTS
        ${CARBIN_CXX_OPTIONS}
        DEPS
        turbo::turbo
        ${GTEST_LIB}
        ${GMOCK_LIB}
        ${GTEST_MAIN_LIB}
)

