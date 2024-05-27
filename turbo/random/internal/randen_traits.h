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

#ifndef TURBO_RANDOM_INTERNAL_RANDEN_TRAITS_H_
#define TURBO_RANDOM_INTERNAL_RANDEN_TRAITS_H_

// HERMETIC NOTE: The randen_hwaes target must not introduce duplicate
// symbols from arbitrary system and other headers, since it may be built
// with different flags from other targets, using different levels of
// optimization, potentially introducing ODR violations.

#include <cstddef>

#include <turbo/base/config.h>

namespace turbo {
TURBO_NAMESPACE_BEGIN
namespace random_internal {

// RANDen = RANDom generator or beetroots in Swiss High German.
// 'Strong' (well-distributed, unpredictable, backtracking-resistant) random
// generator, faster in some benchmarks than std::mt19937_64 and pcg64_c32.
//
// High-level summary:
// 1) Reverie (see "A Robust and Sponge-Like PRNG with Improved Efficiency") is
//    a sponge-like random generator that requires a cryptographic permutation.
//    It improves upon "Provably Robust Sponge-Based PRNGs and KDFs" by
//    achieving backtracking resistance with only one Permute() per buffer.
//
// 2) "Simpira v2: A Family of Efficient Permutations Using the AES Round
//    Function" constructs up to 1024-bit permutations using an improved
//    Generalized Feistel network with 2-round AES-128 functions. This Feistel
//    block shuffle achieves diffusion faster and is less vulnerable to
//    sliced-biclique attacks than the Type-2 cyclic shuffle.
//
// 3) "Improving the Generalized Feistel" and "New criterion for diffusion
//    property" extends the same kind of improved Feistel block shuffle to 16
//    branches, which enables a 2048-bit permutation.
//
// Combine these three ideas and also change Simpira's subround keys from
// structured/low-entropy counters to digits of Pi (or other random source).

// RandenTraits contains the basic algorithm traits, such as the size of the
// state, seed, sponge, etc.
struct RandenTraits {
  // Size of the entire sponge / state for the randen PRNG.
  static constexpr size_t kStateBytes = 256;  // 2048-bit

  // Size of the 'inner' (inaccessible) part of the sponge. Larger values would
  // require more frequent calls to RandenGenerate.
  static constexpr size_t kCapacityBytes = 16;  // 128-bit

  // Size of the default seed consumed by the sponge.
  static constexpr size_t kSeedBytes = kStateBytes - kCapacityBytes;

  // Assuming 128-bit blocks, the number of blocks in the state.
  // Largest size for which security proofs are known.
  static constexpr size_t kFeistelBlocks = 16;

  // Ensures SPRP security and two full subblock diffusions.
  // Must be > 4 * log2(kFeistelBlocks).
  static constexpr size_t kFeistelRounds = 16 + 1;

  // Size of the key. A 128-bit key block is used for every-other
  // feistel block (Type-2 generalized Feistel network) in each round.
  static constexpr size_t kKeyBytes = 16 * kFeistelRounds * kFeistelBlocks / 2;
};

// Randen key arrays. In randen_round_keys.cc
extern const unsigned char kRandenRoundKeys[RandenTraits::kKeyBytes];
extern const unsigned char kRandenRoundKeysBE[RandenTraits::kKeyBytes];

}  // namespace random_internal
TURBO_NAMESPACE_END
}  // namespace turbo

#endif  // TURBO_RANDOM_INTERNAL_RANDEN_TRAITS_H_
