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
//
// -----------------------------------------------------------------------------
// File: seed_sequences.h
// -----------------------------------------------------------------------------
//
// This header contains utilities for creating and working with seed sequences
// conforming to [rand.req.seedseq]. In general, direct construction of seed
// sequences is discouraged, but use-cases for construction of identical bit
// generators (using the same seed sequence) may be helpful (e.g. replaying a
// simulation whose state is derived from variates of a bit generator).

#ifndef TURBO_RANDOM_SEED_SEQUENCES_H_
#define TURBO_RANDOM_SEED_SEQUENCES_H_

#include <iterator>
#include <random>

#include <turbo/base/config.h>
#include <turbo/base/nullability.h>
#include <turbo/random/internal/salted_seed_seq.h>
#include <turbo/random/internal/seed_material.h>
#include <turbo/random/seed_gen_exception.h>
#include <turbo/strings/string_view.h>
#include <turbo/container/span.h>

namespace turbo {
TURBO_NAMESPACE_BEGIN

// -----------------------------------------------------------------------------
// turbo::SeedSeq
// -----------------------------------------------------------------------------
//
// `turbo::SeedSeq` constructs a seed sequence according to [rand.req.seedseq]
// for use within bit generators. `turbo::SeedSeq`, unlike `std::seed_seq`
// additionally salts the generated seeds with extra implementation-defined
// entropy. For that reason, you can use `turbo::SeedSeq` in combination with
// standard library bit generators (e.g. `std::mt19937`) to introduce
// non-determinism in your seeds.
//
// Example:
//
//   turbo::SeedSeq my_seed_seq({a, b, c});
//   std::mt19937 my_bitgen(my_seed_seq);
//
using SeedSeq = random_internal::SaltedSeedSeq<std::seed_seq>;

// -----------------------------------------------------------------------------
// turbo::CreateSeedSeqFrom(bitgen*)
// -----------------------------------------------------------------------------
//
// Constructs a seed sequence conforming to [rand.req.seedseq] using variates
// produced by a provided bit generator.
//
// You should generally avoid direct construction of seed sequences, but
// use-cases for reuse of a seed sequence to construct identical bit generators
// may be helpful (eg. replaying a simulation whose state is derived from bit
// generator values).
//
// If bitgen == nullptr, then behavior is undefined.
//
// Example:
//
//   turbo::BitGen my_bitgen;
//   auto seed_seq = turbo::CreateSeedSeqFrom(&my_bitgen);
//   turbo::BitGen new_engine(seed_seq); // derived from my_bitgen, but not
//                                      // correlated.
//
template <typename URBG>
SeedSeq CreateSeedSeqFrom(URBG* urbg) {
  SeedSeq::result_type
      seed_material[random_internal::kEntropyBlocksNeeded];

  if (!random_internal::ReadSeedMaterialFromURBG(
          urbg, turbo::MakeSpan(seed_material))) {
    random_internal::ThrowSeedGenException();
  }
  return SeedSeq(std::begin(seed_material), std::end(seed_material));
}

// -----------------------------------------------------------------------------
// turbo::MakeSeedSeq()
// -----------------------------------------------------------------------------
//
// Constructs an `turbo::SeedSeq` salting the generated values using
// implementation-defined entropy. The returned sequence can be used to create
// equivalent bit generators correlated using this sequence.
//
// Example:
//
//   auto my_seed_seq = turbo::MakeSeedSeq();
//   std::mt19937 rng1(my_seed_seq);
//   std::mt19937 rng2(my_seed_seq);
//   EXPECT_EQ(rng1(), rng2());
//
SeedSeq MakeSeedSeq();

TURBO_NAMESPACE_END
}  // namespace turbo

#endif  // TURBO_RANDOM_SEED_SEQUENCES_H_
