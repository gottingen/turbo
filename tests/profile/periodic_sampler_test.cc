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

#include <turbo/profiling/internal/periodic_sampler.h>

#include <thread>  // NOLINT(build/c++11)

#include <gmock/gmock.h>
#include <gtest/gtest.h>
#include <turbo/base/attributes.h>
#include <turbo/base/macros.h>

namespace turbo {
TURBO_NAMESPACE_BEGIN
namespace profiling_internal {
namespace {

using testing::Eq;
using testing::Return;
using testing::StrictMock;

class MockPeriodicSampler : public PeriodicSamplerBase {
 public:
  virtual ~MockPeriodicSampler() = default;

  MOCK_METHOD(int, period, (), (const, noexcept));
  MOCK_METHOD(int64_t, GetExponentialBiased, (int), (noexcept));
};

TEST(PeriodicSamplerBaseTest, Sample) {
  StrictMock<MockPeriodicSampler> sampler;

  EXPECT_CALL(sampler, period()).Times(3).WillRepeatedly(Return(16));
  EXPECT_CALL(sampler, GetExponentialBiased(16))
      .WillOnce(Return(2))
      .WillOnce(Return(3))
      .WillOnce(Return(4));

  EXPECT_FALSE(sampler.Sample());
  EXPECT_TRUE(sampler.Sample());

  EXPECT_FALSE(sampler.Sample());
  EXPECT_FALSE(sampler.Sample());
  EXPECT_TRUE(sampler.Sample());

  EXPECT_FALSE(sampler.Sample());
  EXPECT_FALSE(sampler.Sample());
  EXPECT_FALSE(sampler.Sample());
}

TEST(PeriodicSamplerBaseTest, ImmediatelySample) {
  StrictMock<MockPeriodicSampler> sampler;

  EXPECT_CALL(sampler, period()).Times(2).WillRepeatedly(Return(16));
  EXPECT_CALL(sampler, GetExponentialBiased(16))
      .WillOnce(Return(1))
      .WillOnce(Return(2))
      .WillOnce(Return(3));

  EXPECT_TRUE(sampler.Sample());

  EXPECT_FALSE(sampler.Sample());
  EXPECT_TRUE(sampler.Sample());

  EXPECT_FALSE(sampler.Sample());
  EXPECT_FALSE(sampler.Sample());
}

TEST(PeriodicSamplerBaseTest, Disabled) {
  StrictMock<MockPeriodicSampler> sampler;

  EXPECT_CALL(sampler, period()).Times(3).WillRepeatedly(Return(0));

  EXPECT_FALSE(sampler.Sample());
  EXPECT_FALSE(sampler.Sample());
  EXPECT_FALSE(sampler.Sample());
}

TEST(PeriodicSamplerBaseTest, AlwaysOn) {
  StrictMock<MockPeriodicSampler> sampler;

  EXPECT_CALL(sampler, period()).Times(3).WillRepeatedly(Return(1));

  EXPECT_TRUE(sampler.Sample());
  EXPECT_TRUE(sampler.Sample());
  EXPECT_TRUE(sampler.Sample());
}

TEST(PeriodicSamplerBaseTest, Disable) {
  StrictMock<MockPeriodicSampler> sampler;

  EXPECT_CALL(sampler, period()).WillOnce(Return(16));
  EXPECT_CALL(sampler, GetExponentialBiased(16)).WillOnce(Return(3));
  EXPECT_FALSE(sampler.Sample());
  EXPECT_FALSE(sampler.Sample());

  EXPECT_CALL(sampler, period()).Times(2).WillRepeatedly(Return(0));

  EXPECT_FALSE(sampler.Sample());
  EXPECT_FALSE(sampler.Sample());
}

TEST(PeriodicSamplerBaseTest, Enable) {
  StrictMock<MockPeriodicSampler> sampler;

  EXPECT_CALL(sampler, period()).WillOnce(Return(0));
  EXPECT_FALSE(sampler.Sample());

  EXPECT_CALL(sampler, period()).Times(2).WillRepeatedly(Return(16));
  EXPECT_CALL(sampler, GetExponentialBiased(16))
      .Times(2)
      .WillRepeatedly(Return(3));

  EXPECT_FALSE(sampler.Sample());
  EXPECT_FALSE(sampler.Sample());
  EXPECT_TRUE(sampler.Sample());

  EXPECT_FALSE(sampler.Sample());
  EXPECT_FALSE(sampler.Sample());
}

TEST(PeriodicSamplerTest, ConstructConstInit) {
  struct Tag {};
  TURBO_CONST_INIT static PeriodicSampler<Tag> sampler;
  (void)sampler;
}

TEST(PeriodicSamplerTest, DefaultPeriod0) {
  struct Tag {};
  PeriodicSampler<Tag> sampler;
  EXPECT_THAT(sampler.period(), Eq(0));
}

TEST(PeriodicSamplerTest, DefaultPeriod) {
  struct Tag {};
  PeriodicSampler<Tag, 100> sampler;
  EXPECT_THAT(sampler.period(), Eq(100));
}

TEST(PeriodicSamplerTest, SetGlobalPeriod) {
  struct Tag1 {};
  struct Tag2 {};
  PeriodicSampler<Tag1, 25> sampler1;
  PeriodicSampler<Tag2, 50> sampler2;

  EXPECT_THAT(sampler1.period(), Eq(25));
  EXPECT_THAT(sampler2.period(), Eq(50));

  std::thread thread([] {
    PeriodicSampler<Tag1, 25> sampler1;
    PeriodicSampler<Tag2, 50> sampler2;
    EXPECT_THAT(sampler1.period(), Eq(25));
    EXPECT_THAT(sampler2.period(), Eq(50));
    sampler1.SetGlobalPeriod(10);
    sampler2.SetGlobalPeriod(20);
  });
  thread.join();

  EXPECT_THAT(sampler1.period(), Eq(10));
  EXPECT_THAT(sampler2.period(), Eq(20));
}

}  // namespace
}  // namespace profiling_internal
TURBO_NAMESPACE_END
}  // namespace turbo
