// Copyright (C) 2024 EA group inc.
// Author: Jeff.li lijippy@163.com
// All rights reserved.
// This program is free software: you can redistribute it and/or modify
// it under the terms of the GNU Affero General Public License as published
// by the Free Software Foundation, either version 3 of the License, or
// (at your option) any later version.
//
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU Affero General Public License for more details.
//
// You should have received a copy of the GNU Affero General Public License
// along with this program.  If not, see <https://www.gnu.org/licenses/>.
//

#include <turbo/hash/internal/hash.h>

namespace turbo::hash_internal {

    uint64_t MixingHashState::CombineLargeContiguousImpl32(
            uint64_t state, const unsigned char *first, size_t len) {
        while (len >= PiecewiseChunkSize()) {
            state = Mix(state,
                        hash_internal::CityHash32(reinterpret_cast<const char *>(first),
                                                  PiecewiseChunkSize()));
            len -= PiecewiseChunkSize();
            first += PiecewiseChunkSize();
        }
        // Handle the remainder.
        return CombineContiguousImpl(state, first, len,
                                     std::integral_constant<int, 4>{});
    }

    uint64_t MixingHashState::CombineLargeContiguousImpl64(
            uint64_t state, const unsigned char *first, size_t len) {
        while (len >= PiecewiseChunkSize()) {
            state = Mix(state, Hash64(first, PiecewiseChunkSize()));
            len -= PiecewiseChunkSize();
            first += PiecewiseChunkSize();
        }
        // Handle the remainder.
        return CombineContiguousImpl(state, first, len,
                                     std::integral_constant<int, 8>{});
    }

    TURBO_CONST_INIT const void *const MixingHashState::kSeed = &kSeed;

    // The salt array used by LowLevelHash. This array is NOT the mechanism used to
    // make turbo::Hash non-deterministic between program invocations.  See `Seed()`
    // for that mechanism.
    //
    // Any random values are fine. These values are just digits from the decimal
    // part of pi.
    // https://en.wikipedia.org/wiki/Nothing-up-my-sleeve_number
    constexpr uint64_t kHashSalt[5] = {
            uint64_t{0x243F6A8885A308D3}, uint64_t{0x13198A2E03707344},
            uint64_t{0xA4093822299F31D0}, uint64_t{0x082EFA98EC4E6C89},
            uint64_t{0x452821E638D01377},
    };

    uint64_t MixingHashState::LowLevelHashImpl(const unsigned char *data,
                                               size_t len) {
        return LowLevelHashLenGt16(data, len, Seed(), kHashSalt);
    }

}  // namespace turbo::hash_internal
