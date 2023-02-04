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

#include "turbo/time/civil_time.h"

#include <limits>
#include <sstream>
#include <type_traits>

#include "turbo/platform/port.h"
#include "gtest/gtest.h"

namespace {

TEST(CivilTime, DefaultConstruction) {
  turbo::CivilSecond ss;
  EXPECT_EQ("1970-01-01T00:00:00", turbo::FormatCivilTime(ss));

  turbo::CivilMinute mm;
  EXPECT_EQ("1970-01-01T00:00", turbo::FormatCivilTime(mm));

  turbo::CivilHour hh;
  EXPECT_EQ("1970-01-01T00", turbo::FormatCivilTime(hh));

  turbo::CivilDay d;
  EXPECT_EQ("1970-01-01", turbo::FormatCivilTime(d));

  turbo::CivilMonth m;
  EXPECT_EQ("1970-01", turbo::FormatCivilTime(m));

  turbo::CivilYear y;
  EXPECT_EQ("1970", turbo::FormatCivilTime(y));
}

TEST(CivilTime, StructMember) {
  struct S {
    turbo::CivilDay day;
  };
  S s = {};
  EXPECT_EQ(turbo::CivilDay{}, s.day);
}

TEST(CivilTime, FieldsConstruction) {
  EXPECT_EQ("2015-01-02T03:04:05",
            turbo::FormatCivilTime(turbo::CivilSecond(2015, 1, 2, 3, 4, 5)));
  EXPECT_EQ("2015-01-02T03:04:00",
            turbo::FormatCivilTime(turbo::CivilSecond(2015, 1, 2, 3, 4)));
  EXPECT_EQ("2015-01-02T03:00:00",
            turbo::FormatCivilTime(turbo::CivilSecond(2015, 1, 2, 3)));
  EXPECT_EQ("2015-01-02T00:00:00",
            turbo::FormatCivilTime(turbo::CivilSecond(2015, 1, 2)));
  EXPECT_EQ("2015-01-01T00:00:00",
            turbo::FormatCivilTime(turbo::CivilSecond(2015, 1)));
  EXPECT_EQ("2015-01-01T00:00:00",
            turbo::FormatCivilTime(turbo::CivilSecond(2015)));

  EXPECT_EQ("2015-01-02T03:04",
            turbo::FormatCivilTime(turbo::CivilMinute(2015, 1, 2, 3, 4, 5)));
  EXPECT_EQ("2015-01-02T03:04",
            turbo::FormatCivilTime(turbo::CivilMinute(2015, 1, 2, 3, 4)));
  EXPECT_EQ("2015-01-02T03:00",
            turbo::FormatCivilTime(turbo::CivilMinute(2015, 1, 2, 3)));
  EXPECT_EQ("2015-01-02T00:00",
            turbo::FormatCivilTime(turbo::CivilMinute(2015, 1, 2)));
  EXPECT_EQ("2015-01-01T00:00",
            turbo::FormatCivilTime(turbo::CivilMinute(2015, 1)));
  EXPECT_EQ("2015-01-01T00:00",
            turbo::FormatCivilTime(turbo::CivilMinute(2015)));

  EXPECT_EQ("2015-01-02T03",
            turbo::FormatCivilTime(turbo::CivilHour(2015, 1, 2, 3, 4, 5)));
  EXPECT_EQ("2015-01-02T03",
            turbo::FormatCivilTime(turbo::CivilHour(2015, 1, 2, 3, 4)));
  EXPECT_EQ("2015-01-02T03",
            turbo::FormatCivilTime(turbo::CivilHour(2015, 1, 2, 3)));
  EXPECT_EQ("2015-01-02T00",
            turbo::FormatCivilTime(turbo::CivilHour(2015, 1, 2)));
  EXPECT_EQ("2015-01-01T00",
            turbo::FormatCivilTime(turbo::CivilHour(2015, 1)));
  EXPECT_EQ("2015-01-01T00",
            turbo::FormatCivilTime(turbo::CivilHour(2015)));

  EXPECT_EQ("2015-01-02",
            turbo::FormatCivilTime(turbo::CivilDay(2015, 1, 2, 3, 4, 5)));
  EXPECT_EQ("2015-01-02",
            turbo::FormatCivilTime(turbo::CivilDay(2015, 1, 2, 3, 4)));
  EXPECT_EQ("2015-01-02",
            turbo::FormatCivilTime(turbo::CivilDay(2015, 1, 2, 3)));
  EXPECT_EQ("2015-01-02",
            turbo::FormatCivilTime(turbo::CivilDay(2015, 1, 2)));
  EXPECT_EQ("2015-01-01",
            turbo::FormatCivilTime(turbo::CivilDay(2015, 1)));
  EXPECT_EQ("2015-01-01",
            turbo::FormatCivilTime(turbo::CivilDay(2015)));

  EXPECT_EQ("2015-01",
            turbo::FormatCivilTime(turbo::CivilMonth(2015, 1, 2, 3, 4, 5)));
  EXPECT_EQ("2015-01",
            turbo::FormatCivilTime(turbo::CivilMonth(2015, 1, 2, 3, 4)));
  EXPECT_EQ("2015-01",
            turbo::FormatCivilTime(turbo::CivilMonth(2015, 1, 2, 3)));
  EXPECT_EQ("2015-01",
            turbo::FormatCivilTime(turbo::CivilMonth(2015, 1, 2)));
  EXPECT_EQ("2015-01",
            turbo::FormatCivilTime(turbo::CivilMonth(2015, 1)));
  EXPECT_EQ("2015-01",
            turbo::FormatCivilTime(turbo::CivilMonth(2015)));

  EXPECT_EQ("2015",
            turbo::FormatCivilTime(turbo::CivilYear(2015, 1, 2, 3, 4, 5)));
  EXPECT_EQ("2015",
            turbo::FormatCivilTime(turbo::CivilYear(2015, 1, 2, 3, 4)));
  EXPECT_EQ("2015",
            turbo::FormatCivilTime(turbo::CivilYear(2015, 1, 2, 3)));
  EXPECT_EQ("2015",
            turbo::FormatCivilTime(turbo::CivilYear(2015, 1, 2)));
  EXPECT_EQ("2015",
            turbo::FormatCivilTime(turbo::CivilYear(2015, 1)));
  EXPECT_EQ("2015",
            turbo::FormatCivilTime(turbo::CivilYear(2015)));
}

TEST(CivilTime, FieldsConstructionLimits) {
  const int kIntMax = std::numeric_limits<int>::max();
  EXPECT_EQ("2038-01-19T03:14:07",
            turbo::FormatCivilTime(turbo::CivilSecond(
                1970, 1, 1, 0, 0, kIntMax)));
  EXPECT_EQ("6121-02-11T05:21:07",
            turbo::FormatCivilTime(turbo::CivilSecond(
                1970, 1, 1, 0, kIntMax, kIntMax)));
  EXPECT_EQ("251104-11-20T12:21:07",
            turbo::FormatCivilTime(turbo::CivilSecond(
                1970, 1, 1, kIntMax, kIntMax, kIntMax)));
  EXPECT_EQ("6130715-05-30T12:21:07",
            turbo::FormatCivilTime(turbo::CivilSecond(
                1970, 1, kIntMax, kIntMax, kIntMax, kIntMax)));
  EXPECT_EQ("185087685-11-26T12:21:07",
            turbo::FormatCivilTime(turbo::CivilSecond(
                1970, kIntMax, kIntMax, kIntMax, kIntMax, kIntMax)));

  const int kIntMin = std::numeric_limits<int>::min();
  EXPECT_EQ("1901-12-13T20:45:52",
            turbo::FormatCivilTime(turbo::CivilSecond(
                1970, 1, 1, 0, 0, kIntMin)));
  EXPECT_EQ("-2182-11-20T18:37:52",
            turbo::FormatCivilTime(turbo::CivilSecond(
                1970, 1, 1, 0, kIntMin, kIntMin)));
  EXPECT_EQ("-247165-02-11T10:37:52",
            turbo::FormatCivilTime(turbo::CivilSecond(
                1970, 1, 1, kIntMin, kIntMin, kIntMin)));
  EXPECT_EQ("-6126776-08-01T10:37:52",
            turbo::FormatCivilTime(turbo::CivilSecond(
                1970, 1, kIntMin, kIntMin, kIntMin, kIntMin)));
  EXPECT_EQ("-185083747-10-31T10:37:52",
            turbo::FormatCivilTime(turbo::CivilSecond(
                1970, kIntMin, kIntMin, kIntMin, kIntMin, kIntMin)));
}

TEST(CivilTime, RangeLimits) {
  const turbo::civil_year_t kYearMax =
      std::numeric_limits<turbo::civil_year_t>::max();
  EXPECT_EQ(turbo::CivilYear(kYearMax),
            turbo::CivilYear::max());
  EXPECT_EQ(turbo::CivilMonth(kYearMax, 12),
            turbo::CivilMonth::max());
  EXPECT_EQ(turbo::CivilDay(kYearMax, 12, 31),
            turbo::CivilDay::max());
  EXPECT_EQ(turbo::CivilHour(kYearMax, 12, 31, 23),
            turbo::CivilHour::max());
  EXPECT_EQ(turbo::CivilMinute(kYearMax, 12, 31, 23, 59),
            turbo::CivilMinute::max());
  EXPECT_EQ(turbo::CivilSecond(kYearMax, 12, 31, 23, 59, 59),
            turbo::CivilSecond::max());

  const turbo::civil_year_t kYearMin =
      std::numeric_limits<turbo::civil_year_t>::min();
  EXPECT_EQ(turbo::CivilYear(kYearMin),
            turbo::CivilYear::min());
  EXPECT_EQ(turbo::CivilMonth(kYearMin, 1),
            turbo::CivilMonth::min());
  EXPECT_EQ(turbo::CivilDay(kYearMin, 1, 1),
            turbo::CivilDay::min());
  EXPECT_EQ(turbo::CivilHour(kYearMin, 1, 1, 0),
            turbo::CivilHour::min());
  EXPECT_EQ(turbo::CivilMinute(kYearMin, 1, 1, 0, 0),
            turbo::CivilMinute::min());
  EXPECT_EQ(turbo::CivilSecond(kYearMin, 1, 1, 0, 0, 0),
            turbo::CivilSecond::min());
}

TEST(CivilTime, ImplicitCrossAlignment) {
  turbo::CivilYear year(2015);
  turbo::CivilMonth month = year;
  turbo::CivilDay day = month;
  turbo::CivilHour hour = day;
  turbo::CivilMinute minute = hour;
  turbo::CivilSecond second = minute;

  second = year;
  EXPECT_EQ(second, year);
  second = month;
  EXPECT_EQ(second, month);
  second = day;
  EXPECT_EQ(second, day);
  second = hour;
  EXPECT_EQ(second, hour);
  second = minute;
  EXPECT_EQ(second, minute);

  minute = year;
  EXPECT_EQ(minute, year);
  minute = month;
  EXPECT_EQ(minute, month);
  minute = day;
  EXPECT_EQ(minute, day);
  minute = hour;
  EXPECT_EQ(minute, hour);

  hour = year;
  EXPECT_EQ(hour, year);
  hour = month;
  EXPECT_EQ(hour, month);
  hour = day;
  EXPECT_EQ(hour, day);

  day = year;
  EXPECT_EQ(day, year);
  day = month;
  EXPECT_EQ(day, month);

  month = year;
  EXPECT_EQ(month, year);

  // Ensures unsafe conversions are not allowed.
  EXPECT_FALSE(
      (std::is_convertible<turbo::CivilSecond, turbo::CivilMinute>::value));
  EXPECT_FALSE(
      (std::is_convertible<turbo::CivilSecond, turbo::CivilHour>::value));
  EXPECT_FALSE(
      (std::is_convertible<turbo::CivilSecond, turbo::CivilDay>::value));
  EXPECT_FALSE(
      (std::is_convertible<turbo::CivilSecond, turbo::CivilMonth>::value));
  EXPECT_FALSE(
      (std::is_convertible<turbo::CivilSecond, turbo::CivilYear>::value));

  EXPECT_FALSE(
      (std::is_convertible<turbo::CivilMinute, turbo::CivilHour>::value));
  EXPECT_FALSE(
      (std::is_convertible<turbo::CivilMinute, turbo::CivilDay>::value));
  EXPECT_FALSE(
      (std::is_convertible<turbo::CivilMinute, turbo::CivilMonth>::value));
  EXPECT_FALSE(
      (std::is_convertible<turbo::CivilMinute, turbo::CivilYear>::value));

  EXPECT_FALSE(
      (std::is_convertible<turbo::CivilHour, turbo::CivilDay>::value));
  EXPECT_FALSE(
      (std::is_convertible<turbo::CivilHour, turbo::CivilMonth>::value));
  EXPECT_FALSE(
      (std::is_convertible<turbo::CivilHour, turbo::CivilYear>::value));

  EXPECT_FALSE(
      (std::is_convertible<turbo::CivilDay, turbo::CivilMonth>::value));
  EXPECT_FALSE(
      (std::is_convertible<turbo::CivilDay, turbo::CivilYear>::value));

  EXPECT_FALSE(
      (std::is_convertible<turbo::CivilMonth, turbo::CivilYear>::value));
}

TEST(CivilTime, ExplicitCrossAlignment) {
  //
  // Assign from smaller units -> larger units
  //

  turbo::CivilSecond second(2015, 1, 2, 3, 4, 5);
  EXPECT_EQ("2015-01-02T03:04:05", turbo::FormatCivilTime(second));

  turbo::CivilMinute minute(second);
  EXPECT_EQ("2015-01-02T03:04", turbo::FormatCivilTime(minute));

  turbo::CivilHour hour(minute);
  EXPECT_EQ("2015-01-02T03", turbo::FormatCivilTime(hour));

  turbo::CivilDay day(hour);
  EXPECT_EQ("2015-01-02", turbo::FormatCivilTime(day));

  turbo::CivilMonth month(day);
  EXPECT_EQ("2015-01", turbo::FormatCivilTime(month));

  turbo::CivilYear year(month);
  EXPECT_EQ("2015", turbo::FormatCivilTime(year));

  //
  // Now assign from larger units -> smaller units
  //

  month = turbo::CivilMonth(year);
  EXPECT_EQ("2015-01", turbo::FormatCivilTime(month));

  day = turbo::CivilDay(month);
  EXPECT_EQ("2015-01-01", turbo::FormatCivilTime(day));

  hour = turbo::CivilHour(day);
  EXPECT_EQ("2015-01-01T00", turbo::FormatCivilTime(hour));

  minute = turbo::CivilMinute(hour);
  EXPECT_EQ("2015-01-01T00:00", turbo::FormatCivilTime(minute));

  second = turbo::CivilSecond(minute);
  EXPECT_EQ("2015-01-01T00:00:00", turbo::FormatCivilTime(second));
}

// Metafunction to test whether difference is allowed between two types.
template <typename T1, typename T2>
struct HasDiff {
  template <typename U1, typename U2>
  static std::false_type test(...);
  template <typename U1, typename U2>
  static std::true_type test(decltype(std::declval<U1>() - std::declval<U2>()));
  static constexpr bool value = decltype(test<T1, T2>(0))::value;
};

TEST(CivilTime, DisallowCrossAlignedDifference) {
  // Difference is allowed between types with the same alignment.
  static_assert(HasDiff<turbo::CivilSecond, turbo::CivilSecond>::value, "");
  static_assert(HasDiff<turbo::CivilMinute, turbo::CivilMinute>::value, "");
  static_assert(HasDiff<turbo::CivilHour, turbo::CivilHour>::value, "");
  static_assert(HasDiff<turbo::CivilDay, turbo::CivilDay>::value, "");
  static_assert(HasDiff<turbo::CivilMonth, turbo::CivilMonth>::value, "");
  static_assert(HasDiff<turbo::CivilYear, turbo::CivilYear>::value, "");

  // Difference is disallowed between types with different alignments.
  static_assert(!HasDiff<turbo::CivilSecond, turbo::CivilMinute>::value, "");
  static_assert(!HasDiff<turbo::CivilSecond, turbo::CivilHour>::value, "");
  static_assert(!HasDiff<turbo::CivilSecond, turbo::CivilDay>::value, "");
  static_assert(!HasDiff<turbo::CivilSecond, turbo::CivilMonth>::value, "");
  static_assert(!HasDiff<turbo::CivilSecond, turbo::CivilYear>::value, "");

  static_assert(!HasDiff<turbo::CivilMinute, turbo::CivilHour>::value, "");
  static_assert(!HasDiff<turbo::CivilMinute, turbo::CivilDay>::value, "");
  static_assert(!HasDiff<turbo::CivilMinute, turbo::CivilMonth>::value, "");
  static_assert(!HasDiff<turbo::CivilMinute, turbo::CivilYear>::value, "");

  static_assert(!HasDiff<turbo::CivilHour, turbo::CivilDay>::value, "");
  static_assert(!HasDiff<turbo::CivilHour, turbo::CivilMonth>::value, "");
  static_assert(!HasDiff<turbo::CivilHour, turbo::CivilYear>::value, "");

  static_assert(!HasDiff<turbo::CivilDay, turbo::CivilMonth>::value, "");
  static_assert(!HasDiff<turbo::CivilDay, turbo::CivilYear>::value, "");

  static_assert(!HasDiff<turbo::CivilMonth, turbo::CivilYear>::value, "");
}

TEST(CivilTime, ValueSemantics) {
  const turbo::CivilHour a(2015, 1, 2, 3);
  const turbo::CivilHour b = a;
  const turbo::CivilHour c(b);
  turbo::CivilHour d;
  d = c;
  EXPECT_EQ("2015-01-02T03", turbo::FormatCivilTime(d));
}

TEST(CivilTime, Relational) {
  // Tests that the alignment unit is ignored in comparison.
  const turbo::CivilYear year(2014);
  const turbo::CivilMonth month(year);
  EXPECT_EQ(year, month);

#define TEST_RELATIONAL(OLDER, YOUNGER) \
  do {                                  \
    EXPECT_FALSE(OLDER < OLDER);        \
    EXPECT_FALSE(OLDER > OLDER);        \
    EXPECT_TRUE(OLDER >= OLDER);        \
    EXPECT_TRUE(OLDER <= OLDER);        \
    EXPECT_FALSE(YOUNGER < YOUNGER);    \
    EXPECT_FALSE(YOUNGER > YOUNGER);    \
    EXPECT_TRUE(YOUNGER >= YOUNGER);    \
    EXPECT_TRUE(YOUNGER <= YOUNGER);    \
    EXPECT_EQ(OLDER, OLDER);            \
    EXPECT_NE(OLDER, YOUNGER);          \
    EXPECT_LT(OLDER, YOUNGER);          \
    EXPECT_LE(OLDER, YOUNGER);          \
    EXPECT_GT(YOUNGER, OLDER);          \
    EXPECT_GE(YOUNGER, OLDER);          \
  } while (0)

  // Alignment is ignored in comparison (verified above), so CivilSecond is
  // used to test comparison in all field positions.
  TEST_RELATIONAL(turbo::CivilSecond(2014, 1, 1, 0, 0, 0),
                  turbo::CivilSecond(2015, 1, 1, 0, 0, 0));
  TEST_RELATIONAL(turbo::CivilSecond(2014, 1, 1, 0, 0, 0),
                  turbo::CivilSecond(2014, 2, 1, 0, 0, 0));
  TEST_RELATIONAL(turbo::CivilSecond(2014, 1, 1, 0, 0, 0),
                  turbo::CivilSecond(2014, 1, 2, 0, 0, 0));
  TEST_RELATIONAL(turbo::CivilSecond(2014, 1, 1, 0, 0, 0),
                  turbo::CivilSecond(2014, 1, 1, 1, 0, 0));
  TEST_RELATIONAL(turbo::CivilSecond(2014, 1, 1, 1, 0, 0),
                  turbo::CivilSecond(2014, 1, 1, 1, 1, 0));
  TEST_RELATIONAL(turbo::CivilSecond(2014, 1, 1, 1, 1, 0),
                  turbo::CivilSecond(2014, 1, 1, 1, 1, 1));

  // Tests the relational operators of two different civil-time types.
  TEST_RELATIONAL(turbo::CivilDay(2014, 1, 1),
                  turbo::CivilMinute(2014, 1, 1, 1, 1));
  TEST_RELATIONAL(turbo::CivilDay(2014, 1, 1),
                  turbo::CivilMonth(2014, 2));

#undef TEST_RELATIONAL
}

TEST(CivilTime, Arithmetic) {
  turbo::CivilSecond second(2015, 1, 2, 3, 4, 5);
  EXPECT_EQ("2015-01-02T03:04:06", turbo::FormatCivilTime(second += 1));
  EXPECT_EQ("2015-01-02T03:04:07", turbo::FormatCivilTime(second + 1));
  EXPECT_EQ("2015-01-02T03:04:08", turbo::FormatCivilTime(2 + second));
  EXPECT_EQ("2015-01-02T03:04:05", turbo::FormatCivilTime(second - 1));
  EXPECT_EQ("2015-01-02T03:04:05", turbo::FormatCivilTime(second -= 1));
  EXPECT_EQ("2015-01-02T03:04:05", turbo::FormatCivilTime(second++));
  EXPECT_EQ("2015-01-02T03:04:07", turbo::FormatCivilTime(++second));
  EXPECT_EQ("2015-01-02T03:04:07", turbo::FormatCivilTime(second--));
  EXPECT_EQ("2015-01-02T03:04:05", turbo::FormatCivilTime(--second));

  turbo::CivilMinute minute(2015, 1, 2, 3, 4);
  EXPECT_EQ("2015-01-02T03:05", turbo::FormatCivilTime(minute += 1));
  EXPECT_EQ("2015-01-02T03:06", turbo::FormatCivilTime(minute + 1));
  EXPECT_EQ("2015-01-02T03:07", turbo::FormatCivilTime(2 + minute));
  EXPECT_EQ("2015-01-02T03:04", turbo::FormatCivilTime(minute - 1));
  EXPECT_EQ("2015-01-02T03:04", turbo::FormatCivilTime(minute -= 1));
  EXPECT_EQ("2015-01-02T03:04", turbo::FormatCivilTime(minute++));
  EXPECT_EQ("2015-01-02T03:06", turbo::FormatCivilTime(++minute));
  EXPECT_EQ("2015-01-02T03:06", turbo::FormatCivilTime(minute--));
  EXPECT_EQ("2015-01-02T03:04", turbo::FormatCivilTime(--minute));

  turbo::CivilHour hour(2015, 1, 2, 3);
  EXPECT_EQ("2015-01-02T04", turbo::FormatCivilTime(hour += 1));
  EXPECT_EQ("2015-01-02T05", turbo::FormatCivilTime(hour + 1));
  EXPECT_EQ("2015-01-02T06", turbo::FormatCivilTime(2 + hour));
  EXPECT_EQ("2015-01-02T03", turbo::FormatCivilTime(hour - 1));
  EXPECT_EQ("2015-01-02T03", turbo::FormatCivilTime(hour -= 1));
  EXPECT_EQ("2015-01-02T03", turbo::FormatCivilTime(hour++));
  EXPECT_EQ("2015-01-02T05", turbo::FormatCivilTime(++hour));
  EXPECT_EQ("2015-01-02T05", turbo::FormatCivilTime(hour--));
  EXPECT_EQ("2015-01-02T03", turbo::FormatCivilTime(--hour));

  turbo::CivilDay day(2015, 1, 2);
  EXPECT_EQ("2015-01-03", turbo::FormatCivilTime(day += 1));
  EXPECT_EQ("2015-01-04", turbo::FormatCivilTime(day + 1));
  EXPECT_EQ("2015-01-05", turbo::FormatCivilTime(2 + day));
  EXPECT_EQ("2015-01-02", turbo::FormatCivilTime(day - 1));
  EXPECT_EQ("2015-01-02", turbo::FormatCivilTime(day -= 1));
  EXPECT_EQ("2015-01-02", turbo::FormatCivilTime(day++));
  EXPECT_EQ("2015-01-04", turbo::FormatCivilTime(++day));
  EXPECT_EQ("2015-01-04", turbo::FormatCivilTime(day--));
  EXPECT_EQ("2015-01-02", turbo::FormatCivilTime(--day));

  turbo::CivilMonth month(2015, 1);
  EXPECT_EQ("2015-02", turbo::FormatCivilTime(month += 1));
  EXPECT_EQ("2015-03", turbo::FormatCivilTime(month + 1));
  EXPECT_EQ("2015-04", turbo::FormatCivilTime(2 + month));
  EXPECT_EQ("2015-01", turbo::FormatCivilTime(month - 1));
  EXPECT_EQ("2015-01", turbo::FormatCivilTime(month -= 1));
  EXPECT_EQ("2015-01", turbo::FormatCivilTime(month++));
  EXPECT_EQ("2015-03", turbo::FormatCivilTime(++month));
  EXPECT_EQ("2015-03", turbo::FormatCivilTime(month--));
  EXPECT_EQ("2015-01", turbo::FormatCivilTime(--month));

  turbo::CivilYear year(2015);
  EXPECT_EQ("2016", turbo::FormatCivilTime(year += 1));
  EXPECT_EQ("2017", turbo::FormatCivilTime(year + 1));
  EXPECT_EQ("2018", turbo::FormatCivilTime(2 + year));
  EXPECT_EQ("2015", turbo::FormatCivilTime(year - 1));
  EXPECT_EQ("2015", turbo::FormatCivilTime(year -= 1));
  EXPECT_EQ("2015", turbo::FormatCivilTime(year++));
  EXPECT_EQ("2017", turbo::FormatCivilTime(++year));
  EXPECT_EQ("2017", turbo::FormatCivilTime(year--));
  EXPECT_EQ("2015", turbo::FormatCivilTime(--year));
}

TEST(CivilTime, ArithmeticLimits) {
  const int kIntMax = std::numeric_limits<int>::max();
  const int kIntMin = std::numeric_limits<int>::min();

  turbo::CivilSecond second(1970, 1, 1, 0, 0, 0);
  second += kIntMax;
  EXPECT_EQ("2038-01-19T03:14:07", turbo::FormatCivilTime(second));
  second -= kIntMax;
  EXPECT_EQ("1970-01-01T00:00:00", turbo::FormatCivilTime(second));
  second += kIntMin;
  EXPECT_EQ("1901-12-13T20:45:52", turbo::FormatCivilTime(second));
  second -= kIntMin;
  EXPECT_EQ("1970-01-01T00:00:00", turbo::FormatCivilTime(second));

  turbo::CivilMinute minute(1970, 1, 1, 0, 0);
  minute += kIntMax;
  EXPECT_EQ("6053-01-23T02:07", turbo::FormatCivilTime(minute));
  minute -= kIntMax;
  EXPECT_EQ("1970-01-01T00:00", turbo::FormatCivilTime(minute));
  minute += kIntMin;
  EXPECT_EQ("-2114-12-08T21:52", turbo::FormatCivilTime(minute));
  minute -= kIntMin;
  EXPECT_EQ("1970-01-01T00:00", turbo::FormatCivilTime(minute));

  turbo::CivilHour hour(1970, 1, 1, 0);
  hour += kIntMax;
  EXPECT_EQ("246953-10-09T07", turbo::FormatCivilTime(hour));
  hour -= kIntMax;
  EXPECT_EQ("1970-01-01T00", turbo::FormatCivilTime(hour));
  hour += kIntMin;
  EXPECT_EQ("-243014-03-24T16", turbo::FormatCivilTime(hour));
  hour -= kIntMin;
  EXPECT_EQ("1970-01-01T00", turbo::FormatCivilTime(hour));

  turbo::CivilDay day(1970, 1, 1);
  day += kIntMax;
  EXPECT_EQ("5881580-07-11", turbo::FormatCivilTime(day));
  day -= kIntMax;
  EXPECT_EQ("1970-01-01", turbo::FormatCivilTime(day));
  day += kIntMin;
  EXPECT_EQ("-5877641-06-23", turbo::FormatCivilTime(day));
  day -= kIntMin;
  EXPECT_EQ("1970-01-01", turbo::FormatCivilTime(day));

  turbo::CivilMonth month(1970, 1);
  month += kIntMax;
  EXPECT_EQ("178958940-08", turbo::FormatCivilTime(month));
  month -= kIntMax;
  EXPECT_EQ("1970-01", turbo::FormatCivilTime(month));
  month += kIntMin;
  EXPECT_EQ("-178955001-05", turbo::FormatCivilTime(month));
  month -= kIntMin;
  EXPECT_EQ("1970-01", turbo::FormatCivilTime(month));

  turbo::CivilYear year(0);
  year += kIntMax;
  EXPECT_EQ("2147483647", turbo::FormatCivilTime(year));
  year -= kIntMax;
  EXPECT_EQ("0", turbo::FormatCivilTime(year));
  year += kIntMin;
  EXPECT_EQ("-2147483648", turbo::FormatCivilTime(year));
  year -= kIntMin;
  EXPECT_EQ("0", turbo::FormatCivilTime(year));
}

TEST(CivilTime, Difference) {
  turbo::CivilSecond second(2015, 1, 2, 3, 4, 5);
  EXPECT_EQ(0, second - second);
  EXPECT_EQ(10, (second + 10) - second);
  EXPECT_EQ(-10, (second - 10) - second);

  turbo::CivilMinute minute(2015, 1, 2, 3, 4);
  EXPECT_EQ(0, minute - minute);
  EXPECT_EQ(10, (minute + 10) - minute);
  EXPECT_EQ(-10, (minute - 10) - minute);

  turbo::CivilHour hour(2015, 1, 2, 3);
  EXPECT_EQ(0, hour - hour);
  EXPECT_EQ(10, (hour + 10) - hour);
  EXPECT_EQ(-10, (hour - 10) - hour);

  turbo::CivilDay day(2015, 1, 2);
  EXPECT_EQ(0, day - day);
  EXPECT_EQ(10, (day + 10) - day);
  EXPECT_EQ(-10, (day - 10) - day);

  turbo::CivilMonth month(2015, 1);
  EXPECT_EQ(0, month - month);
  EXPECT_EQ(10, (month + 10) - month);
  EXPECT_EQ(-10, (month - 10) - month);

  turbo::CivilYear year(2015);
  EXPECT_EQ(0, year - year);
  EXPECT_EQ(10, (year + 10) - year);
  EXPECT_EQ(-10, (year - 10) - year);
}

TEST(CivilTime, DifferenceLimits) {
  const turbo::civil_diff_t kDiffMax =
      std::numeric_limits<turbo::civil_diff_t>::max();
  const turbo::civil_diff_t kDiffMin =
      std::numeric_limits<turbo::civil_diff_t>::min();

  // Check day arithmetic at the end of the year range.
  const turbo::CivilDay max_day(kDiffMax, 12, 31);
  EXPECT_EQ(1, max_day - (max_day - 1));
  EXPECT_EQ(-1, (max_day - 1) - max_day);

  // Check day arithmetic at the start of the year range.
  const turbo::CivilDay min_day(kDiffMin, 1, 1);
  EXPECT_EQ(1, (min_day + 1) - min_day);
  EXPECT_EQ(-1, min_day - (min_day + 1));

  // Check the limits of the return value.
  const turbo::CivilDay d1(1970, 1, 1);
  const turbo::CivilDay d2(25252734927768524, 7, 27);
  EXPECT_EQ(kDiffMax, d2 - d1);
  EXPECT_EQ(kDiffMin, d1 - (d2 + 1));
}

TEST(CivilTime, Properties) {
  turbo::CivilSecond ss(2015, 2, 3, 4, 5, 6);
  EXPECT_EQ(2015, ss.year());
  EXPECT_EQ(2, ss.month());
  EXPECT_EQ(3, ss.day());
  EXPECT_EQ(4, ss.hour());
  EXPECT_EQ(5, ss.minute());
  EXPECT_EQ(6, ss.second());
  EXPECT_EQ(turbo::Weekday::tuesday, turbo::GetWeekday(ss));
  EXPECT_EQ(34, turbo::GetYearDay(ss));

  turbo::CivilMinute mm(2015, 2, 3, 4, 5, 6);
  EXPECT_EQ(2015, mm.year());
  EXPECT_EQ(2, mm.month());
  EXPECT_EQ(3, mm.day());
  EXPECT_EQ(4, mm.hour());
  EXPECT_EQ(5, mm.minute());
  EXPECT_EQ(0, mm.second());
  EXPECT_EQ(turbo::Weekday::tuesday, turbo::GetWeekday(mm));
  EXPECT_EQ(34, turbo::GetYearDay(mm));

  turbo::CivilHour hh(2015, 2, 3, 4, 5, 6);
  EXPECT_EQ(2015, hh.year());
  EXPECT_EQ(2, hh.month());
  EXPECT_EQ(3, hh.day());
  EXPECT_EQ(4, hh.hour());
  EXPECT_EQ(0, hh.minute());
  EXPECT_EQ(0, hh.second());
  EXPECT_EQ(turbo::Weekday::tuesday, turbo::GetWeekday(hh));
  EXPECT_EQ(34, turbo::GetYearDay(hh));

  turbo::CivilDay d(2015, 2, 3, 4, 5, 6);
  EXPECT_EQ(2015, d.year());
  EXPECT_EQ(2, d.month());
  EXPECT_EQ(3, d.day());
  EXPECT_EQ(0, d.hour());
  EXPECT_EQ(0, d.minute());
  EXPECT_EQ(0, d.second());
  EXPECT_EQ(turbo::Weekday::tuesday, turbo::GetWeekday(d));
  EXPECT_EQ(34, turbo::GetYearDay(d));

  turbo::CivilMonth m(2015, 2, 3, 4, 5, 6);
  EXPECT_EQ(2015, m.year());
  EXPECT_EQ(2, m.month());
  EXPECT_EQ(1, m.day());
  EXPECT_EQ(0, m.hour());
  EXPECT_EQ(0, m.minute());
  EXPECT_EQ(0, m.second());
  EXPECT_EQ(turbo::Weekday::sunday, turbo::GetWeekday(m));
  EXPECT_EQ(32, turbo::GetYearDay(m));

  turbo::CivilYear y(2015, 2, 3, 4, 5, 6);
  EXPECT_EQ(2015, y.year());
  EXPECT_EQ(1, y.month());
  EXPECT_EQ(1, y.day());
  EXPECT_EQ(0, y.hour());
  EXPECT_EQ(0, y.minute());
  EXPECT_EQ(0, y.second());
  EXPECT_EQ(turbo::Weekday::thursday, turbo::GetWeekday(y));
  EXPECT_EQ(1, turbo::GetYearDay(y));
}

TEST(CivilTime, Format) {
  turbo::CivilSecond ss;
  EXPECT_EQ("1970-01-01T00:00:00", turbo::FormatCivilTime(ss));

  turbo::CivilMinute mm;
  EXPECT_EQ("1970-01-01T00:00", turbo::FormatCivilTime(mm));

  turbo::CivilHour hh;
  EXPECT_EQ("1970-01-01T00", turbo::FormatCivilTime(hh));

  turbo::CivilDay d;
  EXPECT_EQ("1970-01-01", turbo::FormatCivilTime(d));

  turbo::CivilMonth m;
  EXPECT_EQ("1970-01", turbo::FormatCivilTime(m));

  turbo::CivilYear y;
  EXPECT_EQ("1970", turbo::FormatCivilTime(y));
}

TEST(CivilTime, Parse) {
  turbo::CivilSecond ss;
  turbo::CivilMinute mm;
  turbo::CivilHour hh;
  turbo::CivilDay d;
  turbo::CivilMonth m;
  turbo::CivilYear y;

  // CivilSecond OK; others fail
  EXPECT_TRUE(turbo::ParseCivilTime("2015-01-02T03:04:05", &ss));
  EXPECT_EQ("2015-01-02T03:04:05", turbo::FormatCivilTime(ss));
  EXPECT_FALSE(turbo::ParseCivilTime("2015-01-02T03:04:05", &mm));
  EXPECT_FALSE(turbo::ParseCivilTime("2015-01-02T03:04:05", &hh));
  EXPECT_FALSE(turbo::ParseCivilTime("2015-01-02T03:04:05", &d));
  EXPECT_FALSE(turbo::ParseCivilTime("2015-01-02T03:04:05", &m));
  EXPECT_FALSE(turbo::ParseCivilTime("2015-01-02T03:04:05", &y));

  // CivilMinute OK; others fail
  EXPECT_FALSE(turbo::ParseCivilTime("2015-01-02T03:04", &ss));
  EXPECT_TRUE(turbo::ParseCivilTime("2015-01-02T03:04", &mm));
  EXPECT_EQ("2015-01-02T03:04", turbo::FormatCivilTime(mm));
  EXPECT_FALSE(turbo::ParseCivilTime("2015-01-02T03:04", &hh));
  EXPECT_FALSE(turbo::ParseCivilTime("2015-01-02T03:04", &d));
  EXPECT_FALSE(turbo::ParseCivilTime("2015-01-02T03:04", &m));
  EXPECT_FALSE(turbo::ParseCivilTime("2015-01-02T03:04", &y));

  // CivilHour OK; others fail
  EXPECT_FALSE(turbo::ParseCivilTime("2015-01-02T03", &ss));
  EXPECT_FALSE(turbo::ParseCivilTime("2015-01-02T03", &mm));
  EXPECT_TRUE(turbo::ParseCivilTime("2015-01-02T03", &hh));
  EXPECT_EQ("2015-01-02T03", turbo::FormatCivilTime(hh));
  EXPECT_FALSE(turbo::ParseCivilTime("2015-01-02T03", &d));
  EXPECT_FALSE(turbo::ParseCivilTime("2015-01-02T03", &m));
  EXPECT_FALSE(turbo::ParseCivilTime("2015-01-02T03", &y));

  // CivilDay OK; others fail
  EXPECT_FALSE(turbo::ParseCivilTime("2015-01-02", &ss));
  EXPECT_FALSE(turbo::ParseCivilTime("2015-01-02", &mm));
  EXPECT_FALSE(turbo::ParseCivilTime("2015-01-02", &hh));
  EXPECT_TRUE(turbo::ParseCivilTime("2015-01-02", &d));
  EXPECT_EQ("2015-01-02", turbo::FormatCivilTime(d));
  EXPECT_FALSE(turbo::ParseCivilTime("2015-01-02", &m));
  EXPECT_FALSE(turbo::ParseCivilTime("2015-01-02", &y));

  // CivilMonth OK; others fail
  EXPECT_FALSE(turbo::ParseCivilTime("2015-01", &ss));
  EXPECT_FALSE(turbo::ParseCivilTime("2015-01", &mm));
  EXPECT_FALSE(turbo::ParseCivilTime("2015-01", &hh));
  EXPECT_FALSE(turbo::ParseCivilTime("2015-01", &d));
  EXPECT_TRUE(turbo::ParseCivilTime("2015-01", &m));
  EXPECT_EQ("2015-01", turbo::FormatCivilTime(m));
  EXPECT_FALSE(turbo::ParseCivilTime("2015-01", &y));

  // CivilYear OK; others fail
  EXPECT_FALSE(turbo::ParseCivilTime("2015", &ss));
  EXPECT_FALSE(turbo::ParseCivilTime("2015", &mm));
  EXPECT_FALSE(turbo::ParseCivilTime("2015", &hh));
  EXPECT_FALSE(turbo::ParseCivilTime("2015", &d));
  EXPECT_FALSE(turbo::ParseCivilTime("2015", &m));
  EXPECT_TRUE(turbo::ParseCivilTime("2015", &y));
  EXPECT_EQ("2015", turbo::FormatCivilTime(y));
}

TEST(CivilTime, FormatAndParseLenient) {
  turbo::CivilSecond ss;
  EXPECT_EQ("1970-01-01T00:00:00", turbo::FormatCivilTime(ss));

  turbo::CivilMinute mm;
  EXPECT_EQ("1970-01-01T00:00", turbo::FormatCivilTime(mm));

  turbo::CivilHour hh;
  EXPECT_EQ("1970-01-01T00", turbo::FormatCivilTime(hh));

  turbo::CivilDay d;
  EXPECT_EQ("1970-01-01", turbo::FormatCivilTime(d));

  turbo::CivilMonth m;
  EXPECT_EQ("1970-01", turbo::FormatCivilTime(m));

  turbo::CivilYear y;
  EXPECT_EQ("1970", turbo::FormatCivilTime(y));

  EXPECT_TRUE(turbo::ParseLenientCivilTime("2015-01-02T03:04:05", &ss));
  EXPECT_EQ("2015-01-02T03:04:05", turbo::FormatCivilTime(ss));

  EXPECT_TRUE(turbo::ParseLenientCivilTime("2015-01-02T03:04:05", &mm));
  EXPECT_EQ("2015-01-02T03:04", turbo::FormatCivilTime(mm));

  EXPECT_TRUE(turbo::ParseLenientCivilTime("2015-01-02T03:04:05", &hh));
  EXPECT_EQ("2015-01-02T03", turbo::FormatCivilTime(hh));

  EXPECT_TRUE(turbo::ParseLenientCivilTime("2015-01-02T03:04:05", &d));
  EXPECT_EQ("2015-01-02", turbo::FormatCivilTime(d));

  EXPECT_TRUE(turbo::ParseLenientCivilTime("2015-01-02T03:04:05", &m));
  EXPECT_EQ("2015-01", turbo::FormatCivilTime(m));

  EXPECT_TRUE(turbo::ParseLenientCivilTime("2015-01-02T03:04:05", &y));
  EXPECT_EQ("2015", turbo::FormatCivilTime(y));
}

TEST(CivilTime, ParseEdgeCases) {
  turbo::CivilSecond ss;
  EXPECT_TRUE(
      turbo::ParseLenientCivilTime("9223372036854775807-12-31T23:59:59", &ss));
  EXPECT_EQ("9223372036854775807-12-31T23:59:59", turbo::FormatCivilTime(ss));
  EXPECT_TRUE(
      turbo::ParseLenientCivilTime("-9223372036854775808-01-01T00:00:00", &ss));
  EXPECT_EQ("-9223372036854775808-01-01T00:00:00", turbo::FormatCivilTime(ss));

  turbo::CivilMinute mm;
  EXPECT_TRUE(
      turbo::ParseLenientCivilTime("9223372036854775807-12-31T23:59", &mm));
  EXPECT_EQ("9223372036854775807-12-31T23:59", turbo::FormatCivilTime(mm));
  EXPECT_TRUE(
      turbo::ParseLenientCivilTime("-9223372036854775808-01-01T00:00", &mm));
  EXPECT_EQ("-9223372036854775808-01-01T00:00", turbo::FormatCivilTime(mm));

  turbo::CivilHour hh;
  EXPECT_TRUE(
      turbo::ParseLenientCivilTime("9223372036854775807-12-31T23", &hh));
  EXPECT_EQ("9223372036854775807-12-31T23", turbo::FormatCivilTime(hh));
  EXPECT_TRUE(
      turbo::ParseLenientCivilTime("-9223372036854775808-01-01T00", &hh));
  EXPECT_EQ("-9223372036854775808-01-01T00", turbo::FormatCivilTime(hh));

  turbo::CivilDay d;
  EXPECT_TRUE(turbo::ParseLenientCivilTime("9223372036854775807-12-31", &d));
  EXPECT_EQ("9223372036854775807-12-31", turbo::FormatCivilTime(d));
  EXPECT_TRUE(turbo::ParseLenientCivilTime("-9223372036854775808-01-01", &d));
  EXPECT_EQ("-9223372036854775808-01-01", turbo::FormatCivilTime(d));

  turbo::CivilMonth m;
  EXPECT_TRUE(turbo::ParseLenientCivilTime("9223372036854775807-12", &m));
  EXPECT_EQ("9223372036854775807-12", turbo::FormatCivilTime(m));
  EXPECT_TRUE(turbo::ParseLenientCivilTime("-9223372036854775808-01", &m));
  EXPECT_EQ("-9223372036854775808-01", turbo::FormatCivilTime(m));

  turbo::CivilYear y;
  EXPECT_TRUE(turbo::ParseLenientCivilTime("9223372036854775807", &y));
  EXPECT_EQ("9223372036854775807", turbo::FormatCivilTime(y));
  EXPECT_TRUE(turbo::ParseLenientCivilTime("-9223372036854775808", &y));
  EXPECT_EQ("-9223372036854775808", turbo::FormatCivilTime(y));

  // Tests some valid, but interesting, cases
  EXPECT_TRUE(turbo::ParseLenientCivilTime("0", &ss)) << ss;
  EXPECT_EQ(turbo::CivilYear(0), ss);
  EXPECT_TRUE(turbo::ParseLenientCivilTime("0-1", &ss)) << ss;
  EXPECT_EQ(turbo::CivilMonth(0, 1), ss);
  EXPECT_TRUE(turbo::ParseLenientCivilTime(" 2015 ", &ss)) << ss;
  EXPECT_EQ(turbo::CivilYear(2015), ss);
  EXPECT_TRUE(turbo::ParseLenientCivilTime(" 2015-6 ", &ss)) << ss;
  EXPECT_EQ(turbo::CivilMonth(2015, 6), ss);
  EXPECT_TRUE(turbo::ParseLenientCivilTime("2015-6-7", &ss)) << ss;
  EXPECT_EQ(turbo::CivilDay(2015, 6, 7), ss);
  EXPECT_TRUE(turbo::ParseLenientCivilTime(" 2015-6-7 ", &ss)) << ss;
  EXPECT_EQ(turbo::CivilDay(2015, 6, 7), ss);
  EXPECT_TRUE(turbo::ParseLenientCivilTime("2015-06-07T10:11:12 ", &ss)) << ss;
  EXPECT_EQ(turbo::CivilSecond(2015, 6, 7, 10, 11, 12), ss);
  EXPECT_TRUE(turbo::ParseLenientCivilTime(" 2015-06-07T10:11:12 ", &ss)) << ss;
  EXPECT_EQ(turbo::CivilSecond(2015, 6, 7, 10, 11, 12), ss);
  EXPECT_TRUE(turbo::ParseLenientCivilTime("-01-01", &ss)) << ss;
  EXPECT_EQ(turbo::CivilMonth(-1, 1), ss);

  // Tests some invalid cases
  EXPECT_FALSE(turbo::ParseLenientCivilTime("01-01-2015", &ss)) << ss;
  EXPECT_FALSE(turbo::ParseLenientCivilTime("2015-", &ss)) << ss;
  EXPECT_FALSE(turbo::ParseLenientCivilTime("0xff-01", &ss)) << ss;
  EXPECT_FALSE(turbo::ParseLenientCivilTime("2015-02-30T04:05:06", &ss)) << ss;
  EXPECT_FALSE(turbo::ParseLenientCivilTime("2015-02-03T04:05:96", &ss)) << ss;
  EXPECT_FALSE(turbo::ParseLenientCivilTime("X2015-02-03T04:05:06", &ss)) << ss;
  EXPECT_FALSE(turbo::ParseLenientCivilTime("2015-02-03T04:05:003", &ss)) << ss;
  EXPECT_FALSE(turbo::ParseLenientCivilTime("2015 -02-03T04:05:06", &ss)) << ss;
  EXPECT_FALSE(turbo::ParseLenientCivilTime("2015-02-03-04:05:06", &ss)) << ss;
  EXPECT_FALSE(turbo::ParseLenientCivilTime("2015:02:03T04-05-06", &ss)) << ss;
  EXPECT_FALSE(turbo::ParseLenientCivilTime("9223372036854775808", &y)) << y;
}

TEST(CivilTime, OutputStream) {
  turbo::CivilSecond cs(2016, 2, 3, 4, 5, 6);
  {
    std::stringstream ss;
    ss << std::left << std::setfill('.');
    ss << std::setw(3) << 'X';
    ss << std::setw(21) << turbo::CivilYear(cs);
    ss << std::setw(3) << 'X';
    EXPECT_EQ("X..2016.................X..", ss.str());
  }
  {
    std::stringstream ss;
    ss << std::left << std::setfill('.');
    ss << std::setw(3) << 'X';
    ss << std::setw(21) << turbo::CivilMonth(cs);
    ss << std::setw(3) << 'X';
    EXPECT_EQ("X..2016-02..............X..", ss.str());
  }
  {
    std::stringstream ss;
    ss << std::left << std::setfill('.');
    ss << std::setw(3) << 'X';
    ss << std::setw(21) << turbo::CivilDay(cs);
    ss << std::setw(3) << 'X';
    EXPECT_EQ("X..2016-02-03...........X..", ss.str());
  }
  {
    std::stringstream ss;
    ss << std::left << std::setfill('.');
    ss << std::setw(3) << 'X';
    ss << std::setw(21) << turbo::CivilHour(cs);
    ss << std::setw(3) << 'X';
    EXPECT_EQ("X..2016-02-03T04........X..", ss.str());
  }
  {
    std::stringstream ss;
    ss << std::left << std::setfill('.');
    ss << std::setw(3) << 'X';
    ss << std::setw(21) << turbo::CivilMinute(cs);
    ss << std::setw(3) << 'X';
    EXPECT_EQ("X..2016-02-03T04:05.....X..", ss.str());
  }
  {
    std::stringstream ss;
    ss << std::left << std::setfill('.');
    ss << std::setw(3) << 'X';
    ss << std::setw(21) << turbo::CivilSecond(cs);
    ss << std::setw(3) << 'X';
    EXPECT_EQ("X..2016-02-03T04:05:06..X..", ss.str());
  }
  {
    std::stringstream ss;
    ss << std::left << std::setfill('.');
    ss << std::setw(3) << 'X';
    ss << std::setw(21) << turbo::Weekday::wednesday;
    ss << std::setw(3) << 'X';
    EXPECT_EQ("X..Wednesday............X..", ss.str());
  }
}

TEST(CivilTime, Weekday) {
  turbo::CivilDay d(1970, 1, 1);
  EXPECT_EQ(turbo::Weekday::thursday, turbo::GetWeekday(d)) << d;

  // We used to get this wrong for years < -30.
  d = turbo::CivilDay(-31, 12, 24);
  EXPECT_EQ(turbo::Weekday::wednesday, turbo::GetWeekday(d)) << d;
}

TEST(CivilTime, NextPrevWeekday) {
  // Jan 1, 1970 was a Thursday.
  const turbo::CivilDay thursday(1970, 1, 1);

  // Thursday -> Thursday
  turbo::CivilDay d = turbo::NextWeekday(thursday, turbo::Weekday::thursday);
  EXPECT_EQ(7, d - thursday) << d;
  EXPECT_EQ(d - 14, turbo::PrevWeekday(thursday, turbo::Weekday::thursday));

  // Thursday -> Friday
  d = turbo::NextWeekday(thursday, turbo::Weekday::friday);
  EXPECT_EQ(1, d - thursday) << d;
  EXPECT_EQ(d - 7, turbo::PrevWeekday(thursday, turbo::Weekday::friday));

  // Thursday -> Saturday
  d = turbo::NextWeekday(thursday, turbo::Weekday::saturday);
  EXPECT_EQ(2, d - thursday) << d;
  EXPECT_EQ(d - 7, turbo::PrevWeekday(thursday, turbo::Weekday::saturday));

  // Thursday -> Sunday
  d = turbo::NextWeekday(thursday, turbo::Weekday::sunday);
  EXPECT_EQ(3, d - thursday) << d;
  EXPECT_EQ(d - 7, turbo::PrevWeekday(thursday, turbo::Weekday::sunday));

  // Thursday -> Monday
  d = turbo::NextWeekday(thursday, turbo::Weekday::monday);
  EXPECT_EQ(4, d - thursday) << d;
  EXPECT_EQ(d - 7, turbo::PrevWeekday(thursday, turbo::Weekday::monday));

  // Thursday -> Tuesday
  d = turbo::NextWeekday(thursday, turbo::Weekday::tuesday);
  EXPECT_EQ(5, d - thursday) << d;
  EXPECT_EQ(d - 7, turbo::PrevWeekday(thursday, turbo::Weekday::tuesday));

  // Thursday -> Wednesday
  d = turbo::NextWeekday(thursday, turbo::Weekday::wednesday);
  EXPECT_EQ(6, d - thursday) << d;
  EXPECT_EQ(d - 7, turbo::PrevWeekday(thursday, turbo::Weekday::wednesday));
}

// NOTE: Run this with --copt=-ftrapv to detect overflow problems.
TEST(CivilTime, DifferenceWithHugeYear) {
  turbo::CivilDay d1(9223372036854775807, 1, 1);
  turbo::CivilDay d2(9223372036854775807, 12, 31);
  EXPECT_EQ(364, d2 - d1);

  d1 = turbo::CivilDay(-9223372036854775807 - 1, 1, 1);
  d2 = turbo::CivilDay(-9223372036854775807 - 1, 12, 31);
  EXPECT_EQ(365, d2 - d1);

  // Check the limits of the return value at the end of the year range.
  d1 = turbo::CivilDay(9223372036854775807, 1, 1);
  d2 = turbo::CivilDay(9198119301927009252, 6, 6);
  EXPECT_EQ(9223372036854775807, d1 - d2);
  d2 = d2 - 1;
  EXPECT_EQ(-9223372036854775807 - 1, d2 - d1);

  // Check the limits of the return value at the start of the year range.
  d1 = turbo::CivilDay(-9223372036854775807 - 1, 1, 1);
  d2 = turbo::CivilDay(-9198119301927009254, 7, 28);
  EXPECT_EQ(9223372036854775807, d2 - d1);
  d2 = d2 + 1;
  EXPECT_EQ(-9223372036854775807 - 1, d1 - d2);

  // Check the limits of the return value from either side of year 0.
  d1 = turbo::CivilDay(-12626367463883278, 9, 3);
  d2 = turbo::CivilDay(12626367463883277, 3, 28);
  EXPECT_EQ(9223372036854775807, d2 - d1);
  d2 = d2 + 1;
  EXPECT_EQ(-9223372036854775807 - 1, d1 - d2);
}

// NOTE: Run this with --copt=-ftrapv to detect overflow problems.
TEST(CivilTime, DifferenceNoIntermediateOverflow) {
  // The difference up to the minute field would be below the minimum
  // int64_t, but the 52 extra seconds brings us back to the minimum.
  turbo::CivilSecond s1(-292277022657, 1, 27, 8, 29 - 1, 52);
  turbo::CivilSecond s2(1970, 1, 1, 0, 0 - 1, 0);
  EXPECT_EQ(-9223372036854775807 - 1, s1 - s2);

  // The difference up to the minute field would be above the maximum
  // int64_t, but the -53 extra seconds brings us back to the maximum.
  s1 = turbo::CivilSecond(292277026596, 12, 4, 15, 30, 7 - 7);
  s2 = turbo::CivilSecond(1970, 1, 1, 0, 0, 0 - 7);
  EXPECT_EQ(9223372036854775807, s1 - s2);
}

TEST(CivilTime, NormalizeSimpleOverflow) {
  turbo::CivilSecond cs;
  cs = turbo::CivilSecond(2013, 11, 15, 16, 32, 59 + 1);
  EXPECT_EQ("2013-11-15T16:33:00", turbo::FormatCivilTime(cs));
  cs = turbo::CivilSecond(2013, 11, 15, 16, 59 + 1, 14);
  EXPECT_EQ("2013-11-15T17:00:14", turbo::FormatCivilTime(cs));
  cs = turbo::CivilSecond(2013, 11, 15, 23 + 1, 32, 14);
  EXPECT_EQ("2013-11-16T00:32:14", turbo::FormatCivilTime(cs));
  cs = turbo::CivilSecond(2013, 11, 30 + 1, 16, 32, 14);
  EXPECT_EQ("2013-12-01T16:32:14", turbo::FormatCivilTime(cs));
  cs = turbo::CivilSecond(2013, 12 + 1, 15, 16, 32, 14);
  EXPECT_EQ("2014-01-15T16:32:14", turbo::FormatCivilTime(cs));
}

TEST(CivilTime, NormalizeSimpleUnderflow) {
  turbo::CivilSecond cs;
  cs = turbo::CivilSecond(2013, 11, 15, 16, 32, 0 - 1);
  EXPECT_EQ("2013-11-15T16:31:59", turbo::FormatCivilTime(cs));
  cs = turbo::CivilSecond(2013, 11, 15, 16, 0 - 1, 14);
  EXPECT_EQ("2013-11-15T15:59:14", turbo::FormatCivilTime(cs));
  cs = turbo::CivilSecond(2013, 11, 15, 0 - 1, 32, 14);
  EXPECT_EQ("2013-11-14T23:32:14", turbo::FormatCivilTime(cs));
  cs = turbo::CivilSecond(2013, 11, 1 - 1, 16, 32, 14);
  EXPECT_EQ("2013-10-31T16:32:14", turbo::FormatCivilTime(cs));
  cs = turbo::CivilSecond(2013, 1 - 1, 15, 16, 32, 14);
  EXPECT_EQ("2012-12-15T16:32:14", turbo::FormatCivilTime(cs));
}

TEST(CivilTime, NormalizeMultipleOverflow) {
  turbo::CivilSecond cs(2013, 12, 31, 23, 59, 59 + 1);
  EXPECT_EQ("2014-01-01T00:00:00", turbo::FormatCivilTime(cs));
}

TEST(CivilTime, NormalizeMultipleUnderflow) {
  turbo::CivilSecond cs(2014, 1, 1, 0, 0, 0 - 1);
  EXPECT_EQ("2013-12-31T23:59:59", turbo::FormatCivilTime(cs));
}

TEST(CivilTime, NormalizeOverflowLimits) {
  turbo::CivilSecond cs;

  const int kintmax = std::numeric_limits<int>::max();
  cs = turbo::CivilSecond(0, kintmax, kintmax, kintmax, kintmax, kintmax);
  EXPECT_EQ("185085715-11-27T12:21:07", turbo::FormatCivilTime(cs));

  const int kintmin = std::numeric_limits<int>::min();
  cs = turbo::CivilSecond(0, kintmin, kintmin, kintmin, kintmin, kintmin);
  EXPECT_EQ("-185085717-10-31T10:37:52", turbo::FormatCivilTime(cs));
}

TEST(CivilTime, NormalizeComplexOverflow) {
  turbo::CivilSecond cs;
  cs = turbo::CivilSecond(2013, 11, 15, 16, 32, 14 + 123456789);
  EXPECT_EQ("2017-10-14T14:05:23", turbo::FormatCivilTime(cs));
  cs = turbo::CivilSecond(2013, 11, 15, 16, 32 + 1234567, 14);
  EXPECT_EQ("2016-03-22T00:39:14", turbo::FormatCivilTime(cs));
  cs = turbo::CivilSecond(2013, 11, 15, 16 + 123456, 32, 14);
  EXPECT_EQ("2027-12-16T16:32:14", turbo::FormatCivilTime(cs));
  cs = turbo::CivilSecond(2013, 11, 15 + 1234, 16, 32, 14);
  EXPECT_EQ("2017-04-02T16:32:14", turbo::FormatCivilTime(cs));
  cs = turbo::CivilSecond(2013, 11 + 123, 15, 16, 32, 14);
  EXPECT_EQ("2024-02-15T16:32:14", turbo::FormatCivilTime(cs));
}

TEST(CivilTime, NormalizeComplexUnderflow) {
  turbo::CivilSecond cs;
  cs = turbo::CivilSecond(1999, 3, 0, 0, 0, 0);  // year 400
  EXPECT_EQ("1999-02-28T00:00:00", turbo::FormatCivilTime(cs));
  cs = turbo::CivilSecond(2013, 11, 15, 16, 32, 14 - 123456789);
  EXPECT_EQ("2009-12-17T18:59:05", turbo::FormatCivilTime(cs));
  cs = turbo::CivilSecond(2013, 11, 15, 16, 32 - 1234567, 14);
  EXPECT_EQ("2011-07-12T08:25:14", turbo::FormatCivilTime(cs));
  cs = turbo::CivilSecond(2013, 11, 15, 16 - 123456, 32, 14);
  EXPECT_EQ("1999-10-16T16:32:14", turbo::FormatCivilTime(cs));
  cs = turbo::CivilSecond(2013, 11, 15 - 1234, 16, 32, 14);
  EXPECT_EQ("2010-06-30T16:32:14", turbo::FormatCivilTime(cs));
  cs = turbo::CivilSecond(2013, 11 - 123, 15, 16, 32, 14);
  EXPECT_EQ("2003-08-15T16:32:14", turbo::FormatCivilTime(cs));
}

TEST(CivilTime, NormalizeMishmash) {
  turbo::CivilSecond cs;
  cs = turbo::CivilSecond(2013, 11 - 123, 15 + 1234, 16 - 123456, 32 + 1234567,
                         14 - 123456789);
  EXPECT_EQ("1991-05-09T03:06:05", turbo::FormatCivilTime(cs));
  cs = turbo::CivilSecond(2013, 11 + 123, 15 - 1234, 16 + 123456, 32 - 1234567,
                         14 + 123456789);
  EXPECT_EQ("2036-05-24T05:58:23", turbo::FormatCivilTime(cs));

  cs = turbo::CivilSecond(2013, 11, -146097 + 1, 16, 32, 14);
  EXPECT_EQ("1613-11-01T16:32:14", turbo::FormatCivilTime(cs));
  cs = turbo::CivilSecond(2013, 11 + 400 * 12, -146097 + 1, 16, 32, 14);
  EXPECT_EQ("2013-11-01T16:32:14", turbo::FormatCivilTime(cs));
}

// Convert all the days from 1970-1-1 to 1970-1-146097 (aka 2369-12-31)
// and check that they normalize to the expected time.  146097 days span
// the 400-year Gregorian cycle used during normalization.
TEST(CivilTime, NormalizeAllTheDays) {
  turbo::CivilDay expected(1970, 1, 1);
  for (int day = 1; day <= 146097; ++day) {
    turbo::CivilSecond cs(1970, 1, day, 0, 0, 0);
    EXPECT_EQ(expected, cs);
    ++expected;
  }
}

TEST(CivilTime, NormalizeWithHugeYear) {
  turbo::CivilMonth c(9223372036854775807, 1);
  EXPECT_EQ("9223372036854775807-01", turbo::FormatCivilTime(c));
  c = c - 1;  // Causes normalization
  EXPECT_EQ("9223372036854775806-12", turbo::FormatCivilTime(c));

  c = turbo::CivilMonth(-9223372036854775807 - 1, 1);
  EXPECT_EQ("-9223372036854775808-01", turbo::FormatCivilTime(c));
  c = c + 12;  // Causes normalization
  EXPECT_EQ("-9223372036854775807-01", turbo::FormatCivilTime(c));
}

TEST(CivilTime, LeapYears) {
  const turbo::CivilSecond s1(2013, 2, 28 + 1, 0, 0, 0);
  EXPECT_EQ("2013-03-01T00:00:00", turbo::FormatCivilTime(s1));

  const turbo::CivilSecond s2(2012, 2, 28 + 1, 0, 0, 0);
  EXPECT_EQ("2012-02-29T00:00:00", turbo::FormatCivilTime(s2));

  const turbo::CivilSecond s3(1900, 2, 28 + 1, 0, 0, 0);
  EXPECT_EQ("1900-03-01T00:00:00", turbo::FormatCivilTime(s3));

  const struct {
    int year;
    int days;
    struct {
      int month;
      int day;
    } leap_day;  // The date of the day after Feb 28.
  } kLeapYearTable[]{
      {1900, 365, {3, 1}},
      {1999, 365, {3, 1}},
      {2000, 366, {2, 29}},  // leap year
      {2001, 365, {3, 1}},
      {2002, 365, {3, 1}},
      {2003, 365, {3, 1}},
      {2004, 366, {2, 29}},  // leap year
      {2005, 365, {3, 1}},
      {2006, 365, {3, 1}},
      {2007, 365, {3, 1}},
      {2008, 366, {2, 29}},  // leap year
      {2009, 365, {3, 1}},
      {2100, 365, {3, 1}},
  };

  for (int i = 0; i < TURBO_ARRAY_SIZE(kLeapYearTable); ++i) {
    const int y = kLeapYearTable[i].year;
    const int m = kLeapYearTable[i].leap_day.month;
    const int d = kLeapYearTable[i].leap_day.day;
    const int n = kLeapYearTable[i].days;

    // Tests incrementing through the leap day.
    const turbo::CivilDay feb28(y, 2, 28);
    const turbo::CivilDay next_day = feb28 + 1;
    EXPECT_EQ(m, next_day.month());
    EXPECT_EQ(d, next_day.day());

    // Tests difference in days of leap years.
    const turbo::CivilYear year(feb28);
    const turbo::CivilYear next_year = year + 1;
    EXPECT_EQ(n, turbo::CivilDay(next_year) - turbo::CivilDay(year));
  }
}

TEST(CivilTime, FirstThursdayInMonth) {
  const turbo::CivilDay nov1(2014, 11, 1);
  const turbo::CivilDay thursday =
      turbo::NextWeekday(nov1 - 1, turbo::Weekday::thursday);
  EXPECT_EQ("2014-11-06", turbo::FormatCivilTime(thursday));

  // Bonus: Date of Thanksgiving in the United States
  // Rule: Fourth Thursday of November
  const turbo::CivilDay thanksgiving = thursday +  7 * 3;
  EXPECT_EQ("2014-11-27", turbo::FormatCivilTime(thanksgiving));
}

TEST(CivilTime, DocumentationExample) {
  turbo::CivilSecond second(2015, 6, 28, 1, 2, 3);  // 2015-06-28 01:02:03
  turbo::CivilMinute minute(second);                // 2015-06-28 01:02:00
  turbo::CivilDay day(minute);                      // 2015-06-28 00:00:00

  second -= 1;                    // 2015-06-28 01:02:02
  --second;                       // 2015-06-28 01:02:01
  EXPECT_EQ(minute, second - 1);  // Comparison between types
  EXPECT_LT(minute, second);

  // int diff = second - minute;  // ERROR: Mixed types, won't compile

  turbo::CivilDay june_1(2015, 6, 1);  // Pass fields to c'tor.
  int diff = day - june_1;            // Num days between 'day' and June 1
  EXPECT_EQ(27, diff);

  // Fields smaller than alignment are floored to their minimum value.
  turbo::CivilDay day_floor(2015, 1, 2, 9, 9, 9);
  EXPECT_EQ(0, day_floor.hour());  // 09:09:09 is floored
  EXPECT_EQ(turbo::CivilDay(2015, 1, 2), day_floor);

  // Unspecified fields default to their minium value
  turbo::CivilDay day_default(2015);  // Defaults to Jan 1
  EXPECT_EQ(turbo::CivilDay(2015, 1, 1), day_default);

  // Iterates all the days of June.
  turbo::CivilMonth june(day);  // CivilDay -> CivilMonth
  turbo::CivilMonth july = june + 1;
  for (turbo::CivilDay day = june_1; day < july; ++day) {
    // ...
  }
}

}  // namespace
