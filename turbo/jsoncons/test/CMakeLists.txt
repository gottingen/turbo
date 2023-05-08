#
# Copyright 2023 The Turbo Authors.
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

set(unit_tests_src
        bson/bson_cursor_tests.cc
        bson/bson_encoder_tests.cc
        bson/bson_reader_tests.cc
        bson/bson_decimal128_tests.cc
        bson/bson_oid_tests.cc
        bson/bson_test_suite.cc
        bson/encode_decode_bson_tests.cc
        cbor/cbor_bitset_traits_tests.cc
        cbor/cbor_cursor_tests.cc
        cbor/cbor_event_reader_tests.cc
        cbor/cbor_encoder_tests.cc
        cbor/cbor_event_visitor_tests.cc
        cbor/cbor_reader_tests.cc
        cbor/cbor_tests.cc
        cbor/cbor_typed_array_tests.cc
        cbor/decode_cbor_tests.cc
        cbor/encode_cbor_tests.cc
        csv/csv_cursor_tests.cc
        csv/csv_subfield_tests.cc
        csv/csv_tests.cc
        csv/encode_decode_csv_tests.cc
        fuzz_regression/fuzz_regression_tests.cc
        jmespath/jmespath_expression_tests.cc
        jmespath/jmespath_tests.cc
        mergepatch/mergepatch_test_suite.cc
        jsonpatch/jsonpatch_test_suite.cc
        jsonpatch/jsonpatch_tests.cc
        jsonpath/jsonpath_flatten_tests.cc
        jsonpath/jsonpath_custom_function_tests.cc
        jsonpath/jsonpath_json_query_tests.cc
        jsonpath/jsonpath_json_replace_tests.cc
        jsonpath/jsonpath_test_suite.cc
        jsonpath/jsonpath_stateful_allocator_tests.cc
        jsonpointer/jsonpointer_flatten_tests.cc
        jsonpointer/jsonpointer_tests.cc
        jsonschema/format_validator_tests.cc
        jsonschema/jsonschema_defaults_tests.cc
        jsonschema/jsonschema_output_format_tests.cc
        jsonschema/jsonschema_tests.cc
        jsonschema/schema_version_tests.cc
        msgpack/decode_msgpack_tests.cc
        msgpack/encode_msgpack_tests.cc
        msgpack/msgpack_bitset_traits_tests.cc
        msgpack/msgpack_cursor_tests.cc
        msgpack/msgpack_event_reader_tests.cc
        msgpack/msgpack_encoder_tests.cc
        msgpack/msgpack_tests.cc
        msgpack/msgpack_timestamp_tests.cc
        corelib/bigint_tests.cc
        corelib/source_adaptor_tests.cc
        corelib/byte_string_tests.cc
        corelib/value_converter_tests.cc
        corelib/decode_traits_tests.cc
        corelib/detail/optional_tests.cc
        corelib/detail/span_tests.cc
        corelib/detail/string_view_tests.cc
        corelib/detail/string_wrapper_tests.cc
        corelib/detail/to_integer_tests.cc
        corelib/double_round_trip_tests.cc
        corelib/double_to_string_tests.cc
        corelib/dtoa_tests.cc
        corelib/encode_decode_json_tests.cc
        corelib/encode_traits_tests.cc
        corelib/error_recovery_tests.cc
        corelib/json_array_tests.cc
        corelib/json_as_tests.cc
        corelib/json_bitset_traits_tests.cc
        corelib/json_checker_tests.cc
        corelib/json_comparator_tests.cc
        corelib/json_const_pointer_tests.cc
        corelib/json_constructor_tests.cc
        corelib/json_cursor_tests.cc
        corelib/json_encoder_tests.cc
        corelib/json_exception_tests.cc
        corelib/json_filter_tests.cc
        corelib/json_in_place_update_tests.cc
        corelib/json_integer_tests.cc
        corelib/json_less_tests.cc
        corelib/json_line_split_tests.cc
        corelib/json_literal_operator_tests.cc
        corelib/json_object_tests.cc
        corelib/json_options_tests.cc
        corelib/json_parse_error_tests.cc
        corelib/json_parser_position_tests.cc
        corelib/json_parser_tests.cc
        corelib/json_proxy_tests.cc
        corelib/json_push_back_tests.cc
        corelib/json_reader_exception_tests.cc
        corelib/json_reader_tests.cc
        corelib/json_storage_tests.cc
        corelib/json_swap_tests.cc
        corelib/json_traits_macro_functional_tests.cc
        corelib/json_traits_macro_tests.cc
        corelib/json_traits_macro_limit_tests.cc
        corelib/json_traits_name_macro_tests.cc
        corelib/json_type_traits_chrono_tests.cc
        corelib/json_type_traits_container_tests.cc
        corelib/json_type_traits_tests.cc
        corelib/json_validation_tests.cc
        corelib/jsoncons_tests.cc
        corelib/JSONTestSuite_tests.cc
        corelib/ojson_tests.cc
        corelib/order_preserving_json_object_tests.cc
        corelib/parse_string_tests.cc
        corelib/short_string_tests.cc
        corelib/source_tests.cc
        corelib/staj_iterator_tests.cc
        corelib/stateful_allocator_tests.cc
        corelib/string_to_double_tests.cc
        corelib/unicode_conv_tests.cc
        corelib/wjson_tests.cc
        ubjson/decode_ubjson_tests.cc
        ubjson/encode_ubjson_tests.cc
        ubjson/ubjson_cursor_tests.cc
        ubjson/ubjson_encoder_tests.cc
        corelib/main.cc
)

include_directories(${PROJECT_SOURCE_DIR}/third-party)
include_directories(${PROJECT_SOURCE_DIR}/turbo/jsoncons/test)

file(COPY bson/input DESTINATION ${PROJECT_BINARY_DIR}/turbo/jsoncons/test/bson)

file(COPY corelib/input DESTINATION ${PROJECT_BINARY_DIR}/turbo/jsoncons/test/corelib)
file(COPY corelib/output DESTINATION  ${PROJECT_BINARY_DIR}/turbo/jsoncons/test/corelib)

file(COPY csv/input DESTINATION  ${PROJECT_BINARY_DIR}/turbo/jsoncons/test/csv)
file(COPY fuzz_regression/input DESTINATION ${PROJECT_BINARY_DIR}/turbo/jsoncons/test/fuzz_regression)
file(COPY jmespath/input DESTINATION ${PROJECT_BINARY_DIR}/turbo/jsoncons/test/jmespath)
file(COPY jsonpath/input DESTINATION ${PROJECT_BINARY_DIR}/turbo/jsoncons/test/jsonpath)
file(COPY jsonpatch/input DESTINATION ${PROJECT_BINARY_DIR}/turbo/jsoncons/test/jsonpatch)
file(COPY jsonschema/input DESTINATION ${PROJECT_BINARY_DIR}/turbo/jsoncons/test/jsonschema)
file(COPY mergepatch/input DESTINATION ${PROJECT_BINARY_DIR}/turbo/jsoncons/test/mergepatch)

turbo_cc_test(
        NAME
        jsoncons_test
        SRCS
        ${unit_tests_src}
        COPTS
        ${TURBO_TEST_COPTS}
        DEPS
        turbo::turbo
        GTest::gmock_main
)

