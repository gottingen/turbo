#
# Copyright 2020 The Turbo Authors.
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
        match_test
        SOURCES
        "match_test.cc"
        COPTS
        ${CARBIN_TEST_COPTS}
        DEPS
        turbo::turbo
        ${GTEST_LIB}
        ${GMOCK_MAIN_LIB}
)

carbin_cc_test(
        NAME
        inlined_string_test
        SOURCES
        "inlined_string_test.cc"
        COPTS
        ${CARBIN_TEST_COPTS}
        DEPS
        turbo::turbo
        ${GTEST_LIB}
        ${GMOCK_MAIN_LIB}
)

carbin_cc_test(
        NAME
        escaping_test
        SOURCES
        "escaping_test.cc"
        COPTS
        ${CARBIN_TEST_COPTS}
        DEPS
        turbo::turbo
        ${GTEST_LIB}
        ${GMOCK_MAIN_LIB}
)

carbin_cc_test(
        NAME
        ascii_test
        SOURCES
        "ascii_test.cc"
        COPTS
        ${CARBIN_TEST_COPTS}
        DEPS
        turbo::turbo
        ${GTEST_LIB}
        ${GMOCK_MAIN_LIB}
)

carbin_cc_test(
        NAME
        damerau_levenshtein_distance_test
        SOURCES
        "damerau_levenshtein_distance_test.cc"
        COPTS
        ${CARBIN_TEST_COPTS}
        DEPS
        turbo::turbo
        ${GTEST_LIB}
        ${GMOCK_MAIN_LIB}
)

carbin_cc_test(
        NAME
        memutil_test
        SOURCES
        "memutil_test.cc"
        COPTS
        ${CARBIN_TEST_COPTS}
        DEPS
        turbo::turbo
        ${GTEST_LIB}
        ${GMOCK_MAIN_LIB}
)

carbin_cc_test(
        NAME
        string_constant_test
        SOURCES
        "string_constant_test.cc"
        COPTS
        ${CARBIN_TEST_COPTS}
        DEPS
        turbo::turbo
        ${GTEST_LIB}
        ${GMOCK_MAIN_LIB}
)

carbin_cc_test(
        NAME
        substitute_test
        SOURCES
        "substitute_test.cc"
        COPTS
        ${CARBIN_TEST_COPTS}
        DEPS
        turbo::turbo
        ${GTEST_LIB}
        ${GMOCK_MAIN_LIB}
)

carbin_cc_test(
        NAME
        str_replace_test
        SOURCES
        "str_replace_test.cc"
        COPTS
        ${CARBIN_TEST_COPTS}
        DEPS
        turbo::turbo
        ${GTEST_LIB}
        ${GMOCK_MAIN_LIB}
)

carbin_cc_test(
        NAME
        str_split_test
        SOURCES
        "str_split_test.cc"
        COPTS
        ${CARBIN_TEST_COPTS}
        DEPS
        turbo::turbo
        ${GTEST_LIB}
        ${GMOCK_LIB}
        ${GMOCK_MAIN_LIB}
)

carbin_cc_test(
        NAME
        cord_buffer_test
        SOURCES
        "cord_buffer_test.cc"
        COPTS
        ${CARBIN_TEST_COPTS}
        DEPS
        turbo::turbo
        ${GTEST_LIB}
        ${GMOCK_MAIN_LIB}
)

carbin_cc_test(
        NAME
        ostringstream_test
        SOURCES
        "ostringstream_test.cc"
        COPTS
        ${CARBIN_TEST_COPTS}
        DEPS
        turbo::turbo
        ${GTEST_LIB}
        ${GMOCK_MAIN_LIB}
)

carbin_cc_test(
        NAME
        resize_uninitialized_test
        SOURCES
        "resize_uninitialized_test.cc"
        COPTS
        ${CARBIN_TEST_COPTS}
        DEPS
        turbo::turbo
        ${GTEST_LIB}
        ${GMOCK_MAIN_LIB}
)

