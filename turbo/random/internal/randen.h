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

#ifndef TURBO_RANDOM_INTERNAL_RANDEN_H_
#define TURBO_RANDOM_INTERNAL_RANDEN_H_

#include <cstddef>

#include <turbo/random/internal/platform.h>
#include <turbo/random/internal/randen_hwaes.h>
#include <turbo/random/internal/randen_slow.h>
#include <turbo/random/internal/randen_traits.h>

namespace turbo {
TURBO_NAMESPACE_BEGIN
namespace random_internal {

// RANDen = RANDom generator or beetroots in Swiss High German.
// 'Strong' (well-distributed, unpredictable, backtracking-resistant) random
// generator, faster in some benchmarks than std::mt19937_64 and pcg64_c32.
//
// Randen implements the basic state manipulation methods.
class Randen {
 public:
  static constexpr size_t kStateBytes = RandenTraits::kStateBytes;
  static constexpr size_t kCapacityBytes = RandenTraits::kCapacityBytes;
  static constexpr size_t kSeedBytes = RandenTraits::kSeedBytes;

  ~Randen() = default;

  Randen();

  // Generate updates the randen sponge. The outer portion of the sponge
  // (kCapacityBytes .. kStateBytes) may be consumed as PRNG state.
  // REQUIRES: state points to kStateBytes of state.
  inline void Generate(void* state) const {
#if TURBO_RANDOM_INTERNAL_AES_DISPATCH
    // HW AES Dispatch.
    if (has_crypto_) {
      RandenHwAes::Generate(keys_, state);
    } else {
      RandenSlow::Generate(keys_, state);
    }
#elif TURBO_HAVE_ACCELERATED_AES
    // HW AES is enabled.
    RandenHwAes::Generate(keys_, state);
#else
    // HW AES is disabled.
    RandenSlow::Generate(keys_, state);
#endif
  }

  // Absorb incorporates additional seed material into the randen sponge.  After
  // absorb returns, Generate must be called before the state may be consumed.
  // REQUIRES: seed points to kSeedBytes of seed.
  // REQUIRES: state points to kStateBytes of state.
  inline void Absorb(const void* seed, void* state) const {
#if TURBO_RANDOM_INTERNAL_AES_DISPATCH
    // HW AES Dispatch.
    if (has_crypto_) {
      RandenHwAes::Absorb(seed, state);
    } else {
      RandenSlow::Absorb(seed, state);
    }
#elif TURBO_HAVE_ACCELERATED_AES
    // HW AES is enabled.
    RandenHwAes::Absorb(seed, state);
#else
    // HW AES is disabled.
    RandenSlow::Absorb(seed, state);
#endif
  }

 private:
  const void* keys_;
#if TURBO_RANDOM_INTERNAL_AES_DISPATCH
  bool has_crypto_;
#endif
};

}  // namespace random_internal
TURBO_NAMESPACE_END
}  // namespace turbo

#endif  // TURBO_RANDOM_INTERNAL_RANDEN_H_
