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

#include <tests/random/mock_distributions.h>

#include <cmath>
#include <limits>

#include <gmock/gmock.h>
#include <gtest/gtest.h>
#include <turbo/numeric/int128.h>
#include <turbo/random/distributions.h>
#include <tests/random/mocking_bit_gen.h>
#include <turbo/random/random.h>

namespace {
using ::testing::Return;

TEST(MockDistributions, Examples) {
  turbo::MockingBitGen gen;

  EXPECT_NE(turbo::Uniform<int>(gen, 1, 1000000), 20);
  EXPECT_CALL(turbo::MockUniform<int>(), Call(gen, 1, 1000000))
      .WillOnce(Return(20));
  EXPECT_EQ(turbo::Uniform<int>(gen, 1, 1000000), 20);

  EXPECT_NE(turbo::Uniform<double>(gen, 0.0, 100.0), 5.0);
  EXPECT_CALL(turbo::MockUniform<double>(), Call(gen, 0.0, 100.0))
      .WillOnce(Return(5.0));
  EXPECT_EQ(turbo::Uniform<double>(gen, 0.0, 100.0), 5.0);

  EXPECT_NE(turbo::Exponential<double>(gen, 1.0), 42);
  EXPECT_CALL(turbo::MockExponential<double>(), Call(gen, 1.0))
      .WillOnce(Return(42));
  EXPECT_EQ(turbo::Exponential<double>(gen, 1.0), 42);

  EXPECT_NE(turbo::Poisson<int>(gen, 1.0), 500);
  EXPECT_CALL(turbo::MockPoisson<int>(), Call(gen, 1.0)).WillOnce(Return(500));
  EXPECT_EQ(turbo::Poisson<int>(gen, 1.0), 500);

  EXPECT_NE(turbo::Bernoulli(gen, 0.000001), true);
  EXPECT_CALL(turbo::MockBernoulli(), Call(gen, 0.000001))
      .WillOnce(Return(true));
  EXPECT_EQ(turbo::Bernoulli(gen, 0.000001), true);

  EXPECT_NE(turbo::Beta<double>(gen, 3.0, 2.0), 0.567);
  EXPECT_CALL(turbo::MockBeta<double>(), Call(gen, 3.0, 2.0))
      .WillOnce(Return(0.567));
  EXPECT_EQ(turbo::Beta<double>(gen, 3.0, 2.0), 0.567);

  EXPECT_NE(turbo::Zipf<int>(gen, 1000000, 2.0, 1.0), 1221);
  EXPECT_CALL(turbo::MockZipf<int>(), Call(gen, 1000000, 2.0, 1.0))
      .WillOnce(Return(1221));
  EXPECT_EQ(turbo::Zipf<int>(gen, 1000000, 2.0, 1.0), 1221);

  EXPECT_NE(turbo::Gaussian<double>(gen, 0.0, 1.0), 0.001);
  EXPECT_CALL(turbo::MockGaussian<double>(), Call(gen, 0.0, 1.0))
      .WillOnce(Return(0.001));
  EXPECT_EQ(turbo::Gaussian<double>(gen, 0.0, 1.0), 0.001);

  EXPECT_NE(turbo::LogUniform<int>(gen, 0, 1000000, 2), 2040);
  EXPECT_CALL(turbo::MockLogUniform<int>(), Call(gen, 0, 1000000, 2))
      .WillOnce(Return(2040));
  EXPECT_EQ(turbo::LogUniform<int>(gen, 0, 1000000, 2), 2040);
}

TEST(MockUniform, OutOfBoundsIsAllowed) {
  turbo::MockingBitGen gen;

  EXPECT_CALL(turbo::MockUniform<int>(), Call(gen, 1, 100)).WillOnce(Return(0));
  EXPECT_EQ(turbo::Uniform<int>(gen, 1, 100), 0);
}

TEST(ValidatedMockDistributions, UniformUInt128Works) {
  turbo::random_internal::MockingBitGenImpl<true> gen;

  EXPECT_CALL(turbo::MockUniform<turbo::uint128>(), Call(gen))
      .WillOnce(Return(turbo::Uint128Max()));
  EXPECT_EQ(turbo::Uniform<turbo::uint128>(gen), turbo::Uint128Max());
}

TEST(ValidatedMockDistributions, UniformDoubleBoundaryCases) {
  turbo::random_internal::MockingBitGenImpl<true> gen;

  EXPECT_CALL(turbo::MockUniform<double>(), Call(gen, 1.0, 10.0))
      .WillOnce(Return(
          std::nextafter(10.0, -std::numeric_limits<double>::infinity())));
  EXPECT_EQ(turbo::Uniform<double>(gen, 1.0, 10.0),
            std::nextafter(10.0, -std::numeric_limits<double>::infinity()));

  EXPECT_CALL(turbo::MockUniform<double>(),
              Call(turbo::IntervalOpen, gen, 1.0, 10.0))
      .WillOnce(Return(
          std::nextafter(10.0, -std::numeric_limits<double>::infinity())));
  EXPECT_EQ(turbo::Uniform<double>(turbo::IntervalOpen, gen, 1.0, 10.0),
            std::nextafter(10.0, -std::numeric_limits<double>::infinity()));

  EXPECT_CALL(turbo::MockUniform<double>(),
              Call(turbo::IntervalOpen, gen, 1.0, 10.0))
      .WillOnce(
          Return(std::nextafter(1.0, std::numeric_limits<double>::infinity())));
  EXPECT_EQ(turbo::Uniform<double>(turbo::IntervalOpen, gen, 1.0, 10.0),
            std::nextafter(1.0, std::numeric_limits<double>::infinity()));
}

TEST(ValidatedMockDistributions, UniformDoubleEmptyRangeCases) {
  turbo::random_internal::MockingBitGenImpl<true> gen;

  ON_CALL(turbo::MockUniform<double>(), Call(turbo::IntervalOpen, gen, 1.0, 1.0))
      .WillByDefault(Return(1.0));
  EXPECT_EQ(turbo::Uniform<double>(turbo::IntervalOpen, gen, 1.0, 1.0), 1.0);

  ON_CALL(turbo::MockUniform<double>(),
          Call(turbo::IntervalOpenClosed, gen, 1.0, 1.0))
      .WillByDefault(Return(1.0));
  EXPECT_EQ(turbo::Uniform<double>(turbo::IntervalOpenClosed, gen, 1.0, 1.0),
            1.0);

  ON_CALL(turbo::MockUniform<double>(),
          Call(turbo::IntervalClosedOpen, gen, 1.0, 1.0))
      .WillByDefault(Return(1.0));
  EXPECT_EQ(turbo::Uniform<double>(turbo::IntervalClosedOpen, gen, 1.0, 1.0),
            1.0);
}

TEST(ValidatedMockDistributions, UniformIntEmptyRangeCases) {
  turbo::random_internal::MockingBitGenImpl<true> gen;

  ON_CALL(turbo::MockUniform<int>(), Call(turbo::IntervalOpen, gen, 1, 1))
      .WillByDefault(Return(1));
  EXPECT_EQ(turbo::Uniform<int>(turbo::IntervalOpen, gen, 1, 1), 1);

  ON_CALL(turbo::MockUniform<int>(), Call(turbo::IntervalOpenClosed, gen, 1, 1))
      .WillByDefault(Return(1));
  EXPECT_EQ(turbo::Uniform<int>(turbo::IntervalOpenClosed, gen, 1, 1), 1);

  ON_CALL(turbo::MockUniform<int>(), Call(turbo::IntervalClosedOpen, gen, 1, 1))
      .WillByDefault(Return(1));
  EXPECT_EQ(turbo::Uniform<int>(turbo::IntervalClosedOpen, gen, 1, 1), 1);
}

TEST(ValidatedMockUniformDeathTest, Examples) {
  turbo::random_internal::MockingBitGenImpl<true> gen;

  EXPECT_DEATH_IF_SUPPORTED(
      {
        EXPECT_CALL(turbo::MockUniform<int>(), Call(gen, 1, 100))
            .WillOnce(Return(0));
        turbo::Uniform<int>(gen, 1, 100);
      },
      " 0 is not in \\[1, 100\\)");
  EXPECT_DEATH_IF_SUPPORTED(
      {
        EXPECT_CALL(turbo::MockUniform<int>(), Call(gen, 1, 100))
            .WillOnce(Return(101));
        turbo::Uniform<int>(gen, 1, 100);
      },
      " 101 is not in \\[1, 100\\)");
  EXPECT_DEATH_IF_SUPPORTED(
      {
        EXPECT_CALL(turbo::MockUniform<int>(), Call(gen, 1, 100))
            .WillOnce(Return(100));
        turbo::Uniform<int>(gen, 1, 100);
      },
      " 100 is not in \\[1, 100\\)");

  EXPECT_DEATH_IF_SUPPORTED(
      {
        EXPECT_CALL(turbo::MockUniform<int>(),
                    Call(turbo::IntervalOpen, gen, 1, 100))
            .WillOnce(Return(1));
        turbo::Uniform<int>(turbo::IntervalOpen, gen, 1, 100);
      },
      " 1 is not in \\(1, 100\\)");
  EXPECT_DEATH_IF_SUPPORTED(
      {
        EXPECT_CALL(turbo::MockUniform<int>(),
                    Call(turbo::IntervalOpen, gen, 1, 100))
            .WillOnce(Return(101));
        turbo::Uniform<int>(turbo::IntervalOpen, gen, 1, 100);
      },
      " 101 is not in \\(1, 100\\)");
  EXPECT_DEATH_IF_SUPPORTED(
      {
        EXPECT_CALL(turbo::MockUniform<int>(),
                    Call(turbo::IntervalOpen, gen, 1, 100))
            .WillOnce(Return(100));
        turbo::Uniform<int>(turbo::IntervalOpen, gen, 1, 100);
      },
      " 100 is not in \\(1, 100\\)");

  EXPECT_DEATH_IF_SUPPORTED(
      {
        EXPECT_CALL(turbo::MockUniform<int>(),
                    Call(turbo::IntervalOpenClosed, gen, 1, 100))
            .WillOnce(Return(1));
        turbo::Uniform<int>(turbo::IntervalOpenClosed, gen, 1, 100);
      },
      " 1 is not in \\(1, 100\\]");
  EXPECT_DEATH_IF_SUPPORTED(
      {
        EXPECT_CALL(turbo::MockUniform<int>(),
                    Call(turbo::IntervalOpenClosed, gen, 1, 100))
            .WillOnce(Return(101));
        turbo::Uniform<int>(turbo::IntervalOpenClosed, gen, 1, 100);
      },
      " 101 is not in \\(1, 100\\]");

  EXPECT_DEATH_IF_SUPPORTED(
      {
        EXPECT_CALL(turbo::MockUniform<int>(),
                    Call(turbo::IntervalOpenClosed, gen, 1, 100))
            .WillOnce(Return(0));
        turbo::Uniform<int>(turbo::IntervalOpenClosed, gen, 1, 100);
      },
      " 0 is not in \\(1, 100\\]");
  EXPECT_DEATH_IF_SUPPORTED(
      {
        EXPECT_CALL(turbo::MockUniform<int>(),
                    Call(turbo::IntervalOpenClosed, gen, 1, 100))
            .WillOnce(Return(101));
        turbo::Uniform<int>(turbo::IntervalOpenClosed, gen, 1, 100);
      },
      " 101 is not in \\(1, 100\\]");

  EXPECT_DEATH_IF_SUPPORTED(
      {
        EXPECT_CALL(turbo::MockUniform<int>(),
                    Call(turbo::IntervalClosed, gen, 1, 100))
            .WillOnce(Return(0));
        turbo::Uniform<int>(turbo::IntervalClosed, gen, 1, 100);
      },
      " 0 is not in \\[1, 100\\]");
  EXPECT_DEATH_IF_SUPPORTED(
      {
        EXPECT_CALL(turbo::MockUniform<int>(),
                    Call(turbo::IntervalClosed, gen, 1, 100))
            .WillOnce(Return(101));
        turbo::Uniform<int>(turbo::IntervalClosed, gen, 1, 100);
      },
      " 101 is not in \\[1, 100\\]");
}

TEST(ValidatedMockUniformDeathTest, DoubleBoundaryCases) {
  turbo::random_internal::MockingBitGenImpl<true> gen;

  EXPECT_DEATH_IF_SUPPORTED(
      {
        EXPECT_CALL(turbo::MockUniform<double>(), Call(gen, 1.0, 10.0))
            .WillOnce(Return(10.0));
        EXPECT_EQ(turbo::Uniform<double>(gen, 1.0, 10.0), 10.0);
      },
      " 10 is not in \\[1, 10\\)");

  EXPECT_DEATH_IF_SUPPORTED(
      {
        EXPECT_CALL(turbo::MockUniform<double>(),
                    Call(turbo::IntervalOpen, gen, 1.0, 10.0))
            .WillOnce(Return(10.0));
        EXPECT_EQ(turbo::Uniform<double>(turbo::IntervalOpen, gen, 1.0, 10.0),
                  10.0);
      },
      " 10 is not in \\(1, 10\\)");

  EXPECT_DEATH_IF_SUPPORTED(
      {
        EXPECT_CALL(turbo::MockUniform<double>(),
                    Call(turbo::IntervalOpen, gen, 1.0, 10.0))
            .WillOnce(Return(1.0));
        EXPECT_EQ(turbo::Uniform<double>(turbo::IntervalOpen, gen, 1.0, 10.0),
                  1.0);
      },
      " 1 is not in \\(1, 10\\)");
}

}  // namespace
