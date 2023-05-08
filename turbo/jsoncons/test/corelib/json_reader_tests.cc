// Copyright 2013-2023 Daniel Parker
// Distributed under Boost license

#include "turbo/jsoncons/json.h"
#include "turbo/jsoncons/json_encoder.h"
#include "turbo/jsoncons/json_reader.h"
#include "common/FreeListAllocator.h"
#include "catch/catch.hpp"
#include <sstream>
#include <vector>
#include <utility>
#include <ctime>

using namespace turbo;

void test_json_reader_error(const std::string& text, const std::error_code& ec)
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

void test_json_reader_ec(const std::string& text, const std::error_code& expected)
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

#if !(defined(__GNUC__))
// gcc 4.8 basic_string doesn't satisfy C++11 allocator requirements
TEST_CASE("json_reader constructors")
{
    std::string input = R"(
{
  "store": {
    "book": [
      {
        "category": "reference",
        "author": "Margaret Weis",
        "title": "Dragonlance Series",
        "price": 31.96
      },
      {
        "category": "reference",
        "author": "Brent Weeks",
        "title": "Night Angel Trilogy",
        "price": 14.70
      }
    ]
  }
}
)";

    SECTION("stateful allocator")
    {
        using my_json = basic_json<char,sorted_policy,FreeListAllocator<char>>;

        FreeListAllocator<char> my_allocator{1}; 

        json_decoder<my_json,FreeListAllocator<char>> decoder(result_allocator_arg, my_allocator,
                                                              my_allocator);
        basic_json_reader<char,string_source<char>,FreeListAllocator<char>> reader(input, decoder, my_allocator);
        reader.read();

        my_json j = decoder.get_result();
        //std::cout << pretty_print(j) << "\n";
    }
}

TEST_CASE("test_missing_separator")
{
    std::string jtext = R"({"field1"{}})";    

    test_json_reader_error(jtext, turbo::json_errc::expected_colon);
    test_json_reader_ec(jtext, turbo::json_errc::expected_colon);
}

TEST_CASE("test_read_invalid_value")
{
    std::string jtext = R"({"field1":ru})";    

    test_json_reader_error(jtext,turbo::json_errc::expected_value);
    test_json_reader_ec(jtext, turbo::json_errc::expected_value);
}

TEST_CASE("test_read_unexpected_end_of_file")
{
    std::string jtext = R"({"field1":{})";    

    test_json_reader_error(jtext, turbo::json_errc::unexpected_eof);
    test_json_reader_ec(jtext, turbo::json_errc::unexpected_eof);
}


TEST_CASE("test_read_value_not_found")
{
    std::string jtext = R"({"name":})";    

    test_json_reader_error(jtext, turbo::json_errc::expected_value);
    test_json_reader_ec(jtext, turbo::json_errc::expected_value);
}

TEST_CASE("test_read_escaped_characters")
{
    std::string input("[\"\\n\\b\\f\\r\\t\"]");
    std::string expected("\n\b\f\r\t");

    json o = json::parse(input);
    CHECK(expected == o[0].as<std::string>());
}


TEST_CASE("test_read_expected_colon")
{
    test_json_reader_error("{\"name\" 10}", turbo::json_errc::expected_colon);
    test_json_reader_error("{\"name\" true}", turbo::json_errc::expected_colon);
    test_json_reader_error("{\"name\" false}", turbo::json_errc::expected_colon);
    test_json_reader_error("{\"name\" null}", turbo::json_errc::expected_colon);
    test_json_reader_error("{\"name\" \"value\"}", turbo::json_errc::expected_colon);
    test_json_reader_error("{\"name\" {}}", turbo::json_errc::expected_colon);
    test_json_reader_error("{\"name\" []}", turbo::json_errc::expected_colon);
}

TEST_CASE("test_read_expected_key")
{
    test_json_reader_error("{10}", turbo::json_errc::expected_key);
    test_json_reader_error("{true}", turbo::json_errc::expected_key);
    test_json_reader_error("{false}", turbo::json_errc::expected_key);
    test_json_reader_error("{null}", turbo::json_errc::expected_key);
    test_json_reader_error("{{}}", turbo::json_errc::expected_key);
    test_json_reader_error("{[]}", turbo::json_errc::expected_key);
}

