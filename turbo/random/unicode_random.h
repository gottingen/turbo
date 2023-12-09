// Copyright 2023 The Elastic-AI Authors.
// part of Elastic AI Search
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
//

#ifndef TURBO_RANDOM_UNICODE_RANDOM_H_
#define TURBO_RANDOM_UNICODE_RANDOM_H_

#include <cstdint>
#include <vector>
#include "turbo/random/engine.h"
#include "turbo/random/discrete_distribution.h"

namespace turbo {

    class Utf8Random {
    public:
        Utf8Random(uint32_t seed, int prob_1byte, int prob_2bytes,
                   int prob_3bytes, int prob_4bytes);

        std::vector<uint8_t> generate(size_t output_bytes);

        std::pair<std::vector<uint8_t>,size_t> generate_counted(size_t output_bytes);

        std::vector<uint8_t> generate(size_t output_bytes, long seed);
    private:
        BitGen _gen;
        discrete_distribution<> bytes_count;
        uniform_int_distribution<int> val_7bit{0x00, 0x7f}; // 0b0xxxxxxx
        uniform_int_distribution<int> val_6bit{0x00, 0x3f}; // 0b10xxxxxx
        uniform_int_distribution<int> val_5bit{0x00, 0x1f}; // 0b110xxxxx
        uniform_int_distribution<int> val_4bit{0x00, 0x0f}; // 0b1110xxxx
        uniform_int_distribution<int> val_3bit{0x00, 0x07}; // 0b11110xxx
    };
}
#endif  // TURBO_RANDOM_UNICODE_RANDOM_H_
