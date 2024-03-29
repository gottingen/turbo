// Copyright 2023 The Turbo Authors.
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//      https://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.

#define DOCTEST_CONFIG_IMPLEMENT_WITH_MAIN

#include "turbo/testing/test.h"

#include "turbo/unicode/converter.h"
#include "transcode_test_base.h"

#include <array>
#include <iostream>


#include "turbo/random/random.h"


namespace {
    std::array<size_t, 7> input_size{7, 16, 12, 64, 67, 128, 256};

    using turbo::tests::helpers::transcode_utf32_to_utf16_test_base;

    constexpr int trials = 1000;
}

TEST_CASE("convert_into_2_UTF16_bytes") {
    for (size_t trial = 0; trial < trials; trial++) {
        if ((trial % 100) == 0) {
            std::cout << ".";
            std::cout.flush();
        }
        // range for 2 UTF-16 bytes
        turbo::FixedUniformRanges<uint32_t, uint64_t> random({{0x0000, 0xd7ff},
                                                              {0xe000, 0xffff}});

        auto procedure = [](const char32_t *utf32, size_t size, char16_t *utf16le) -> size_t {
            std::vector<char16_t> utf16be(size);
            turbo::UnicodeResult res = turbo::convert_utf32_to_utf16be_with_errors(utf32, size, utf16be.data());
            REQUIRE_EQ(res.error, turbo::UnicodeError::SUCCESS);
            turbo::change_endianness_utf16(utf16be.data(), res.count, utf16le);
            return res.count;
        };
        auto size_procedure = [](const char32_t *utf32, size_t size) -> size_t {
            return turbo::utf16_length_from_utf32(utf32, size);
        };
        for (size_t size: input_size) {
            transcode_utf32_to_utf16_test_base test([&random]() { return random(); }, size);
            REQUIRE(test(procedure));
            REQUIRE(test.check_size(size_procedure));
        }
    }
}

TEST_CASE("convert_into_4_UTF16_bytes") {
    for (size_t trial = 0; trial < trials; trial++) {
        if ((trial % 100) == 0) {
            std::cout << ".";
            std::cout.flush();
        }
        // range for 4 UTF-16 bytes
        turbo::FixedUniformRanges<uint32_t, uint64_t> random({{0x10000, 0x10ffff}});

        auto procedure = [](const char32_t *utf32, size_t size, char16_t *utf16le) -> size_t {
            std::vector<char16_t> utf16be(2 * size);
            turbo::UnicodeResult res = turbo::convert_utf32_to_utf16be_with_errors(utf32, size, utf16be.data());
            REQUIRE_EQ(res.error, turbo::UnicodeError::SUCCESS);
            turbo::change_endianness_utf16(utf16be.data(), res.count, utf16le);
            return res.count;
        };
        auto size_procedure = [](const char32_t *utf32, size_t size) -> size_t {
            return turbo::utf16_length_from_utf32(utf32, size);
        };
        for (size_t size: input_size) {
            transcode_utf32_to_utf16_test_base test([&random]() { return random(); }, size);
            REQUIRE(test(procedure));
            REQUIRE(test.check_size(size_procedure));
        }
    }
}

TEST_CASE("convert_into_2_or_4_UTF16_bytes") {
    for (size_t trial = 0; trial < trials; trial++) {
        if ((trial % 100) == 0) {
            std::cout << ".";
            std::cout.flush();
        }
        // range for 2 or 4 UTF-16 bytes (all codepoints)
        turbo::FixedUniformRanges<uint32_t, uint64_t> random({{0x0000, 0xd7ff},
                                                              {0xe000, 0xffff},
                                                              {0x10000, 0x10ffff}});

        auto procedure = [](const char32_t *utf32, size_t size, char16_t *utf16le) -> size_t {
            std::vector<char16_t> utf16be(2 * size);
            turbo::UnicodeResult res = turbo::convert_utf32_to_utf16be_with_errors(utf32, size, utf16be.data());
            REQUIRE_EQ(res.error, turbo::UnicodeError::SUCCESS);
            turbo::change_endianness_utf16(utf16be.data(), res.count, utf16le);
            return res.count;
        };
        auto size_procedure = [](const char32_t *utf32, size_t size) -> size_t {
            return turbo::utf16_length_from_utf32(utf32, size);
        };
        for (size_t size: input_size) {
            transcode_utf32_to_utf16_test_base test([&random]() { return random(); }, size);
            REQUIRE(test(procedure));
            REQUIRE(test.check_size(size_procedure));
        }
    }
}

TEST_CASE("convert_fails_if_there_is_surrogate") {
    const size_t size = 64;
    transcode_utf32_to_utf16_test_base test([]() { return '*'; }, size + 32);

    for (char32_t surrogate = 0xd800; surrogate <= 0xdfff; surrogate++) {
        for (size_t i = 0; i < size; i++) {
            auto procedure = [&i](const char32_t *utf32, size_t size, char16_t *utf16le) -> size_t {
                std::vector<char16_t> utf16be(2 * size);
                turbo::UnicodeResult res = turbo::convert_utf32_to_utf16be_with_errors(utf32, size, utf16be.data());
                REQUIRE_EQ(res.error, turbo::UnicodeError::SURROGATE);
                REQUIRE_EQ(res.count, i);
                return 0;
            };
            const auto old = test.input_utf32[i];
            test.input_utf32[i] = surrogate;
            REQUIRE(test(procedure));
            test.input_utf32[i] = old;
        }
    }
}

TEST_CASE("convert_fails_if_input_too_large") {

    turbo::FixedUniform<uint32_t> generator(0x110000, 0xffffffff);

    const size_t size = 64;
    transcode_utf32_to_utf16_test_base test([]() { return '*'; }, size + 32);

    for (size_t j = 0; j < 1000; j++) {
        uint32_t wrong_value = generator();
        for (size_t i = 0; i < size; i++) {
            auto procedure = [&i](const char32_t *utf32, size_t size, char16_t *utf16le) -> size_t {
                std::vector<char16_t> utf16be(2 * size);
                turbo::UnicodeResult res = turbo::convert_utf32_to_utf16be_with_errors(utf32, size, utf16be.data());
                REQUIRE_EQ(res.error, turbo::UnicodeError::TOO_LARGE);
                REQUIRE_EQ(res.count, i);
                return 0;
            };
            auto old = test.input_utf32[i];
            test.input_utf32[i] = wrong_value;
            REQUIRE(test(procedure));
            test.input_utf32[i] = old;
        }
    }
}

