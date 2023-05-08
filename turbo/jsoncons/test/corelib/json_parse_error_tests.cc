// Copyright 2013-2023 Daniel Parker
// Distributed under Boost license

#include "turbo/jsoncons/json.h"
#include "turbo/jsoncons/json_encoder.h"
#include "turbo/jsoncons/json_reader.h"
#include "catch/catch.hpp"
#include <sstream>
#include <vector>
#include <utility>
#include <ctime>

using namespace turbo;

void test_parse_error(const std::string& text, const std::error_code& ec)
{
    REQUIRE_THROWS(json::parse(text));
    JSONCONS_TRY
    {
        json::parse(text);        
    }
    JSONCONS_CATCH (const ser_error& e)
    {
        if (e.code() != ec)
        {
            std::cout << text << std::endl;
            std::cout << e.code().value() << " " << e.what() << std::endl; 
        }
        CHECK(ec == e.code());
    }
}

void test_parse_ec(const std::string& text, const std::error_code& expected)
{
    std::error_code ec;

    std::istringstream is(text);
    json_decoder<json> decoder;
    json_stream_reader reader(is,decoder);

    reader.read(ec);
    //std::cerr << text << std::endl;
    //std::cerr << ec.message() 
    //          << " at line " << reader.line() 
    //          << " and column " << reader.column() << std::endl;

    CHECK(ec);
    CHECK(ec == expected);
}

TEST_CASE("test_parse_missing_separator")
{
    std::string jtext = R"({"field1"{}})";    

    test_parse_error(jtext, turbo::json_errc::expected_colon);
    test_parse_ec(jtext, turbo::json_errc::expected_colon);
}

TEST_CASE("test_invalid_value")
{
    std::string jtext = R"({"field1":ru})";    

    test_parse_error(jtext,turbo::json_errc::expected_value);
    test_parse_ec(jtext, turbo::json_errc::expected_value);
}


TEST_CASE("test_unexpected_end_of_file")
{
    std::string jtext = R"({"field1":{})";    

    test_parse_error(jtext, turbo::json_errc::unexpected_eof);
    test_parse_ec(jtext, turbo::json_errc::unexpected_eof);
}

TEST_CASE("test_value_not_found")
{
    std::string jtext = R"({"name":})";    

    test_parse_error(jtext, turbo::json_errc::expected_value);
    test_parse_ec(jtext, turbo::json_errc::expected_value);
}

TEST_CASE("test_escaped_characters")
{
    std::string input("[\"\\n\\b\\f\\r\\t\"]");
    std::string expected("\n\b\f\r\t");

    json o = json::parse(input);
    CHECK(expected == o[0].as<std::string>());
}


TEST_CASE("test_expected_colon")
{
    test_parse_error("{\"name\" 10}", turbo::json_errc::expected_colon);
    test_parse_error("{\"name\" true}", turbo::json_errc::expected_colon);
    test_parse_error("{\"name\" false}", turbo::json_errc::expected_colon);
    test_parse_error("{\"name\" null}", turbo::json_errc::expected_colon);
    test_parse_error("{\"name\" \"value\"}", turbo::json_errc::expected_colon);
    test_parse_error("{\"name\" {}}", turbo::json_errc::expected_colon);
    test_parse_error("{\"name\" []}", turbo::json_errc::expected_colon);
}

TEST_CASE("test_expected_key")
{
    test_parse_error("{10}", turbo::json_errc::expected_key);
    test_parse_error("{true}", turbo::json_errc::expected_key);
    test_parse_error("{false}", turbo::json_errc::expected_key);
    test_parse_error("{null}", turbo::json_errc::expected_key);
    test_parse_error("{{}}", turbo::json_errc::expected_key);
    test_parse_error("{[]}", turbo::json_errc::expected_key);
}

TEST_CASE("test_expected_value")
{
    test_parse_error("[tru]", turbo::json_errc::invalid_value);
    test_parse_error("[fa]", turbo::json_errc::invalid_value);
    test_parse_error("[n]", turbo::json_errc::invalid_value);
}

