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

namespace simdutf {
namespace tests {
namespace helpers {

  constexpr int32_t number_code_points = 0x0010ffff - (0xdfff - 0xd800);
  constexpr int32_t length_first_range = 0x0000d7ff;
  constexpr int32_t length_second_range = 0x0010ffff - 0x0000e000;
  /*
    Generates valid random UTF-32
  */
  class random_utf32 {
    std::mt19937 gen;

  public:
    random_utf32(uint32_t seed)
      : gen{seed},
        range({double(length_first_range) / double(number_code_points), double(length_second_range) / double(number_code_points)}) {}
    // Uniformly randomize over the two ranges

    std::vector<char32_t> generate(size_t size);
    std::vector<char32_t> generate(size_t size, long seed);
  private:
    std::discrete_distribution<> range;
    std::uniform_int_distribution<uint32_t> first_range{0x00000000, 0x0000d7ff};
    std::uniform_int_distribution<uint32_t> second_range{0x0000e000, 0x0010ffff};
    uint32_t generate();
  };

} // namespace helpers
} // namespace tests
} // namespace simdutf
