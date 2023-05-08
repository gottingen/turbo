// Copyright 2013-2023 Daniel Parker
// Distributed under Boost license

#include "turbo/jsoncons/json.h"
#include "turbo/jsoncons/json_encoder.h"
#include "catch/catch.hpp"
#include <sstream>
#include <vector>
#include <utility>
#include <ctime>
#include <map>
#include <iterator>

using namespace turbo;

TEST_CASE("json proxy tests")
{
    SECTION("test 1")
    {
        json j(json_object_arg, {{"a",json()},{"b",2}});

        //operator basic_json&()
        json& j1 = j["a"];
        CHECK(j1 == j.at("a"));

        j["a"]["c"] = 3;

        json& j3 = j["a"]["c"];
        CHECK(j3 == j.at("a").at("c"));
    }
    SECTION("Test 2")
    {
        json expected(json_array_arg, { "author","category","price","title" });
    }
    SECTION("Test 3")
    {
        json j(json_object_arg, { {"a",json()},{"b",2} });

        //operator basic_json&()
        json& j1 = j["a"];
        CHECK(j1 == j.at("a"));

        j["a"]["c"] = 3;

        json jv(json_array_arg);
        jv.push_back(j["a"]["c"]);

    }
    SECTION("dump test")
    {
        json j = json::parse(R"(
         {"a" : {}, "b" : 2} 
         )");

        json& j1 = j["b"];
        std::string output;
        j1.dump(output);
        CHECK(output == std::string("2"));
    }
}

