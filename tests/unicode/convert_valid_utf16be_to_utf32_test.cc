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

  using turbo::tests::helpers::transcode_utf16_to_utf32_test_base;

  constexpr int trials = 1000;
}

TEST_CASE("convert_2_UTF16_bytes") {
  for(size_t trial = 0; trial < trials; trial ++) {
    if ((trial % 100) == 0) { std::cout << "."; std::cout.flush(); }
    // range for 2-byte UTF-16 (no surrogate pairs)
    turbo::FixedUniformRanges<uint32_t, uint64_t> random({{0x0000, 0x007f},
                                                     {0x0080, 0x07ff},
                                                     {0x0800, 0xd7ff},
                                                     {0xe000, 0xffff}});

    auto procedure = [](const char16_t* utf16le, size_t size, char32_t* utf32) -> size_t {
      std::vector<char16_t> utf16be(size);
      turbo::change_endianness_utf16(utf16le, size, utf16be.data());
      return turbo::convert_valid_utf16be_to_utf32(utf16be.data(), size, utf32);
    };

    for (size_t size: input_size) {
      transcode_utf16_to_utf32_test_base test([&random](){return random();}, size);
      REQUIRE(test(procedure));
    }
  }
}

TEST_CASE("convert_with_surrogate_pairs") {
  for(size_t trial = 0; trial < trials; trial ++) {
    if ((trial % 100) == 0) { std::cout << "."; std::cout.flush(); }
    // some surrogate pairs
    turbo::FixedUniformRanges<uint32_t, uint64_t> random({{0x0800, 0xd800-1},
                                                     {0xe000, 0x10ffff}});

    auto procedure = [](const char16_t* utf16le, size_t size, char32_t* utf32) -> size_t {
      std::vector<char16_t> utf16be(size);
      turbo::change_endianness_utf16(utf16le, size, utf16be.data());
      return turbo::convert_valid_utf16be_to_utf32(utf16be.data(), size, utf32);
    };

    for (size_t size: input_size) {
      transcode_utf16_to_utf32_test_base test([&random](){return random();}, size);
      REQUIRE(test(procedure));
    }
  }
}

#if TURBO_IS_BIG_ENDIAN
// todo: port the next test.
#else
namespace {
  std::vector<std::vector<char16_t>> all_combinations() {
    const char16_t V_1byte_start  = 0x0042; // non-surrogate word the yields 1 UTF-8 byte
    const char16_t V_2bytes_start = 0x017f; // non-surrogate word the yields 2 UTF-8 bytes
    const char16_t V_3bytes_start = 0xefff; // non-surrogate word the yields 3 UTF-8 bytes
    const char16_t L        = 0xd9ca; // low surrogate
    const char16_t H        = 0xde42; // high surrogate

    std::vector<std::vector<char16_t>> result;
    std::vector<char16_t> row(32, '*');

    std::array<int, 8> pattern{0};
    while (true) {
      //if (result.size() > 5) break;

      // 1. produce output
      char16_t V_1byte = V_1byte_start;
      char16_t V_2bytes = V_2bytes_start;
      char16_t V_3bytes = V_3bytes_start;
      for (int i=0; i < 8; i++) {
        switch (pattern[i]) {
          case 0:
            row[i] = V_1byte++;
            break;
          case 1:
            row[i] = V_2bytes++;
            break;
          case 2:
            row[i] = V_3bytes++;
            break;
          case 3:
            row[i] = L;
            break;
          case 4:
            row[i] = H;
            break;
          default:
            abort();
        }
      } // for

      if (row[7] == L) {
        row[8] = H; // make input valid
        result.push_back(row);

        row[8] = V_1byte; // broken input
        result.push_back(row);
      } else {
        row[8] = V_1byte;
        result.push_back(row);
      }

      // next pattern
      int i = 0;
      int carry = 1;
      for (/**/; i < 8 && carry; i++) {
        pattern[i] += carry;
        if (pattern[i] == 5) {
          pattern[i] = 0;
          carry = 1;
        } else
          carry = 0;
      }

      if (carry == 1 and i == 8)
        break;

    } // while

    return result;
  }
}

TEST_CASE("all_possible_8_codepoint_combinations") {
  auto procedure = [](const char16_t* utf16le, size_t size, char32_t* utf32) -> size_t {
    std::vector<char16_t> utf16be(size);
    turbo::change_endianness_utf16(utf16le, size, utf16be.data());
    return turbo::convert_valid_utf16be_to_utf32(utf16be.data(), size, utf32);
  };

  std::vector<char> output_utf32(256, ' ');
  const auto& combinations = all_combinations();
  for (const auto& input_utf16: combinations) {
    if (turbo::validate_utf16(input_utf16.data(), input_utf16.size())) {
      transcode_utf16_to_utf32_test_base test(input_utf16);
      REQUIRE(test(procedure));
    }
  }
}
#endif

