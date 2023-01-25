// Copyright 2018 The Abseil Authors.
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

#include "turbo/random/mock_distributions.h"

#include "gtest/gtest.h"
#include "turbo/random/mocking_bit_gen.h"
#include "turbo/random/random.h"

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

}  // namespace
