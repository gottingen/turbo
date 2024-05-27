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

#include <turbo/random/seed_sequences.h>

#include <iterator>
#include <random>

#include <gmock/gmock.h>
#include <gtest/gtest.h>
#include <turbo/random/internal/nonsecure_base.h>
#include <turbo/random/random.h>
namespace {

TEST(SeedSequences, Examples) {
  {
    turbo::SeedSeq seed_seq({1, 2, 3});
    turbo::BitGen bitgen(seed_seq);

    EXPECT_NE(0, bitgen());
  }
  {
    turbo::BitGen engine;
    auto seed_seq = turbo::CreateSeedSeqFrom(&engine);
    turbo::BitGen bitgen(seed_seq);

    EXPECT_NE(engine(), bitgen());
  }
  {
    auto seed_seq = turbo::MakeSeedSeq();
    std::mt19937 random(seed_seq);

    EXPECT_NE(0, random());
  }
}

TEST(CreateSeedSeqFrom, CompatibleWithStdTypes) {
  using ExampleNonsecureURBG =
      turbo::random_internal::NonsecureURBGBase<std::minstd_rand0>;

  // Construct a URBG instance.
  ExampleNonsecureURBG rng;

  // Construct a Seed Sequence from its variates.
  auto seq_from_rng = turbo::CreateSeedSeqFrom(&rng);

  // Ensure that another URBG can be validly constructed from the Seed Sequence.
  std::mt19937_64{seq_from_rng};
}

TEST(CreateSeedSeqFrom, CompatibleWithBitGenerator) {
  // Construct a URBG instance.
  turbo::BitGen rng;

  // Construct a Seed Sequence from its variates.
  auto seq_from_rng = turbo::CreateSeedSeqFrom(&rng);

  // Ensure that another URBG can be validly constructed from the Seed Sequence.
  std::mt19937_64{seq_from_rng};
}

TEST(CreateSeedSeqFrom, CompatibleWithInsecureBitGen) {
  // Construct a URBG instance.
  turbo::InsecureBitGen rng;

  // Construct a Seed Sequence from its variates.
  auto seq_from_rng = turbo::CreateSeedSeqFrom(&rng);

  // Ensure that another URBG can be validly constructed from the Seed Sequence.
  std::mt19937_64{seq_from_rng};
}

TEST(CreateSeedSeqFrom, CompatibleWithRawURBG) {
  // Construct a URBG instance.
  std::random_device urandom;

  // Construct a Seed Sequence from its variates, using 64b of seed-material.
  auto seq_from_rng = turbo::CreateSeedSeqFrom(&urandom);

  // Ensure that another URBG can be validly constructed from the Seed Sequence.
  std::mt19937_64{seq_from_rng};
}

template <typename URBG>
void TestReproducibleVariateSequencesForNonsecureURBG() {
  const size_t kNumVariates = 1000;

  URBG rng;
  // Reused for both RNG instances.
  auto reusable_seed = turbo::CreateSeedSeqFrom(&rng);

  typename URBG::result_type variates[kNumVariates];
  {
    URBG child(reusable_seed);
    for (auto& variate : variates) {
      variate = child();
    }
  }
  // Ensure that variate-sequence can be "replayed" by identical RNG.
  {
    URBG child(reusable_seed);
    for (auto& variate : variates) {
      ASSERT_EQ(variate, child());
    }
  }
}

TEST(CreateSeedSeqFrom, ReproducesVariateSequencesForInsecureBitGen) {
  TestReproducibleVariateSequencesForNonsecureURBG<turbo::InsecureBitGen>();
}

TEST(CreateSeedSeqFrom, ReproducesVariateSequencesForBitGenerator) {
  TestReproducibleVariateSequencesForNonsecureURBG<turbo::BitGen>();
}
}  // namespace
