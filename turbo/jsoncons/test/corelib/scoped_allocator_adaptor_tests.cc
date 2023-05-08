// Copyright 2013-2023 Daniel Parker
// Distributed under Boost license

#include "turbo/jsoncons/json.h"
#include "turbo/jsoncons/json_encoder.h"
#include "common/FreeListAllocator.h"
#include <sstream>
#include <vector>
#include <utility>
#include <ctime>
#include <cstddef>
#include "catch/catch.hpp"
#include <scoped_allocator>

#if defined(JSONCONS_HAS_STATEFUL_ALLOCATOR)

template<typename T>
using ScopedTestAllocator = std::scoped_allocator_adaptor<FreeListAllocator<T>>;

using xjson = turbo::basic_json<char, turbo::sorted_policy, ScopedTestAllocator<char>>;


TEST_CASE("scoped_allocator_adaptor tests")
{
    ScopedTestAllocator<char> alloc(true);

    SECTION("construct")
    {
        //xjson(1, turbo::semantic_tag::none);
    }
}

#endif

