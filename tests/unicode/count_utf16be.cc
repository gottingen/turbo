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

#include <algorithm>
#include <array>
#include <iostream>
#include <random>
#include <stdexcept>

#include "turbo/random/random.h"




namespace {
std::array<size_t, 7> input_size{7, 16, 12, 64, 67, 128, 256};

using turbo::tests::helpers::transcode_utf8_to_utf16_test_base;
} // namespace

TEST_CASE("count_just_one_word") {
  for (size_t trial = 0; trial < 10000; trial++) {
    if ((trial % 100) == 0) {
      std::cout << ".";
      std::cout.flush();
    }


    turbo::Utf16Generator random(1, 0);

    for (size_t size : input_size) {
      auto generated = random.generate_counted(size);
      std::vector<char16_t> utf16be(generated.first.size());
      turbo::change_endianness_utf16(reinterpret_cast<const char16_t *>(generated.first.data()), generated.first.size(), utf16be.data());
      size_t count = turbo::count_utf16be(utf16be.data(), size);
      REQUIRE_EQ(count, generated.second);
    }
  }
}
TEST_CASE("count_1_or_2_UTF16_words") {
  for (size_t trial = 0; trial < 10000; trial++) {
    if ((trial % 100) == 0) {
      std::cout << ".";
      std::cout.flush();
    }


    turbo::Utf16Generator random(1, 1);

    for (size_t size : input_size) {
      auto generated = random.generate_counted(size);
      std::vector<char16_t> utf16be(generated.first.size());
      turbo::change_endianness_utf16(reinterpret_cast<const char16_t *>(generated.first.data()), generated.first.size(), utf16be.data());
      size_t count = turbo::count_utf16be(utf16be.data(), size);
      REQUIRE_EQ(count, generated.second);
    }
  }
}

TEST_CASE("count_2_UTF16_words") {
  for (size_t trial = 0; trial < 10000; trial++) {


    turbo::Utf16Generator random(0, 1);

    for (size_t size : input_size) {

      auto generated = random.generate_counted(size);
      std::vector<char16_t> utf16be(generated.first.size());
      turbo::change_endianness_utf16(reinterpret_cast<const char16_t *>(generated.first.data()), generated.first.size(), utf16be.data());
      size_t count = turbo::count_utf16be(utf16be.data(), size);
      REQUIRE_EQ(count, generated.second);
    }
  }
}


