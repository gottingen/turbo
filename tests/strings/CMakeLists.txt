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
        NAMESPACE
        turbo
        NAME
        match_test
        MODULE strings
        SOURCES
        "match_test.cc"
        CXXOPTS
        ${CARBIN_CXX_OPTIONS}
        LINKS
        turbo::turbo
)

carbin_cc_test(
        NAMESPACE
        turbo
        NAME
        inlined_string_test
        MODULE strings
        SOURCES
        "inlined_string_test.cc"
        CXXOPTS
        ${CARBIN_CXX_OPTIONS}
        LINKS
        turbo::turbo
        ${GTEST_LIB}
        ${GTEST_MAIN_LIB}
)

carbin_cc_test(
        NAMESPACE
        turbo
        NAME
        escaping_test
        MODULE strings
        SOURCES
        "escaping_test.cc"
        CXXOPTS
        ${CARBIN_CXX_OPTIONS}
        LINKS
        turbo::turbo
        ${GTEST_LIB}
        ${GTEST_MAIN_LIB}
)

carbin_cc_test(
        NAMESPACE
        turbo
        NAME
        ascii_test
        MODULE strings
        SOURCES
        "ascii_test.cc"
        CXXOPTS
        ${CARBIN_CXX_OPTIONS}
        LINKS
        turbo::turbo
        ${GTEST_LIB}
        ${GTEST_MAIN_LIB}
)


carbin_cc_test(
        NAME
        memutil_test
        MODULE strings
        SOURCES
        "memutil_test.cc"
        CXXOPTS
        ${CARBIN_CXX_OPTIONS}
        LINKS
        turbo::turbo
)

carbin_cc_test(
        NAMESPACE
        turbo
        NAME
        string_constant_test
        MODULE strings
        SOURCES
        "string_constant_test.cc"
        CXXOPTS
        ${CARBIN_CXX_OPTIONS}
        LINKS
        turbo::turbo
        ${GTEST_LIB}
        ${GTEST_MAIN_LIB}
)

carbin_cc_test(
        NAMESPACE
        turbo
        NAME
        str_replace_test
        MODULE strings
        SOURCES
        "str_replace_test.cc"
        CXXOPTS
        ${CARBIN_CXX_OPTIONS}
        LINKS
        turbo::turbo
)
#[[
carbin_cc_test(
        NAME
        str_split_test
        MODULE strings
        SOURCES
        "str_split_test.cc"
        CXXOPTS
        ${CARBIN_CXX_OPTIONS}
        LINKS
        turbo::turbo
        ${GTEST_LIB}
        ${GMOCK_LIB}
        ${GMOCK_MAIN_LIB}
)
]]
carbin_cc_test(
        NAMESPACE
        turbo
        NAME
        cord_buffer_test
        MODULE strings
        SOURCES
        "cord_buffer_test.cc"
        CXXOPTS
        ${CARBIN_CXX_OPTIONS}
        LINKS
        turbo::turbo
        ${GTEST_LIB}
        ${GTEST_MAIN_LIB}
)

carbin_cc_test(
        NAME
        ostringstream_test
        MODULE strings
        SOURCES
        "ostringstream_test.cc"
        CXXOPTS
        ${CARBIN_CXX_OPTIONS}
        LINKS
        turbo::turbo
        ${GTEST_LIB}
        ${GTEST_MAIN_LIB}
)

carbin_cc_test(
        NAME
        resize_uninitialized_test
        MODULE strings
        SOURCES
        "resize_uninitialized_test.cc"
        CXXOPTS
        ${CARBIN_CXX_OPTIONS}
        LINKS
        turbo::turbo
        ${GTEST_LIB}
        ${GTEST_MAIN_LIB}
)

carbin_cc_test(
        NAME
        numbers_test
        MODULE strings
        SOURCES
        "numbers_test_common.h"
        "numbers_test.cc"
        CXXOPTS
        ${CARBIN_CXX_OPTIONS}
        LINKS
        turbo::pow10_helper
        turbo::turbo
        ${GTEST_LIB}
        ${GTEST_MAIN_LIB}
)

carbin_cc_test(
        NAME
        strip_test
        MODULE strings
        SOURCES
        "strip_test.cc"
        CXXOPTS
        ${CARBIN_CXX_OPTIONS}
        LINKS
        turbo::turbo
)

