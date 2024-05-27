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

#include <turbo/random/internal/seed_material.h>

#include <bitset>
#include <cstdlib>
#include <cstring>
#include <random>

#include <gmock/gmock.h>
#include <gtest/gtest.h>

#ifdef __ANDROID__
// Android assert messages only go to system log, so death tests cannot inspect
// the message for matching.
#define TURBO_EXPECT_DEATH_IF_SUPPORTED(statement, regex) \
  EXPECT_DEATH_IF_SUPPORTED(statement, ".*")
#else
#define TURBO_EXPECT_DEATH_IF_SUPPORTED(statement, regex) \
  EXPECT_DEATH_IF_SUPPORTED(statement, regex)
#endif

namespace {

using testing::Each;
using testing::ElementsAre;
using testing::Eq;
using testing::Ne;
using testing::Pointwise;

TEST(SeedBitsToBlocks, VerifyCases) {
  EXPECT_EQ(0, turbo::random_internal::SeedBitsToBlocks(0));
  EXPECT_EQ(1, turbo::random_internal::SeedBitsToBlocks(1));
  EXPECT_EQ(1, turbo::random_internal::SeedBitsToBlocks(31));
  EXPECT_EQ(1, turbo::random_internal::SeedBitsToBlocks(32));
  EXPECT_EQ(2, turbo::random_internal::SeedBitsToBlocks(33));
  EXPECT_EQ(4, turbo::random_internal::SeedBitsToBlocks(127));
  EXPECT_EQ(4, turbo::random_internal::SeedBitsToBlocks(128));
  EXPECT_EQ(5, turbo::random_internal::SeedBitsToBlocks(129));
}

TEST(ReadSeedMaterialFromOSEntropy, SuccessiveReadsAreDistinct) {
  constexpr size_t kSeedMaterialSize = 64;
  uint32_t seed_material_1[kSeedMaterialSize] = {};
  uint32_t seed_material_2[kSeedMaterialSize] = {};

  EXPECT_TRUE(turbo::random_internal::ReadSeedMaterialFromOSEntropy(
      turbo::Span<uint32_t>(seed_material_1, kSeedMaterialSize)));
  EXPECT_TRUE(turbo::random_internal::ReadSeedMaterialFromOSEntropy(
      turbo::Span<uint32_t>(seed_material_2, kSeedMaterialSize)));

  EXPECT_THAT(seed_material_1, Pointwise(Ne(), seed_material_2));
}

TEST(ReadSeedMaterialFromOSEntropy, ReadZeroBytesIsNoOp) {
  uint32_t seed_material[32] = {};
  std::memset(seed_material, 0xAA, sizeof(seed_material));
  EXPECT_TRUE(turbo::random_internal::ReadSeedMaterialFromOSEntropy(
      turbo::Span<uint32_t>(seed_material, 0)));

  EXPECT_THAT(seed_material, Each(Eq(0xAAAAAAAA)));
}

TEST(ReadSeedMaterialFromOSEntropy, NullPtrVectorArgument) {
#ifdef NDEBUG
  EXPECT_FALSE(turbo::random_internal::ReadSeedMaterialFromOSEntropy(
      turbo::Span<uint32_t>(nullptr, 32)));
#else
  bool result;
  TURBO_EXPECT_DEATH_IF_SUPPORTED(
      result = turbo::random_internal::ReadSeedMaterialFromOSEntropy(
          turbo::Span<uint32_t>(nullptr, 32)),
      "!= nullptr");
  (void)result;  // suppress unused-variable warning
#endif
}

TEST(ReadSeedMaterialFromURBG, SeedMaterialEqualsVariateSequence) {
  // Two default-constructed instances of std::mt19937_64 are guaranteed to
  // produce equal variate-sequences.
  std::mt19937 urbg_1;
  std::mt19937 urbg_2;
  constexpr size_t kSeedMaterialSize = 1024;
  uint32_t seed_material[kSeedMaterialSize] = {};

  EXPECT_TRUE(turbo::random_internal::ReadSeedMaterialFromURBG(
      &urbg_1, turbo::Span<uint32_t>(seed_material, kSeedMaterialSize)));
  for (uint32_t seed : seed_material) {
    EXPECT_EQ(seed, urbg_2());
  }
}

TEST(ReadSeedMaterialFromURBG, ReadZeroBytesIsNoOp) {
  std::mt19937_64 urbg;
  uint32_t seed_material[32];
  std::memset(seed_material, 0xAA, sizeof(seed_material));
  EXPECT_TRUE(turbo::random_internal::ReadSeedMaterialFromURBG(
      &urbg, turbo::Span<uint32_t>(seed_material, 0)));

  EXPECT_THAT(seed_material, Each(Eq(0xAAAAAAAA)));
}

TEST(ReadSeedMaterialFromURBG, NullUrbgArgument) {
  constexpr size_t kSeedMaterialSize = 32;
  uint32_t seed_material[kSeedMaterialSize];
#ifdef NDEBUG
  EXPECT_FALSE(turbo::random_internal::ReadSeedMaterialFromURBG<std::mt19937_64>(
      nullptr, turbo::Span<uint32_t>(seed_material, kSeedMaterialSize)));
#else
  bool result;
  TURBO_EXPECT_DEATH_IF_SUPPORTED(
      result = turbo::random_internal::ReadSeedMaterialFromURBG<std::mt19937_64>(
          nullptr, turbo::Span<uint32_t>(seed_material, kSeedMaterialSize)),
      "!= nullptr");
  (void)result;  // suppress unused-variable warning
#endif
}

TEST(ReadSeedMaterialFromURBG, NullPtrVectorArgument) {
  std::mt19937_64 urbg;
#ifdef NDEBUG
  EXPECT_FALSE(turbo::random_internal::ReadSeedMaterialFromURBG(
      &urbg, turbo::Span<uint32_t>(nullptr, 32)));
#else
  bool result;
  TURBO_EXPECT_DEATH_IF_SUPPORTED(
      result = turbo::random_internal::ReadSeedMaterialFromURBG(
          &urbg, turbo::Span<uint32_t>(nullptr, 32)),
      "!= nullptr");
  (void)result;  // suppress unused-variable warning
#endif
}

// The avalanche effect is a desirable cryptographic property of hashes in which
// changing a single bit in the input causes each bit of the output to be
// changed with probability near 50%.
//
// https://en.wikipedia.org/wiki/Avalanche_effect

TEST(MixSequenceIntoSeedMaterial, AvalancheEffectTestOneBitLong) {
  std::vector<uint32_t> seed_material = {1, 2, 3, 4, 5, 6, 7, 8};

  // For every 32-bit number with exactly one bit set, verify the avalanche
  // effect holds.  In order to reduce flakiness of tests, accept values
  // anywhere in the range of 30%-70%.
  for (uint32_t v = 1; v != 0; v <<= 1) {
    std::vector<uint32_t> seed_material_copy = seed_material;
    turbo::random_internal::MixIntoSeedMaterial(
        turbo::Span<uint32_t>(&v, 1),
        turbo::Span<uint32_t>(seed_material_copy.data(),
                             seed_material_copy.size()));

    uint32_t changed_bits = 0;
    for (size_t i = 0; i < seed_material.size(); i++) {
      std::bitset<sizeof(uint32_t) * 8> bitset(seed_material[i] ^
                                               seed_material_copy[i]);
      changed_bits += bitset.count();
    }

    EXPECT_LE(changed_bits, 0.7 * sizeof(uint32_t) * 8 * seed_material.size());
    EXPECT_GE(changed_bits, 0.3 * sizeof(uint32_t) * 8 * seed_material.size());
  }
}

TEST(MixSequenceIntoSeedMaterial, AvalancheEffectTestOneBitShort) {
  std::vector<uint32_t> seed_material = {1};

  // For every 32-bit number with exactly one bit set, verify the avalanche
  // effect holds.  In order to reduce flakiness of tests, accept values
  // anywhere in the range of 30%-70%.
  for (uint32_t v = 1; v != 0; v <<= 1) {
    std::vector<uint32_t> seed_material_copy = seed_material;
    turbo::random_internal::MixIntoSeedMaterial(
        turbo::Span<uint32_t>(&v, 1),
        turbo::Span<uint32_t>(seed_material_copy.data(),
                             seed_material_copy.size()));

    uint32_t changed_bits = 0;
    for (size_t i = 0; i < seed_material.size(); i++) {
      std::bitset<sizeof(uint32_t) * 8> bitset(seed_material[i] ^
                                               seed_material_copy[i]);
      changed_bits += bitset.count();
    }

    EXPECT_LE(changed_bits, 0.7 * sizeof(uint32_t) * 8 * seed_material.size());
    EXPECT_GE(changed_bits, 0.3 * sizeof(uint32_t) * 8 * seed_material.size());
  }
}

}  // namespace
