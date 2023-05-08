// Copyright 2013-2023 Daniel Parker
// Distributed under Boost license

#include "turbo/jsoncons/json.h"
#include "turbo/jsoncons/json_encoder.h"
#include "turbo/jsoncons/json_filter.h"
#include "catch/catch.hpp"
#include <sstream>
#include <vector>
#include <utility>
#include <ctime>
#include <new>

using namespace turbo;

TEST_CASE("test_small_string")
{
    json s("ABCD");
    CHECK(s.storage_kind() == turbo::json_storage_kind::short_string_value);
    CHECK(s.as<std::string>() == std::string("ABCD"));

    json t(s);
    CHECK(t.storage_kind() == turbo::json_storage_kind::short_string_value);
    CHECK(t.as<std::string>() == std::string("ABCD"));

    json q;
    q = s;
    CHECK(q.storage_kind() == turbo::json_storage_kind::short_string_value);
    CHECK(q.as<std::string>() == std::string("ABCD"));
}


