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

#include <turbo/flags/flag.h>

#include <string>

#include <gtest/gtest.h>
#include <turbo/flags/reflection.h>
#include <turbo/time/civil_time.h>
#include <turbo/time/time.h>

TURBO_FLAG(turbo::CivilSecond, test_flag_civil_second,
          turbo::CivilSecond(2015, 1, 2, 3, 4, 5), "");
TURBO_FLAG(turbo::CivilMinute, test_flag_civil_minute,
          turbo::CivilMinute(2015, 1, 2, 3, 4), "");
TURBO_FLAG(turbo::CivilHour, test_flag_civil_hour, turbo::CivilHour(2015, 1, 2, 3),
          "");
TURBO_FLAG(turbo::CivilDay, test_flag_civil_day, turbo::CivilDay(2015, 1, 2), "");
TURBO_FLAG(turbo::CivilMonth, test_flag_civil_month, turbo::CivilMonth(2015, 1),
          "");
TURBO_FLAG(turbo::CivilYear, test_flag_civil_year, turbo::CivilYear(2015), "");

TURBO_FLAG(turbo::Duration, test_duration_flag, turbo::Seconds(5),
          "For testing support for Duration flags");
TURBO_FLAG(turbo::Time, test_time_flag, turbo::InfinitePast(),
          "For testing support for Time flags");