carbin_cc_test(
        NAME
        char_map_test
        MODULE strings
        SOURCES
        "char_map_test.cc"
        CXXOPTS
        ${CARBIN_CXX_OPTIONS}
        LINKS
        turbo::turbo
)

carbin_cc_test(
        NAME
        charconv_test
        MODULE strings
        SOURCES
        "charconv_test.cc"
        CXXOPTS
        ${CARBIN_CXX_OPTIONS}
        LINKS
        turbo::turbo
        turbo::pow10_helper
)

carbin_cc_test(
        NAME
        charconv_parse_test
        MODULE strings
        SOURCES
        "charconv_parse_test.cc"
        CXXOPTS
        ${CARBIN_CXX_OPTIONS}
        LINKS
        turbo::turbo
)

carbin_cc_test(
        NAME
        charconv_bigint_test
        MODULE strings
        SOURCES
        "charconv_bigint_test.cc"
        CXXOPTS
        ${CARBIN_CXX_OPTIONS}
        LINKS
        turbo::turbo
)


# Internal-only target, do not depend on directly.
carbin_cc_library(
        NAMESPACE
        turbo
        NAME
        pow10_helper
        SOURCES
        "pow10_helper.cc"
        CXXOPTS
        ${CARBIN_CXX_OPTIONS}
        LINKS
        turbo::turbo
        ${GTEST_LIB}
)

carbin_cc_test(
        NAME
        pow10_helper_test
        MODULE strings
        SOURCES
        "pow10_helper_test.cc"
        CXXOPTS
        ${CARBIN_CXX_OPTIONS}
        LINKS
        turbo::pow10_helper
        turbo::turbo
        ${GTEST_LIB}
        ${GTEST_MAIN_LIB}
)


carbin_cc_test(
        NAME
        cordz_update_tracker_test
        MODULE strings
        SOURCES
        "cordz_update_tracker_test.cc"
        CXXOPTS
        ${CARBIN_CXX_OPTIONS}
        LINKS
        turbo::turbo
        ${GTEST_LIB}
        ${GTEST_MAIN_LIB}
)


carbin_cc_test(
        NAME
        cordz_functions_test
        MODULE strings
        SOURCES
        "cordz_functions_test.cc"
        LINKS
        turbo::turbo
        ${GTEST_LIB}
        ${GTEST_MAIN_LIB}
)


carbin_cc_test(
        NAME
        cordz_handle_test
        MODULE strings
        SOURCES
        "cordz_handle_test.cc"
        LINKS
        turbo::turbo
        ${GTEST_LIB}
        ${GTEST_MAIN_LIB}
)

carbin_cc_test(
        NAME
        cordz_info_test
        MODULE strings
        SOURCES
        "cordz_info_test.cc"
        CXXOPTS
        ${CARBIN_CXX_OPTIONS}
        LINKS
        turbo::turbo
        ${GTEST_LIB}
        ${GTEST_MAIN_LIB}
)

carbin_cc_test(
        NAME
        cordz_info_statistics_test
        MODULE strings
        SOURCES
        "cordz_info_statistics_test.cc"
        CXXOPTS
        ${CARBIN_CXX_OPTIONS}
        LINKS
        turbo::turbo
        ${GTEST_LIB}
        ${GMOCK_LIB}
        ${GTEST_MAIN_LIB}
)

carbin_cc_test(
        NAME
        cordz_sample_token_test
        MODULE strings
        SOURCES
        "cordz_sample_token_test.cc"
        CXXOPTS
        ${CARBIN_CXX_OPTIONS}
        LINKS
        turbo::turbo
        ${GTEST_LIB}
        ${GTEST_MAIN_LIB}
)


carbin_cc_test(
        NAME
        cordz_update_scope_test
        MODULE strings
        SOURCES
        "cordz_update_scope_test.cc"
        CXXOPTS
        ${CARBIN_CXX_OPTIONS}
        LINKS
        turbo::turbo
        ${GTEST_LIB}
        ${GTEST_MAIN_LIB}
)

