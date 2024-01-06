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
  for(size_t trial = 0; trial < trials; trial ++) {
    if ((trial % 100) == 0) { std::cout << "."; std::cout.flush(); }
    // range for 2 UTF-16 bytes
    turbo::FixedUniformRanges<uint32_t, uint64_t> random({{0x0000, 0xd7ff},
                                                     {0xe000, 0xffff}});

    auto procedure = [](const char32_t* utf32, size_t size, char16_t* utf16) -> size_t {
      return turbo::convert_utf32_to_utf16le(utf32, size, utf16);
    };
    for (size_t size: input_size) {
      transcode_utf32_to_utf16_test_base test([&random](){return random();}, size);
      REQUIRE(test(procedure));
    }
  }
}

TEST_CASE("convert_into_4_UTF16_bytes") {
  for(size_t trial = 0; trial < trials; trial ++) {
    if ((trial % 100) == 0) { std::cout << "."; std::cout.flush(); }
    // range for 4 UTF-16 bytes
    turbo::FixedUniformRanges<uint32_t, uint64_t> random({{0x10000, 0x10ffff}});

    auto procedure = [](const char32_t* utf32, size_t size, char16_t* utf16) -> size_t {
      return turbo::convert_utf32_to_utf16le(utf32, size, utf16);
    };
    for (size_t size: input_size) {
      transcode_utf32_to_utf16_test_base test([&random](){return random();}, size);
      REQUIRE(test(procedure));
    }
  }
}

TEST_CASE("convert_into_2_or_4_UTF16_bytes") {
  for(size_t trial = 0; trial < trials; trial ++) {
    if ((trial % 100) == 0) { std::cout << "."; std::cout.flush(); }
    // range for 2 or 4 UTF-16 bytes (all codepoints)
    turbo::FixedUniformRanges<uint32_t, uint64_t> random({{0x0000, 0xd7ff},
                                                     {0xe000, 0xffff},
                                                     {0x10000, 0x10ffff}});

    auto procedure = [](const char32_t* utf32, size_t size, char16_t* utf16) -> size_t {
      return turbo::convert_utf32_to_utf16le(utf32, size, utf16);
    };
    for (size_t size: input_size) {
      transcode_utf32_to_utf16_test_base test([&random](){return random();}, size);
      REQUIRE(test(procedure));
    }
  }
}

