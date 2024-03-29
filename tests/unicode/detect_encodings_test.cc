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
#include "turbo/unicode/codec/encode_utf16.h"
#include "transcode_test_base.h"

#include <algorithm>
#include <array>
#include <iostream>
#include <random>
#include <stdexcept>

#include "turbo/random/random.h"


namespace {
    std::array<size_t, 7> input_size{8, 16, 12, 64, 68, 128, 256};
} // namespace


TEST_CASE("boommmmm") {
    const char *utf8_bom = "\xef\xbb\xbf";
    const char *utf16be_bom = "\xfe\xff";
    const char *utf16le_bom = "\xff\xfe";
    REQUIRE(turbo::detect_encodings(utf8_bom, 3) == turbo::EncodingType::UTF8);
    REQUIRE(turbo::detect_encodings(utf16be_bom, 2) == turbo::EncodingType::UTF16_BE);
    REQUIRE(turbo::detect_encodings(utf16le_bom, 2) == turbo::EncodingType::UTF16_LE);
}


TEST_CASE("pure_utf8_ASCII") {
    for (size_t trial = 0; trial < 10000; trial++) {
        if ((trial % 100) == 0) {
            std::cout << ".";
            std::cout.flush();
        }


        turbo::Utf8Generator random(1, 0, 0, 0);

        for (size_t size: input_size) {
            auto generated = random.generate_counted(size);
            auto expected = turbo::EncodingType::UTF8 | turbo::EncodingType::UTF16_LE;
            auto actual = turbo::detect_encodings(
                    reinterpret_cast<const char *>(generated.first.data()),
                    size);
            REQUIRE(actual == expected);
        }
    }
}

TEST_CASE("pure_utf16_ASCII") {
    for (size_t trial = 0; trial < 10000; trial++) {
        if ((trial % 100) == 0) {
            std::cout << ".";
            std::cout.flush();
        }


        turbo::FixedUniform<int> random(0, 127);

        for (size_t size: input_size) {
            std::vector<uint16_t> generated;
            for (int i = 0; i < size / 2; i++) {
                generated.push_back(uint16_t(random()));
            }
            auto expected = turbo::EncodingType::UTF8 | turbo::EncodingType::UTF16_LE;
            auto actual = turbo::detect_encodings(reinterpret_cast<const char *>(generated.data()), size);
            REQUIRE((static_cast<int>(actual) & static_cast<int>(expected)));
        }
    }
}

TEST_CASE("pure_utf32_ASCII") {
    for (size_t trial = 0; trial < 10000; trial++) {
        if ((trial % 100) == 0) {
            std::cout << ".";
            std::cout.flush();
        }


        turbo::FixedUniform<int> random(0, 0x7f);

        for (size_t size: input_size) {
            std::vector<uint32_t> generated;
            for (int i = 0; i < size / 4; i++) {
                generated.push_back(random());
            }
            auto expected = turbo::EncodingType::UTF8 | turbo::EncodingType::UTF16_LE | turbo::EncodingType::UTF32_LE;
            auto actual = turbo::detect_encodings(
                    reinterpret_cast<const char *>(generated.data()),
                    size);
            REQUIRE(actual == expected);
        }
    }
}

#if TURBO_IS_BIG_ENDIAN
// todo: port this test for big-endian platforms.
#else

TEST_CASE("no_utf8_bytes_no_surrogates") {
    for (size_t trial = 0; trial < 10000; trial++) {
        if ((trial % 100) == 0) {
            std::cout << ".";
            std::cout.flush();
        }


        turbo::FixedUniformRanges<uint32_t, uint64_t> random({{0x007f, 0xd800 - 1},
                                                              {0xe000, 0xffff}});

        for (size_t size: input_size) {
            std::vector<uint32_t> generated;
            for (int i = 0; i < size / 4; i++) {
                generated.push_back(random());
            }
            auto expected = turbo::EncodingType::UTF16_LE |turbo::EncodingType::UTF16_BE | turbo::EncodingType::UTF32_LE;
            auto actual = turbo::detect_encodings(reinterpret_cast<const char *>(generated.data()), size);
            turbo::println("actual: {}", actual);
            REQUIRE((actual & expected));
        }
    }
}

#endif

