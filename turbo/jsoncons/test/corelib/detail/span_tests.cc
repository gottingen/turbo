// Copyright 2013-2023 Daniel Parker
// Distributed under Boost license

#include "catch/catch.hpp"
#include "turbo/jsoncons/detail/span.h"
#include <iostream>
#include <vector>

TEST_CASE("turbo::detail::span constructor tests")
{
    SECTION("turbo::detail::span()")
    {
        turbo::detail::span<const uint8_t> s;
        CHECK(s.empty());
    }
    SECTION("turbo::detail::span(pointer,size_type)")
    {
        std::vector<uint8_t> v = {1,2,3,4};
        turbo::detail::span<const uint8_t> s(v.data(), v.size());
        CHECK(s.size() == v.size());
        CHECK(s.data() == v.data());
    }
    SECTION("turbo::detail::span(C& c)")
    {
        using C = std::vector<uint8_t>;
        C c = {1,2,3,4};

        turbo::detail::span<const uint8_t> s(c);
        CHECK(s.size() == c.size());
        CHECK(s.data() == c.data());
    }
    SECTION("turbo::detail::span(C c[])")
    {
        double c[] = {1,2,3,4};

        turbo::detail::span<const double> s{ c };
        CHECK(s.size() == 4);
        CHECK(s.data() == c);
    }
    SECTION("turbo::detail::span(std::array)")
    {
        std::array<double,4> c = {1,2,3,4};

        turbo::detail::span<double> s(c);
        CHECK(s.size() == 4);
        CHECK(s.data() == c.data());
    }
}

