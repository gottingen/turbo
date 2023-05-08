// Copyright 2013-2023 Daniel Parker
// Distributed under Boost license

#include "turbo/jsoncons/json.h"
#include "turbo/jsoncons/detail/string_wrapper.h"
#include "catch/catch.hpp"
#include <sstream>
#include <vector>
#include <utility>
#include <ctime>

using namespace turbo;

TEST_CASE("test string_wrapper char")
{
    std::string input = "Hello World";
    turbo::detail::string_wrapper<char, std::allocator<char>> s(input.data(), input.size(), std::allocator<char>());

    CHECK(input == std::string(s.c_str()));
    CHECK(s.length() == 11);
}

TEST_CASE("test string_wrapper wchar_t")
{
    std::wstring input = L"Hello World";
    turbo::detail::string_wrapper<wchar_t, std::allocator<wchar_t>> s(input.data(), input.size(), std::allocator<wchar_t>());

    CHECK(input == std::wstring(s.c_str()));
    CHECK(s.length() == 11);
}

TEST_CASE("test tagged_string_wrapper char")
{
    std::string input = "Hello World";
    turbo::detail::tagged_string_wrapper<char, std::allocator<char>> s(input.data(), input.size(), 100, std::allocator<char>());

    CHECK(input == std::string(s.c_str()));
    CHECK(s.tag() == 100);
    CHECK(s.length() == 11);
}

TEST_CASE("test tagged_string_wrapper wchar_t")
{
    std::wstring input = L"Hello World";
    turbo::detail::tagged_string_wrapper<wchar_t, std::allocator<wchar_t>> s(input.data(), input.size(), 100, std::allocator<wchar_t>());

    CHECK(input == std::wstring(s.c_str()));
    CHECK(s.tag() == 100);
    CHECK(s.length() == 11);
}

