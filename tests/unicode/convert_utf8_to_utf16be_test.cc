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

#include <array>
#include <iostream>
#include "transcode_test_base.h"
#include "turbo/random/random.h"
#include <memory>

namespace {
    std::array<size_t, 7> input_size{7, 16, 12, 64, 67, 128, 256};


    constexpr size_t trials = 10000;
#if TURBO_WITH_AVX2
    static_assert(std::is_same_v<turbo::unicode::default_engine, turbo::unicode::avx2_engine>,
                  "Default engine should be AVX2");
#elif !define(TURBO_PROCESSOR_ARM)
    static_assert(std::is_same_v<turbo::unicode::default_engine, turbo::unicode::scalar_engine>,
                  "Default engine should be scalar");
#endif
}
using turbo::tests::helpers::transcode_utf8_to_utf16_test_base;

TEST_CASE("convert_pure_ASCII") {
    for (size_t trial = 0; trial < trials; trial++) {
        if ((trial % 100) == 0) {
            std::cout << ".";
            std::cout.flush();
        }
        size_t counter = 0;
        auto generator = [&counter]() -> uint32_t {
            return counter++ & 0x7f;
        };

        auto procedure = [](const char *utf8, size_t size, char16_t *utf16le) -> size_t {
            std::vector<char16_t> utf16be(2 * size);  // Assume each UTF-8 byte is converted into two UTF-16 bytes
            size_t len = turbo::convert_utf8_to_utf16be(utf8, size, utf16be.data());
            turbo::change_endianness_utf16(utf16be.data(), len, utf16le);
            return len;
        };
        auto size_procedure = [](const char *utf8, size_t size) -> size_t {
            return turbo::utf16_length_from_utf8(utf8, size);
        };

        for (size_t size: input_size) {
            transcode_utf8_to_utf16_test_base test(generator, size);
            REQUIRE(test(procedure));
            REQUIRE(test.check_size(size_procedure));
        }
    }
}


TEST_CASE("convert_1_or_2_UTF8_bytes") {
    for (size_t trial = 0; trial < trials; trial++) {
        if ((trial % 100) == 0) {
            std::cout << ".";
            std::cout.flush();
        }
        turbo::FixedUniform<int> random(0x0000, 0x07ff); // range for 1 or 2 UTF-8 bytes

        auto procedure = [](const char *utf8, size_t size, char16_t *utf16le) -> size_t {
            std::vector<char16_t> utf16be(2 * size);  // Assume each UTF-8 byte is converted into two UTF-16 bytes
            size_t len = turbo::convert_utf8_to_utf16be(utf8, size, utf16be.data());
            turbo::change_endianness_utf16(utf16be.data(), len, utf16le);
            return len;
        };
        auto size_procedure = [](const char *utf8, size_t size) -> size_t {
            return turbo::utf16_length_from_utf8(utf8, size);
        };
        for (size_t size: input_size) {
            transcode_utf8_to_utf16_test_base test([&random]() { return random(); }, size);
            REQUIRE(test(procedure));
            REQUIRE(test.check_size(size_procedure));
        }
    }
}

TEST_CASE("convert_1_or_2_or_3_UTF8_bytes") {
    for (size_t trial = 0; trial < trials; trial++) {
        if ((trial % 100) == 0) {
            std::cout << ".";
            std::cout.flush();
        }
        // range for 1, 2 or 3 UTF-8 bytes
        turbo::FixedUniformRanges<uint32_t, uint64_t> random({{0x0000, 0xd7ff},
                                                              {0xe000, 0xffff}});

        auto procedure = [](const char *utf8, size_t size, char16_t *utf16le) -> size_t {
            std::vector<char16_t> utf16be(2 * size);  // Assume each UTF-8 byte is converted into two UTF-16 bytes
            size_t len = turbo::convert_utf8_to_utf16be(utf8, size, utf16be.data());
            turbo::change_endianness_utf16(utf16be.data(), len, utf16le);
            return len;
        };
        auto size_procedure = [](const char *utf8, size_t size) -> size_t {
            return turbo::utf16_length_from_utf8(utf8, size);
        };
        for (size_t size: input_size) {
            transcode_utf8_to_utf16_test_base test([&random]() { return random(); }, size);
            REQUIRE(test(procedure));
            REQUIRE(test.check_size(size_procedure));
        }
    }
}

TEST_CASE("convert_3_or_4_UTF8_bytes") {
    for (size_t trial = 0; trial < trials; trial++) {
        if ((trial % 100) == 0) {
            std::cout << ".";
            std::cout.flush();
        }
        turbo::FixedUniformRanges<uint32_t, uint64_t> random({{0x0800, 0xd800 - 1},
                                                              {0xe000, 0x10ffff}}); // range for 3 or 4 UTF-8 bytes

        auto procedure = [](const char *utf8, size_t size, char16_t *utf16le) -> size_t {
            std::vector<char16_t> utf16be(size);
            size_t len = turbo::convert_utf8_to_utf16be(utf8, size, utf16be.data());
            turbo::change_endianness_utf16(utf16be.data(), len, utf16le);
            return len;
        };
        auto size_procedure = [](const char *utf8, size_t size) -> size_t {
            return turbo::utf16_length_from_utf8(utf8, size);
        };
        for (size_t size: input_size) {
            transcode_utf8_to_utf16_test_base test([&random]() { return random(); }, size);
            REQUIRE(test(procedure));
            REQUIRE(test.check_size(size_procedure));
        }
    }
}
