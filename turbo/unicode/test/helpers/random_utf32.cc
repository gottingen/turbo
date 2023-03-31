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

#include "random_utf32.h"

#include <stdexcept>

namespace turbo {
namespace tests {
namespace helpers {

  std::vector<char32_t> random_utf32::generate(size_t size) {

    std::vector<char32_t> result;
    result.reserve(size);

    size_t count{0};
    for(;count < size; count++) {
      const uint32_t value = generate();
      result.push_back(value);
    }

    return result;
  }


  std::vector<char32_t> random_utf32::generate(size_t size, long seed) {
    gen.seed(seed);
    return generate(size);
  }

  uint32_t random_utf32::generate() {
    switch (range(gen)) {
      case 0:
        return first_range(gen);
      case 1:
        return second_range(gen);
      default:
        abort();
    }
  }

} // namespace helpers
} // namespace tests
} // namespace turbo
