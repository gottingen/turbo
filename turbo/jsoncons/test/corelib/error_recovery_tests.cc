
// Copyright 2013-2023 Daniel Parker
// Distributed under Boost license

#include "turbo/jsoncons/json.h"
#include "turbo/jsoncons/json_encoder.h"
#include "turbo/jsoncons/json_reader.h"
#include <sstream>
#include <vector>
#include <utility>
#include <ctime>
#include "catch/catch.hpp"

using namespace turbo;

class relaxed_error_handler
{
public:

    bool operator()(const std::error_code& ec,
                    const ser_context&) noexcept 
    {
        if (ec == turbo::json_errc::extra_comma)
        {
            return true;
        }
        return false;
    }
};

TEST_CASE("test_array_extra_comma")
{
    relaxed_error_handler err_handler;

    json expected = json::parse("[1,2,3]");
    json val = json::parse("[1,2,3,]", err_handler);

    CHECK(val == expected);
}

TEST_CASE("test_object_extra_comma")
{
    relaxed_error_handler err_handler;

    json expected = json::parse(R"(
    {
        "first" : 1,
        "second" : 2
    }
    )", 
    err_handler);

    json val = json::parse(R"(
    {
        "first" : 1,
        "second" : 2,
    }
    )", 
    err_handler);

    CHECK(val == expected);
}

TEST_CASE("test_name_without_quotes")
{
    //relaxed_error_handler err_handler;

    /*json val = json::parse(R"(
    {
        first : 1,
        second : 2
    }
    )", 
    err_handler);

    std::cout << val << std::endl;*/
}


