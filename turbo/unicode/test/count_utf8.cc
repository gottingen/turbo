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

#include <algorithm>
#include <array>
#include <iostream>
#include <random>
#include <stdexcept>

#include <turbo/unicode/test/helpers/random_int.h>
#include <turbo/unicode/test/helpers/transcode_test_base.h>
#include <turbo/unicode/test/helpers/random_utf8.h>
#include <turbo/unicode/test/helpers/test.h>

namespace {
std::array<size_t, 9> input_size{7, 12, 16, 64, 67, 128, 256, 511, 1000};

using simdutf::tests::helpers::transcode_utf8_to_utf16_test_base;
} // namespace

TEST(count_pure_ASCII) {
  for (size_t trial = 0; trial < 10000; trial++) {
    if ((trial % 100) == 0) {
      std::cout << ".";
      std::cout.flush();
    }
    uint32_t seed{1234};

    simdutf::tests::helpers::random_utf8 random(seed, 1, 0, 0, 0);

    for (size_t size : input_size) {
      auto generated = random.generate_counted(size);
      ASSERT_TRUE(implementation.count_utf8(
                      reinterpret_cast<const char *>(generated.first.data()),
                      size) == generated.second);
    }
  }
}

TEST(count_1_or_2_UTF8_bytes) {
  for (size_t trial = 0; trial < 10000; trial++) {
    if ((trial % 100) == 0) {
      std::cout << ".";
      std::cout.flush();
    }
    uint32_t seed{1234};

    simdutf::tests::helpers::random_utf8 random(seed, 1, 1, 0, 0);

    for (size_t size : input_size) {
      auto generated = random.generate_counted(size);
      ASSERT_TRUE(implementation.count_utf8(
                      reinterpret_cast<const char *>(generated.first.data()),
                      size) == generated.second);
    }
  }
}

TEST(count_1_or_2_or_3_UTF8_bytes) {
  for (size_t trial = 0; trial < 10000; trial++) {
    if ((trial % 100) == 0) {
      std::cout << ".";
      std::cout.flush();
    }
    uint32_t seed{1234};

    simdutf::tests::helpers::random_utf8 random(seed, 1, 1, 1, 0);

    for (size_t size : input_size) {
      auto generated = random.generate_counted(size);
      ASSERT_TRUE(implementation.count_utf8(
                      reinterpret_cast<const char *>(generated.first.data()),
                      size) == generated.second);
    }
  }
}

TEST(count_1_2_3_or_4_UTF8_bytes) {
  for (size_t trial = 0; trial < 10000; trial++) {
    uint32_t seed{1234};

    simdutf::tests::helpers::random_utf8 random(seed, 1, 1, 1, 1);

    for (size_t size : input_size) {
      auto generated = random.generate_counted(size);
      ASSERT_TRUE(implementation.count_utf8(
                      reinterpret_cast<const char *>(generated.first.data()),
                      size) == generated.second);
    }
  }
}

int main(int argc, char* argv[]) {
  return simdutf::test::main(argc, argv);
}