carbin_cc_test(
        NAME
        str_join_test
        SOURCES
        "str_join_test.cc"
        COPTS
        ${CARBIN_TEST_COPTS}
        DEPS
        turbo::turbo
        ${GTEST_LIB}
        ${GMOCK_MAIN_LIB}
)

carbin_cc_test(
        NAME
        str_cat_test
        SOURCES
        "str_cat_test.cc"
        COPTS
        ${CARBIN_TEST_COPTS}
        DEPS
        turbo::turbo
        ${GTEST_LIB}
        ${GMOCK_MAIN_LIB}
)

carbin_cc_test(
        NAME
        numbers_test
        SOURCES
        "numbers_test_common.h"
        "numbers_test.cc"
        COPTS
        ${CARBIN_TEST_COPTS}
        DEPS
        turbo::pow10_helper
        turbo::turbo
        ${GTEST_LIB}
        ${GMOCK_MAIN_LIB}
)

carbin_cc_test(
        NAME
        strip_test
        SOURCES
        "strip_test.cc"
        COPTS
        ${CARBIN_TEST_COPTS}
        DEPS
        turbo::turbo
        ${GTEST_LIB}
        ${GMOCK_MAIN_LIB}
)

carbin_cc_test(
        NAME
        char_map_test
        SOURCES
        "char_map_test.cc"
        COPTS
        ${CARBIN_TEST_COPTS}
        DEPS
        turbo::turbo
        ${GTEST_LIB}
        ${GMOCK_MAIN_LIB}
)

carbin_cc_test(
        NAME
        charconv_test
        SOURCES
        "charconv_test.cc"
        COPTS
        ${CARBIN_TEST_COPTS}
        DEPS
        turbo::turbo
        ${GTEST_LIB}
        turbo::pow10_helper
        ${GMOCK_MAIN_LIB}
)

carbin_cc_test(
        NAME
        charconv_parse_test
        SOURCES
        "charconv_parse_test.cc"
        COPTS
        ${CARBIN_TEST_COPTS}
        DEPS
        turbo::turbo
        ${GTEST_LIB}
        ${GMOCK_MAIN_LIB}
)

carbin_cc_test(
        NAME
        charconv_bigint_test
        SOURCES
        "charconv_bigint_test.cc"
        COPTS
        ${CARBIN_TEST_COPTS}
        DEPS
        turbo::turbo
        ${GTEST_LIB}
        ${GMOCK_MAIN_LIB}
)


carbin_cc_test(
        NAME
        str_format_test
        SOURCES
        "str_format_test.cc"
        COPTS
        ${CARBIN_TEST_COPTS}
        DEPS
        turbo::turbo
        ${GTEST_LIB}
        ${GMOCK_MAIN_LIB}
)


# Internal-only target, do not depend on directly.
carbin_cc_library(
        NAME
        pow10_helper
        SOURCES
        "pow10_helper.cc"
        COPTS
        ${CARBIN_TEST_COPTS}
        DEPS
        turbo::turbo
        ${GTEST_LIB}
)

carbin_cc_test(
        NAME
        pow10_helper_test
        SOURCES
        "pow10_helper_test.cc"
        COPTS
        ${CARBIN_TEST_COPTS}
        DEPS
        turbo::pow10_helper
        turbo::turbo
        ${GTEST_LIB}
        ${GMOCK_MAIN_LIB}
)


carbin_cc_test(
        NAME
        cordz_update_tracker_test
        SOURCES
        "cordz_update_tracker_test.cc"
        COPTS
        ${CARBIN_TEST_COPTS}
        DEPS
        turbo::turbo
        ${GTEST_LIB}
        ${GMOCK_MAIN_LIB}
)


carbin_cc_test(
        NAME
        cordz_functions_test
        SOURCES
        "cordz_functions_test.cc"
        DEPS
        turbo::turbo
        ${GTEST_LIB}
        ${GMOCK_MAIN_LIB}
)


carbin_cc_test(
        NAME
        cordz_handle_test
        SOURCES
        "cordz_handle_test.cc"
        DEPS
        turbo::turbo
        ${GTEST_LIB}
        ${GMOCK_MAIN_LIB}
)

