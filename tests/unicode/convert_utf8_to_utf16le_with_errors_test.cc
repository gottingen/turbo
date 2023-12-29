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
#include "turbo/format/print.h"
#include <memory>

namespace {
    std::array<size_t, 7> input_size{7, 16, 12, 64, 67, 128, 256};

    using turbo::tests::helpers::transcode_utf8_to_utf16_test_base;

    constexpr size_t trials = 10000;
    constexpr size_t num_trials = 1000;
    constexpr size_t fix_size = 512;
}

TEST_CASE("issue_213") {
    const char *buf = "\x01\x9a\x84";
    // We select the byte 0x84. It is a continuation byte so it is possible
    // that the predicted output might be zero.
    size_t expected_size = turbo::utf16_length_from_utf8(buf + 2, 1);
    std::unique_ptr<char16_t[]> buffer(new char16_t[expected_size]);
    turbo::result r = turbo::convert_utf8_to_utf16le_with_errors(buf + 2, 1, buffer.get());
    REQUIRE(r.error != turbo::SUCCESS);
    // r.count: In case of error, indicates the position of the error in the input.
    // In case of success, indicates the number of words validated/written.
    REQUIRE(r.count == 0);
}

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

        auto procedure = [](const char *utf8, size_t size, char16_t *utf16) -> size_t {
            turbo::result res = turbo::convert_utf8_to_utf16le_with_errors(utf8, size, utf16);
            REQUIRE_EQ(res.error, turbo::error_code::SUCCESS);
            REQUIRE_EQ(res.count, size);
            return res.count;
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

        auto procedure = [](const char *utf8, size_t size, char16_t *utf16) -> size_t {
            turbo::result res = turbo::convert_utf8_to_utf16le_with_errors(utf8, size, utf16);
            REQUIRE_EQ(res.error, turbo::error_code::SUCCESS);
            return res.count;
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

        auto procedure = [](const char *utf8, size_t size, char16_t *utf16) -> size_t {
            turbo::result res = turbo::convert_utf8_to_utf16le_with_errors(utf8, size, utf16);
            REQUIRE_EQ(res.error, turbo::error_code::SUCCESS);
            return res.count;
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

        auto procedure = [](const char *utf8, size_t size, char16_t *utf16) -> size_t {
            turbo::result res = turbo::convert_utf8_to_utf16le_with_errors(utf8, size, utf16);
            REQUIRE_EQ(res.error, turbo::error_code::SUCCESS);
            return res.count;
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

TEST_CASE("header_bits_error") {
    turbo::FixedUniformRanges<uint32_t, uint64_t> random({{0x0000, 0xd800 - 1},
                                                     {0xe000, 0x10ffff}});

    for (size_t trial = 0; trial < num_trials; trial++) {
        transcode_utf8_to_utf16_test_base test([&random]() { return random(); }, fix_size);

        for (int i = 0; i < fix_size; i++) {
            if ((test.input_utf8[i] & 0b11000000) != 0b10000000) {  // Only process leading bytes
                auto procedure = [ &i](const char *utf8, size_t size, char16_t *utf16) -> size_t {
                    turbo::result res = turbo::convert_utf8_to_utf16le_with_errors(utf8, size, utf16);
                    REQUIRE_EQ(res.error, turbo::error_code::HEADER_BITS);
                    REQUIRE_EQ(res.count, i);
                    return 0;
                };
                const unsigned char old = test.input_utf8[i];
                test.input_utf8[i] = uint8_t(0b11111000);
                REQUIRE(test(procedure));
                test.input_utf8[i] = old;
            }
        }
    }
}

TEST_CASE("too_short_error") {

    turbo::FixedUniformRanges<uint32_t, uint64_t> random({{0x0000, 0xd800 - 1},
                                                     {0xe000, 0x10ffff}});
    for (size_t trial = 0; trial < num_trials; trial++) {
        transcode_utf8_to_utf16_test_base test([&random]() { return random(); }, fix_size);
        int leading_byte_pos = 0;
        for (int i = 0; i < fix_size; i++) {
            if ((test.input_utf8[i] & 0b11000000) ==
                0b10000000) {  // Only process continuation bytes by making them leading bytes
                auto procedure = [ &leading_byte_pos](const char *utf8, size_t size,
                                                                      char16_t *utf16) -> size_t {
                    turbo::result res = turbo::convert_utf8_to_utf16le_with_errors(utf8, size, utf16);
                    REQUIRE_EQ(res.error, turbo::error_code::TOO_SHORT);
                    REQUIRE_EQ(res.count, leading_byte_pos);
                    return 0;
                };
                const unsigned char old = test.input_utf8[i];
                test.input_utf8[i] = uint8_t(0b11100000);
                REQUIRE(test(procedure));
                test.input_utf8[i] = old;
            } else {
                leading_byte_pos = i;
            }
        }
    }
}

TEST_CASE("too_long_error") {

    turbo::FixedUniformRanges<uint32_t, uint64_t> random({{0x0000, 0xd800 - 1},
                                                     {0xe000, 0x10ffff}});
    for (size_t trial = 0; trial < num_trials; trial++) {
        transcode_utf8_to_utf16_test_base test([&random]() { return random(); }, fix_size);
        for (int i = 1; i < fix_size; i++) {
            if (((test.input_utf8[i] & 0b11000000) !=
                 0b10000000)) {  // Only process leading bytes by making them continuation bytes
                auto procedure = [ &i](const char *utf8, size_t size, char16_t *utf16) -> size_t {
                    turbo::result res = turbo::convert_utf8_to_utf16le_with_errors(utf8, size, utf16);
                    REQUIRE_EQ(res.error, turbo::error_code::TOO_LONG);
                    REQUIRE_EQ(res.count, i);
                    return 0;
                };
                const unsigned char old = test.input_utf8[i];
                test.input_utf8[i] = uint8_t(0b10000000);
                REQUIRE(test(procedure));
                test.input_utf8[i] = old;
            }
        }
    }
}

TEST_CASE("overlong_error") {

    //turbo::FixedUniformRanges<uint32_t, uint64_t> random({{0x0000, 0xd800 - 1},
    //                                                 {0xe000, 0x10ffff}});

    turbo::FixedUniformRanges<uint32_t, uint64_t> random({{0x0000, 0xd800 - 1},
                                                   {0xe000, 0x10ffff}});
    //turbo::Utf8Generator random(1,1,1,1);

    for (size_t trial = 0; trial < num_trials; trial++) {
        transcode_utf8_to_utf16_test_base test([&random]() {
            auto r =  random();
            turbo::Println("r:{:x}", r);
            return r;}, fix_size);
        for (int i = 1; i < fix_size; i++) {
            if ((unsigned char) test.input_utf8[i] >=
                (unsigned char) 0b11000000) { // Only non-ASCII leading bytes can be overlong
                auto procedure = [ &i](const char *utf8, size_t size, char16_t *utf16) -> size_t {
                    turbo::result res = turbo::convert_utf8_to_utf16le_with_errors(utf8, size, utf16);
                    REQUIRE_EQ(res.error, turbo::error_code::OVERLONG);
                    REQUIRE_EQ(res.count, i);
                    return 0;
                };
                const unsigned char old = test.input_utf8[i];
                const unsigned char second_old = test.input_utf8[i + 1];
                if ((old & 0b11100000) == 0b11000000) { // two-bytes case, change to a value less or equal than 0x7f
                    test.input_utf8[i] = char(0b11000000);
                } else if ((old & 0b11110000) ==
                           0b11100000) {  // three-bytes case, change to a value less or equal than 0x7ff
                    test.input_utf8[i] = char(0b11100000);
                    test.input_utf8[i + 1] = test.input_utf8[i + 1] & 0b11011111;
                } else {  // four-bytes case, change to a value less or equal than 0xffff
                    test.input_utf8[i] = char(0b11110000);
                    test.input_utf8[i + 1] = test.input_utf8[i + 1] & 0b11001111;
                }
                REQUIRE(test(procedure));
                test.input_utf8[i] = old;
                test.input_utf8[i + 1] = second_old;
            }
        }
    }
}

TEST_CASE("too_large_error") {

    turbo::FixedUniformRanges<uint32_t, uint64_t> random({{0x0000, 0xd800 - 1},
                                                     {0xe000, 0x10ffff}});
    for (size_t trial = 0; trial < num_trials; trial++) {
        transcode_utf8_to_utf16_test_base test([&random]() {  return  random();}, fix_size);
        for (int i = 1; i < fix_size; i++) {
            if ((test.input_utf8[i] & 0b11111000) == 0b11110000) { // Can only have too large error in 4-bytes case
                auto procedure = [ &i](const char *utf8, size_t size, char16_t *utf16) -> size_t {
                    turbo::result res = turbo::convert_utf8_to_utf16le_with_errors(utf8, size, utf16);
                    REQUIRE_EQ(res.error, turbo::error_code::TOO_LARGE);
                    REQUIRE_EQ(res.count, i);
                    return 0;
                };
                test.input_utf8[i] += ((test.input_utf8[i] & 0b100) == 0b100) ? 0b10
                                                                              : 0b100;   // Make sure we get too large error and not header bits error
                REQUIRE(test(procedure));
                test.input_utf8[i] -= 0b100;
            }
        }
    }
}

TEST_CASE("surrogate_error") {

    //turbo::FixedUniformRanges<uint32_t, uint64_t> random({{0x0000, 0xd800 - 1},
    //                                                 {0xe000, 0x10ffff}});
    turbo::Utf8Generator random(1,1,1,1);
    for (size_t trial = 0; trial < num_trials; trial++) {
        transcode_utf8_to_utf16_test_base test([&random]() {
            auto r = random.generate(4);
            uint32_t rr = *r.data();
            return rr;
            }, fix_size);
        for (int i = 1; i < fix_size; i++) {
            if ((test.input_utf8[i] & 0b11110000) == 0b11100000) { // Can only have surrogate error in 3-bytes case
                auto procedure = [ &i](const char *utf8, size_t size, char16_t *utf16) -> size_t {
                    turbo::result res = turbo::convert_utf8_to_utf16le_with_errors(utf8, size, utf16);
                    REQUIRE_EQ(res.error, turbo::error_code::SURROGATE);
                    REQUIRE_EQ(res.count, i);
                    return 0;
                };
                const unsigned char old = test.input_utf8[i];
                const unsigned char second_old = test.input_utf8[i + 1];
                test.input_utf8[i] = char(0b11101101);
                for (int s = 0x8; s < 0xf; s++) {  // Modify second byte to create a surrogate codepoint
                    test.input_utf8[i + 1] = (test.input_utf8[i + 1] & 0b11000011) | (s << 2);
                    REQUIRE(test(procedure));
                }
                test.input_utf8[i] = old;
                test.input_utf8[i + 1] = second_old;
            }
        }
    }
}
