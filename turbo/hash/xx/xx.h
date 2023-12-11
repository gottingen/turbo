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


#ifndef TURBO_HASH_XX_XX_H_
#define TURBO_HASH_XX_XX_H_

#include <cstdint>
#include <cstdlib>  // for size_t.

#include <utility>

#include "turbo/platform/port.h"
#include "turbo/hash/fwd.h"

namespace turbo {

    struct xx_hash_tag {

        static constexpr const char* name() {
            return "city_hash";
        }

        constexpr static bool available() {
            return true;
        }
    };

    template <>
    struct hasher_engine<xx_hash_tag> {

        static uint32_t hash32(const char *s, size_t len);

        static size_t hash64(const char *s, size_t len);

        static size_t hash64_with_seed(const char *s, size_t len, uint64_t seed);
    };

}  // namespace turbo

#endif  // TURBO_HASH_XX_XX_H_