TEST_CASE("two_utf8_bytes") {
    for (size_t trial = 0; trial < 10000; trial++) {
        if ((trial % 100) == 0) {
            std::cout << ".";
            std::cout.flush();
        }


        turbo::Utf8Generator random(0, 1, 0, 0);

        for (size_t size: input_size) {
            auto generated = random.generate_counted(size);
            auto expected = turbo::EncodingType::UTF8 | turbo::EncodingType::UTF16_LE;
            auto actual = turbo::detect_encodings(
                    reinterpret_cast<const char *>(generated.first.data()),
                    size);
            if (actual != expected) {
                if ((actual & turbo::EncodingType::UTF8) == 0) {
                    std::cout << "failed to detect valid UTF-8." << std::endl;
                }
                if ((actual & turbo::EncodingType::UTF16_LE) == 0) {
                    std::cout << "failed to detect valid UTF-16LE." << std::endl;
                }
            }
            REQUIRE(actual == expected);
        }
    }
}

TEST_CASE("utf_16_surrogates") {
    for (size_t trial = 0; trial < 10000; trial++) {
        if ((trial % 100) == 0) {
            std::cout << ".";
            std::cout.flush();
        }


        turbo::Utf16Generator random(0, 1);

        for (size_t size: input_size) {
            auto generated = random.generate_counted(size / 2);
            auto expected = turbo::EncodingType::UTF16_LE;
            auto actual = turbo::detect_encodings(
                    reinterpret_cast<const char *>(generated.first.data()),
                    size);
            REQUIRE(actual == expected);
        }
    }
}

#if TURBO_IS_BIG_ENDIAN
// todo: port this test for big-endian platforms.
#else

TEST_CASE("utf32_surrogates") {
    for (size_t trial = 0; trial < 10000; trial++) {
        if ((trial % 100) == 0) {
            std::cout << ".";
            std::cout.flush();
        }


        turbo::FixedUniform<int> random_prefix(0x10000, 0x10ffff);
        turbo::FixedUniform<int> random_suffix(0xd800, 0xdfff);

        for (size_t size: input_size) {
            std::vector<uint32_t> generated;
            for (int i = 0; i < size / 4; i++) {
                generated.push_back((random_prefix() & 0xffff0000) + random_suffix());
            }
            auto expected = turbo::EncodingType::UTF32_LE;
            auto actual = turbo::detect_encodings(
                    reinterpret_cast<const char *>(generated.data()),
                    size);
            REQUIRE(actual == expected);
        }
    }
}

#endif


#if TURBO_IS_BIG_ENDIAN
// todo: port this test for big-endian platforms.
#else

TEST_CASE("edge_surrogate") {
    for (size_t trial = 0; trial < 10000; trial++) {
        if ((trial % 100) == 0) {
            std::cout << ".";
            std::cout.flush();
        }


        turbo::FixedUniform<int> random(0x10000, 0x10ffff);

        const size_t size = 512;
        std::vector<uint16_t> generated(size / 2, 0);
        int i = 31;
        while (i + 32 < (size / 2) - 1) {
            char16_t W1;
            char16_t W2;
            REQUIRE_EQ(turbo::unicode::utf16::encode(random(), W1, W2), 2);
            generated[i] = W1;
            generated[i + 1] = W2;
            i += 32;
        }
        auto expected = turbo::EncodingType::UTF16_LE;
        auto actual = turbo::detect_encodings(
                reinterpret_cast<const char *>(generated.data()),
                size);
        REQUIRE(actual == expected);
    }
}

#endif

TEST_CASE("tail_utf8") {
    for (size_t trial = 0; trial < 10000; trial++) {
        if ((trial % 100) == 0) {
            std::cout << ".";
            std::cout.flush();
        }


        turbo::Utf8Generator random(0, 0, 1, 0);
        std::array<size_t, 5> multiples_three{12, 54, 66, 126, 252};
        for (size_t size: multiples_three) {
            auto generated = random.generate_counted(size);
            auto expected = turbo::EncodingType::UTF8 | turbo::EncodingType::UTF16_LE;
            auto actual = turbo::detect_encodings(
                    reinterpret_cast<const char *>(generated.first.data()),
                    size);
            REQUIRE(actual == expected);
        }
    }
}


