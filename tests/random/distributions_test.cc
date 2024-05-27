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

#include <turbo/random/distributions.h>

#include <cfloat>
#include <cmath>
#include <cstdint>
#include <limits>
#include <type_traits>
#include <utility>
#include <vector>

#include <gtest/gtest.h>
#include <turbo/meta/type_traits.h>
#include <turbo/numeric/int128.h>
#include <tests/random/distribution_test_util.h>
#include <turbo/random/random.h>

namespace {

constexpr int kSize = 400000;

class RandomDistributionsTest : public testing::Test {};

struct Invalid {};

template <typename A, typename B>
auto InferredUniformReturnT(int)
    -> decltype(turbo::Uniform(std::declval<turbo::InsecureBitGen&>(),
                              std::declval<A>(), std::declval<B>()));

template <typename, typename>
Invalid InferredUniformReturnT(...);

template <typename TagType, typename A, typename B>
auto InferredTaggedUniformReturnT(int)
    -> decltype(turbo::Uniform(std::declval<TagType>(),
                              std::declval<turbo::InsecureBitGen&>(),
                              std::declval<A>(), std::declval<B>()));

template <typename, typename, typename>
Invalid InferredTaggedUniformReturnT(...);

// Given types <A, B, Expect>, CheckArgsInferType() verifies that
//
//   turbo::Uniform(gen, A{}, B{})
//
// returns the type "Expect".
//
// This interface can also be used to assert that a given turbo::Uniform()
// overload does not exist / will not compile. Given types <A, B>, the
// expression
//
//   decltype(turbo::Uniform(..., std::declval<A>(), std::declval<B>()))
//
// will not compile, leaving the definition of InferredUniformReturnT<A, B> to
// resolve (via SFINAE) to the overload which returns type "Invalid". This
// allows tests to assert that an invocation such as
//
//   turbo::Uniform(gen, 1.23f, std::numeric_limits<int>::max() - 1)
//
// should not compile, since neither type, float nor int, can precisely
// represent both endpoint-values. Writing:
//
//   CheckArgsInferType<float, int, Invalid>()
//
// will assert that this overload does not exist.
template <typename A, typename B, typename Expect>
void CheckArgsInferType() {
  static_assert(
      turbo::conjunction<
          std::is_same<Expect, decltype(InferredUniformReturnT<A, B>(0))>,
          std::is_same<Expect,
                       decltype(InferredUniformReturnT<B, A>(0))>>::value,
      "");
  static_assert(
      turbo::conjunction<
          std::is_same<Expect, decltype(InferredTaggedUniformReturnT<
                                        turbo::IntervalOpenOpenTag, A, B>(0))>,
          std::is_same<Expect,
                       decltype(InferredTaggedUniformReturnT<
                                turbo::IntervalOpenOpenTag, B, A>(0))>>::value,
      "");
}

template <typename A, typename B, typename ExplicitRet>
auto ExplicitUniformReturnT(int) -> decltype(turbo::Uniform<ExplicitRet>(
                                     std::declval<turbo::InsecureBitGen&>(),
                                     std::declval<A>(), std::declval<B>()));

template <typename, typename, typename ExplicitRet>
Invalid ExplicitUniformReturnT(...);

template <typename TagType, typename A, typename B, typename ExplicitRet>
auto ExplicitTaggedUniformReturnT(int)
    -> decltype(turbo::Uniform<ExplicitRet>(
        std::declval<TagType>(), std::declval<turbo::InsecureBitGen&>(),
        std::declval<A>(), std::declval<B>()));

template <typename, typename, typename, typename ExplicitRet>
Invalid ExplicitTaggedUniformReturnT(...);

// Given types <A, B, Expect>, CheckArgsReturnExpectedType() verifies that
//
//   turbo::Uniform<Expect>(gen, A{}, B{})
//
// returns the type "Expect", and that the function-overload has the signature
//
//   Expect(URBG&, Expect, Expect)
template <typename A, typename B, typename Expect>
void CheckArgsReturnExpectedType() {
  static_assert(
      turbo::conjunction<
          std::is_same<Expect,
                       decltype(ExplicitUniformReturnT<A, B, Expect>(0))>,
          std::is_same<Expect, decltype(ExplicitUniformReturnT<B, A, Expect>(
                                   0))>>::value,
      "");
  static_assert(
      turbo::conjunction<
          std::is_same<Expect,
                       decltype(ExplicitTaggedUniformReturnT<
                                turbo::IntervalOpenOpenTag, A, B, Expect>(0))>,
          std::is_same<Expect, decltype(ExplicitTaggedUniformReturnT<
                                        turbo::IntervalOpenOpenTag, B, A,
                                        Expect>(0))>>::value,
      "");
}

// Takes the type of `turbo::Uniform<R>(gen)` if valid or `Invalid` otherwise.
template <typename R>
auto UniformNoBoundsReturnT(int)
    -> decltype(turbo::Uniform<R>(std::declval<turbo::InsecureBitGen&>()));

template <typename>
Invalid UniformNoBoundsReturnT(...);

TEST_F(RandomDistributionsTest, UniformTypeInference) {
  // Infers common types.
  CheckArgsInferType<uint16_t, uint16_t, uint16_t>();
  CheckArgsInferType<uint32_t, uint32_t, uint32_t>();
  CheckArgsInferType<uint64_t, uint64_t, uint64_t>();
  CheckArgsInferType<int16_t, int16_t, int16_t>();
  CheckArgsInferType<int32_t, int32_t, int32_t>();
  CheckArgsInferType<int64_t, int64_t, int64_t>();
  CheckArgsInferType<float, float, float>();
  CheckArgsInferType<double, double, double>();

  // Explicitly-specified return-values override inferences.
  CheckArgsReturnExpectedType<int16_t, int16_t, int32_t>();
  CheckArgsReturnExpectedType<uint16_t, uint16_t, int32_t>();
  CheckArgsReturnExpectedType<int16_t, int16_t, int64_t>();
  CheckArgsReturnExpectedType<int16_t, int32_t, int64_t>();
  CheckArgsReturnExpectedType<int16_t, int32_t, double>();
  CheckArgsReturnExpectedType<float, float, double>();
  CheckArgsReturnExpectedType<int, int, int16_t>();

  // Properly promotes uint16_t.
  CheckArgsInferType<uint16_t, uint32_t, uint32_t>();
  CheckArgsInferType<uint16_t, uint64_t, uint64_t>();
  CheckArgsInferType<uint16_t, int32_t, int32_t>();
  CheckArgsInferType<uint16_t, int64_t, int64_t>();
  CheckArgsInferType<uint16_t, float, float>();
  CheckArgsInferType<uint16_t, double, double>();

  // Properly promotes int16_t.
  CheckArgsInferType<int16_t, int32_t, int32_t>();
  CheckArgsInferType<int16_t, int64_t, int64_t>();
  CheckArgsInferType<int16_t, float, float>();
  CheckArgsInferType<int16_t, double, double>();

  // Invalid (u)int16_t-pairings do not compile.
  // See "CheckArgsInferType" comments above, for how this is achieved.
  CheckArgsInferType<uint16_t, int16_t, Invalid>();
  CheckArgsInferType<int16_t, uint32_t, Invalid>();
  CheckArgsInferType<int16_t, uint64_t, Invalid>();

  // Properly promotes uint32_t.
  CheckArgsInferType<uint32_t, uint64_t, uint64_t>();
  CheckArgsInferType<uint32_t, int64_t, int64_t>();
  CheckArgsInferType<uint32_t, double, double>();

  // Properly promotes int32_t.
  CheckArgsInferType<int32_t, int64_t, int64_t>();
  CheckArgsInferType<int32_t, double, double>();

  // Invalid (u)int32_t-pairings do not compile.
  CheckArgsInferType<uint32_t, int32_t, Invalid>();
  CheckArgsInferType<int32_t, uint64_t, Invalid>();
  CheckArgsInferType<int32_t, float, Invalid>();
  CheckArgsInferType<uint32_t, float, Invalid>();

  // Invalid (u)int64_t-pairings do not compile.
  CheckArgsInferType<uint64_t, int64_t, Invalid>();
  CheckArgsInferType<int64_t, float, Invalid>();
  CheckArgsInferType<int64_t, double, Invalid>();

  // Properly promotes float.
  CheckArgsInferType<float, double, double>();
}

TEST_F(RandomDistributionsTest, UniformExamples) {
  // Examples.
  turbo::InsecureBitGen gen;
  EXPECT_NE(1, turbo::Uniform(gen, static_cast<uint16_t>(0), 1.0f));
  EXPECT_NE(1, turbo::Uniform(gen, 0, 1.0));
  EXPECT_NE(1, turbo::Uniform(turbo::IntervalOpenOpen, gen,
                             static_cast<uint16_t>(0), 1.0f));
  EXPECT_NE(1, turbo::Uniform(turbo::IntervalOpenOpen, gen, 0, 1.0));
  EXPECT_NE(1, turbo::Uniform(turbo::IntervalOpenOpen, gen, -1, 1.0));
  EXPECT_NE(1, turbo::Uniform<double>(turbo::IntervalOpenOpen, gen, -1, 1));
  EXPECT_NE(1, turbo::Uniform<float>(turbo::IntervalOpenOpen, gen, 0, 1));
  EXPECT_NE(1, turbo::Uniform<float>(gen, 0, 1));
}

TEST_F(RandomDistributionsTest, UniformNoBounds) {
  turbo::InsecureBitGen gen;

  turbo::Uniform<uint8_t>(gen);
  turbo::Uniform<uint16_t>(gen);
  turbo::Uniform<uint32_t>(gen);
  turbo::Uniform<uint64_t>(gen);
  turbo::Uniform<turbo::uint128>(gen);

  // Compile-time validity tests.

  // Allows unsigned ints.
  testing::StaticAssertTypeEq<uint8_t,
                              decltype(UniformNoBoundsReturnT<uint8_t>(0))>();
  testing::StaticAssertTypeEq<uint16_t,
                              decltype(UniformNoBoundsReturnT<uint16_t>(0))>();
  testing::StaticAssertTypeEq<uint32_t,
                              decltype(UniformNoBoundsReturnT<uint32_t>(0))>();
  testing::StaticAssertTypeEq<uint64_t,
                              decltype(UniformNoBoundsReturnT<uint64_t>(0))>();
  testing::StaticAssertTypeEq<
      turbo::uint128, decltype(UniformNoBoundsReturnT<turbo::uint128>(0))>();

  // Disallows signed ints.
  testing::StaticAssertTypeEq<Invalid,
                              decltype(UniformNoBoundsReturnT<int8_t>(0))>();
  testing::StaticAssertTypeEq<Invalid,
                              decltype(UniformNoBoundsReturnT<int16_t>(0))>();
  testing::StaticAssertTypeEq<Invalid,
                              decltype(UniformNoBoundsReturnT<int32_t>(0))>();
  testing::StaticAssertTypeEq<Invalid,
                              decltype(UniformNoBoundsReturnT<int64_t>(0))>();
  testing::StaticAssertTypeEq<
      Invalid, decltype(UniformNoBoundsReturnT<turbo::int128>(0))>();

  // Disallows float types.
  testing::StaticAssertTypeEq<Invalid,
                              decltype(UniformNoBoundsReturnT<float>(0))>();
  testing::StaticAssertTypeEq<Invalid,
                              decltype(UniformNoBoundsReturnT<double>(0))>();
}

TEST_F(RandomDistributionsTest, UniformNonsenseRanges) {
  // The ranges used in this test are undefined behavior.
  // The results are arbitrary and subject to future changes.

#if (defined(__i386__) || defined(_M_IX86)) && FLT_EVAL_METHOD != 0
  // We're using an x87-compatible FPU, and intermediate operations can be
  // performed with 80-bit floats. This produces slightly different results from
  // what we expect below.
  GTEST_SKIP()
      << "Skipping the test because we detected x87 floating-point semantics";
#endif

  turbo::InsecureBitGen gen;

  // <uint>
  EXPECT_EQ(0, turbo::Uniform<uint64_t>(gen, 0, 0));
  EXPECT_EQ(1, turbo::Uniform<uint64_t>(gen, 1, 0));
  EXPECT_EQ(0, turbo::Uniform<uint64_t>(turbo::IntervalOpenOpen, gen, 0, 0));
  EXPECT_EQ(1, turbo::Uniform<uint64_t>(turbo::IntervalOpenOpen, gen, 1, 0));

  constexpr auto m = (std::numeric_limits<uint64_t>::max)();

  EXPECT_EQ(m, turbo::Uniform(gen, m, m));
  EXPECT_EQ(m, turbo::Uniform(gen, m, m - 1));
  EXPECT_EQ(m - 1, turbo::Uniform(gen, m - 1, m));
  EXPECT_EQ(m, turbo::Uniform(turbo::IntervalOpenOpen, gen, m, m));
  EXPECT_EQ(m, turbo::Uniform(turbo::IntervalOpenOpen, gen, m, m - 1));
  EXPECT_EQ(m - 1, turbo::Uniform(turbo::IntervalOpenOpen, gen, m - 1, m));

  // <int>
  EXPECT_EQ(0, turbo::Uniform<int64_t>(gen, 0, 0));
  EXPECT_EQ(1, turbo::Uniform<int64_t>(gen, 1, 0));
  EXPECT_EQ(0, turbo::Uniform<int64_t>(turbo::IntervalOpenOpen, gen, 0, 0));
  EXPECT_EQ(1, turbo::Uniform<int64_t>(turbo::IntervalOpenOpen, gen, 1, 0));

  constexpr auto l = (std::numeric_limits<int64_t>::min)();
  constexpr auto r = (std::numeric_limits<int64_t>::max)();

  EXPECT_EQ(l, turbo::Uniform(gen, l, l));
  EXPECT_EQ(r, turbo::Uniform(gen, r, r));
  EXPECT_EQ(r, turbo::Uniform(gen, r, r - 1));
  EXPECT_EQ(r - 1, turbo::Uniform(gen, r - 1, r));
  EXPECT_EQ(l, turbo::Uniform(turbo::IntervalOpenOpen, gen, l, l));
  EXPECT_EQ(r, turbo::Uniform(turbo::IntervalOpenOpen, gen, r, r));
  EXPECT_EQ(r, turbo::Uniform(turbo::IntervalOpenOpen, gen, r, r - 1));
  EXPECT_EQ(r - 1, turbo::Uniform(turbo::IntervalOpenOpen, gen, r - 1, r));

  // <double>
  const double e = std::nextafter(1.0, 2.0);  // 1 + epsilon
  const double f = std::nextafter(1.0, 0.0);  // 1 - epsilon
  const double g = std::numeric_limits<double>::denorm_min();

  EXPECT_EQ(1.0, turbo::Uniform(gen, 1.0, e));
  EXPECT_EQ(1.0, turbo::Uniform(gen, 1.0, f));
  EXPECT_EQ(0.0, turbo::Uniform(gen, 0.0, g));

  EXPECT_EQ(e, turbo::Uniform(turbo::IntervalOpenOpen, gen, 1.0, e));
  EXPECT_EQ(f, turbo::Uniform(turbo::IntervalOpenOpen, gen, 1.0, f));
  EXPECT_EQ(g, turbo::Uniform(turbo::IntervalOpenOpen, gen, 0.0, g));
}

// TODO(lar): Validate properties of non-default interval-semantics.
TEST_F(RandomDistributionsTest, UniformReal) {
  std::vector<double> values(kSize);

  turbo::InsecureBitGen gen;
  for (int i = 0; i < kSize; i++) {
    values[i] = turbo::Uniform(gen, 0, 1.0);
  }

  const auto moments =
      turbo::random_internal::ComputeDistributionMoments(values);
  EXPECT_NEAR(0.5, moments.mean, 0.02);
  EXPECT_NEAR(1 / 12.0, moments.variance, 0.02);
  EXPECT_NEAR(0.0, moments.skewness, 0.02);
  EXPECT_NEAR(9 / 5.0, moments.kurtosis, 0.02);
}

TEST_F(RandomDistributionsTest, UniformInt) {
  std::vector<double> values(kSize);

  turbo::InsecureBitGen gen;
  for (int i = 0; i < kSize; i++) {
    const int64_t kMax = 1000000000000ll;
    int64_t j = turbo::Uniform(turbo::IntervalClosedClosed, gen, 0, kMax);
    // convert to double.
    values[i] = static_cast<double>(j) / static_cast<double>(kMax);
  }

  const auto moments =
      turbo::random_internal::ComputeDistributionMoments(values);
  EXPECT_NEAR(0.5, moments.mean, 0.02);
  EXPECT_NEAR(1 / 12.0, moments.variance, 0.02);
  EXPECT_NEAR(0.0, moments.skewness, 0.02);
  EXPECT_NEAR(9 / 5.0, moments.kurtosis, 0.02);

  /*
  // NOTE: These are not supported by turbo::Uniform, which is specialized
  // on integer and real valued types.

  enum E { E0, E1 };    // enum
  enum S : int { S0, S1 };    // signed enum
  enum U : unsigned int { U0, U1 };  // unsigned enum

  turbo::Uniform(gen, E0, E1);
  turbo::Uniform(gen, S0, S1);
  turbo::Uniform(gen, U0, U1);
  */
}

TEST_F(RandomDistributionsTest, Exponential) {
  std::vector<double> values(kSize);

  turbo::InsecureBitGen gen;
  for (int i = 0; i < kSize; i++) {
    values[i] = turbo::Exponential<double>(gen);
  }

  const auto moments =
      turbo::random_internal::ComputeDistributionMoments(values);
  EXPECT_NEAR(1.0, moments.mean, 0.02);
  EXPECT_NEAR(1.0, moments.variance, 0.025);
  EXPECT_NEAR(2.0, moments.skewness, 0.1);
  EXPECT_LT(5.0, moments.kurtosis);
}

TEST_F(RandomDistributionsTest, PoissonDefault) {
  std::vector<double> values(kSize);

  turbo::InsecureBitGen gen;
  for (int i = 0; i < kSize; i++) {
    values[i] = turbo::Poisson<int64_t>(gen);
  }

  const auto moments =
      turbo::random_internal::ComputeDistributionMoments(values);
  EXPECT_NEAR(1.0, moments.mean, 0.02);
  EXPECT_NEAR(1.0, moments.variance, 0.02);
  EXPECT_NEAR(1.0, moments.skewness, 0.025);
  EXPECT_LT(2.0, moments.kurtosis);
}

TEST_F(RandomDistributionsTest, PoissonLarge) {
  constexpr double kMean = 100000000.0;
  std::vector<double> values(kSize);

  turbo::InsecureBitGen gen;
  for (int i = 0; i < kSize; i++) {
    values[i] = turbo::Poisson<int64_t>(gen, kMean);
  }

  const auto moments =
      turbo::random_internal::ComputeDistributionMoments(values);
  EXPECT_NEAR(kMean, moments.mean, kMean * 0.015);
  EXPECT_NEAR(kMean, moments.variance, kMean * 0.015);
  EXPECT_NEAR(std::sqrt(kMean), moments.skewness, kMean * 0.02);
  EXPECT_LT(2.0, moments.kurtosis);
}

TEST_F(RandomDistributionsTest, Bernoulli) {
  constexpr double kP = 0.5151515151;
  std::vector<double> values(kSize);

  turbo::InsecureBitGen gen;
  for (int i = 0; i < kSize; i++) {
    values[i] = turbo::Bernoulli(gen, kP);
  }

  const auto moments =
      turbo::random_internal::ComputeDistributionMoments(values);
  EXPECT_NEAR(kP, moments.mean, 0.01);
}

TEST_F(RandomDistributionsTest, Beta) {
  constexpr double kAlpha = 2.0;
  constexpr double kBeta = 3.0;
  std::vector<double> values(kSize);

  turbo::InsecureBitGen gen;
  for (int i = 0; i < kSize; i++) {
    values[i] = turbo::Beta(gen, kAlpha, kBeta);
  }

  const auto moments =
      turbo::random_internal::ComputeDistributionMoments(values);
  EXPECT_NEAR(0.4, moments.mean, 0.01);
}

TEST_F(RandomDistributionsTest, Zipf) {
  std::vector<double> values(kSize);

  turbo::InsecureBitGen gen;
  for (int i = 0; i < kSize; i++) {
    values[i] = turbo::Zipf<int64_t>(gen, 100);
  }

  // The mean of a zipf distribution is: H(N, s-1) / H(N,s).
  // Given the parameter v = 1, this gives the following function:
  // (Hn(100, 1) - Hn(1,1)) / (Hn(100,2) - Hn(1,2)) = 6.5944
  const auto moments =
      turbo::random_internal::ComputeDistributionMoments(values);
  EXPECT_NEAR(6.5944, moments.mean, 2000) << moments;
}

TEST_F(RandomDistributionsTest, Gaussian) {
  std::vector<double> values(kSize);

  turbo::InsecureBitGen gen;
  for (int i = 0; i < kSize; i++) {
    values[i] = turbo::Gaussian<double>(gen);
  }

  const auto moments =
      turbo::random_internal::ComputeDistributionMoments(values);
  EXPECT_NEAR(0.0, moments.mean, 0.02);
  EXPECT_NEAR(1.0, moments.variance, 0.04);
  EXPECT_NEAR(0, moments.skewness, 0.2);
  EXPECT_NEAR(3.0, moments.kurtosis, 0.5);
}

TEST_F(RandomDistributionsTest, LogUniform) {
  std::vector<double> values(kSize);

  turbo::InsecureBitGen gen;
  for (int i = 0; i < kSize; i++) {
    values[i] = turbo::LogUniform<int64_t>(gen, 0, (1 << 10) - 1);
  }

  // The mean is the sum of the fractional means of the uniform distributions:
  // [0..0][1..1][2..3][4..7][8..15][16..31][32..63]
  // [64..127][128..255][256..511][512..1023]
  const double mean = (0 + 1 + 1 + 2 + 3 + 4 + 7 + 8 + 15 + 16 + 31 + 32 + 63 +
                       64 + 127 + 128 + 255 + 256 + 511 + 512 + 1023) /
                      (2.0 * 11.0);

  const auto moments =
      turbo::random_internal::ComputeDistributionMoments(values);
  EXPECT_NEAR(mean, moments.mean, 2) << moments;
}

}  // namespace
