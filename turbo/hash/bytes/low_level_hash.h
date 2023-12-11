// Copyright 2020 The Turbo Authors
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//     https://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.
//
// This file provides the Google-internal implementation of LowLevelHash.
//
// LowLevelHash is a fast hash function for hash tables, the fastest we've
// currently (late 2020) found that passes the SMHasher tests. The algorithm
// relies on intrinsic 128-bit multiplication for speed. This is not meant to be
// secure - just fast.
//
// It is closely based on a version of wyhash, but does not maintain or
// guarantee future compatibility with it.

#ifndef TURBO_HASH_BYTES_LOW_LEVEL_HASH_H_
#define TURBO_HASH_BYTES_LOW_LEVEL_HASH_H_

#include <stdint.h>
#include <stdlib.h>

#include "turbo/platform/port.h"
#include "turbo/hash/fwd.h"

namespace turbo::hash_internal {

    // Hash function for a byte array. A 64-bit seed and a set of five 64-bit
    // integers are hashed into the result.
    //
    // To allow all hashable types (including std::string_view and Span) to depend on
    // this algorithm, we keep the API low-level, with as few dependencies as
    // possible.
    uint64_t LowLevelHash(const void *data, size_t len, uint64_t seed,
                          const uint64_t salt[5]);

}  // namespace turbo::hash_internal

namespace turbo {
    namespace bytes_internal {
        constexpr uint64_t kDefaultHashSalt[5] = {
                uint64_t{0x243F6A8885A308D3}, uint64_t{0x13198A2E03707344},
                uint64_t{0xA4093822299F31D0}, uint64_t{0x082EFA98EC4E6C89},
                uint64_t{0x452821E638D01377},
        };
    } // namespace bytes_internal

    struct bytes_hash_tag {

        static constexpr const char* name() {
            return "bytes_hash";
        }

        constexpr static bool available() {
            return true;
        }
    };

    template <>
    struct hasher_engine<bytes_hash_tag> {

        static size_t hash64(const char *s, size_t len);

        static size_t hash64_with_seed(const char *s, size_t len, uint64_t seed);
    };

    inline size_t hasher_engine<bytes_hash_tag>::hash64(const char *s, size_t len) {
        return hash_internal::LowLevelHash(s, len, 0, bytes_internal::kDefaultHashSalt);
    }

    inline size_t hasher_engine<bytes_hash_tag>::hash64_with_seed(const char *s, size_t len, uint64_t seed) {
        return hash_internal::LowLevelHash(s, len, seed, bytes_internal::kDefaultHashSalt);
    }
}
#endif  // TURBO_HASH_BYTES_LOW_LEVEL_HASH_H_
