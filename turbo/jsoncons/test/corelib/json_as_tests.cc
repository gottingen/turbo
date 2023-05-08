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

TEST_CASE("json integer as string")
{
    SECTION("0xabcdef")
    {
        turbo::json j("0xabcdef");
        CHECK(j.as<int32_t>() == 11259375);
    }
    SECTION("0x123456789")
    {
        turbo::json j("0x123456789");
        CHECK(j.as<int64_t>() == 4886718345);
    }
    SECTION("0XABCDEF")
    {
        turbo::json j("0XABCDEF");
        CHECK(j.as<uint32_t>() == 11259375u);
    }
    SECTION("0X123456789")
    {
        turbo::json j("0X123456789");
        CHECK(j.as<uint64_t>() == 4886718345);
    }
    SECTION("0x0")
    {
        turbo::json j("0x0");
        CHECK(j.as<int>() == 0);
    }
    SECTION("0777")
    {
        turbo::json j("0777");
        CHECK(j.as<int>() == 511);
    }
    SECTION("0b1001")
    {
        turbo::json j("0b1001");
        CHECK(j.as<int>() == 9);
    }
    SECTION("0B1001")
    {
        turbo::json j("0B1001");
        CHECK(j.as<int>() == 9);
    }
}

TEST_CASE("json::is_object on proxy")
{
    turbo::json root = turbo::json::parse(R"({"key":"value"})");

    CHECK_FALSE(root["key1"].is_object());
}

TEST_CASE("json::as<turbo::string_view>()")
{
    std::string s1("Short");
    turbo::json j1(s1);
    CHECK(j1.as<turbo::string_view>() == turbo::string_view(s1));

    std::string s2("String to long for short string");
    turbo::json j2(s2);
    CHECK(j2.as<turbo::string_view>() == turbo::string_view(s2));
}

TEST_CASE("json::as<turbo::bigint>()")
{
    SECTION("from integer")
    {
        turbo::json j(-1000);
        CHECK(bool(j.as<turbo::bigint>() == turbo::bigint(-1000)));
    }
    SECTION("from unsigned integer")
    {
        turbo::json j(1000u);
        CHECK(bool(j.as<turbo::bigint>() == turbo::bigint(1000u)));
    }
    SECTION("from double")
    {
        turbo::json j(1000.0);
        CHECK(bool(j.as<turbo::bigint>() == turbo::bigint(1000)));
    }
    SECTION("from bigint")
    {
        std::string s = "-18446744073709551617";
        turbo::json j(s,  turbo::semantic_tag::bigint);
        CHECK(bool(j.as<turbo::bigint>() == turbo::bigint::from_string(s)));
    }
}

#if (defined(__GNUC__) || defined(__clang__)) && defined(TURBO_HAVE_INTRINSIC_INT128)
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wpedantic"
TEST_CASE("json::as<__int128>()")
{
    std::string s1 = "-18446744073709551617";

    __int128 n;
    auto result = turbo::detail::to_integer_unchecked(s1.data(),s1.size(), n);
    REQUIRE(result.ec == turbo::detail::to_integer_errc());

    turbo::json j(s1);

    __int128 val = j.as<__int128>();

    std::string s2;
    turbo::detail::from_integer(val, s2);

    std::string s3;
    turbo::detail::from_integer(n, s3);

    CHECK((n == val));
}

TEST_CASE("json::as<unsigned __int128>()")
{
    std::string s1 = "18446744073709551616";

    unsigned __int128 n;

    auto result = turbo::detail::to_integer_unchecked(s1.data(),s1.size(), n);
    REQUIRE(result.ec == turbo::detail::to_integer_errc());

    turbo::json j(s1);

    unsigned __int128 val = j.as<unsigned __int128>();

    std::string s2;
    turbo::detail::from_integer(val, s2);
    std::string s3;
    turbo::detail::from_integer(n, s3);

    CHECK((n == val));
}
#pragma GCC diagnostic pop
#endif

TEST_CASE("as byte string tests")
{
    SECTION("byte_string_arg hint")
    {
        std::vector<uint8_t> v = {'H','e','l','l','o'};
        turbo::json j(turbo::byte_string_arg, v, turbo::semantic_tag::base64);
        turbo::json sj(j.as<std::string>());

        auto u = sj.as<std::vector<uint8_t>>(turbo::byte_string_arg,
                                             turbo::semantic_tag::base64);

        CHECK(u == v);
    }
    SECTION("as std::vector<char>")
    {
        std::vector<char> v = {'H','e','l','l','o'};
        turbo::json j(turbo::byte_string_arg, v, turbo::semantic_tag::base64);

        auto u = j.as<std::vector<char>>();

        CHECK(u == v);
    }
    SECTION("as<std::vector<char>>(turbo::byte_string_arg, ...")
    {
        std::vector<char> v = {'H','e','l','l','o'};
        turbo::json j(turbo::byte_string_arg, v, turbo::semantic_tag::base64);
        turbo::json sj(j.as<std::string>());

        auto u = sj.as<std::vector<char>>(turbo::byte_string_arg,
                                          turbo::semantic_tag::base64);

        CHECK(u == v);
    }
}

