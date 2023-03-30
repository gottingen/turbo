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

#include "random_int.h"

#include <stdexcept>

namespace simdutf { namespace tests { namespace helpers {

    RandomInt::RandomInt(uint64_t lo, uint64_t hi, uint64_t seed) noexcept
      : gen(std::mt19937::result_type(seed)), distr{lo, hi} {}

    uint32_t RandomInt::operator()() noexcept {
      return uint32_t(distr(gen));
    }

    RandomIntRanges::RandomIntRanges(std::initializer_list<std::pair<uint64_t, uint64_t>> ranges_, uint64_t seed) noexcept
      : gen(std::mt19937::result_type(seed)) {

      for (const auto& lohi: ranges_) {
        ranges.emplace_back(lohi.first, lohi.second);
      }

      range_index = Distribution(0, ranges.size() - 1);
    }

    uint32_t RandomIntRanges::operator()() noexcept {
      const size_t index = size_t(range_index(gen));
      return uint32_t(ranges[index](gen));
    }
}}}
