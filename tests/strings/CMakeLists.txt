#
# Copyright (C) 2024 EA group inc.
# Author: Jeff.li lijippy@163.com
# All rights reserved.
# This program is free software: you can redistribute it and/or modify
# it under the terms of the GNU Affero General Public License as published
# by the Free Software Foundation, either version 3 of the License, or
# (at your option) any later version.
#
# This program is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
# GNU Affero General Public License for more details.
#
# You should have received a copy of the GNU Affero General Public License
# along with this program.  If not, see <https://www.gnu.org/licenses/>.
#


set(STRINGS_TEST_SRC
        arg_test
        ascii_test
        #atod_manual_test
        bind_test
        char_formatting_test
        charconv_bigint_test
        charconv_parse_test
        charconv_test
        charset_test
        checker_test
        convert_test
        cord_buffer_test
        cord_data_edge_test
        cord_rep_btree_navigator_test
        cord_rep_btree_reader_test
        cord_rep_btree_test
        cord_rep_crc_test
        cord_test
        cordz_functions_test
        cordz_handle_test
        cordz_info_statistics_test
        cordz_info_test
        cordz_sample_token_test
        cordz_test
        cordz_update_scope_test
        cordz_update_tracker_test
        damerau_levenshtein_distance_test
        escaping_test
        extension_test
        has_stringify_test
        has_ostream_operator_test
        match_test
        memutil_test
        numbers_test
        ostringstream_test
        output_test
        parser_test
        pow10_helper_test
        resize_uninitialized_test
        str_cat_test
        str_format_test
        str_join_test
        str_replace_test
        str_split_test
        string_constant_test
        string_view_test
        strip_test
        substitute_test
        uri_test
        utf8_test
)
foreach (test ${STRINGS_TEST_SRC})

    carbin_cc_test(
            NAME ${test}
            MODULE strings
            SOURCES ${test}.cc
            LINKS
            turbo::turbo
            time_test_util_static
            GTest::gtest
            GTest::gmock
            GTest::gmock_main
            ${CARBIN_DEPS_LINK}
            CXXOPTS ${CARBIN_CXX_OPTIONS}
    )
endforeach ()