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


#ifndef TURBO_HASH_HASH_H_
#define TURBO_HASH_HASH_H_

#include "turbo/hash/mix/simple_mix.h"
#include "turbo/hash/mix/murmur_mix.h"
#include "turbo/hash/city/city.h"
#include "turbo/hash/bytes/low_level_hash.h"

namespace turbo {

    using default_mixer = simple_mix;

    template<int N, typename E>
    struct hash_mixer_traits {
        using type = typename std::conditional<N == 4,
                typename E::mix4, typename E::mix8>::type;
    };

    template<int N, typename R = size_t, typename Engine = default_mixer>
    struct hash_mixer {
        static_assert(sizeof(R) >= N, "R must be larger than N");
        static_assert(N == 4 || N == 8, "N must be 4 or 8");
        constexpr R operator()(size_t key) const {
            using engine_type = typename hash_mixer_traits<N, Engine>::type;
            return static_cast<R>(engine_type::mix(key));
        }
    };

    template<typename R, typename Engine = default_mixer>
    constexpr typename std::enable_if_t<!std::is_same_v<R, void> && std::is_integral_v<R> &&!is_mix_engine<R>::value ,R>
            hash_mixer4(size_t key) {
        return hash_mixer<4, R, Engine>()(key);
    }

    template<typename Engine = default_mixer, std::enable_if_t<is_mix_engine<Engine>::value , int> = 0>
    constexpr size_t hash_mixer4(size_t key) {
        return hash_mixer<4, size_t, Engine>()(key);
    }


    template<typename R, typename Engine = default_mixer>
    constexpr typename std::enable_if_t<!std::is_same_v<R, void> && std::is_integral_v<R> &&!is_mix_engine<R>::value ,R>
    hash_mixer8(size_t key) {
        return hash_mixer<8, R, Engine>()(key);
    }

    template<typename Engine = default_mixer, std::enable_if_t<is_mix_engine<Engine>::value , int> = 0>
    constexpr size_t hash_mixer8(size_t key) {
        return hash_mixer<8, size_t, Engine>()(key);
    }


}  // namespace turbo

#endif  // TURBO_HASH_HASH_H_
