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

    using turbo::tests::helpers::transcode_utf32_to_utf8_test_base;

    constexpr int trials = 1000;
}

TEST_CASE("convert_pure_ASCII") {
    size_t counter = 0;
    auto generator = [&counter]() -> uint32_t {
        return counter++ & 0x7f;
    };

    auto procedure = [](const char32_t *utf32, size_t size, char *utf8) -> size_t {
        return turbo::convert_utf32_to_utf8(utf32, size, utf8);
    };
    auto size_procedure = [](const char32_t *utf32, size_t size) -> size_t {
        return turbo::utf8_length_from_utf32(utf32, size);
    };
    std::array<size_t, 4> input_size{7, 16, 24, 67};
    for (size_t size: input_size) {
        transcode_utf32_to_utf8_test_base test(generator, size);
        REQUIRE(test(procedure));
        REQUIRE(test.check_size(size_procedure));
    }
}

TEST_CASE("convert_into_1_or_2_UTF8_bytes") {
    for (size_t trial = 0; trial < trials; trial++) {
        if ((trial % 100) == 0) {
            std::cout << ".";
            std::cout.flush();
        }
        turbo::FixedUniform<int> random(0x0000, 0x07ff); // range for 1 or 2 UTF-8 bytes

        auto procedure = [](const char32_t *utf32, size_t size, char *utf8) -> size_t {
            return turbo::convert_utf32_to_utf8(utf32, size, utf8);
        };
        auto size_procedure = [](const char32_t *utf32, size_t size) -> size_t {
            return turbo::utf8_length_from_utf32(utf32, size);
        };
        for (size_t size: input_size) {
            transcode_utf32_to_utf8_test_base test([&random]() { return random(); }, size);
            REQUIRE(test(procedure));
            REQUIRE(test.check_size(size_procedure));
        }
    }
}

TEST_CASE("convert_into_1_or_2_or_3_UTF8_bytes") {
    for (size_t trial = 0; trial < trials; trial++) {
        if ((trial % 100) == 0) {
            std::cout << ".";
            std::cout.flush();
        }
        // range for 1, 2 or 3 UTF-8 bytes
        turbo::FixedUniformRanges<uint32_t, uint64_t> random({{0x0000, 0x007f},
                                                              {0x0080, 0x07ff},
                                                              {0x0800, 0xd7ff},
                                                              {0xe000, 0xffff}});

        auto procedure = [](const char32_t *utf32, size_t size, char *utf8) -> size_t {
            return turbo::convert_utf32_to_utf8(utf32, size, utf8);
        };
        auto size_procedure = [](const char32_t *utf32, size_t size) -> size_t {
            return turbo::utf8_length_from_utf32(utf32, size);
        };
        for (size_t size: input_size) {
            transcode_utf32_to_utf8_test_base test([&random]() { return random(); }, size);
            REQUIRE(test(procedure));
            REQUIRE(test.check_size(size_procedure));
        }
    }
}

TEST_CASE("convert_into_3_or_4_UTF8_bytes") {
    for (size_t trial = 0; trial < trials; trial++) {
        if ((trial % 100) == 0) {
            std::cout << ".";
            std::cout.flush();
        }
        // range for 3 or 4 UTF-8 bytes
        turbo::FixedUniformRanges<uint32_t, uint64_t> random({{0x0800, 0xd800 - 1},
                                                              {0xe000, 0x10ffff}});

        auto procedure = [](const char32_t *utf32, size_t size, char *utf8) -> size_t {
            return turbo::convert_utf32_to_utf8(utf32, size, utf8);
        };
        auto size_procedure = [](const char32_t *utf32, size_t size) -> size_t {
            return turbo::utf8_length_from_utf32(utf32, size);
        };
        for (size_t size: input_size) {
            transcode_utf32_to_utf8_test_base test([&random]() { return random(); }, size);
            REQUIRE(test(procedure));
            REQUIRE(test.check_size(size_procedure));
        }
    }
}

TEST_CASE("convert_fails_if_there_is_surrogate") {
    auto procedure = [](const char32_t *utf32, size_t size, char *utf8) -> size_t {
        return turbo::convert_utf32_to_utf8(utf32, size, utf8);
    };
    const size_t size = 64;
    transcode_utf32_to_utf8_test_base test([]() { return '*'; }, size + 32);

    for (char32_t surrogate = 0xd800; surrogate <= 0xdfff; surrogate++) {
        for (size_t i = 0; i < size; i++) {
            const auto old = test.input_utf32[i];
            test.input_utf32[i] = surrogate;
            REQUIRE(test(procedure));
            test.input_utf32[i] = old;
        }
    }
}

TEST_CASE("convert_fails_if_input_too_large") {
    turbo::FixedUniform<uint32_t> generator(0x110000, 0xffffffff);

    auto procedure = [](const char32_t *utf32, size_t size, char *utf8) -> size_t {
        return turbo::convert_utf32_to_utf8(utf32, size, utf8);
    };
    const size_t size = 64;
    transcode_utf32_to_utf8_test_base test([]() { return '*'; }, size + 32);

    for (size_t j = 0; j < 1000; j++) {
        uint32_t wrong_value = generator();
        for (size_t i = 0; i < size; i++) {
            auto old = test.input_utf32[i];
            test.input_utf32[i] = wrong_value;
            REQUIRE(test(procedure));
            test.input_utf32[i] = old;
        }
    }
}