TEST_CASE("test_parse_primitive_pass")
{
    json val;
    CHECK_NOTHROW((val=json::parse("null")));
    CHECK(val == json::null());
    CHECK_NOTHROW((val=json::parse("false")));
    CHECK(val == json(false));
    CHECK_NOTHROW((val=json::parse("true")));
    CHECK(val == json(true));
    CHECK_NOTHROW((val=json::parse("10")));
    CHECK(val == json(10));
    CHECK_NOTHROW((val=json::parse("1.999")));
    CHECK(val == json(1.999));
    CHECK_NOTHROW((val=json::parse("\"string\"")));
    CHECK(val == json("string"));
}

TEST_CASE("test_parse_empty_structures")
{
    json val;
    CHECK_NOTHROW((val=json::parse("{}")));
    CHECK_NOTHROW((val=json::parse("[]")));
    CHECK_NOTHROW((val=json::parse("{\"object\":{},\"array\":[]}")));
    CHECK_NOTHROW((val=json::parse("[[],{}]")));
}

TEST_CASE("test_parse_primitive_fail")
{
    test_parse_error("null {}", turbo::json_errc::extra_character);
    test_parse_error("n ", turbo::json_errc::invalid_value);
    test_parse_error("nu ", turbo::json_errc::invalid_value);
    test_parse_error("nul ", turbo::json_errc::invalid_value);
    test_parse_error("false {}", turbo::json_errc::extra_character);
    test_parse_error("fals ", turbo::json_errc::invalid_value);
    test_parse_error("true []", turbo::json_errc::extra_character);
    test_parse_error("tru ", turbo::json_errc::invalid_value);
    test_parse_error("10 {}", turbo::json_errc::extra_character);
    test_parse_error("1a ", turbo::json_errc::invalid_number);
    test_parse_error("1.999 []", turbo::json_errc::extra_character);
    test_parse_error("1e0-1", turbo::json_errc::invalid_number);
    test_parse_error("\"string\"{}", turbo::json_errc::extra_character);
    test_parse_error("\"string\"[]", turbo::json_errc::extra_character);
}

TEST_CASE("test_multiple")
{
    std::string in="{\"a\":1,\"b\":2,\"c\":3}{\"a\":4,\"b\":5,\"c\":6}";
    //std::cout << in << std::endl;

    std::istringstream is(in);

    turbo::json_decoder<json> decoder;
    json_stream_reader reader(is,decoder);

    REQUIRE_FALSE(reader.eof());
    reader.read_next();
    CHECK_FALSE(reader.eof());
    json val = decoder.get_result();
    CHECK(1 == val["a"].as<int>());

    REQUIRE_FALSE(reader.eof());
    reader.read_next();
    CHECK(reader.eof());
    json val2 = decoder.get_result();
    CHECK(4 == val2["a"].as<int>());
}

TEST_CASE("test_uinteger_overflow")
{
    uint64_t m = (std::numeric_limits<uint64_t>::max)();
    std::string s1 = std::to_string(m);
    std::string s2 = s1;
    s2.push_back('0');
    
    json j1 =  json::parse(s1);
    CHECK(j1.is_uint64());
    CHECK(m == j1.as<uint64_t>());

    json j2 =  json::parse(s2);
    CHECK_FALSE(j2.is_uint64());
    CHECK(j2.is<turbo::bigint>());
    CHECK(s2 == j2.as<std::string>());
}
TEST_CASE("test_negative_integer_overflow")
{
    int64_t m = (std::numeric_limits<int64_t>::lowest)();
    std::string s1 = std::to_string(m);
    std::string s2 = s1;
    s2.push_back('0');
    
    json j1 =  json::parse(s1);
    CHECK(m == j1.as<int64_t>());

    json j2 =  json::parse(s2);
    CHECK_FALSE(j2.is_int64());
    CHECK(j2.is_bignum());
    CHECK(s2 == j2.as<std::string>());
}

TEST_CASE("test_positive_integer_overflow")
{
    int64_t m = (std::numeric_limits<int64_t>::max)();
    std::string s1 = std::to_string(m);
    std::string s2 = s1;
    s2.push_back('0');

    json j1 =  json::parse(s1);
    CHECK(m == j1.as<int64_t>());

    json j2 =  json::parse(s2);
    CHECK_FALSE(j2.is_int64());
    CHECK(j2.is<turbo::bigint>());
    CHECK(s2 == j2.as<std::string>());
}




