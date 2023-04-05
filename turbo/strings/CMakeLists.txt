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


turbo_cc_test(
  NAME
    match_test
  SRCS
    "match_test.cc"
  COPTS
    ${TURBO_TEST_COPTS}
  DEPS
    turbo::turbo
    GTest::gmock_main
)

turbo_cc_test(
        NAME
        inlined_string_test
        SRCS
        "inlined_string_test.cc"
        COPTS
        ${TURBO_TEST_COPTS}
        DEPS
        turbo::turbo
        GTest::gmock_main
)

turbo_cc_test(
  NAME
    escaping_test
  SRCS
    "escaping_test.cc"
  COPTS
    ${TURBO_TEST_COPTS}
  DEPS
    turbo::turbo
    GTest::gmock_main
)

turbo_cc_test(
  NAME
    ascii_test
  SRCS
    "ascii_test.cc"
  COPTS
    ${TURBO_TEST_COPTS}
  DEPS
    turbo::turbo
    GTest::gmock_main
)

turbo_cc_test(
  NAME
    damerau_levenshtein_distance_test
  SRCS
    "internal/damerau_levenshtein_distance_test.cc"
  COPTS
    ${TURBO_TEST_COPTS}
  DEPS
    turbo::turbo
    GTest::gmock_main
)

turbo_cc_test(
  NAME
    memutil_test
  SRCS
    "internal/memutil.h"
    "internal/memutil_test.cc"
  COPTS
    ${TURBO_TEST_COPTS}
  DEPS
    turbo::turbo
    GTest::gmock_main
)

turbo_cc_test(
  NAME
    string_constant_test
  SRCS
    "internal/string_constant_test.cc"
  COPTS
    ${TURBO_TEST_COPTS}
  DEPS
    turbo::turbo
    GTest::gmock_main
)

turbo_cc_test(
  NAME
    string_view_test
  SRCS
    "string_piece_test.cc"
  COPTS
    ${TURBO_TEST_COPTS}
  DEPS
    turbo::turbo
    GTest::gmock_main
)

turbo_cc_test(
  NAME
    substitute_test
  SRCS
    "substitute_test.cc"
  COPTS
    ${TURBO_TEST_COPTS}
  DEPS
    turbo::turbo
    GTest::gmock_main
)

turbo_cc_test(
  NAME
    str_replace_test
  SRCS
    "str_replace_test.cc"
  COPTS
    ${TURBO_TEST_COPTS}
  DEPS
    turbo::turbo
    GTest::gmock_main
)

turbo_cc_test(
  NAME
    str_split_test
  SRCS
    "str_split_test.cc"
  COPTS
    ${TURBO_TEST_COPTS}
  DEPS
    turbo::turbo
    GTest::gmock_main
)

turbo_cc_test(
  NAME
    ostringstream_test
  SRCS
    "internal/ostringstream_test.cc"
  COPTS
    ${TURBO_TEST_COPTS}
  DEPS
    turbo::turbo
    GTest::gmock_main
)

turbo_cc_test(
  NAME
    resize_uninitialized_test
  SRCS
    "internal/resize_uninitialized.h"
    "internal/resize_uninitialized_test.cc"
  COPTS
    ${TURBO_TEST_COPTS}
  DEPS
    turbo::turbo
    GTest::gmock_main
)

turbo_cc_test(
  NAME
    str_join_test
  SRCS
    "str_join_test.cc"
  COPTS
    ${TURBO_TEST_COPTS}
  DEPS
    turbo::turbo
    GTest::gmock_main
)

turbo_cc_test(
  NAME
    str_cat_test
  SRCS
    "str_cat_test.cc"
  COPTS
    ${TURBO_TEST_COPTS}
  DEPS
    turbo::turbo
    GTest::gmock_main
)

turbo_cc_test(
  NAME
    numbers_test
  SRCS
    "internal/numbers_test_common.h"
    "numbers_test.cc"
  COPTS
    ${TURBO_TEST_COPTS}
  DEPS
    turbo::pow10_helper
    turbo::turbo
    GTest::gmock_main
)

