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

#ifndef TURBO_RANDOM_INTERNAL_SEED_MATERIAL_H_
#define TURBO_RANDOM_INTERNAL_SEED_MATERIAL_H_

#include <cassert>
#include <cstdint>
#include <cstdlib>
#include <string>
#include <vector>

#include <turbo/base/attributes.h>
#include <turbo/random/internal/fast_uniform_bits.h>
#include <turbo/types/optional.h>
#include <turbo/container/span.h>

namespace turbo {
TURBO_NAMESPACE_BEGIN
namespace random_internal {

// Returns the number of 32-bit blocks needed to contain the given number of
// bits.
constexpr size_t SeedBitsToBlocks(size_t seed_size) {
  return (seed_size + 31) / 32;
}

// Amount of entropy (measured in bits) used to instantiate a Seed Sequence,
// with which to create a URBG.
constexpr size_t kEntropyBitsNeeded = 256;

// Amount of entropy (measured in 32-bit blocks) used to instantiate a Seed
// Sequence, with which to create a URBG.
constexpr size_t kEntropyBlocksNeeded =
    random_internal::SeedBitsToBlocks(kEntropyBitsNeeded);

static_assert(kEntropyBlocksNeeded > 0,
              "Entropy used to seed URBGs must be nonzero.");

// Attempts to fill a span of uint32_t-values using an OS-provided source of
// true entropy (eg. /dev/urandom) into an array of uint32_t blocks of data. The
// resulting array may be used to initialize an instance of a class conforming
// to the C++ Standard "Seed Sequence" concept [rand.req.seedseq].
//
// If values.data() == nullptr, the behavior is undefined.
TURBO_MUST_USE_RESULT
bool ReadSeedMaterialFromOSEntropy(turbo::Span<uint32_t> values);

// Attempts to fill a span of uint32_t-values using variates generated by an
// existing instance of a class conforming to the C++ Standard "Uniform Random
// Bit Generator" concept [rand.req.urng]. The resulting data may be used to
// initialize an instance of a class conforming to the C++ Standard
// "Seed Sequence" concept [rand.req.seedseq].
//
// If urbg == nullptr or values.data() == nullptr, the behavior is undefined.
template <typename URBG>
TURBO_MUST_USE_RESULT bool ReadSeedMaterialFromURBG(
    URBG* urbg, turbo::Span<uint32_t> values) {
  random_internal::FastUniformBits<uint32_t> distr;

  assert(urbg != nullptr && values.data() != nullptr);
  if (urbg == nullptr || values.data() == nullptr) {
    return false;
  }

  for (uint32_t& seed_value : values) {
    seed_value = distr(*urbg);
  }
  return true;
}

// Mixes given sequence of values with into given sequence of seed material.
// Time complexity of this function is O(sequence.size() *
// seed_material.size()).
//
// Algorithm is based on code available at
// https://gist.github.com/imneme/540829265469e673d045
// by Melissa O'Neill.
void MixIntoSeedMaterial(turbo::Span<const uint32_t> sequence,
                         turbo::Span<uint32_t> seed_material);

// Returns salt value.
//
// Salt is obtained only once and stored in static variable.
//
// May return empty value if optaining the salt was not possible.
turbo::optional<uint32_t> GetSaltMaterial();

}  // namespace random_internal
TURBO_NAMESPACE_END
}  // namespace turbo

#endif  // TURBO_RANDOM_INTERNAL_SEED_MATERIAL_H_
