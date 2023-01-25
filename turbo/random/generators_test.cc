// Copyright 2017 The Turbo Authors.
//
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

#include <cstddef>
#include <cstdint>
#include <random>
#include <vector>

#include "gtest/gtest.h"
#include "turbo/random/distributions.h"
#include "turbo/random/random.h"

namespace {

template <typename URBG>
void TestUniform(URBG* gen) {
  // [a, b) default-semantics, inferred types.
  turbo::Uniform(*gen, 0, 100);     // int
  turbo::Uniform(*gen, 0, 1.0);     // Promoted to double
  turbo::Uniform(*gen, 0.0f, 1.0);  // Promoted to double
  turbo::Uniform(*gen, 0.0, 1.0);   // double
  turbo::Uniform(*gen, -1, 1L);     // Promoted to long

  // Roll a die.
  turbo::Uniform(turbo::IntervalClosedClosed, *gen, 1, 6);

  // Get a fraction.
  turbo::Uniform(turbo::IntervalOpenOpen, *gen, 0.0, 1.0);

  // Assign a value to a random element.
  std::vector<int> elems = {10, 20, 30, 40, 50};
  elems[turbo::Uniform(*gen, 0u, elems.size())] = 5;
  elems[turbo::Uniform<size_t>(*gen, 0, elems.size())] = 3;

  // Choose some epsilon around zero.
  turbo::Uniform(turbo::IntervalOpenOpen, *gen, -1.0, 1.0);

  // (a, b) semantics, inferred types.
  turbo::Uniform(turbo::IntervalOpenOpen, *gen, 0, 1.0);  // Promoted to double

  // Explict overriding of types.
  turbo::Uniform<int>(*gen, 0, 100);
  turbo::Uniform<int8_t>(*gen, 0, 100);
  turbo::Uniform<int16_t>(*gen, 0, 100);
  turbo::Uniform<uint16_t>(*gen, 0, 100);
  turbo::Uniform<int32_t>(*gen, 0, 1 << 10);
  turbo::Uniform<uint32_t>(*gen, 0, 1 << 10);
  turbo::Uniform<int64_t>(*gen, 0, 1 << 10);
  turbo::Uniform<uint64_t>(*gen, 0, 1 << 10);

  turbo::Uniform<float>(*gen, 0.0, 1.0);
  turbo::Uniform<float>(*gen, 0, 1);
  turbo::Uniform<float>(*gen, -1, 1);
  turbo::Uniform<double>(*gen, 0.0, 1.0);

  turbo::Uniform<float>(*gen, -1.0, 0);
  turbo::Uniform<double>(*gen, -1.0, 0);

  // Tagged
  turbo::Uniform<double>(turbo::IntervalClosedClosed, *gen, 0, 1);
  turbo::Uniform<double>(turbo::IntervalClosedOpen, *gen, 0, 1);
  turbo::Uniform<double>(turbo::IntervalOpenOpen, *gen, 0, 1);
  turbo::Uniform<double>(turbo::IntervalOpenClosed, *gen, 0, 1);
  turbo::Uniform<double>(turbo::IntervalClosedClosed, *gen, 0, 1);
  turbo::Uniform<double>(turbo::IntervalOpenOpen, *gen, 0, 1);

  turbo::Uniform<int>(turbo::IntervalClosedClosed, *gen, 0, 100);
  turbo::Uniform<int>(turbo::IntervalClosedOpen, *gen, 0, 100);
  turbo::Uniform<int>(turbo::IntervalOpenOpen, *gen, 0, 100);
  turbo::Uniform<int>(turbo::IntervalOpenClosed, *gen, 0, 100);
  turbo::Uniform<int>(turbo::IntervalClosedClosed, *gen, 0, 100);
  turbo::Uniform<int>(turbo::IntervalOpenOpen, *gen, 0, 100);

  // With *generator as an R-value reference.
  turbo::Uniform<int>(URBG(), 0, 100);
  turbo::Uniform<double>(URBG(), 0.0, 1.0);
}

template <typename URBG>
void TestExponential(URBG* gen) {
  turbo::Exponential<float>(*gen);
  turbo::Exponential<double>(*gen);
  turbo::Exponential<double>(URBG());
}

template <typename URBG>
void TestPoisson(URBG* gen) {
  // [rand.dist.pois] Indicates that the std::poisson_distribution
  // is parameterized by IntType, however MSVC does not allow 8-bit
  // types.
  turbo::Poisson<int>(*gen);
  turbo::Poisson<int16_t>(*gen);
  turbo::Poisson<uint16_t>(*gen);
  turbo::Poisson<int32_t>(*gen);
  turbo::Poisson<uint32_t>(*gen);
  turbo::Poisson<int64_t>(*gen);
  turbo::Poisson<uint64_t>(*gen);
  turbo::Poisson<uint64_t>(URBG());
  turbo::Poisson<turbo::int128>(*gen);
  turbo::Poisson<turbo::uint128>(*gen);
}

template <typename URBG>
void TestBernoulli(URBG* gen) {
  turbo::Bernoulli(*gen, 0.5);
  turbo::Bernoulli(*gen, 0.5);
}

template <typename URBG>
void TestZipf(URBG* gen) {
  turbo::Zipf<int>(*gen, 100);
  turbo::Zipf<int8_t>(*gen, 100);
  turbo::Zipf<int16_t>(*gen, 100);
  turbo::Zipf<uint16_t>(*gen, 100);
  turbo::Zipf<int32_t>(*gen, 1 << 10);
  turbo::Zipf<uint32_t>(*gen, 1 << 10);
  turbo::Zipf<int64_t>(*gen, 1 << 10);
  turbo::Zipf<uint64_t>(*gen, 1 << 10);
  turbo::Zipf<uint64_t>(URBG(), 1 << 10);
  turbo::Zipf<turbo::int128>(*gen, 1 << 10);
  turbo::Zipf<turbo::uint128>(*gen, 1 << 10);
}

template <typename URBG>
void TestGaussian(URBG* gen) {
  turbo::Gaussian<float>(*gen, 1.0, 1.0);
  turbo::Gaussian<double>(*gen, 1.0, 1.0);
  turbo::Gaussian<double>(URBG(), 1.0, 1.0);
}

template <typename URBG>
void TestLogNormal(URBG* gen) {
  turbo::LogUniform<int>(*gen, 0, 100);
  turbo::LogUniform<int8_t>(*gen, 0, 100);
  turbo::LogUniform<int16_t>(*gen, 0, 100);
  turbo::LogUniform<uint16_t>(*gen, 0, 100);
  turbo::LogUniform<int32_t>(*gen, 0, 1 << 10);
  turbo::LogUniform<uint32_t>(*gen, 0, 1 << 10);
  turbo::LogUniform<int64_t>(*gen, 0, 1 << 10);
  turbo::LogUniform<uint64_t>(*gen, 0, 1 << 10);
  turbo::LogUniform<uint64_t>(URBG(), 0, 1 << 10);
  turbo::LogUniform<turbo::int128>(*gen, 0, 1 << 10);
  turbo::LogUniform<turbo::uint128>(*gen, 0, 1 << 10);
}

template <typename URBG>
void CompatibilityTest() {
  URBG gen;

  TestUniform(&gen);
  TestExponential(&gen);
  TestPoisson(&gen);
  TestBernoulli(&gen);
  TestZipf(&gen);
  TestGaussian(&gen);
  TestLogNormal(&gen);
}

TEST(std_mt19937_64, Compatibility) {
  // Validate with std::mt19937_64
  CompatibilityTest<std::mt19937_64>();
}

TEST(BitGen, Compatibility) {
  // Validate with turbo::BitGen
  CompatibilityTest<turbo::BitGen>();
}

TEST(InsecureBitGen, Compatibility) {
  // Validate with turbo::InsecureBitGen
  CompatibilityTest<turbo::InsecureBitGen>();
}

}  // namespace