turbo_cc_test(
  NAME
    strip_test
  SRCS
    "strip_test.cc"
  COPTS
    ${TURBO_TEST_COPTS}
  DEPS
    turbo::turbo
    GTest::gmock_main
)

turbo_cc_test(
  NAME
    char_map_test
  SRCS
    "internal/char_map_test.cc"
  COPTS
    ${TURBO_TEST_COPTS}
  DEPS
    turbo::turbo
    GTest::gmock_main
)

turbo_cc_test(
  NAME
    charconv_test
  SRCS
    "charconv_test.cc"
  COPTS
    ${TURBO_TEST_COPTS}
  DEPS
    turbo::turbo
    turbo::pow10_helper
    GTest::gmock_main
)

turbo_cc_test(
  NAME
    charconv_parse_test
  SRCS
    "internal/charconv_parse.h"
    "internal/charconv_parse_test.cc"
  COPTS
    ${TURBO_TEST_COPTS}
  DEPS
    turbo::turbo
    GTest::gmock_main
)

turbo_cc_test(
  NAME
    charconv_bigint_test
  SRCS
    "internal/charconv_bigint.h"
    "internal/charconv_bigint_test.cc"
    "internal/charconv_parse.h"
  COPTS
    ${TURBO_TEST_COPTS}
  DEPS
    turbo::turbo
    GTest::gmock_main
)


turbo_cc_test(
  NAME
    str_format_test
  SRCS
    "str_format_test.cc"
  COPTS
    ${TURBO_TEST_COPTS}
  DEPS
    turbo::turbo
    GTest::gmock_main
)

turbo_cc_test(
  NAME
    str_format_extension_test
  SRCS
    "internal/str_format/extension_test.cc"
  COPTS
    ${TURBO_TEST_COPTS}
  DEPS
    turbo::turbo
    GTest::gmock_main
)

turbo_cc_test(
  NAME
    str_format_arg_test
  SRCS
    "internal/str_format/arg_test.cc"
  COPTS
    ${TURBO_TEST_COPTS}
  DEPS
    turbo::turbo
    GTest::gmock_main
)

turbo_cc_test(
  NAME
    str_format_bind_test
  SRCS
    "internal/str_format/bind_test.cc"
  COPTS
    ${TURBO_TEST_COPTS}
  DEPS
    turbo::turbo
    GTest::gmock_main
)

turbo_cc_test(
  NAME
    str_format_checker_test
  SRCS
    "internal/str_format/checker_test.cc"
  COPTS
    ${TURBO_TEST_COPTS}
  DEPS
    turbo::turbo
    GTest::gmock_main
)

turbo_cc_test(
  NAME
    str_format_convert_test
  SRCS
    "internal/str_format/convert_test.cc"
  COPTS
    ${TURBO_TEST_COPTS}
  DEPS
    turbo::turbo
    GTest::gmock_main
)

turbo_cc_test(
  NAME
    str_format_output_test
  SRCS
    "internal/str_format/output_test.cc"
  COPTS
    ${TURBO_TEST_COPTS}
  DEPS
    turbo::turbo
    GTest::gmock_main
)

turbo_cc_test(
  NAME
    str_format_parser_test
  SRCS
    "internal/str_format/parser_test.cc"
  COPTS
    ${TURBO_TEST_COPTS}
  DEPS
    turbo::turbo
    GTest::gmock_main
)

# Internal-only target, do not depend on directly.
turbo_cc_library(
  NAME
    pow10_helper
  HDRS
    "internal/pow10_helper.h"
  SRCS
    "internal/pow10_helper.cc"
  COPTS
    ${TURBO_TEST_COPTS}
  DEPS
    turbo::turbo
  TESTONLY
)

turbo_cc_test(
  NAME
    pow10_helper_test
  SRCS
    "internal/pow10_helper_test.cc"
  COPTS
    ${TURBO_TEST_COPTS}
  DEPS
    turbo::pow10_helper
    turbo::turbo
    GTest::gmock_main
)