TEST_CASE("test_read_expected_value")
{
    test_json_reader_error("[tru]", turbo::json_errc::invalid_value);
    test_json_reader_error("[fa]", turbo::json_errc::invalid_value);
    test_json_reader_error("[n]", turbo::json_errc::invalid_value);
}

TEST_CASE("test_read_primitive_pass")
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

TEST_CASE("test_read_empty_structures")
{
    json val;
    CHECK_NOTHROW((val=json::parse("{}")));
    CHECK_NOTHROW((val=json::parse("[]")));
    CHECK_NOTHROW((val=json::parse("{\"object\":{},\"array\":[]}")));
    CHECK_NOTHROW((val=json::parse("[[],{}]")));
}

TEST_CASE("test_read_primitive_fail")
{
    test_json_reader_error("null {}", turbo::json_errc::extra_character);
    test_json_reader_error("n ", turbo::json_errc::invalid_value);
    test_json_reader_error("nu ", turbo::json_errc::invalid_value);
    test_json_reader_error("nul ", turbo::json_errc::invalid_value);
    test_json_reader_error("false {}", turbo::json_errc::extra_character);
    test_json_reader_error("fals ", turbo::json_errc::invalid_value);
    test_json_reader_error("true []", turbo::json_errc::extra_character);
    test_json_reader_error("tru ", turbo::json_errc::invalid_value);
    test_json_reader_error("10 {}", turbo::json_errc::extra_character);
    test_json_reader_error("1a ", turbo::json_errc::invalid_number);
    test_json_reader_error("1.999 []", turbo::json_errc::extra_character);
    test_json_reader_error("1e0-1", turbo::json_errc::invalid_number);
    test_json_reader_error("\"string\"{}", turbo::json_errc::extra_character);
    test_json_reader_error("\"string\"[]", turbo::json_errc::extra_character);
}
#endif

TEST_CASE("test_read_multiple")
{
    std::string in="{\"a\":1,\"b\":2,\"c\":3}{\"a\":4,\"b\":5,\"c\":6}";
    //std::cout << in << std::endl;

    std::istringstream is(in);

    turbo::json_decoder<json> decoder;
    json_stream_reader reader(is,decoder);

    REQUIRE_FALSE(reader.eof());
    reader.read_next();
    json val = decoder.get_result();
    CHECK(1 == val["a"].as<int>());
    REQUIRE_FALSE(reader.eof());
    reader.read_next();
    json val2 = decoder.get_result();
    CHECK(4 == val2["a"].as<int>());
    CHECK(reader.eof());
}

TEST_CASE("json_reader read from string test")
{
    std::string s = R"(
{
  "store": {
    "book": [
      {
        "category": "reference",
        "author": "Margaret Weis",
        "title": "Dragonlance Series",
        "price": 31.96
      },
      {
        "category": "reference",
        "author": "Brent Weeks",
        "title": "Night Angel Trilogy",
        "price": 14.70
      }
    ]
  }
}
)";

    json_decoder<json> decoder;
    json_string_reader reader(s, decoder);
    reader.read();
    json j = decoder.get_result();

    REQUIRE(j.is_object());
    REQUIRE(j.size() == 1);
    REQUIRE(j[0].is_object());
    REQUIRE(j[0].size() == 1);
    REQUIRE(j[0][0].is_array());
    REQUIRE(j[0][0].size() == 2);
    CHECK(j[0][0][0]["category"].as<std::string>() == std::string("reference"));
    CHECK(j[0][0][1]["author"].as<std::string>() == std::string("Brent Weeks"));
}

TEST_CASE("json_reader json lines")
{
    SECTION("json lines")
    {
        std::string data = R"(
    ["Name", "Session", "Score", "Completed"]
    ["Gilbert", "2013", 24, true]
    ["Alexa", "2013", 29, true]
    ["May", "2012B", 14, false]
    ["Deloise", "2012A", 19, true] 
        )";

        std::stringstream is(data);
        json_decoder<json> decoder;
        json_stream_reader reader(is, decoder);

        CHECK(!reader.eof());
        reader.read_next();
        CHECK(!reader.eof());
        reader.read_next();
        CHECK(!reader.eof());
        reader.read_next();
        CHECK(!reader.eof());
        reader.read_next();
        CHECK(!reader.eof());
        reader.read_next();
        CHECK(!reader.eof());
        reader.read_next();
        CHECK(reader.eof());
    }
}
