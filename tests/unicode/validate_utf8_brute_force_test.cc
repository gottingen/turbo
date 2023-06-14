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
#include <cstddef>
#include <cstdint>
#include <random>
#include <iostream>
#include <iomanip>

#include <tests/unicode/helpers/random_utf8.h>
#include <tests/unicode/reference/validate_utf8.h>
#include <tests/unicode/helpers/test.h>

TEST(brute_force) {
  uint32_t seed{1234};
  turbo::tests::helpers::random_utf8 gen_1_2_3_4(seed, 1, 1, 1, 1);
  size_t total = 1000;
  for (size_t i = 0; i < total; i++) {

    auto UTF8 = gen_1_2_3_4.generate(rand() % 256);
    if (!implementation.ValidateUtf8((const char *)UTF8.data(), UTF8.size())) {
      std::cerr << "bug" << std::endl;
      ASSERT_TRUE(false);
    }
    for (size_t flip = 0; flip < 1000; ++flip) {
      // we are going to hack the string as long as it is UTF-8
      const int bitflip{1 << (rand() % 8)};
      UTF8[rand() % UTF8.size()] = uint8_t(bitflip); // we flip exactly one bit
      bool is_ok =
          implementation.ValidateUtf8((const char *)UTF8.data(), UTF8.size());
      bool is_ok_basic =
          turbo::tests::reference::ValidateUtf8((const char *)UTF8.data(), UTF8.size());
      if (is_ok != is_ok_basic) {
        std::cerr << "bug" << std::endl;
        ASSERT_TRUE(false);
      }
    }
  }
  puts("OK");
}


int main(int argc, char* argv[]) {
  return turbo::test::main(argc, argv);
}