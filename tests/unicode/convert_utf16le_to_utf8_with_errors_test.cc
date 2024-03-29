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

    using turbo::tests::helpers::transcode_utf16_to_utf8_test_base;

    constexpr int trials = 1000;
}

TEST_CASE("convert_pure_ASCII") {
    size_t counter = 0;
    auto generator = [&counter]() -> uint32_t {
        return counter++ & 0x7f;
    };

    auto procedure = [](const char16_t *utf16, size_t size, char *utf8) -> size_t {
        turbo::UnicodeResult res = turbo::convert_utf16le_to_utf8_with_errors(utf16, size, utf8);
        REQUIRE_EQ(res.error, turbo::UnicodeError::SUCCESS);
        return res.count;
    };
    auto size_procedure = [](const char16_t *utf16, size_t size) -> size_t {
        return turbo::utf8_length_from_utf16le(utf16, size);
    };
    std::array<size_t, 1> input_size{16};
    for (size_t size: input_size) {
        transcode_utf16_to_utf8_test_base test(generator, size);
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

        auto procedure = [](const char16_t *utf16, size_t size, char *utf8) -> size_t {
            turbo::UnicodeResult res = turbo::convert_utf16le_to_utf8_with_errors(utf16, size, utf8);
            REQUIRE_EQ(res.error, turbo::UnicodeError::SUCCESS);
            return res.count;
        };
        auto size_procedure = [](const char16_t *utf16, size_t size) -> size_t {
            return turbo::utf8_length_from_utf16le(utf16, size);
        };
        for (size_t size: input_size) {
            transcode_utf16_to_utf8_test_base test([&random]() { return random(); }, size);
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

        auto procedure = [](const char16_t *utf16, size_t size, char *utf8) -> size_t {
            turbo::UnicodeResult res = turbo::convert_utf16le_to_utf8_with_errors(utf16, size, utf8);
            REQUIRE_EQ(res.error, turbo::UnicodeError::SUCCESS);
            return res.count;
        };
        auto size_procedure = [](const char16_t *utf16, size_t size) -> size_t {
            return turbo::utf8_length_from_utf16le(utf16, size);
        };
        for (size_t size: input_size) {
            transcode_utf16_to_utf8_test_base test([&random]() { return random(); }, size);
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

        auto procedure = [](const char16_t *utf16, size_t size, char *utf8) -> size_t {
            turbo::UnicodeResult res = turbo::convert_utf16le_to_utf8_with_errors(utf16, size, utf8);
            REQUIRE_EQ(res.error, turbo::UnicodeError::SUCCESS);
            return res.count;
        };
        auto size_procedure = [](const char16_t *utf16, size_t size) -> size_t {
            return turbo::utf8_length_from_utf16le(utf16, size);
        };
        for (size_t size: input_size) {
            transcode_utf16_to_utf8_test_base test([&random]() { return random(); }, size);
            REQUIRE(test(procedure));
            REQUIRE(test.check_size(size_procedure));
        }
    }
}

#if TURBO_IS_BIG_ENDIAN
// todo: port the next test.
#else

TEST_CASE("convert_fails_if_there_is_sole_low_surrogate") {
    const size_t size = 64;
    transcode_utf16_to_utf8_test_base test([]() { return '*'; }, size + 32);

    for (char16_t low_surrogate = 0xdc00; low_surrogate <= 0xdfff; low_surrogate++) {
        for (size_t i = 0; i < size; i++) {
            auto procedure = [&i](const char16_t *utf16, size_t size, char *utf8) -> size_t {
                turbo::UnicodeResult res = turbo::convert_utf16le_to_utf8_with_errors(utf16, size, utf8);
                REQUIRE_EQ(res.error, turbo::UnicodeError::SURROGATE);
                REQUIRE_EQ(res.count, i);
                return 0;
            };
            const auto old = test.input_utf16[i];
            test.input_utf16[i] = low_surrogate;
            REQUIRE(test(procedure));
            test.input_utf16[i] = old;
        }
    }
}

#endif

#if TURBO_IS_BIG_ENDIAN
// todo: port the next test.
#else

TEST_CASE("convert_fails_if_there_is_sole_high_surrogate") {
    const size_t size = 64;
    transcode_utf16_to_utf8_test_base test([]() { return '*'; }, size + 32);

    for (char16_t high_surrogate = 0xdc00; high_surrogate <= 0xdfff; high_surrogate++) {
        for (size_t i = 0; i < size; i++) {
            auto procedure = [&i](const char16_t *utf16, size_t size, char *utf8) -> size_t {
                turbo::UnicodeResult res = turbo::convert_utf16le_to_utf8_with_errors(utf16, size, utf8);
                REQUIRE_EQ(res.error, turbo::UnicodeError::SURROGATE);
                REQUIRE_EQ(res.count, i);
                return 0;
            };
            const auto old = test.input_utf16[i];
            test.input_utf16[i] = high_surrogate;
            REQUIRE(test(procedure));
            test.input_utf16[i] = old;
        }
    }
}

#endif

#if TURBO_IS_BIG_ENDIAN
// todo: port the next test.
#else

TEST_CASE("convert_fails_if_there_is_low_surrogate_is_followed_by_another_low_surrogate") {
    const size_t size = 64;
    transcode_utf16_to_utf8_test_base test([]() { return '*'; }, size + 32);

    for (char16_t low_surrogate = 0xdc00; low_surrogate <= 0xdfff; low_surrogate++) {
        for (size_t i = 0; i < size - 1; i++) {
            auto procedure = [&i](const char16_t *utf16, size_t size, char *utf8) -> size_t {
                turbo::UnicodeResult res = turbo::convert_utf16le_to_utf8_with_errors(utf16, size, utf8);
                REQUIRE_EQ(res.error, turbo::UnicodeError::SURROGATE);
                REQUIRE_EQ(res.count, i);
                return 0;
            };
            const auto old0 = test.input_utf16[i + 0];
            const auto old1 = test.input_utf16[i + 1];
            test.input_utf16[i + 0] = low_surrogate;
            test.input_utf16[i + 1] = low_surrogate;
            REQUIRE(test(procedure));
            test.input_utf16[i + 0] = old0;
            test.input_utf16[i + 1] = old1;
        }
    }
}

#endif

#if TURBO_IS_BIG_ENDIAN
// todo: port the next test.
#else

TEST_CASE("convert_fails_if_there_is_surrogate_pair_is_followed_by_high_surrogate") {
    const size_t size = 64;
    transcode_utf16_to_utf8_test_base test([]() { return '*'; }, size + 32);

    const char16_t low_surrogate = 0xd801;
    const char16_t high_surrogate = 0xdc02;
    for (size_t i = 0; i < size - 2; i++) {
        auto procedure = [&i](const char16_t *utf16, size_t size, char *utf8) -> size_t {
            turbo::UnicodeResult res = turbo::convert_utf16le_to_utf8_with_errors(utf16, size, utf8);
            REQUIRE_EQ(res.error, turbo::UnicodeError::SURROGATE);
            REQUIRE_EQ(res.count, i + 2);
            return 0;
        };
        const auto old0 = test.input_utf16[i + 0];
        const auto old1 = test.input_utf16[i + 1];
        const auto old2 = test.input_utf16[i + 2];
        test.input_utf16[i + 0] = low_surrogate;
        test.input_utf16[i + 1] = high_surrogate;
        test.input_utf16[i + 2] = high_surrogate;
        REQUIRE(test(procedure));
        test.input_utf16[i + 0] = old0;
        test.input_utf16[i + 1] = old1;
        test.input_utf16[i + 2] = old2;
    }
}

#endif


