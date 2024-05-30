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

#include <turbo/times/cctz/time_zone.h>

#include <gtest/gtest.h>
#include <tests/times/test_util.h>
#include <turbo/times/time.h>

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
  turbo::TimeZone explicit_utc = turbo::TimeZone::utc();
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
  EXPECT_EQ("UTC", turbo::TimeZone::utc().name());
}

TEST(TimeZone, Fixed) {
  const turbo::TimeZone tz = turbo::TimeZone::fixed(123);
  const cctz::time_zone cz = cctz::fixed_time_zone(cctz::seconds(123));
  EXPECT_EQ(tz, turbo::TimeZone(cz));
}

TEST(TimeZone, LocalTimeZone) {
  const turbo::TimeZone local_tz = turbo::TimeZone::local();
  turbo::TimeZone tz = turbo::time_internal::LoadTimeZone("localtime");
  EXPECT_EQ(tz, local_tz);
}

TEST(TimeZone, NamedTimeZones) {
  turbo::TimeZone nyc = turbo::time_internal::LoadTimeZone("America/New_York");
  EXPECT_EQ("America/New_York", nyc.name());
  turbo::TimeZone syd = turbo::time_internal::LoadTimeZone("Australia/Sydney");
  EXPECT_EQ("Australia/Sydney", syd.name());
  turbo::TimeZone fixed = turbo::TimeZone::fixed((((3 * 60) + 25) * 60) + 45);
  EXPECT_EQ("Fixed/UTC+03:25:45", fixed.name());
}

TEST(TimeZone, Failures) {
  turbo::TimeZone tz = turbo::time_internal::LoadTimeZone("America/Los_Angeles");
  EXPECT_FALSE(turbo::TimeZone::load("Invalid/TimeZone", &tz));
  EXPECT_EQ(turbo::TimeZone::utc(), tz);  // guaranteed fallback to UTC

  // Ensures that the load still fails on a subsequent attempt.
  tz = turbo::time_internal::LoadTimeZone("America/Los_Angeles");
  EXPECT_FALSE(turbo::TimeZone::load("Invalid/TimeZone", &tz));
  EXPECT_EQ(turbo::TimeZone::utc(), tz);  // guaranteed fallback to UTC

  // Loading an empty string timezone should fail.
  tz = turbo::time_internal::LoadTimeZone("America/Los_Angeles");
  EXPECT_FALSE(turbo::TimeZone::load("", &tz));
  EXPECT_EQ(turbo::TimeZone::utc(), tz);  // guaranteed fallback to UTC
}

}  // namespace
