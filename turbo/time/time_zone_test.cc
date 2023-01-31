// Copyright 2022 The Turbo Authors.
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

#include "turbo/time/internal/cctz/include/cctz/time_zone.h"

#include "gtest/gtest.h"
#include "turbo/time/internal/test_util.h"
#include "turbo/time/time.h"

namespace cctz = turbo::time_internal::cctz;

namespace {

TEST(TimeZone, ValueSemantics) {
  turbo::TimeZone tz;
  turbo::TimeZone tz2 = tz;  // Copy-construct
  EXPECT_EQ(tz, tz2);
  tz2 = tz;  // Copy-assign
  EXPECT_EQ(tz, tz2);
}

TEST(TimeZone, Equality) {
  turbo::TimeZone a, b;
  EXPECT_EQ(a, b);
  EXPECT_EQ(a.name(), b.name());

  turbo::TimeZone implicit_utc;
  turbo::TimeZone explicit_utc = turbo::UTCTimeZone();
  EXPECT_EQ(implicit_utc, explicit_utc);
  EXPECT_EQ(implicit_utc.name(), explicit_utc.name());

  turbo::TimeZone la = turbo::time_internal::LoadTimeZone("America/Los_Angeles");
  turbo::TimeZone nyc = turbo::time_internal::LoadTimeZone("America/New_York");
  EXPECT_NE(la, nyc);
}

TEST(TimeZone, CCTZConversion) {
  const cctz::time_zone cz = cctz::utc_time_zone();
  const turbo::TimeZone tz(cz);
  EXPECT_EQ(cz, cctz::time_zone(tz));
}

TEST(TimeZone, DefaultTimeZones) {
  turbo::TimeZone tz;
  EXPECT_EQ("UTC", turbo::TimeZone().name());
  EXPECT_EQ("UTC", turbo::UTCTimeZone().name());
}

TEST(TimeZone, FixedTimeZone) {
  const turbo::TimeZone tz = turbo::FixedTimeZone(123);
  const cctz::time_zone cz = cctz::fixed_time_zone(cctz::seconds(123));
  EXPECT_EQ(tz, turbo::TimeZone(cz));
}

TEST(TimeZone, LocalTimeZone) {
  const turbo::TimeZone local_tz = turbo::LocalTimeZone();
  turbo::TimeZone tz = turbo::time_internal::LoadTimeZone("localtime");
  EXPECT_EQ(tz, local_tz);
}

TEST(TimeZone, NamedTimeZones) {
  turbo::TimeZone nyc = turbo::time_internal::LoadTimeZone("America/New_York");
  EXPECT_EQ("America/New_York", nyc.name());
  turbo::TimeZone syd = turbo::time_internal::LoadTimeZone("Australia/Sydney");
  EXPECT_EQ("Australia/Sydney", syd.name());
  turbo::TimeZone fixed = turbo::FixedTimeZone((((3 * 60) + 25) * 60) + 45);
  EXPECT_EQ("Fixed/UTC+03:25:45", fixed.name());
}

TEST(TimeZone, Failures) {
  turbo::TimeZone tz = turbo::time_internal::LoadTimeZone("America/Los_Angeles");
  EXPECT_FALSE(LoadTimeZone("Invalid/TimeZone", &tz));
  EXPECT_EQ(turbo::UTCTimeZone(), tz);  // guaranteed fallback to UTC

  // Ensures that the load still fails on a subsequent attempt.
  tz = turbo::time_internal::LoadTimeZone("America/Los_Angeles");
  EXPECT_FALSE(LoadTimeZone("Invalid/TimeZone", &tz));
  EXPECT_EQ(turbo::UTCTimeZone(), tz);  // guaranteed fallback to UTC

  // Loading an empty string timezone should fail.
  tz = turbo::time_internal::LoadTimeZone("America/Los_Angeles");
  EXPECT_FALSE(LoadTimeZone("", &tz));
  EXPECT_EQ(turbo::UTCTimeZone(), tz);  // guaranteed fallback to UTC
}

}  // namespace