turbo_cc_test(
  NAME
    cordz_update_tracker_test
  SRCS
    "internal/cordz_update_tracker_test.cc"
  COPTS
    ${TURBO_TEST_COPTS}
  DEPS
    turbo::turbo
    turbo::sync
    GTest::gmock_main
)


turbo_cc_test(
  NAME
    cordz_functions_test
  SRCS
    "internal/cordz_functions_test.cc"
  DEPS
    turbo::turbo
    GTest::gmock_main
)



turbo_cc_test(
  NAME
    cordz_handle_test
  SRCS
    "internal/cordz_handle_test.cc"
  DEPS
    turbo::turbo
    turbo::sync
    GTest::gmock_main
)

turbo_cc_test(
  NAME
    cordz_info_test
  SRCS
    "internal/cordz_info_test.cc"
  COPTS
    ${TURBO_TEST_COPTS}
  DEPS
    turbo::turbo
    GTest::gmock_main
)

turbo_cc_test(
  NAME
    cordz_info_statistics_test
  SRCS
    "internal/cordz_info_statistics_test.cc"
  COPTS
    ${TURBO_TEST_COPTS}
  DEPS
    turbo::turbo
    turbo::sync
    GTest::gmock_main
)

turbo_cc_test(
  NAME
    cordz_sample_token_test
  SRCS
    "internal/cordz_sample_token_test.cc"
  COPTS
    ${TURBO_TEST_COPTS}
  DEPS
    turbo::turbo
    turbo::sync
    GTest::gmock_main
)


turbo_cc_test(
  NAME
    cordz_update_scope_test
  SRCS
    "internal/cordz_update_scope_test.cc"
  COPTS
    ${TURBO_TEST_COPTS}
  DEPS
    turbo::turbo
    GTest::gmock_main
)


turbo_cc_test(
  NAME
    cord_test
  SRCS
    "cord_test.cc"
  COPTS
    ${TURBO_TEST_COPTS}
  DEPS
    turbo::turbo
    GTest::gmock_main
)

turbo_cc_test(
  NAME
    cord_data_edge_test
  SRCS
    "internal/cord_data_edge_test.cc"
  COPTS
    ${TURBO_TEST_COPTS}
  DEPS
    turbo::turbo
    GTest::gmock_main
)

turbo_cc_test(
  NAME
    cord_rep_btree_test
  SRCS
    "internal/cord_rep_btree_test.cc"
  COPTS
    ${TURBO_TEST_COPTS}
  DEPS
    turbo::turbo
    GTest::gmock_main
)

turbo_cc_test(
  NAME
    cord_rep_btree_navigator_test
  SRCS
    "internal/cord_rep_btree_navigator_test.cc"
  COPTS
    ${TURBO_TEST_COPTS}
  DEPS
    turbo::turbo
    GTest::gmock_main
)

turbo_cc_test(
  NAME
    cord_rep_btree_reader_test
  SRCS
    "internal/cord_rep_btree_reader_test.cc"
  COPTS
    ${TURBO_TEST_COPTS}
  DEPS
    turbo::turbo
    GTest::gmock_main
)

turbo_cc_test(
  NAME
    cord_rep_crc_test
  SRCS
    "internal/cord_rep_crc_test.cc"
  COPTS
    ${TURBO_TEST_COPTS}
  DEPS
    turbo::turbo
    GTest::gmock_main
)

turbo_cc_test(
  NAME
    cord_ring_test
  SRCS
    "cord_ring_test.cc"
  COPTS
    ${TURBO_TEST_COPTS}
  DEPS
    turbo::turbo
    GTest::gmock_main
)

turbo_cc_test(
  NAME
    cord_ring_reader_test
  SRCS
    "cord_ring_reader_test.cc"
  COPTS
    ${TURBO_TEST_COPTS}
  DEPS
    turbo::turbo
    GTest::gmock_main
)

turbo_cc_test(
  NAME
    cordz_test
  SRCS
    "cordz_test.cc"
  COPTS
    ${TURBO_TEST_COPTS}
  DEPS
    turbo::turbo
    GTest::gmock_main
)

