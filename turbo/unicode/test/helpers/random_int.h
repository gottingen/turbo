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

#include <random>
#include <vector>
#include <initializer_list>
#include <utility>
#include <cstdint>

namespace simdutf { namespace tests { namespace helpers {

  class RandomInt {
    std::mt19937 gen;
    std::uniform_int_distribution<uint64_t> distr;

  public:
    RandomInt(uint64_t lo, uint64_t hi, uint64_t seed) noexcept;

    uint32_t operator()() noexcept;
  };

  class RandomIntRanges {
    std::mt19937 gen;
    using Distribution = std::uniform_int_distribution<uint64_t>;

    Distribution range_index;
    std::vector<Distribution> ranges;

  public:
    RandomIntRanges(std::initializer_list<std::pair<uint64_t, uint64_t>> ranges, uint64_t seed) noexcept;

    uint32_t operator()() noexcept;
  };

}}}
