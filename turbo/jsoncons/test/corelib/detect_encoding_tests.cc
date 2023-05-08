// Copyright 2013-2023 Daniel Parker
// Distributed under Boost license

#include "turbo/jsoncons/json.h"
#include "turbo/jsoncons/json_encoder.h"
#include "catch/catch.hpp"
#include <sstream>
#include <vector>
#include <utility>
#include <ctime>

using namespace turbo;

TEST_CASE("Detect json encoding")
{
    SECTION("UTF16LE with lead surrogate")
    {
        std::vector<uint8_t> v = {'\"',0x00,0xD8,0x00,0xDB,0xFF,'\"',0x00};
        auto r = turbo::unicode_traits::detect_json_encoding(v.data(),v.size());
        CHECK(r.encoding == turbo::unicode_traits::encoding_kind::utf16le);
        CHECK(r.ptr == v.data());
    }
    SECTION("UTF16BE with lead surrogate")
    {
        std::vector<uint8_t> v = {0x00,'\"',0x00,0xD8,0xFF,0xDB,0x00,'\"'};
        auto r = turbo::unicode_traits::detect_json_encoding(v.data(),v.size());
        CHECK(r.encoding == turbo::unicode_traits::encoding_kind::utf16be);
        CHECK(r.ptr == v.data());
    }
}

TEST_CASE("Detect encoding from bom")
{
    SECTION("detect utf8")
    {
        std::string input = "\xEF\xBB\xBF[1,2,3]";
        auto r = turbo::unicode_traits::detect_encoding_from_bom(input.data(),input.size());
        REQUIRE(r.encoding == turbo::unicode_traits::encoding_kind::utf8);
        CHECK(r.ptr == (input.data()+3));
    }
}
