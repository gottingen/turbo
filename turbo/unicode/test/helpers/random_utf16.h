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

#pragma once

#include <cstddef>
#include <cstdint>
#include <random>

namespace turbo {
namespace tests {
namespace helpers {

  /*
    Generates valid random UTF-16LE

    It might generate streams consisting:
    - only single 16-bit words (random_utf16(..., 1, 0));
    - only surrogate pairs, two 16-bit words (random_utf16(..., 0, 1))
    - mixed, depending on given probabilities (random_utf16(..., 1, 1))
  */
  class random_utf16 {
    std::mt19937 gen;

  public:
    random_utf16(uint32_t seed, int single_word_prob, int two_words_probability)
      : gen{seed}
      , utf16_length({double(single_word_prob),
                      double(single_word_prob),
                      double(2 * two_words_probability)}) {}

    std::vector<char16_t> generate(size_t size);
    std::vector<char16_t> generate(size_t size, long seed);
    std::pair<std::vector<char16_t>,size_t> generate_counted(size_t size);
  private:
    std::discrete_distribution<> utf16_length;
    std::uniform_int_distribution<uint32_t> single_word0{0x00000000, 0x0000d7ff};
    std::uniform_int_distribution<uint32_t> single_word1{0x0000e000, 0x0000ffff};
    std::uniform_int_distribution<uint32_t> two_words   {0x00010000, 0x0010ffff};
    uint32_t generate();
  };

} // namespace helpers
} // namespace tests
} // namespace turbo