carbin_cc_test(
        NAME
        cord_test
        MODULE strings
        SOURCES
        "cord_test.cc"
        CXXOPTS
        ${CARBIN_CXX_OPTIONS}
        LINKS
        turbo::turbo
        ${GTEST_LIB}
        ${GMOCK_LIB}
        ${GTEST_MAIN_LIB}
)

carbin_cc_test(
        NAME
        cord_data_edge_test
        MODULE strings
        SOURCES
        "cord_data_edge_test.cc"
        CXXOPTS
        ${CARBIN_CXX_OPTIONS}
        LINKS
        turbo::turbo
        ${GTEST_LIB}
        ${GTEST_MAIN_LIB}
)

carbin_cc_test(
        NAME
        cord_rep_btree_test
        MODULE strings
        SOURCES
        "cord_rep_btree_test.cc"
        CXXOPTS
        ${CARBIN_CXX_OPTIONS}
        LINKS
        turbo::turbo
        ${GTEST_LIB}
        ${GMOCK_LIB}
        ${GTEST_MAIN_LIB}
)

carbin_cc_test(
        NAME
        cord_rep_btree_navigator_test
        MODULE strings
        SOURCES
        "cord_rep_btree_navigator_test.cc"
        CXXOPTS
        ${CARBIN_CXX_OPTIONS}
        LINKS
        turbo::turbo
        ${GTEST_LIB}
        ${GTEST_MAIN_LIB}
)

carbin_cc_test(
        NAME
        cord_rep_btree_reader_test
        MODULE strings
        SOURCES
        "cord_rep_btree_reader_test.cc"
        CXXOPTS
        ${CARBIN_CXX_OPTIONS}
        LINKS
        turbo::turbo
        ${GTEST_LIB}
        ${GTEST_MAIN_LIB}
)

carbin_cc_test(
        NAME
        cord_rep_crc_test
        MODULE strings
        SOURCES
        "cord_rep_crc_test.cc"
        CXXOPTS
        ${CARBIN_CXX_OPTIONS}
        LINKS
        turbo::turbo
        ${GTEST_LIB}
        ${GTEST_MAIN_LIB}
)
#TODO(jeff) disable now

carbin_cc_test(
        NAME
        cord_ring_test
        MODULE strings
        SOURCES
        "cord_ring_test.cc"
        CXXOPTS
        ${CARBIN_CXX_OPTIONS}
        LINKS
        turbo::turbo
        ${CARBIN_DEPS_LINK}
        ${GTEST_LIB}
        ${GMOCK_LIB}
        ${GMOCK_MAIN_LIB}
)

carbin_cc_test(
        NAME
        cord_ring_reader_test
        MODULE strings
        SOURCES
        "cord_ring_reader_test.cc"
        CXXOPTS
        ${CARBIN_CXX_OPTIONS}
        LINKS
        turbo::turbo
        ${GTEST_LIB}
        ${GTEST_MAIN_LIB}
)

carbin_cc_test(
        NAME
        cordz_test
        MODULE strings
        SOURCES
        "cordz_test.cc"
        CXXOPTS
        ${CARBIN_CXX_OPTIONS}
        LINKS
        turbo::turbo
        ${GTEST_LIB}
        ${GMOCK_LIB}
        ${GTEST_MAIN_LIB}
)

carbin_cc_test(
        NAME
        str_case_conv_test
        MODULE strings
        SOURCES
        "str_case_conv_test.cc"
        CXXOPTS
        ${CARBIN_CXX_OPTIONS}
        LINKS
        turbo::turbo
)

carbin_cc_test(
        NAME
        str_trim_test
        MODULE strings
        SOURCES
        "str_trim_test.cc"
        CXXOPTS
        ${CARBIN_CXX_OPTIONS}
        LINKS
        turbo::turbo
)

carbin_cc_test(
        NAME
        unicode_view
        MODULE strings
        SOURCES
        "unicode_view_test.cc"
        CXXOPTS
        ${CARBIN_CXX_OPTIONS}
        LINKS
        turbo::turbo
)

carbin_cc_test(
        NAME
        fixed_string_test
        MODULE strings
        SOURCES
        "fixed_string_test.cc"
        CXXOPTS
        ${CARBIN_CXX_OPTIONS}
        LINKS
        turbo::turbo
)