carbin_cc_test(
        NAME
        cordz_info_test
        SOURCES
        "cordz_info_test.cc"
        COPTS
        ${CARBIN_TEST_COPTS}
        DEPS
        turbo::turbo
        ${GTEST_LIB}
        ${GMOCK_MAIN_LIB}
)

carbin_cc_test(
        NAME
        cordz_info_statistics_test
        SOURCES
        "cordz_info_statistics_test.cc"
        COPTS
        ${CARBIN_TEST_COPTS}
        DEPS
        turbo::turbo
        ${GTEST_LIB}
        ${GMOCK_LIB}
        ${GMOCK_MAIN_LIB}
)

carbin_cc_test(
        NAME
        cordz_sample_token_test
        SOURCES
        "cordz_sample_token_test.cc"
        COPTS
        ${CARBIN_TEST_COPTS}
        DEPS
        turbo::turbo
        ${GTEST_LIB}
        ${GMOCK_MAIN_LIB}
)


carbin_cc_test(
        NAME
        cordz_update_scope_test
        SOURCES
        "cordz_update_scope_test.cc"
        COPTS
        ${CARBIN_TEST_COPTS}
        DEPS
        turbo::turbo
        ${GTEST_LIB}
        ${GMOCK_MAIN_LIB}
)


carbin_cc_test(
        NAME
        cord_test
        SOURCES
        "cord_test.cc"
        COPTS
        ${CARBIN_TEST_COPTS}
        DEPS
        turbo::turbo
        ${GTEST_LIB}
        ${GMOCK_MAIN_LIB}
)

carbin_cc_test(
        NAME
        cord_data_edge_test
        SOURCES
        "cord_data_edge_test.cc"
        COPTS
        ${CARBIN_TEST_COPTS}
        DEPS
        turbo::turbo
        ${GTEST_LIB}
        ${GMOCK_MAIN_LIB}
)

carbin_cc_test(
        NAME
        cord_rep_btree_test
        SOURCES
        "cord_rep_btree_test.cc"
        COPTS
        ${CARBIN_TEST_COPTS}
        DEPS
        turbo::turbo
        ${GTEST_LIB}
        ${GMOCK_LIB}
        ${GMOCK_MAIN_LIB}
)

carbin_cc_test(
        NAME
        cord_rep_btree_navigator_test
        SOURCES
        "cord_rep_btree_navigator_test.cc"
        COPTS
        ${CARBIN_TEST_COPTS}
        DEPS
        turbo::turbo
        ${GTEST_LIB}
        ${GMOCK_MAIN_LIB}
)

carbin_cc_test(
        NAME
        cord_rep_btree_reader_test
        SOURCES
        "cord_rep_btree_reader_test.cc"
        COPTS
        ${CARBIN_TEST_COPTS}
        DEPS
        turbo::turbo
        ${GTEST_LIB}
        ${GMOCK_MAIN_LIB}
)

carbin_cc_test(
        NAME
        cord_rep_crc_test
        SOURCES
        "cord_rep_crc_test.cc"
        COPTS
        ${CARBIN_TEST_COPTS}
        DEPS
        turbo::turbo
        ${GTEST_LIB}
        ${GMOCK_MAIN_LIB}
)

carbin_cc_test(
        NAME
        cord_ring_test
        SOURCES
        "cord_ring_test.cc"
        COPTS
        ${CARBIN_TEST_COPTS}
        DEPS
        turbo::turbo
        ${GTEST_LIB}
        ${GMOCK_LIB}
        ${GMOCK_MAIN_LIB}
)

carbin_cc_test(
        NAME
        cord_ring_reader_test
        SOURCES
        "cord_ring_reader_test.cc"
        COPTS
        ${CARBIN_TEST_COPTS}
        DEPS
        turbo::turbo
        ${GTEST_LIB}
        ${GMOCK_MAIN_LIB}
)

carbin_cc_test(
        NAME
        cordz_test
        SOURCES
        "cordz_test.cc"
        COPTS
        ${CARBIN_TEST_COPTS}
        DEPS
        turbo::turbo
        ${GTEST_LIB}
        ${GMOCK_MAIN_LIB}
)

