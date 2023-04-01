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

#include "turbo/unicode/utf.h"

#include <array>
#include <iostream>

#include <turbo/unicode/test/reference/validate_utf32.h>
#include <turbo/unicode/test/reference/decode_utf32.h>
#include <turbo/unicode/test/helpers/transcode_test_base.h>
#include <turbo/unicode/test/helpers/random_int.h>
#include <turbo/unicode/test/helpers/test.h>


namespace {
  std::array<size_t, 7> input_size{7, 16, 12, 64, 67, 128, 256};

  using turbo::tests::helpers::transcode_utf32_to_utf8_test_base;

  constexpr int trials = 1000;
}

TEST(convert_pure_ASCII) {
  size_t counter = 0;
  auto generator = [&counter]() -> uint32_t {
    return counter++ & 0x7f;
  };

  auto procedure = [&implementation](const char32_t* utf32, size_t size, char* utf8) -> size_t {
    return implementation.ConvertValidUtf32ToUtf8(utf32, size, utf8);
  };

  std::array<size_t, 5> input_size{16, 12, 64, 128, 256};
  for (size_t size: input_size) {
    transcode_utf32_to_utf8_test_base test(generator, size);
    ASSERT_TRUE(test(procedure));
  }
}

TEST(convert_into_1_or_2_UTF8_bytes) {
  for(size_t trial = 0; trial < trials; trial ++) {
    uint32_t seed{1234+uint32_t(trial)};
    if ((trial % 100) == 0) { std::cout << "."; std::cout.flush(); }
    turbo::tests::helpers::RandomInt random(0x0000, 0x07ff, seed); // range for 1 or 2 UTF-8 bytes

    auto procedure = [&implementation](const char32_t* utf32, size_t size, char* utf8) -> size_t {
      return implementation.ConvertValidUtf32ToUtf8(utf32, size, utf8);
    };

    for (size_t size: input_size) {
      transcode_utf32_to_utf8_test_base test(random, size);
      ASSERT_TRUE(test(procedure));
    }
  }
}

TEST(convert_into_1_or_2_or_3_UTF8_bytes) {
  for(size_t trial = 0; trial < trials; trial ++) {
    if ((trial % 100) == 0) { std::cout << "."; std::cout.flush(); }
    // range for 1, 2 or 3 UTF-8 bytes
    turbo::tests::helpers::RandomIntRanges random({{0x0000, 0x007f},
                                                     {0x0080, 0x07ff},
                                                     {0x0800, 0xd7ff},
                                                     {0xe000, 0xffff}}, 0);

    auto procedure = [&implementation](const char32_t* utf32, size_t size, char* utf8) -> size_t {
      return implementation.ConvertValidUtf32ToUtf8(utf32, size, utf8);
    };

    for (size_t size: input_size) {
      transcode_utf32_to_utf8_test_base test(random, size);
      ASSERT_TRUE(test(procedure));
    }
  }
}

TEST(convert_into_3_or_4_UTF8_bytes) {
  for(size_t trial = 0; trial < trials; trial ++) {
    if ((trial % 100) == 0) { std::cout << "."; std::cout.flush(); }
    // range for 3 or 4 UTF-8 bytes
    turbo::tests::helpers::RandomIntRanges random({{0x0800, 0xd800-1},
                                                     {0xe000, 0x10ffff}}, 0);

    auto procedure = [&implementation](const char32_t* utf32, size_t size, char* utf8) -> size_t {
      return implementation.ConvertValidUtf32ToUtf8(utf32, size, utf8);
    };

    for (size_t size: input_size) {
      transcode_utf32_to_utf8_test_base test(random, size);
      ASSERT_TRUE(test(procedure));
    }
  }
}

int main(int argc, char* argv[]) {
  return turbo::test::main(argc, argv);
}
