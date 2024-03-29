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
#include <queue>
#include <iostream>

#include "turbo/random/random.h"



namespace {
  std::array<size_t, 7> input_size{7, 16, 12, 64, 67, 128, 256};

  using turbo::tests::helpers::transcode_utf8_to_utf32_test_base;

  constexpr size_t trials = 10000;
}

TEST_CASE("convert_pure_ASCII") {
  for(size_t trial = 0; trial < trials; trial ++) {
    if((trial % 100) == 0) { std::cout << "."; std::cout.flush(); }
    size_t counter = 0;
    auto generator = [&counter]() -> uint32_t {
      return counter++ & 0x7f;
    };

    auto procedure = [](const char* utf8, size_t size, char32_t* utf32) -> size_t {
      return turbo::convert_valid_utf8_to_utf32(utf8, size, utf32);
    };

    for (size_t size: input_size) {
      transcode_utf8_to_utf32_test_base test(generator, size);
      REQUIRE(test(procedure));
    }
  }
}

TEST_CASE("convert_1_or_2_UTF8_bytes") {
  for(size_t trial = 0; trial < trials; trial ++) {
    if((trial % 100) == 0) { std::cout << "."; std::cout.flush(); }
    turbo::FixedUniform<int> random(0x0000, 0x07ff); // range for 1 or 2 UTF-8 bytes

    auto procedure = [](const char* utf8, size_t size, char32_t* utf32) -> size_t {
      return turbo::convert_valid_utf8_to_utf32(utf8, size, utf32);
    };

    for (size_t size: input_size) {
      transcode_utf8_to_utf32_test_base test([&random](){return random();}, size);
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

    auto procedure = [](const char* utf8, size_t size, char32_t* utf32) -> size_t {
      return turbo::convert_valid_utf8_to_utf32(utf8, size, utf32);
    };

    for (size_t size: input_size) {
      transcode_utf8_to_utf32_test_base test([&random](){return random();}, size);
      REQUIRE(test(procedure));
    }
  }
}

TEST_CASE("convert_3_or_4_UTF8_bytes") {
  for(size_t trial = 0; trial < trials; trial ++) {
    if((trial % 100) == 0) { std::cout << "."; std::cout.flush(); }
    turbo::FixedUniformRanges<uint32_t, uint64_t> random({{0x0800, 0xd800-1},
                                                     {0xe000, 0x10ffff}}); // range for 3 or 4 UTF-8 bytes

    auto procedure = [](const char* utf8, size_t size, char32_t* utf32) -> size_t {
      return turbo::convert_valid_utf8_to_utf32(utf8, size, utf32);
    };

    for (size_t size: input_size) {
      transcode_utf8_to_utf32_test_base test([&random](){return random();}, size);
      REQUIRE(test(procedure));
    }
  }
}


TEST_CASE("convert_null_4_UTF8_bytes") {
  for(size_t trial = 0; trial < trials; trial ++) {
    if((trial % 100) == 0) { std::cout << "."; std::cout.flush(); }
    turbo::FixedUniformRanges<uint32_t, uint64_t> random({{0x0000, 0x00000},
                                                     {0x10000, 0x10ffff}}); // range for 3 or 4 UTF-8 bytes

    auto procedure = [](const char* utf8, size_t size, char32_t* utf32) -> size_t {
      return turbo::convert_valid_utf8_to_utf32(utf8, size, utf32);
    };

    for (size_t size: input_size) {
      transcode_utf8_to_utf32_test_base test([&random](){return random();}, size);
      REQUIRE(test(procedure));
    }
  }
}

TEST_CASE("issue132") {


  // range for 2,3 and 4 UTF-8 bytes 
  turbo::FixedUniformRanges<uint32_t, uint64_t> random({{0x080, 0xd800-1},
                                                    {0xe000, 0x10ffff}});

  auto procedure = [](const char* utf8, size_t size, char32_t* utf32) -> size_t {
    return turbo::convert_valid_utf8_to_utf32(utf8, size, utf32);
  };

  const size_t size = 200;
  std::vector<uint32_t> data(size+32, '*');

  for (size_t j = 0; j < 1000; j++) {
    uint32_t non_ascii = random();
    for (size_t i=0; i < size; i++) {
      auto old = data[i];
      data[i] = non_ascii;
      transcode_utf8_to_utf32_test_base test(data);
      REQUIRE(test(procedure));
      data[i] = old;
    }
  }
}

