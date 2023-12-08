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


#ifndef TURBO_RANDOM_FAST_RANDOM_H_
#define TURBO_RANDOM_FAST_RANDOM_H_

#include <cstdint>
#include <string>
#include "turbo/meta/type_traits.h"

namespace turbo {

    struct FastRandSeed {
        uint64_t s[2];
    };

    class FastRandom {
    public:
        static FastRandom* get_instance() {
            static FastRandom instance;
            return &instance;
        }

        static FastRandom* get_thread_instance() {
            static thread_local FastRandom instance;
            return &instance;
        }
    public:
        FastRandom();

        int64_t generate();

        int64_t generate(uint64_t range);

        int64_t generate_64(int64_t lo, int64_t hi);

        uint64_t generate_u64(uint64_t lo, uint64_t hi);

        double generate_double();

        template<class T = int64_t, std::enable_if_t<std::is_integral_v<T> ||std::is_floating_point_v<T>, int>>
        T generate(T lo, T hi) {
            if constexpr (std::is_unsigned_v<T>) {
                return generate_64((uint64_t)lo, (uint64_t)hi);
            } else {
                return generate_u64((int64_t)lo, (int64_t)hi);
            }
        }
    private:
        FastRandSeed _seed;
    };

}  // namespace turbo

#endif  // TURBO_RANDOM_FAST_RANDOM_H_
