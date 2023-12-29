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

#ifndef TURBO_IS_BIG_ENDIAN
#error "TURBO_IS_BIG_ENDIAN should be defined."
#endif

#include <array>
#include <iostream>


#include "turbo/random/random.h"

#include <memory>

namespace {
  std::array<size_t, 9> input_size{7, 12, 16, 64, 67, 128, 256, 511, 1000};

  using turbo::tests::helpers::transcode_utf8_to_utf16_test_base;

  constexpr size_t trials = 10000;
}

TEST_CASE("convert_pure_ASCII") {
  for(size_t trial = 0; trial < trials; trial ++) {
    if((trial % 100) == 0) { std::cout << "."; std::cout.flush(); }
    size_t counter = 0;
    auto generator = [&counter]() -> uint32_t {
      return counter++ & 0x7f;
    };

    auto procedure = [](const char* utf8, size_t size, char16_t* utf16) -> size_t {
      return turbo::convert_valid_utf8_to_utf16le(utf8, size, utf16);
    };

    for (size_t size: input_size) {
      transcode_utf8_to_utf16_test_base test(generator, size);
      REQUIRE(test(procedure));
    }
  }
}

TEST_CASE("convert_1_or_2_UTF8_bytes") {
  for(size_t trial = 0; trial < trials; trial ++) {
    if((trial % 100) == 0) { std::cout << "."; std::cout.flush(); }
    turbo::FixedUniform<int> random(0x0000, 0x07ff); // range for 1 or 2 UTF-8 bytes

    auto procedure = [](const char* utf8, size_t size, char16_t* utf16) -> size_t {
      return turbo::convert_valid_utf8_to_utf16le(utf8, size, utf16);
    };

    for (size_t size: input_size) {
      transcode_utf8_to_utf16_test_base test([&random](){return random();}, size);
      REQUIRE(test(procedure));
    }
  }
}

TEST_CASE("convert_1_or_2_or_3_UTF8_bytes") {
  for(size_t trial = 0; trial < trials; trial ++) {
    if((trial % 100) == 0) { std::cout << "."; std::cout.flush(); }
    // range for 1, 2 or 3 UTF-8 bytes
    turbo::FixedUniformRanges<uint32_t, uint64_t> random({{0x0000, 0xd7ff},
                                                     {0xe000, 0xffff}});

    auto procedure = [](const char* utf8, size_t size, char16_t* utf16) -> size_t {
      return turbo::convert_valid_utf8_to_utf16le(utf8, size, utf16);
    };

    for (size_t size: input_size) {
      transcode_utf8_to_utf16_test_base test([&random](){return random();}, size);
      REQUIRE(test(procedure));
    }
  }
}

TEST_CASE("convert_3_UTF8_bytes") {
  for(size_t trial = 0; trial < trials; trial ++) {
    if((trial % 100) == 0) { std::cout << "."; std::cout.flush(); }
    turbo::FixedUniformRanges<uint32_t, uint64_t> random({{0x0800, 0xd800-1}}); // range for 3 UTF-8 bytes

    auto procedure = [](const char* utf8, size_t size, char16_t* utf16) -> size_t {
      return turbo::convert_valid_utf8_to_utf16le(utf8, size, utf16);
    };

    for (size_t size: input_size) {
      transcode_utf8_to_utf16_test_base test([&random](){return random();}, size);
      REQUIRE(test(procedure));
    }
  }
}

TEST_CASE("convert_3_or_4_UTF8_bytes") {
  for(size_t trial = 0; trial < trials; trial ++) {
    if((trial % 100) == 0) { std::cout << "."; std::cout.flush(); }
    turbo::FixedUniformRanges<uint32_t, uint64_t> random({{0x0800, 0xd800-1},
                                                     {0xe000, 0x10ffff}}); // range for 3 or 4 UTF-8 bytes

    auto procedure = [](const char* utf8, size_t size, char16_t* utf16) -> size_t {
      return turbo::convert_valid_utf8_to_utf16le(utf8, size, utf16);
    };

    for (size_t size: input_size) {
      transcode_utf8_to_utf16_test_base test([&random](){return random();}, size);
      REQUIRE(test(procedure));
    }
  }
}


TEST_CASE("convert_null_4_UTF8_bytes") {
  for(size_t trial = 0; trial < trials; trial ++) {
    if((trial % 100) == 0) { std::cout << "."; std::cout.flush(); }
    turbo::FixedUniformRanges<uint32_t, uint64_t> random({{0x0000, 0x00000},
                                                     {0x10000, 0x10ffff}}); // range for 3 or 4 UTF-8 bytes

    auto procedure = [](const char* utf8, size_t size, char16_t* utf16) -> size_t {
      return turbo::convert_valid_utf8_to_utf16le(utf8, size, utf16);
    };

    for (size_t size: input_size) {
      transcode_utf8_to_utf16_test_base test([&random](){return random();}, size);
      REQUIRE(test(procedure));
    }
  }
}

#if TURBO_IS_BIG_ENDIAN
// todo: port this test for big-endian platforms.
#else
TEST_CASE("issue111") {
  // We stick to ASCII for our source code given that there is no universal way to specify the character encoding of
  // the source files.
  char16_t input[] = u"aaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaa\u30b3aaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaa";
  size_t utf16_len = sizeof(input) / sizeof(char16_t) - 1;
  REQUIRE(turbo::validate_utf16le(input, utf16_len));
  REQUIRE(turbo::utf8_length_from_utf16le(input, utf16_len)
              == 2 + utf16_len);
  size_t utf8_len = turbo::utf8_length_from_utf16le(input, utf16_len);
  std::unique_ptr<char[]> utf8_buffer{new char[utf8_len]};
  REQUIRE(turbo::convert_valid_utf16le_to_utf8(input, utf16_len, utf8_buffer.get())
              == utf8_len);

  std::unique_ptr<char16_t[]> utf16_buffer{new char16_t[utf16_len]};

  REQUIRE(turbo::convert_valid_utf8_to_utf16le(utf8_buffer.get(), utf8_len, utf16_buffer.get())
              == utf16_len);

  REQUIRE(std::char_traits<char16_t>::compare(input, utf16_buffer.get(), utf16_len) == 0);
}
#endif

