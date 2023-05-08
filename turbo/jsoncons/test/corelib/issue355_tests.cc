// Copyright 2013-2023 Daniel Parker
// Distributed under Boost license

#include "turbo/jsoncons/json.h"
#include "turbo/jsoncons/json_encoder.h"
#include "catch/catch.hpp"
#include <sstream>
#include <vector>
#include <utility>
#include <ctime>
#include <list>

using namespace turbo;

TEST_CASE("issue 355 test")
{
    turbo::json someObject;
    turbo::json::array someArray(4);
    someArray[0] = 0;
    someArray[1] = 1;
    someArray[2] = 2;
    someArray[3] = 3;

    // This will end-up making a copy the array, which I did not expect.
    someObject.insert_or_assign("someKey", std::move(someArray));
}