namespace {

bool SetFlagValue(turbo::string_view flag_name, turbo::string_view value) {
  auto* flag = turbo::FindCommandLineFlag(flag_name);
  if (!flag) return false;
  std::string err;
  return flag->ParseFrom(value, &err);
}

bool GetFlagValue(turbo::string_view flag_name, std::string& value) {
  auto* flag = turbo::FindCommandLineFlag(flag_name);
  if (!flag) return false;
  value = flag->CurrentValue();
  return true;
}

TEST(CivilTime, FlagSupport) {
  // Tests the default setting of the flags.
  const turbo::CivilSecond kDefaultSec(2015, 1, 2, 3, 4, 5);
  EXPECT_EQ(turbo::CivilSecond(kDefaultSec),
            turbo::GetFlag(FLAGS_test_flag_civil_second));
  EXPECT_EQ(turbo::CivilMinute(kDefaultSec),
            turbo::GetFlag(FLAGS_test_flag_civil_minute));
  EXPECT_EQ(turbo::CivilHour(kDefaultSec),
            turbo::GetFlag(FLAGS_test_flag_civil_hour));
  EXPECT_EQ(turbo::CivilDay(kDefaultSec),
            turbo::GetFlag(FLAGS_test_flag_civil_day));
  EXPECT_EQ(turbo::CivilMonth(kDefaultSec),
            turbo::GetFlag(FLAGS_test_flag_civil_month));
  EXPECT_EQ(turbo::CivilYear(kDefaultSec),
            turbo::GetFlag(FLAGS_test_flag_civil_year));

  // Sets flags to a new value.
  const turbo::CivilSecond kNewSec(2016, 6, 7, 8, 9, 10);
  turbo::SetFlag(&FLAGS_test_flag_civil_second, turbo::CivilSecond(kNewSec));
  turbo::SetFlag(&FLAGS_test_flag_civil_minute, turbo::CivilMinute(kNewSec));
  turbo::SetFlag(&FLAGS_test_flag_civil_hour, turbo::CivilHour(kNewSec));
  turbo::SetFlag(&FLAGS_test_flag_civil_day, turbo::CivilDay(kNewSec));
  turbo::SetFlag(&FLAGS_test_flag_civil_month, turbo::CivilMonth(kNewSec));
  turbo::SetFlag(&FLAGS_test_flag_civil_year, turbo::CivilYear(kNewSec));

  EXPECT_EQ(turbo::CivilSecond(kNewSec),
            turbo::GetFlag(FLAGS_test_flag_civil_second));
  EXPECT_EQ(turbo::CivilMinute(kNewSec),
            turbo::GetFlag(FLAGS_test_flag_civil_minute));
  EXPECT_EQ(turbo::CivilHour(kNewSec),
            turbo::GetFlag(FLAGS_test_flag_civil_hour));
  EXPECT_EQ(turbo::CivilDay(kNewSec), turbo::GetFlag(FLAGS_test_flag_civil_day));
  EXPECT_EQ(turbo::CivilMonth(kNewSec),
            turbo::GetFlag(FLAGS_test_flag_civil_month));
  EXPECT_EQ(turbo::CivilYear(kNewSec),
            turbo::GetFlag(FLAGS_test_flag_civil_year));
}

TEST(Duration, FlagSupport) {
  EXPECT_EQ(turbo::Seconds(5), turbo::GetFlag(FLAGS_test_duration_flag));

  turbo::SetFlag(&FLAGS_test_duration_flag, turbo::Seconds(10));
  EXPECT_EQ(turbo::Seconds(10), turbo::GetFlag(FLAGS_test_duration_flag));

  EXPECT_TRUE(SetFlagValue("test_duration_flag", "20s"));
  EXPECT_EQ(turbo::Seconds(20), turbo::GetFlag(FLAGS_test_duration_flag));

  std::string current_flag_value;
  EXPECT_TRUE(GetFlagValue("test_duration_flag", current_flag_value));
  EXPECT_EQ("20s", current_flag_value);
}

TEST(Time, FlagSupport) {
  EXPECT_EQ(turbo::InfinitePast(), turbo::GetFlag(FLAGS_test_time_flag));

  const turbo::Time t = turbo::FromCivil(turbo::CivilSecond(2016, 1, 2, 3, 4, 5),
                                       turbo::UTCTimeZone());
  turbo::SetFlag(&FLAGS_test_time_flag, t);
  EXPECT_EQ(t, turbo::GetFlag(FLAGS_test_time_flag));

  // Successful parse
  EXPECT_TRUE(SetFlagValue("test_time_flag", "2016-01-02T03:04:06Z"));
  EXPECT_EQ(t + turbo::Seconds(1), turbo::GetFlag(FLAGS_test_time_flag));
  EXPECT_TRUE(SetFlagValue("test_time_flag", "2016-01-02T03:04:07.0Z"));
  EXPECT_EQ(t + turbo::Seconds(2), turbo::GetFlag(FLAGS_test_time_flag));
  EXPECT_TRUE(SetFlagValue("test_time_flag", "2016-01-02T03:04:08.000Z"));
  EXPECT_EQ(t + turbo::Seconds(3), turbo::GetFlag(FLAGS_test_time_flag));
  EXPECT_TRUE(SetFlagValue("test_time_flag", "2016-01-02T03:04:09+00:00"));
  EXPECT_EQ(t + turbo::Seconds(4), turbo::GetFlag(FLAGS_test_time_flag));
  EXPECT_TRUE(SetFlagValue("test_time_flag", "2016-01-02T03:04:05.123+00:00"));
  EXPECT_EQ(t + turbo::Milliseconds(123), turbo::GetFlag(FLAGS_test_time_flag));
  EXPECT_TRUE(SetFlagValue("test_time_flag", "2016-01-02T03:04:05.123+08:00"));
  EXPECT_EQ(t + turbo::Milliseconds(123) - turbo::Hours(8),
            turbo::GetFlag(FLAGS_test_time_flag));
  EXPECT_TRUE(SetFlagValue("test_time_flag", "infinite-future"));
  EXPECT_EQ(turbo::InfiniteFuture(), turbo::GetFlag(FLAGS_test_time_flag));
  EXPECT_TRUE(SetFlagValue("test_time_flag", "infinite-past"));
  EXPECT_EQ(turbo::InfinitePast(), turbo::GetFlag(FLAGS_test_time_flag));

  EXPECT_FALSE(SetFlagValue("test_time_flag", "2016-01-02T03:04:06"));
  EXPECT_FALSE(SetFlagValue("test_time_flag", "2016-01-02"));
  EXPECT_FALSE(SetFlagValue("test_time_flag", "2016-01-02Z"));
  EXPECT_FALSE(SetFlagValue("test_time_flag", "2016-01-02+00:00"));
  EXPECT_FALSE(SetFlagValue("test_time_flag", "2016-99-99T03:04:06Z"));

  EXPECT_TRUE(SetFlagValue("test_time_flag", "2016-01-02T03:04:05Z"));
  std::string current_flag_value;
  EXPECT_TRUE(GetFlagValue("test_time_flag", current_flag_value));
  EXPECT_EQ("2016-01-02T03:04:05+00:00", current_flag_value);
}

}  // namespace
