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

#include <turbo/times/time.h>

#include <cstdint>
#include <ios>

#include <turbo/times/civil_time.h>

#if defined(_MSC_VER)
#include <winsock2.h>  // for timeval
#endif

#include <chrono>  // NOLINT(build/c++11)
#include <cstring>
#include <ctime>
#include <iomanip>
#include <limits>
#include <string>

#include <gmock/gmock.h>
#include <gtest/gtest.h>
#include <turbo/numeric/int128.h>
#include <turbo/strings/str_format.h>
#include <turbo/times/clock.h>
#include <tests/times/test_util.h>

namespace {

#if defined(GTEST_USES_SIMPLE_RE) && GTEST_USES_SIMPLE_RE
    const char kZoneAbbrRE[] = ".*";  // just punt
#else
    const char kZoneAbbrRE[] = "[A-Za-z]{3,4}|[-+][0-9]{2}([0-9]{2})?";
#endif

// This helper is a macro so that failed expectations show up with the
// correct line numbers.
#define EXPECT_CIVIL_INFO(ci, y, m, d, h, min, s, off, isdst)      \
  do {                                                             \
    EXPECT_EQ(y, ci.cs.year());                                    \
    EXPECT_EQ(m, ci.cs.month());                                   \
    EXPECT_EQ(d, ci.cs.day());                                     \
    EXPECT_EQ(h, ci.cs.hour());                                    \
    EXPECT_EQ(min, ci.cs.minute());                                \
    EXPECT_EQ(s, ci.cs.second());                                  \
    EXPECT_EQ(off, ci.offset);                                     \
    EXPECT_EQ(isdst, ci.is_dst);                                   \
    EXPECT_THAT(ci.zone_abbr, testing::MatchesRegex(kZoneAbbrRE)); \
  } while (0)

    // A gMock matcher to match timespec values. Use this matcher like:
    // timespec ts1, ts2;
    // EXPECT_THAT(ts1, TimespecMatcher(ts2));
    MATCHER_P(TimespecMatcher, ts, "") {
        if (ts.tv_sec == arg.tv_sec && ts.tv_nsec == arg.tv_nsec) return true;
        *result_listener << "expected: {" << ts.tv_sec << ", " << ts.tv_nsec << "} ";
        *result_listener << "actual: {" << arg.tv_sec << ", " << arg.tv_nsec << "}";
        return false;
    }

    // A gMock matcher to match timeval values. Use this matcher like:
    // timeval tv1, tv2;
    // EXPECT_THAT(tv1, TimevalMatcher(tv2));
    MATCHER_P(TimevalMatcher, tv, "") {
        if (tv.tv_sec == arg.tv_sec && tv.tv_usec == arg.tv_usec) return true;
        *result_listener << "expected: {" << tv.tv_sec << ", " << tv.tv_usec << "} ";
        *result_listener << "actual: {" << arg.tv_sec << ", " << arg.tv_usec << "}";
        return false;
    }

    TEST(Time, ConstExpr) {
        constexpr turbo::Time t0 = turbo::Time::from_unix_epoch();
        static_assert(t0 == turbo::Time::from_unix_epoch(), "Time::from_unix_epoch");
        constexpr turbo::Time t1 = turbo::Time::future_infinite();
        static_assert(t1 != turbo::Time::from_unix_epoch(), "InfiniteFuture");
        constexpr turbo::Time t2 = turbo::Time::past_infinite();
        static_assert(t2 != turbo::Time::from_unix_epoch(), "Time::past_infinite");
        constexpr turbo::Time t3 = turbo::Time::from_nanoseconds(0);
        static_assert(t3 == turbo::Time::from_unix_epoch(), "Time::from_nanoseconds");
        constexpr turbo::Time t4 = turbo::Time::from_microseconds(0);
        static_assert(t4 == turbo::Time::from_unix_epoch(), "Time::from_microseconds");
        constexpr turbo::Time t5 = turbo::Time::from_milliseconds(0);
        static_assert(t5 == turbo::Time::from_unix_epoch(), "Time::from_milliseconds");
        constexpr turbo::Time t6 = turbo::Time::from_seconds(0);
        static_assert(t6 == turbo::Time::from_unix_epoch(), "Time::from_seconds");
        constexpr turbo::Time t7 = turbo::Time::from_time_t(0);
        static_assert(t7 == turbo::Time::from_unix_epoch(), "Time::from_time_t");
    }

    TEST(Time, ValueSemantics) {
        turbo::Time a;      // Default construction
        turbo::Time b = a;  // Copy construction
        EXPECT_EQ(a, b);
        turbo::Time c(a);  // Copy construction (again)
        EXPECT_EQ(a, b);
        EXPECT_EQ(a, c);
        EXPECT_EQ(b, c);
        b = c;  // Assignment
        EXPECT_EQ(a, b);
        EXPECT_EQ(a, c);
        EXPECT_EQ(b, c);
    }

    TEST(Time, from_unix_epoch) {
        const auto ci = turbo::TimeZone::utc().at(turbo::Time::from_unix_epoch());
        EXPECT_EQ(turbo::CivilSecond(1970, 1, 1, 0, 0, 0), ci.cs);
        EXPECT_EQ(turbo::Duration::zero(), ci.subsecond);
        EXPECT_EQ(turbo::Weekday::thursday, turbo::GetWeekday(ci.cs));
    }

    TEST(Time, Breakdown) {
        turbo::TimeZone tz = turbo::time_internal::LoadTimeZone("America/New_York");
        turbo::Time t = turbo::Time::from_unix_epoch();

        // The Unix epoch as seen in NYC.
        auto ci = tz.at(t);
        EXPECT_CIVIL_INFO(ci, 1969, 12, 31, 19, 0, 0, -18000, false);
        EXPECT_EQ(turbo::Duration::zero(), ci.subsecond);
        EXPECT_EQ(turbo::Weekday::wednesday, turbo::GetWeekday(ci.cs));

        // Just before the epoch.
        t -= turbo::Duration::nanoseconds(1);
        ci = tz.at(t);
        EXPECT_CIVIL_INFO(ci, 1969, 12, 31, 18, 59, 59, -18000, false);
        EXPECT_EQ(turbo::Duration::nanoseconds(999999999), ci.subsecond);
        EXPECT_EQ(turbo::Weekday::wednesday, turbo::GetWeekday(ci.cs));

        // Some time later.
        t += turbo::Duration::hours(24) * 2735;
        t += turbo::Duration::hours(18) + turbo::Duration::minutes(30) + turbo::Duration::seconds(15) +
             turbo::Duration::nanoseconds(9);
        ci = tz.at(t);
        EXPECT_CIVIL_INFO(ci, 1977, 6, 28, 14, 30, 15, -14400, true);
        EXPECT_EQ(8, ci.subsecond / turbo::Duration::nanoseconds(1));
        EXPECT_EQ(turbo::Weekday::tuesday, turbo::GetWeekday(ci.cs));
    }

    TEST(Time, AdditiveOperators) {
        const turbo::Duration d = turbo::Duration::nanoseconds(1);
        const turbo::Time t0;
        const turbo::Time t1 = t0 + d;

        EXPECT_EQ(d, t1 - t0);
        EXPECT_EQ(-d, t0 - t1);
        EXPECT_EQ(t0, t1 - d);

        turbo::Time t(t0);
        EXPECT_EQ(t0, t);
        t += d;
        EXPECT_EQ(t0 + d, t);
        EXPECT_EQ(d, t - t0);
        t -= d;
        EXPECT_EQ(t0, t);

        // Tests overflow between subseconds and seconds.
        t = turbo::Time::from_unix_epoch();
        t += turbo::Duration::milliseconds(500);
        EXPECT_EQ(turbo::Time::from_unix_epoch() + turbo::Duration::milliseconds(500), t);
        t += turbo::Duration::milliseconds(600);
        EXPECT_EQ(turbo::Time::from_unix_epoch() + turbo::Duration::milliseconds(1100), t);
        t -= turbo::Duration::milliseconds(600);
        EXPECT_EQ(turbo::Time::from_unix_epoch() + turbo::Duration::milliseconds(500), t);
        t -= turbo::Duration::milliseconds(500);
        EXPECT_EQ(turbo::Time::from_unix_epoch(), t);
    }

    TEST(Time, RelationalOperators) {
        constexpr turbo::Time t1 = turbo::Time::from_nanoseconds(0);
        constexpr turbo::Time t2 = turbo::Time::from_nanoseconds(1);
        constexpr turbo::Time t3 = turbo::Time::from_nanoseconds(2);

        static_assert(turbo::Time::from_unix_epoch() == t1, "");
        static_assert(t1 == t1, "");
        static_assert(t2 == t2, "");
        static_assert(t3 == t3, "");

        static_assert(t1 < t2, "");
        static_assert(t2 < t3, "");
        static_assert(t1 < t3, "");

        static_assert(t1 <= t1, "");
        static_assert(t1 <= t2, "");
        static_assert(t2 <= t2, "");
        static_assert(t2 <= t3, "");
        static_assert(t3 <= t3, "");
        static_assert(t1 <= t3, "");

        static_assert(t2 > t1, "");
        static_assert(t3 > t2, "");
        static_assert(t3 > t1, "");

        static_assert(t2 >= t2, "");
        static_assert(t2 >= t1, "");
        static_assert(t3 >= t3, "");
        static_assert(t3 >= t2, "");
        static_assert(t1 >= t1, "");
        static_assert(t3 >= t1, "");
    }

    TEST(Time, Infinity) {
        constexpr turbo::Time ifuture = turbo::Time::future_infinite();
        constexpr turbo::Time ipast = turbo::Time::past_infinite();

        static_assert(ifuture == ifuture, "");
        static_assert(ipast == ipast, "");
        static_assert(ipast < ifuture, "");
        static_assert(ifuture > ipast, "");

        // Arithmetic saturates
        EXPECT_EQ(ifuture, ifuture + turbo::Duration::seconds(1));
        EXPECT_EQ(ifuture, ifuture - turbo::Duration::seconds(1));
        EXPECT_EQ(ipast, ipast + turbo::Duration::seconds(1));
        EXPECT_EQ(ipast, ipast - turbo::Duration::seconds(1));

        EXPECT_EQ(turbo::Duration::max_infinite(), ifuture - ifuture);
        EXPECT_EQ(turbo::Duration::max_infinite(), ifuture - ipast);
        EXPECT_EQ(-turbo::Duration::max_infinite(), ipast - ifuture);
        EXPECT_EQ(-turbo::Duration::max_infinite(), ipast - ipast);

        constexpr turbo::Time t = turbo::Time::from_unix_epoch();  // Any finite time.
        static_assert(t < ifuture, "");
        static_assert(t > ipast, "");

        EXPECT_EQ(ifuture, t + turbo::Duration::max_infinite());
        EXPECT_EQ(ipast, t - turbo::Duration::max_infinite());
    }

    TEST(Time, FloorConversion) {
#define TEST_FLOOR_CONVERSION(TO, FROM) \
  EXPECT_EQ(1, TO(FROM(1001)));         \
  EXPECT_EQ(1, TO(FROM(1000)));         \
  EXPECT_EQ(0, TO(FROM(999)));          \
  EXPECT_EQ(0, TO(FROM(1)));            \
  EXPECT_EQ(0, TO(FROM(0)));            \
  EXPECT_EQ(-1, TO(FROM(-1)));          \
  EXPECT_EQ(-1, TO(FROM(-999)));        \
  EXPECT_EQ(-1, TO(FROM(-1000)));       \
  EXPECT_EQ(-2, TO(FROM(-1001)));

        TEST_FLOOR_CONVERSION(turbo::Time::to_microseconds, turbo::Time::from_nanoseconds);
        TEST_FLOOR_CONVERSION(turbo::Time::to_milliseconds, turbo::Time::from_microseconds);
        TEST_FLOOR_CONVERSION(turbo::Time::to_seconds, turbo::Time::from_milliseconds);
        TEST_FLOOR_CONVERSION(turbo::Time::to_time_t, turbo::Time::from_milliseconds);

#undef TEST_FLOOR_CONVERSION

        // Tests Time::to_nanoseconds.
        EXPECT_EQ(1, turbo::Time::to_nanoseconds(turbo::Time::from_unix_epoch() + turbo::Duration::nanoseconds(3) / 2));
        EXPECT_EQ(1, turbo::Time::to_nanoseconds(turbo::Time::from_unix_epoch() + turbo::Duration::nanoseconds(1)));
        EXPECT_EQ(0, turbo::Time::to_nanoseconds(turbo::Time::from_unix_epoch() + turbo::Duration::nanoseconds(1) / 2));
        EXPECT_EQ(0, turbo::Time::to_nanoseconds(turbo::Time::from_unix_epoch() + turbo::Duration::zero()));
        EXPECT_EQ(-1,
                  turbo::Time::to_nanoseconds(turbo::Time::from_unix_epoch() - turbo::Duration::nanoseconds(1) / 2));
        EXPECT_EQ(-1, turbo::Time::to_nanoseconds(turbo::Time::from_unix_epoch() - turbo::Duration::nanoseconds(1)));
        EXPECT_EQ(-2,
                  turbo::Time::to_nanoseconds(turbo::Time::from_unix_epoch() - turbo::Duration::nanoseconds(3) / 2));

        // Tests Time::to_universal, which uses a different epoch than the tests above.
        EXPECT_EQ(1,
                  turbo::Time::to_universal(turbo::Time::from_universal_epoch() + turbo::Duration::nanoseconds(101)));
        EXPECT_EQ(1,
                  turbo::Time::to_universal(turbo::Time::from_universal_epoch() + turbo::Duration::nanoseconds(100)));
        EXPECT_EQ(0,
                  turbo::Time::to_universal(turbo::Time::from_universal_epoch() + turbo::Duration::nanoseconds(99)));
        EXPECT_EQ(0,
                  turbo::Time::to_universal(turbo::Time::from_universal_epoch() + turbo::Duration::nanoseconds(1)));
        EXPECT_EQ(0,
                  turbo::Time::to_universal(turbo::Time::from_universal_epoch() + turbo::Duration::zero()));
        EXPECT_EQ(-1,
                  turbo::Time::to_universal(turbo::Time::from_universal_epoch() + turbo::Duration::nanoseconds(-1)));
        EXPECT_EQ(-1,
                  turbo::Time::to_universal(turbo::Time::from_universal_epoch() + turbo::Duration::nanoseconds(-99)));
        EXPECT_EQ(
                -1, turbo::Time::to_universal(turbo::Time::from_universal_epoch() + turbo::Duration::nanoseconds(-100)));
        EXPECT_EQ(
                -2, turbo::Time::to_universal(turbo::Time::from_universal_epoch() + turbo::Duration::nanoseconds(-101)));

        // Tests Time::to_timespec()/Time::from_timespec()
        const struct {
            turbo::Time t;
            timespec ts;
        } to_ts[] = {
                {turbo::Time::from_seconds(1) + turbo::Duration::nanoseconds(1),      {1,  1}},
                {turbo::Time::from_seconds(1) + turbo::Duration::nanoseconds(1) / 2,  {1,  0}},
                {turbo::Time::from_seconds(1) + turbo::Duration::zero(),      {1,  0}},
                {turbo::Time::from_seconds(0) + turbo::Duration::zero(),      {0,  0}},
                {turbo::Time::from_seconds(0) - turbo::Duration::nanoseconds(1) / 2,  {-1, 999999999}},
                {turbo::Time::from_seconds(0) - turbo::Duration::nanoseconds(1),      {-1, 999999999}},
                {turbo::Time::from_seconds(-1) + turbo::Duration::nanoseconds(1),     {-1, 1}},
                {turbo::Time::from_seconds(-1) + turbo::Duration::nanoseconds(1) / 2, {-1, 0}},
                {turbo::Time::from_seconds(-1) + turbo::Duration::zero(),     {-1, 0}},
                {turbo::Time::from_seconds(-1) - turbo::Duration::nanoseconds(1) / 2, {-2, 999999999}},
        };
        for (const auto &test: to_ts) {
            EXPECT_THAT(turbo::Time::to_timespec(test.t), TimespecMatcher(test.ts));
        }
        const struct {
            timespec ts;
            turbo::Time t;
        } from_ts[] = {
                {{1,  1},         turbo::Time::from_seconds(1) + turbo::Duration::nanoseconds(1)},
                {{1,  0},         turbo::Time::from_seconds(1) + turbo::Duration::zero()},
                {{0,  0},         turbo::Time::from_seconds(0) + turbo::Duration::zero()},
                {{0,  -1},        turbo::Time::from_seconds(0) - turbo::Duration::nanoseconds(1)},
                {{-1, 999999999}, turbo::Time::from_seconds(0) - turbo::Duration::nanoseconds(1)},
                {{-1, 1},         turbo::Time::from_seconds(-1) + turbo::Duration::nanoseconds(1)},
                {{-1, 0},         turbo::Time::from_seconds(-1) + turbo::Duration::zero()},
                {{-1, -1},        turbo::Time::from_seconds(-1) - turbo::Duration::nanoseconds(1)},
                {{-2, 999999999}, turbo::Time::from_seconds(-1) - turbo::Duration::nanoseconds(1)},
        };
        for (const auto &test: from_ts) {
            EXPECT_EQ(test.t, turbo::Time::from_timespec(test.ts));
        }

        // Tests  turbo::Time::to_timeval()/Time::from_timeval() (same as timespec above)
        const struct {
            turbo::Time t;
            timeval tv;
        } to_tv[] = {
                {turbo::Time::from_seconds(1) + turbo::Duration::microseconds(1),      {1,  1}},
                {turbo::Time::from_seconds(1) + turbo::Duration::microseconds(1) / 2,  {1,  0}},
                {turbo::Time::from_seconds(1) + turbo::Duration::zero(),       {1,  0}},
                {turbo::Time::from_seconds(0) + turbo::Duration::zero(),       {0,  0}},
                {turbo::Time::from_seconds(0) - turbo::Duration::microseconds(1) / 2,  {-1, 999999}},
                {turbo::Time::from_seconds(0) - turbo::Duration::microseconds(1),      {-1, 999999}},
                {turbo::Time::from_seconds(-1) + turbo::Duration::microseconds(1),     {-1, 1}},
                {turbo::Time::from_seconds(-1) + turbo::Duration::microseconds(1) / 2, {-1, 0}},
                {turbo::Time::from_seconds(-1) + turbo::Duration::zero(),      {-1, 0}},
                {turbo::Time::from_seconds(-1) - turbo::Duration::microseconds(1) / 2, {-2, 999999}},
        };
        for (const auto &test: to_tv) {
            EXPECT_THAT(turbo::Time::to_timeval(test.t), TimevalMatcher(test.tv));
        }
        const struct {
            timeval tv;
            turbo::Time t;
        } from_tv[] = {
                {{1,  1},      turbo::Time::from_seconds(1) + turbo::Duration::microseconds(1)},
                {{1,  0},      turbo::Time::from_seconds(1) + turbo::Duration::zero()},
                {{0,  0},      turbo::Time::from_seconds(0) + turbo::Duration::zero()},
                {{0,  -1},     turbo::Time::from_seconds(0) - turbo::Duration::microseconds(1)},
                {{-1, 999999}, turbo::Time::from_seconds(0) - turbo::Duration::microseconds(1)},
                {{-1, 1},      turbo::Time::from_seconds(-1) + turbo::Duration::microseconds(1)},
                {{-1, 0},      turbo::Time::from_seconds(-1) + turbo::Duration::zero()},
                {{-1, -1},     turbo::Time::from_seconds(-1) - turbo::Duration::microseconds(1)},
                {{-2, 999999}, turbo::Time::from_seconds(-1) - turbo::Duration::microseconds(1)},
        };
        for (const auto &test: from_tv) {
            EXPECT_EQ(test.t, turbo::Time::from_timeval(test.tv));
        }

        // Tests flooring near negative infinity.
        const int64_t min_plus_1 = std::numeric_limits<int64_t>::min() + 1;
        EXPECT_EQ(min_plus_1, turbo::Time::to_seconds(turbo::Time::from_seconds(min_plus_1)));
        EXPECT_EQ(std::numeric_limits<int64_t>::min(),
                  turbo::Time::to_seconds(turbo::Time::from_seconds(min_plus_1) -
                                       turbo::Duration::nanoseconds(1) / 2));

        // Tests flooring near positive infinity.
        EXPECT_EQ(std::numeric_limits<int64_t>::max(),
                  turbo::Time::to_seconds(
                          turbo::Time::from_seconds(std::numeric_limits<int64_t>::max()) +
                          turbo::Duration::nanoseconds(1) / 2));
        EXPECT_EQ(std::numeric_limits<int64_t>::max(),
                  turbo::Time::to_seconds(
                          turbo::Time::from_seconds(std::numeric_limits<int64_t>::max())));
        EXPECT_EQ(std::numeric_limits<int64_t>::max() - 1,
                  turbo::Time::to_seconds(
                          turbo::Time::from_seconds(std::numeric_limits<int64_t>::max()) -
                          turbo::Duration::nanoseconds(1) / 2));
    }

    TEST(Time, RoundtripConversion) {
#define TEST_CONVERSION_ROUND_TRIP(SOURCE, FROM, TO, MATCHER) \
  EXPECT_THAT(TO(FROM(SOURCE)), MATCHER(SOURCE))

        // Time::from_nanoseconds() and Time::to_nanoseconds()
        int64_t now_ns = turbo::GetCurrentTimeNanos();
        TEST_CONVERSION_ROUND_TRIP(-1, turbo::Time::from_nanoseconds, turbo::Time::to_nanoseconds,
                                   testing::Eq);
        TEST_CONVERSION_ROUND_TRIP(0, turbo::Time::from_nanoseconds, turbo::Time::to_nanoseconds,
                                   testing::Eq);
        TEST_CONVERSION_ROUND_TRIP(1, turbo::Time::from_nanoseconds, turbo::Time::to_nanoseconds,
                                   testing::Eq);
        TEST_CONVERSION_ROUND_TRIP(now_ns, turbo::Time::from_nanoseconds, turbo::Time::to_nanoseconds,
                                   testing::Eq)
                            << now_ns;

        // Time::from_microseconds() and Time::to_microseconds()
        int64_t now_us = turbo::GetCurrentTimeNanos() / 1000;
        TEST_CONVERSION_ROUND_TRIP(-1, turbo::Time::from_microseconds, turbo::Time::to_microseconds,
                                   testing::Eq);
        TEST_CONVERSION_ROUND_TRIP(0, turbo::Time::from_microseconds, turbo::Time::to_microseconds,
                                   testing::Eq);
        TEST_CONVERSION_ROUND_TRIP(1, turbo::Time::from_microseconds, turbo::Time::to_microseconds,
                                   testing::Eq);
        TEST_CONVERSION_ROUND_TRIP(now_us, turbo::Time::from_microseconds, turbo::Time::to_microseconds,
                                   testing::Eq)
                            << now_us;

        // Time::from_milliseconds() and Time::to_milliseconds()
        int64_t now_ms = turbo::GetCurrentTimeNanos() / 1000000;
        TEST_CONVERSION_ROUND_TRIP(-1, turbo::Time::from_milliseconds, turbo::Time::to_milliseconds,
                                   testing::Eq);
        TEST_CONVERSION_ROUND_TRIP(0, turbo::Time::from_milliseconds, turbo::Time::to_milliseconds,
                                   testing::Eq);
        TEST_CONVERSION_ROUND_TRIP(1, turbo::Time::from_milliseconds, turbo::Time::to_milliseconds,
                                   testing::Eq);
        TEST_CONVERSION_ROUND_TRIP(now_ms, turbo::Time::from_milliseconds, turbo::Time::to_milliseconds,
                                   testing::Eq)
                            << now_ms;

        // Time::from_seconds() and Time::to_seconds()
        int64_t now_s = std::time(nullptr);
        TEST_CONVERSION_ROUND_TRIP(-1, turbo::Time::from_seconds, turbo::Time::to_seconds,
                                   testing::Eq);
        TEST_CONVERSION_ROUND_TRIP(0, turbo::Time::from_seconds, turbo::Time::to_seconds,
                                   testing::Eq);
        TEST_CONVERSION_ROUND_TRIP(1, turbo::Time::from_seconds, turbo::Time::to_seconds,
                                   testing::Eq);
        TEST_CONVERSION_ROUND_TRIP(now_s, turbo::Time::from_seconds, turbo::Time::to_seconds,
                                   testing::Eq)
                            << now_s;

        // Time::from_time_t() and ToTimeT()
        time_t now_time_t = std::time(nullptr);
        TEST_CONVERSION_ROUND_TRIP(-1, turbo::Time::from_time_t, turbo::Time::to_time_t, testing::Eq);
        TEST_CONVERSION_ROUND_TRIP(0, turbo::Time::from_time_t, turbo::Time::to_time_t, testing::Eq);
        TEST_CONVERSION_ROUND_TRIP(1, turbo::Time::from_time_t, turbo::Time::to_time_t, testing::Eq);
        TEST_CONVERSION_ROUND_TRIP(now_time_t, turbo::Time::from_time_t, turbo::Time::to_time_t,
                                   testing::Eq)
                            << now_time_t;

        // Time::from_timeval() and  turbo::Time::to_timeval()
        timeval tv;
        tv.tv_sec = -1;
        tv.tv_usec = 0;
        TEST_CONVERSION_ROUND_TRIP(tv, turbo::Time::from_timeval, turbo::Time::to_timeval,
                                   TimevalMatcher);
        tv.tv_sec = -1;
        tv.tv_usec = 999999;
        TEST_CONVERSION_ROUND_TRIP(tv, turbo::Time::from_timeval, turbo::Time::to_timeval,
                                   TimevalMatcher);
        tv.tv_sec = 0;
        tv.tv_usec = 0;
        TEST_CONVERSION_ROUND_TRIP(tv, turbo::Time::from_timeval, turbo::Time::to_timeval,
                                   TimevalMatcher);
        tv.tv_sec = 0;
        tv.tv_usec = 1;
        TEST_CONVERSION_ROUND_TRIP(tv, turbo::Time::from_timeval, turbo::Time::to_timeval,
                                   TimevalMatcher);
        tv.tv_sec = 1;
        tv.tv_usec = 0;
        TEST_CONVERSION_ROUND_TRIP(tv, turbo::Time::from_timeval, turbo::Time::to_timeval,
                                   TimevalMatcher);

        // Time::from_timespec() and Time::to_timespec()
        timespec ts;
        ts.tv_sec = -1;
        ts.tv_nsec = 0;
        TEST_CONVERSION_ROUND_TRIP(ts, turbo::Time::from_timespec, turbo::Time::to_timespec,
                                   TimespecMatcher);
        ts.tv_sec = -1;
        ts.tv_nsec = 999999999;
        TEST_CONVERSION_ROUND_TRIP(ts, turbo::Time::from_timespec, turbo::Time::to_timespec,
                                   TimespecMatcher);
        ts.tv_sec = 0;
        ts.tv_nsec = 0;
        TEST_CONVERSION_ROUND_TRIP(ts, turbo::Time::from_timespec, turbo::Time::to_timespec,
                                   TimespecMatcher);
        ts.tv_sec = 0;
        ts.tv_nsec = 1;
        TEST_CONVERSION_ROUND_TRIP(ts, turbo::Time::from_timespec, turbo::Time::to_timespec,
                                   TimespecMatcher);
        ts.tv_sec = 1;
        ts.tv_nsec = 0;
        TEST_CONVERSION_ROUND_TRIP(ts, turbo::Time::from_timespec, turbo::Time::to_timespec,
                                   TimespecMatcher);

        // Time::from_udate() and Time::to_udate()
        double now_ud = turbo::GetCurrentTimeNanos() / 1000000;
        TEST_CONVERSION_ROUND_TRIP(-1.5, turbo::Time::from_udate, turbo::Time::to_udate,
                                   testing::DoubleEq);
        TEST_CONVERSION_ROUND_TRIP(-1, turbo::Time::from_udate, turbo::Time::to_udate,
                                   testing::DoubleEq);
        TEST_CONVERSION_ROUND_TRIP(-0.5, turbo::Time::from_udate, turbo::Time::to_udate,
                                   testing::DoubleEq);
        TEST_CONVERSION_ROUND_TRIP(0, turbo::Time::from_udate, turbo::Time::to_udate,
                                   testing::DoubleEq);
        TEST_CONVERSION_ROUND_TRIP(0.5, turbo::Time::from_udate, turbo::Time::to_udate,
                                   testing::DoubleEq);
        TEST_CONVERSION_ROUND_TRIP(1, turbo::Time::from_udate, turbo::Time::to_udate,
                                   testing::DoubleEq);
        TEST_CONVERSION_ROUND_TRIP(1.5, turbo::Time::from_udate, turbo::Time::to_udate,
                                   testing::DoubleEq);
        TEST_CONVERSION_ROUND_TRIP(now_ud, turbo::Time::from_udate, turbo::Time::to_udate,
                                   testing::DoubleEq)
                            << std::fixed << std::setprecision(17) << now_ud;

        // Time::from_universal() and Time::to_universal()
        int64_t now_uni = ((719162LL * (24 * 60 * 60)) * (1000 * 1000 * 10)) +
                          (turbo::GetCurrentTimeNanos() / 100);
        TEST_CONVERSION_ROUND_TRIP(-1, turbo::Time::from_universal, turbo::Time::to_universal,
                                   testing::Eq);
        TEST_CONVERSION_ROUND_TRIP(0, turbo::Time::from_universal, turbo::Time::to_universal,
                                   testing::Eq);
        TEST_CONVERSION_ROUND_TRIP(1, turbo::Time::from_universal, turbo::Time::to_universal,
                                   testing::Eq);
        TEST_CONVERSION_ROUND_TRIP(now_uni, turbo::Time::from_universal, turbo::Time::to_universal,
                                   testing::Eq)
                            << now_uni;

#undef TEST_CONVERSION_ROUND_TRIP
    }

    template<typename Duration>
    std::chrono::system_clock::time_point MakeChronoUnixTime(const Duration &d) {
        return std::chrono::system_clock::from_time_t(0) + d;
    }

    TEST(Time, FromChrono) {
        EXPECT_EQ(turbo::Time::from_time_t(-1),
                  turbo::Time::from_chrono(std::chrono::system_clock::from_time_t(-1)));
        EXPECT_EQ(turbo::Time::from_time_t(0),
                  turbo::Time::from_chrono(std::chrono::system_clock::from_time_t(0)));
        EXPECT_EQ(turbo::Time::from_time_t(1),
                  turbo::Time::from_chrono(std::chrono::system_clock::from_time_t(1)));

        EXPECT_EQ(
                turbo::Time::from_milliseconds(-1),
                turbo::Time::from_chrono(MakeChronoUnixTime(std::chrono::milliseconds(-1))));
        EXPECT_EQ(turbo::Time::from_milliseconds(0),
                  turbo::Time::from_chrono(MakeChronoUnixTime(std::chrono::milliseconds(0))));
        EXPECT_EQ(turbo::Time::from_milliseconds(1),
                  turbo::Time::from_chrono(MakeChronoUnixTime(std::chrono::milliseconds(1))));

        // Chrono doesn't define exactly its range and precision (neither does
        // turbo::Time), so let's simply test +/- ~100 years to make sure things work.
        const auto century_sec = 60 * 60 * 24 * 365 * int64_t{100};
        const auto century = std::chrono::seconds(century_sec);
        const auto chrono_future = MakeChronoUnixTime(century);
        const auto chrono_past = MakeChronoUnixTime(-century);
        EXPECT_EQ(turbo::Time::from_seconds(century_sec),
                  turbo::Time::from_chrono(chrono_future));
        EXPECT_EQ(turbo::Time::from_seconds(-century_sec), turbo::Time::from_chrono(chrono_past));

        // Roundtrip them both back to chrono.
        EXPECT_EQ(chrono_future,
                  turbo::Time::to_chrono(turbo::Time::from_seconds(century_sec)));
        EXPECT_EQ(chrono_past,
                  turbo::Time::to_chrono(turbo::Time::from_seconds(-century_sec)));
    }

    TEST(Time, to_chrono_time) {
        EXPECT_EQ(std::chrono::system_clock::from_time_t(-1),
                  turbo::Time::to_chrono(turbo::Time::from_time_t(-1)));
        EXPECT_EQ(std::chrono::system_clock::from_time_t(0),
                  turbo::Time::to_chrono(turbo::Time::from_time_t(0)));
        EXPECT_EQ(std::chrono::system_clock::from_time_t(1),
                  turbo::Time::to_chrono(turbo::Time::from_time_t(1)));

        EXPECT_EQ(MakeChronoUnixTime(std::chrono::milliseconds(-1)),
                  turbo::Time::to_chrono(turbo::Time::from_milliseconds(-1)));
        EXPECT_EQ(MakeChronoUnixTime(std::chrono::milliseconds(0)),
                  turbo::Time::to_chrono(turbo::Time::from_milliseconds(0)));
        EXPECT_EQ(MakeChronoUnixTime(std::chrono::milliseconds(1)),
                  turbo::Time::to_chrono(turbo::Time::from_milliseconds(1)));

        // Time before the Unix epoch should floor, not trunc.
        const auto tick = turbo::Duration::nanoseconds(1) / 4;
        EXPECT_EQ(std::chrono::system_clock::from_time_t(0) -
                  std::chrono::system_clock::duration(1),
                  turbo::Time::to_chrono(turbo::Time::from_unix_epoch() - tick));
    }

    // Check that turbo::int128 works as a std::chrono::duration representation.
    TEST(Time, Chrono128) {
        // Define a std::chrono::time_point type whose time[sic]_since_epoch() is
        // a signed 128-bit count of attoseconds. This has a range and resolution
        // (currently) beyond those of turbo::Time, and undoubtedly also beyond those
        // of std::chrono::system_clock::time_point.
        //
        // Note: The to/from-chrono support should probably be updated to handle
        // such wide representations.
        using Timestamp =
                std::chrono::time_point<std::chrono::system_clock,
                        std::chrono::duration<turbo::int128, std::atto>>;

        // Expect that we can round-trip the std::chrono::system_clock::time_point
        // extremes through both turbo::Time and Timestamp, and that Timestamp can
        // handle the (current) turbo::Time extremes.
        //
        // Note: We should use std::chrono::floor() instead of time_point_cast(),
        // but floor() is only available since c++17.
        for (const auto tp: {std::chrono::system_clock::time_point::min(),
                             std::chrono::system_clock::time_point::max()}) {
            EXPECT_EQ(tp, turbo::Time::to_chrono(turbo::Time::from_chrono(tp)));
            EXPECT_EQ(tp, std::chrono::time_point_cast<
                    std::chrono::system_clock::time_point::duration>(
                    std::chrono::time_point_cast<Timestamp::duration>(tp)));
        }
        Timestamp::duration::rep v = std::numeric_limits<int64_t>::min();
        v *= Timestamp::duration::period::den;
        auto ts = Timestamp(Timestamp::duration(v));
        ts += std::chrono::duration<int64_t, std::atto>(0);
        EXPECT_EQ(std::numeric_limits<int64_t>::min(),
                  ts.time_since_epoch().count() / Timestamp::duration::period::den);
        EXPECT_EQ(0,
                  ts.time_since_epoch().count() % Timestamp::duration::period::den);
        v = std::numeric_limits<int64_t>::max();
        v *= Timestamp::duration::period::den;
        ts = Timestamp(Timestamp::duration(v));
        ts += std::chrono::duration<int64_t, std::atto>(999999999750000000);
        EXPECT_EQ(std::numeric_limits<int64_t>::max(),
                  ts.time_since_epoch().count() / Timestamp::duration::period::den);
        EXPECT_EQ(999999999750000000,
                  ts.time_since_epoch().count() % Timestamp::duration::period::den);
    }

    TEST(Time, TimeZoneAt) {
        const turbo::TimeZone nyc =
                turbo::time_internal::LoadTimeZone("America/New_York");
        const std::string fmt = "%a, %e %b %Y %H:%M:%S %z (%Z)";

        // A non-transition where the civil time is unique.
        turbo::CivilSecond nov01(2013, 11, 1, 8, 30, 0);
        const auto nov01_ci = nyc.at(nov01);
        EXPECT_EQ(turbo::TimeZone::TimeInfo::UNIQUE, nov01_ci.kind);
        EXPECT_EQ("Fri,  1 Nov 2013 08:30:00 -0400 (EDT)",
                  turbo::Time::format(fmt, nov01_ci.pre, nyc));
        EXPECT_EQ(nov01_ci.pre, nov01_ci.trans);
        EXPECT_EQ(nov01_ci.pre, nov01_ci.post);
        EXPECT_EQ(nov01_ci.pre, turbo::Time::from_civil(nov01, nyc));

        // A Spring DST transition, when there is a gap in civil time
        // and we prefer the later of the possible interpretations of a
        // non-existent time.
        turbo::CivilSecond mar13(2011, 3, 13, 2, 15, 0);
        const auto mar_ci = nyc.at(mar13);
        EXPECT_EQ(turbo::TimeZone::TimeInfo::SKIPPED, mar_ci.kind);
        EXPECT_EQ("Sun, 13 Mar 2011 03:15:00 -0400 (EDT)",
                  turbo::Time::format(fmt, mar_ci.pre, nyc));
        EXPECT_EQ("Sun, 13 Mar 2011 03:00:00 -0400 (EDT)",
                  turbo::Time::format(fmt, mar_ci.trans, nyc));
        EXPECT_EQ("Sun, 13 Mar 2011 01:15:00 -0500 (EST)",
                  turbo::Time::format(fmt, mar_ci.post, nyc));
        EXPECT_EQ(mar_ci.trans, turbo::Time::from_civil(mar13, nyc));

        // A Fall DST transition, when civil times are repeated and
        // we prefer the earlier of the possible interpretations of an
        // ambiguous time.
        turbo::CivilSecond nov06(2011, 11, 6, 1, 15, 0);
        const auto nov06_ci = nyc.at(nov06);
        EXPECT_EQ(turbo::TimeZone::TimeInfo::REPEATED, nov06_ci.kind);
        EXPECT_EQ("Sun,  6 Nov 2011 01:15:00 -0400 (EDT)",
                  turbo::Time::format(fmt, nov06_ci.pre, nyc));
        EXPECT_EQ("Sun,  6 Nov 2011 01:00:00 -0500 (EST)",
                  turbo::Time::format(fmt, nov06_ci.trans, nyc));
        EXPECT_EQ("Sun,  6 Nov 2011 01:15:00 -0500 (EST)",
                  turbo::Time::format(fmt, nov06_ci.post, nyc));
        EXPECT_EQ(nov06_ci.pre, turbo::Time::from_civil(nov06, nyc));

        // Check that (time_t) -1 is handled correctly.
        turbo::CivilSecond minus1(1969, 12, 31, 18, 59, 59);
        const auto minus1_cl = nyc.at(minus1);
        EXPECT_EQ(turbo::TimeZone::TimeInfo::UNIQUE, minus1_cl.kind);
        EXPECT_EQ(-1, turbo::Time::to_time_t(minus1_cl.pre));
        EXPECT_EQ("Wed, 31 Dec 1969 18:59:59 -0500 (EST)",
                  turbo::Time::format(fmt, minus1_cl.pre, nyc));
        EXPECT_EQ("Wed, 31 Dec 1969 23:59:59 +0000 (UTC)",
                  turbo::Time::format(fmt, minus1_cl.pre, turbo::TimeZone::utc()));
    }

    // Time::from_civil(CivilSecond(year, mon, day, hour, min, sec), TimeZone::utc())
    // has a specialized fastpath implementation, which we exercise here.
    TEST(Time, FromCivilUTC) {
        const turbo::TimeZone utc = turbo::TimeZone::utc();
        const std::string fmt = "%a, %e %b %Y %H:%M:%S %z (%Z)";
        const int kMax = std::numeric_limits<int>::max();
        const int kMin = std::numeric_limits<int>::min();
        turbo::Time t;

        // 292091940881 is the last positive year to use the fastpath.
        t = turbo::Time::from_civil(
                turbo::CivilSecond(292091940881, kMax, kMax, kMax, kMax, kMax), utc);
        EXPECT_EQ("Fri, 25 Nov 292277026596 12:21:07 +0000 (UTC)",
                  turbo::Time::format(fmt, t, utc));
        t = turbo::Time::from_civil(
                turbo::CivilSecond(292091940882, kMax, kMax, kMax, kMax, kMax), utc);
        EXPECT_EQ("infinite-future", turbo::Time::format(fmt, t, utc));  // no overflow

        // -292091936940 is the last negative year to use the fastpath.
        t = turbo::Time::from_civil(
                turbo::CivilSecond(-292091936940, kMin, kMin, kMin, kMin, kMin), utc);
        EXPECT_EQ("Fri,  1 Nov -292277022657 10:37:52 +0000 (UTC)",
                  turbo::Time::format(fmt, t, utc));
        t = turbo::Time::from_civil(
                turbo::CivilSecond(-292091936941, kMin, kMin, kMin, kMin, kMin), utc);
        EXPECT_EQ("infinite-past", turbo::Time::format(fmt, t, utc));  // no underflow

        // Check that we're counting leap years correctly.
        t = turbo::Time::from_civil(turbo::CivilSecond(1900, 2, 28, 23, 59, 59), utc);
        EXPECT_EQ("Wed, 28 Feb 1900 23:59:59 +0000 (UTC)",
                  turbo::Time::format(fmt, t, utc));
        t = turbo::Time::from_civil(turbo::CivilSecond(1900, 3, 1, 0, 0, 0), utc);
        EXPECT_EQ("Thu,  1 Mar 1900 00:00:00 +0000 (UTC)",
                  turbo::Time::format(fmt, t, utc));
        t = turbo::Time::from_civil(turbo::CivilSecond(2000, 2, 29, 23, 59, 59), utc);
        EXPECT_EQ("Tue, 29 Feb 2000 23:59:59 +0000 (UTC)",
                  turbo::Time::format(fmt, t, utc));
        t = turbo::Time::from_civil(turbo::CivilSecond(2000, 3, 1, 0, 0, 0), utc);
        EXPECT_EQ("Wed,  1 Mar 2000 00:00:00 +0000 (UTC)",
                  turbo::Time::format(fmt, t, utc));
    }

    TEST(Time, ToTM) {
        const turbo::TimeZone utc = turbo::TimeZone::utc();

        // Compares the results of turbo::ToTM() to gmtime_r() for lots of times over
        // the course of a few days.
        const turbo::Time start =
                turbo::Time::from_civil(turbo::CivilSecond(2014, 1, 2, 3, 4, 5), utc);
        const turbo::Time end =
                turbo::Time::from_civil(turbo::CivilSecond(2014, 1, 5, 3, 4, 5), utc);
        for (turbo::Time t = start; t < end; t += turbo::Duration::seconds(30)) {
            const struct tm tm_bt = turbo::Time::to_tm(t, utc);
            const time_t tt = turbo::Time::to_time_t(t);
            struct tm tm_lc;
#ifdef _WIN32
            gmtime_s(&tm_lc, &tt);
#else
            gmtime_r(&tt, &tm_lc);
#endif
            EXPECT_EQ(tm_lc.tm_year, tm_bt.tm_year);
            EXPECT_EQ(tm_lc.tm_mon, tm_bt.tm_mon);
            EXPECT_EQ(tm_lc.tm_mday, tm_bt.tm_mday);
            EXPECT_EQ(tm_lc.tm_hour, tm_bt.tm_hour);
            EXPECT_EQ(tm_lc.tm_min, tm_bt.tm_min);
            EXPECT_EQ(tm_lc.tm_sec, tm_bt.tm_sec);
            EXPECT_EQ(tm_lc.tm_wday, tm_bt.tm_wday);
            EXPECT_EQ(tm_lc.tm_yday, tm_bt.tm_yday);
            EXPECT_EQ(tm_lc.tm_isdst, tm_bt.tm_isdst);

            ASSERT_FALSE(HasFailure());
        }

        // Checks that the tm_isdst field is correct when in standard time.
        const turbo::TimeZone nyc =
                turbo::time_internal::LoadTimeZone("America/New_York");
        turbo::Time t = turbo::Time::from_civil(turbo::CivilSecond(2014, 3, 1, 0, 0, 0), nyc);
        struct tm tm = turbo::Time::to_tm(t, nyc);
        EXPECT_FALSE(tm.tm_isdst);

        // Checks that the tm_isdst field is correct when in daylight time.
        t = turbo::Time::from_civil(turbo::CivilSecond(2014, 4, 1, 0, 0, 0), nyc);
        tm = turbo::Time::to_tm(t, nyc);
        EXPECT_TRUE(tm.tm_isdst);

        // Checks overflow.
        tm = turbo::Time::to_tm(turbo::Time::future_infinite(), nyc);
        EXPECT_EQ(std::numeric_limits<int>::max() - 1900, tm.tm_year);
        EXPECT_EQ(11, tm.tm_mon);
        EXPECT_EQ(31, tm.tm_mday);
        EXPECT_EQ(23, tm.tm_hour);
        EXPECT_EQ(59, tm.tm_min);
        EXPECT_EQ(59, tm.tm_sec);
        EXPECT_EQ(4, tm.tm_wday);
        EXPECT_EQ(364, tm.tm_yday);
        EXPECT_FALSE(tm.tm_isdst);

        // Checks underflow.
        tm = turbo::Time::to_tm(turbo::Time::past_infinite(), nyc);
        EXPECT_EQ(std::numeric_limits<int>::min(), tm.tm_year);
        EXPECT_EQ(0, tm.tm_mon);
        EXPECT_EQ(1, tm.tm_mday);
        EXPECT_EQ(0, tm.tm_hour);
        EXPECT_EQ(0, tm.tm_min);
        EXPECT_EQ(0, tm.tm_sec);
        EXPECT_EQ(0, tm.tm_wday);
        EXPECT_EQ(0, tm.tm_yday);
        EXPECT_FALSE(tm.tm_isdst);
    }

    TEST(Time, from_tm) {
        const turbo::TimeZone nyc =
                turbo::time_internal::LoadTimeZone("America/New_York");

        // Verifies that tm_isdst doesn't affect anything when the time is unique.
        struct tm tm;
        std::memset(&tm, 0, sizeof(tm));
        tm.tm_year = 2014 - 1900;
        tm.tm_mon = 6 - 1;
        tm.tm_mday = 28;
        tm.tm_hour = 1;
        tm.tm_min = 2;
        tm.tm_sec = 3;
        tm.tm_isdst = -1;
        turbo::Time t = turbo::Time::from_tm(tm, nyc);
        EXPECT_EQ("2014-06-28T01:02:03-04:00", turbo::Time::format(t, nyc));  // DST
        tm.tm_isdst = 0;
        t = turbo::Time::from_tm(tm, nyc);
        EXPECT_EQ("2014-06-28T01:02:03-04:00", turbo::Time::format(t, nyc));  // DST
        tm.tm_isdst = 1;
        t = turbo::Time::from_tm(tm, nyc);
        EXPECT_EQ("2014-06-28T01:02:03-04:00", turbo::Time::format(t, nyc));  // DST

        // Adjusts tm to refer to an ambiguous time.
        tm.tm_year = 2014 - 1900;
        tm.tm_mon = 11 - 1;
        tm.tm_mday = 2;
        tm.tm_hour = 1;
        tm.tm_min = 30;
        tm.tm_sec = 42;
        tm.tm_isdst = -1;
        t = turbo::Time::from_tm(tm, nyc);
        EXPECT_EQ("2014-11-02T01:30:42-04:00", turbo::Time::format(t, nyc));  // DST
        tm.tm_isdst = 0;
        t = turbo::Time::from_tm(tm, nyc);
        EXPECT_EQ("2014-11-02T01:30:42-05:00", turbo::Time::format(t, nyc));  // STD
        tm.tm_isdst = 1;
        t = turbo::Time::from_tm(tm, nyc);
        EXPECT_EQ("2014-11-02T01:30:42-04:00", turbo::Time::format(t, nyc));  // DST

        // Adjusts tm to refer to a skipped time.
        tm.tm_year = 2014 - 1900;
        tm.tm_mon = 3 - 1;
        tm.tm_mday = 9;
        tm.tm_hour = 2;
        tm.tm_min = 30;
        tm.tm_sec = 42;
        tm.tm_isdst = -1;
        t = turbo::Time::from_tm(tm, nyc);
        EXPECT_EQ("2014-03-09T03:30:42-04:00", turbo::Time::format(t, nyc));  // DST
        tm.tm_isdst = 0;
        t = turbo::Time::from_tm(tm, nyc);
        EXPECT_EQ("2014-03-09T01:30:42-05:00", turbo::Time::format(t, nyc));  // STD
        tm.tm_isdst = 1;
        t = turbo::Time::from_tm(tm, nyc);
        EXPECT_EQ("2014-03-09T03:30:42-04:00", turbo::Time::format(t, nyc));  // DST

        // Adjusts tm to refer to a time with a year larger than 2147483647.
        tm.tm_year = 2147483647 - 1900 + 1;
        tm.tm_mon = 6 - 1;
        tm.tm_mday = 28;
        tm.tm_hour = 1;
        tm.tm_min = 2;
        tm.tm_sec = 3;
        tm.tm_isdst = -1;
        t = turbo::Time::from_tm(tm, turbo::TimeZone::utc());
        EXPECT_EQ("2147483648-06-28T01:02:03+00:00",
                  turbo::Time::format(t, turbo::TimeZone::utc()));

        // Adjusts tm to refer to a time with a very large month.
        tm.tm_year = 2019 - 1900;
        tm.tm_mon = 2147483647;
        tm.tm_mday = 28;
        tm.tm_hour = 1;
        tm.tm_min = 2;
        tm.tm_sec = 3;
        tm.tm_isdst = -1;
        t = turbo::Time::from_tm(tm, turbo::TimeZone::utc());
        EXPECT_EQ("178958989-08-28T01:02:03+00:00",
                  turbo::Time::format(t, turbo::TimeZone::utc()));
    }

    TEST(Time, TMRoundTrip) {
        const turbo::TimeZone nyc =
                turbo::time_internal::LoadTimeZone("America/New_York");

        // Test round-tripping across a skipped transition
        turbo::Time start = turbo::Time::from_civil(turbo::CivilHour(2014, 3, 9, 0), nyc);
        turbo::Time end = turbo::Time::from_civil(turbo::CivilHour(2014, 3, 9, 4), nyc);
        for (turbo::Time t = start; t < end; t += turbo::Duration::minutes(1)) {
            struct tm tm = turbo::Time::to_tm(t, nyc);
            turbo::Time rt = turbo::Time::from_tm(tm, nyc);
            EXPECT_EQ(rt, t);
        }

        // Test round-tripping across an ambiguous transition
        start = turbo::Time::from_civil(turbo::CivilHour(2014, 11, 2, 0), nyc);
        end = turbo::Time::from_civil(turbo::CivilHour(2014, 11, 2, 4), nyc);
        for (turbo::Time t = start; t < end; t += turbo::Duration::minutes(1)) {
            struct tm tm = turbo::Time::to_tm(t, nyc);
            turbo::Time rt = turbo::Time::from_tm(tm, nyc);
            EXPECT_EQ(rt, t);
        }

        // Test round-tripping of unique instants crossing a day boundary
        start = turbo::Time::from_civil(turbo::CivilHour(2014, 6, 27, 22), nyc);
        end = turbo::Time::from_civil(turbo::CivilHour(2014, 6, 28, 4), nyc);
        for (turbo::Time t = start; t < end; t += turbo::Duration::minutes(1)) {
            struct tm tm = turbo::Time::to_tm(t, nyc);
            turbo::Time rt = turbo::Time::from_tm(tm, nyc);
            EXPECT_EQ(rt, t);
        }
    }

    TEST(Time, Range) {
        // The API's documented range is +/- 100 billion years.
        const turbo::Duration range = turbo::Duration::hours(24) * 365.2425 * 100000000000;

        // Arithmetic and comparison still works at +/-range around base values.
        turbo::Time bases[2] = {turbo::Time::from_unix_epoch(), turbo::Time::current_time()};
        for (const auto base: bases) {
            turbo::Time bottom = base - range;
            EXPECT_GT(bottom, bottom - turbo::Duration::nanoseconds(1));
            EXPECT_LT(bottom, bottom + turbo::Duration::nanoseconds(1));
            turbo::Time top = base + range;
            EXPECT_GT(top, top - turbo::Duration::nanoseconds(1));
            EXPECT_LT(top, top + turbo::Duration::nanoseconds(1));
            turbo::Duration full_range = 2 * range;
            EXPECT_EQ(full_range, top - bottom);
            EXPECT_EQ(-full_range, bottom - top);
        }
    }

    TEST(Time, Limits) {
        // It is an implementation detail that Time().rep_ == Duration::zero(),
        // and that the resolution of a Duration is 1/4 of a nanosecond.
        const turbo::Time zero;
        const turbo::Time max =
                zero + turbo::Duration::seconds(std::numeric_limits<int64_t>::max()) +
                turbo::Duration::nanoseconds(999999999) + turbo::Duration::nanoseconds(3) / 4;
        const turbo::Time min =
                zero + turbo::Duration::seconds(std::numeric_limits<int64_t>::min());

        // Some simple max/min bounds checks.
        EXPECT_LT(max, turbo::Time::future_infinite());
        EXPECT_GT(min, turbo::Time::past_infinite());
        EXPECT_LT(zero, max);
        EXPECT_GT(zero, min);
        EXPECT_GE(turbo::Time::from_unix_epoch(), min);
        EXPECT_LT(turbo::Time::from_unix_epoch(), max);

        // Check sign of Time differences.
        EXPECT_LT(turbo::Duration::zero(), max - zero);
        EXPECT_LT(turbo::Duration::zero(),
                  zero - turbo::Duration::nanoseconds(1) / 4 - min);  // avoid zero - min

        // Arithmetic works at max - 0.25ns and min + 0.25ns.
        EXPECT_GT(max, max - turbo::Duration::nanoseconds(1) / 4);
        EXPECT_LT(min, min + turbo::Duration::nanoseconds(1) / 4);
    }

    TEST(Time, ConversionSaturation) {
        const turbo::TimeZone utc = turbo::TimeZone::utc();
        turbo::Time t;

        const auto max_time_t = std::numeric_limits<time_t>::max();
        const auto min_time_t = std::numeric_limits<time_t>::min();
        time_t tt = max_time_t - 1;
        t = turbo::Time::from_time_t(tt);
        tt = turbo::Time::to_time_t(t);
        EXPECT_EQ(max_time_t - 1, tt);
        t += turbo::Duration::seconds(1);
        tt = turbo::Time::to_time_t(t);
        EXPECT_EQ(max_time_t, tt);
        t += turbo::Duration::seconds(1);  // no effect
        tt = turbo::Time::to_time_t(t);
        EXPECT_EQ(max_time_t, tt);

        tt = min_time_t + 1;
        t = turbo::Time::from_time_t(tt);
        tt = turbo::Time::to_time_t(t);
        EXPECT_EQ(min_time_t + 1, tt);
        t -= turbo::Duration::seconds(1);
        tt = turbo::Time::to_time_t(t);
        EXPECT_EQ(min_time_t, tt);
        t -= turbo::Duration::seconds(1);  // no effect
        tt = turbo::Time::to_time_t(t);
        EXPECT_EQ(min_time_t, tt);

        const auto max_timeval_sec =
                std::numeric_limits<decltype(timeval::tv_sec)>::max();
        const auto min_timeval_sec =
                std::numeric_limits<decltype(timeval::tv_sec)>::min();
        timeval tv;
        tv.tv_sec = max_timeval_sec;
        tv.tv_usec = 999998;
        t = turbo::Time::from_timeval(tv);
        tv = turbo::Time::to_timeval(t);
        EXPECT_EQ(max_timeval_sec, tv.tv_sec);
        EXPECT_EQ(999998, tv.tv_usec);
        t += turbo::Duration::microseconds(1);
        tv = turbo::Time::to_timeval(t);
        EXPECT_EQ(max_timeval_sec, tv.tv_sec);
        EXPECT_EQ(999999, tv.tv_usec);
        t += turbo::Duration::microseconds(1);  // no effect
        tv = turbo::Time::to_timeval(t);
        EXPECT_EQ(max_timeval_sec, tv.tv_sec);
        EXPECT_EQ(999999, tv.tv_usec);

        tv.tv_sec = min_timeval_sec;
        tv.tv_usec = 1;
        t = turbo::Time::from_timeval(tv);
        tv = turbo::Time::to_timeval(t);
        EXPECT_EQ(min_timeval_sec, tv.tv_sec);
        EXPECT_EQ(1, tv.tv_usec);
        t -= turbo::Duration::microseconds(1);
        tv = turbo::Time::to_timeval(t);
        EXPECT_EQ(min_timeval_sec, tv.tv_sec);
        EXPECT_EQ(0, tv.tv_usec);
        t -= turbo::Duration::microseconds(1);  // no effect
        tv = turbo::Time::to_timeval(t);
        EXPECT_EQ(min_timeval_sec, tv.tv_sec);
        EXPECT_EQ(0, tv.tv_usec);

        const auto max_timespec_sec =
                std::numeric_limits<decltype(timespec::tv_sec)>::max();
        const auto min_timespec_sec =
                std::numeric_limits<decltype(timespec::tv_sec)>::min();
        timespec ts;
        ts.tv_sec = max_timespec_sec;
        ts.tv_nsec = 999999998;
        t = turbo::Time::from_timespec(ts);
        ts = turbo::Time::to_timespec(t);
        EXPECT_EQ(max_timespec_sec, ts.tv_sec);
        EXPECT_EQ(999999998, ts.tv_nsec);
        t += turbo::Duration::nanoseconds(1);
        ts = turbo::Time::to_timespec(t);
        EXPECT_EQ(max_timespec_sec, ts.tv_sec);
        EXPECT_EQ(999999999, ts.tv_nsec);
        t += turbo::Duration::nanoseconds(1);  // no effect
        ts = turbo::Time::to_timespec(t);
        EXPECT_EQ(max_timespec_sec, ts.tv_sec);
        EXPECT_EQ(999999999, ts.tv_nsec);

        ts.tv_sec = min_timespec_sec;
        ts.tv_nsec = 1;
        t = turbo::Time::from_timespec(ts);
        ts = turbo::Time::to_timespec(t);
        EXPECT_EQ(min_timespec_sec, ts.tv_sec);
        EXPECT_EQ(1, ts.tv_nsec);
        t -= turbo::Duration::nanoseconds(1);
        ts = turbo::Time::to_timespec(t);
        EXPECT_EQ(min_timespec_sec, ts.tv_sec);
        EXPECT_EQ(0, ts.tv_nsec);
        t -= turbo::Duration::nanoseconds(1);  // no effect
        ts = turbo::Time::to_timespec(t);
        EXPECT_EQ(min_timespec_sec, ts.tv_sec);
        EXPECT_EQ(0, ts.tv_nsec);

        // Checks how TimeZone::at() saturates on infinities.
        auto ci = utc.at(turbo::Time::future_infinite());
        EXPECT_CIVIL_INFO(ci, std::numeric_limits<int64_t>::max(), 12, 31, 23, 59, 59,
                          0, false);
        EXPECT_EQ(turbo::Duration::max_infinite(), ci.subsecond);
        EXPECT_EQ(turbo::Weekday::thursday, turbo::GetWeekday(ci.cs));
        EXPECT_EQ(365, turbo::GetYearDay(ci.cs));
        EXPECT_STREQ("-00", ci.zone_abbr);  // artifact of TimeZone::at()
        ci = utc.at(turbo::Time::past_infinite());
        EXPECT_CIVIL_INFO(ci, std::numeric_limits<int64_t>::min(), 1, 1, 0, 0, 0, 0,
                          false);
        EXPECT_EQ(-turbo::Duration::max_infinite(), ci.subsecond);
        EXPECT_EQ(turbo::Weekday::sunday, turbo::GetWeekday(ci.cs));
        EXPECT_EQ(1, turbo::GetYearDay(ci.cs));
        EXPECT_STREQ("-00", ci.zone_abbr);  // artifact of TimeZone::at()

        // Approach the maximal Time value from below.
        t = turbo::Time::from_civil(turbo::CivilSecond(292277026596, 12, 4, 15, 30, 6), utc);
        EXPECT_EQ("292277026596-12-04T15:30:06+00:00",
                  turbo::Time::format(turbo::RFC3339_full, t, utc));
        t = turbo::Time::from_civil(turbo::CivilSecond(292277026596, 12, 4, 15, 30, 7), utc);
        EXPECT_EQ("292277026596-12-04T15:30:07+00:00",
                  turbo::Time::format(turbo::RFC3339_full, t, utc));
        EXPECT_EQ(
                turbo::Time::from_unix_epoch() + turbo::Duration::seconds(std::numeric_limits<int64_t>::max()),
                t);

        // Checks that we can also get the maximal Time value for a far-east zone.
        const turbo::TimeZone plus14 = turbo::TimeZone::fixed(14 * 60 * 60);
        t = turbo::Time::from_civil(turbo::CivilSecond(292277026596, 12, 5, 5, 30, 7), plus14);
        EXPECT_EQ("292277026596-12-05T05:30:07+14:00",
                  turbo::Time::format(turbo::RFC3339_full, t, plus14));
        EXPECT_EQ(
                turbo::Time::from_unix_epoch() + turbo::Duration::seconds(std::numeric_limits<int64_t>::max()),
                t);

        // One second later should push us to infinity.
        t = turbo::Time::from_civil(turbo::CivilSecond(292277026596, 12, 4, 15, 30, 8), utc);
        EXPECT_EQ("infinite-future", turbo::Time::format(turbo::RFC3339_full, t, utc));

        // Approach the minimal Time value from above.
        t = turbo::Time::from_civil(turbo::CivilSecond(-292277022657, 1, 27, 8, 29, 53), utc);
        EXPECT_EQ("-292277022657-01-27T08:29:53+00:00",
                  turbo::Time::format(turbo::RFC3339_full, t, utc));
        t = turbo::Time::from_civil(turbo::CivilSecond(-292277022657, 1, 27, 8, 29, 52), utc);
        EXPECT_EQ("-292277022657-01-27T08:29:52+00:00",
                  turbo::Time::format(turbo::RFC3339_full, t, utc));
        EXPECT_EQ(
                turbo::Time::from_unix_epoch() + turbo::Duration::seconds(std::numeric_limits<int64_t>::min()),
                t);

        // Checks that we can also get the minimal Time value for a far-west zone.
        const turbo::TimeZone minus12 = turbo::TimeZone::fixed(-12 * 60 * 60);
        t = turbo::Time::from_civil(turbo::CivilSecond(-292277022657, 1, 26, 20, 29, 52),
                             minus12);
        EXPECT_EQ("-292277022657-01-26T20:29:52-12:00",
                  turbo::Time::format(turbo::RFC3339_full, t, minus12));
        EXPECT_EQ(
                turbo::Time::from_unix_epoch() + turbo::Duration::seconds(std::numeric_limits<int64_t>::min()),
                t);

        // One second before should push us to -infinity.
        t = turbo::Time::from_civil(turbo::CivilSecond(-292277022657, 1, 27, 8, 29, 51), utc);
        EXPECT_EQ("infinite-past", turbo::Time::format(turbo::RFC3339_full, t, utc));
    }

    // In zones with POSIX-style recurring rules we use special logic to
    // handle conversions in the distant future.  Here we check the limits
    // of those conversions, particularly with respect to integer overflow.
    TEST(Time, ExtendedConversionSaturation) {
        const turbo::TimeZone syd =
                turbo::time_internal::LoadTimeZone("Australia/Sydney");
        const turbo::TimeZone nyc =
                turbo::time_internal::LoadTimeZone("America/New_York");
        const turbo::Time max =
                turbo::Time::from_seconds(std::numeric_limits<int64_t>::max());
        turbo::TimeZone::CivilInfo ci;
        turbo::Time t;

        // The maximal time converted in each zone.
        ci = syd.at(max);
        EXPECT_CIVIL_INFO(ci, 292277026596, 12, 5, 2, 30, 7, 39600, true);
        t = turbo::Time::from_civil(turbo::CivilSecond(292277026596, 12, 5, 2, 30, 7), syd);
        EXPECT_EQ(max, t);
        ci = nyc.at(max);
        EXPECT_CIVIL_INFO(ci, 292277026596, 12, 4, 10, 30, 7, -18000, false);
        t = turbo::Time::from_civil(turbo::CivilSecond(292277026596, 12, 4, 10, 30, 7), nyc);
        EXPECT_EQ(max, t);

        // One second later should push us to infinity.
        t = turbo::Time::from_civil(turbo::CivilSecond(292277026596, 12, 5, 2, 30, 8), syd);
        EXPECT_EQ(turbo::Time::future_infinite(), t);
        t = turbo::Time::from_civil(turbo::CivilSecond(292277026596, 12, 4, 10, 30, 8), nyc);
        EXPECT_EQ(turbo::Time::future_infinite(), t);

        // And we should stick there.
        t = turbo::Time::from_civil(turbo::CivilSecond(292277026596, 12, 5, 2, 30, 9), syd);
        EXPECT_EQ(turbo::Time::future_infinite(), t);
        t = turbo::Time::from_civil(turbo::CivilSecond(292277026596, 12, 4, 10, 30, 9), nyc);
        EXPECT_EQ(turbo::Time::future_infinite(), t);

        // All the way up to a saturated date/time, without overflow.
        t = turbo::Time::from_civil(turbo::CivilSecond::max(), syd);
        EXPECT_EQ(turbo::Time::future_infinite(), t);
        t = turbo::Time::from_civil(turbo::CivilSecond::max(), nyc);
        EXPECT_EQ(turbo::Time::future_infinite(), t);
    }

    TEST(Time, FromCivilAlignment) {
        const turbo::TimeZone utc = turbo::TimeZone::utc();
        const turbo::CivilSecond cs(2015, 2, 3, 4, 5, 6);
        turbo::Time t = turbo::Time::from_civil(cs, utc);
        EXPECT_EQ("2015-02-03T04:05:06+00:00", turbo::Time::format(t, utc));
        t = turbo::Time::from_civil(turbo::CivilMinute(cs), utc);
        EXPECT_EQ("2015-02-03T04:05:00+00:00", turbo::Time::format(t, utc));
        t = turbo::Time::from_civil(turbo::CivilHour(cs), utc);
        EXPECT_EQ("2015-02-03T04:00:00+00:00", turbo::Time::format(t, utc));
        t = turbo::Time::from_civil(turbo::CivilDay(cs), utc);
        EXPECT_EQ("2015-02-03T00:00:00+00:00", turbo::Time::format(t, utc));
        t = turbo::Time::from_civil(turbo::CivilMonth(cs), utc);
        EXPECT_EQ("2015-02-01T00:00:00+00:00", turbo::Time::format(t, utc));
        t = turbo::Time::from_civil(turbo::CivilYear(cs), utc);
        EXPECT_EQ("2015-01-01T00:00:00+00:00", turbo::Time::format(t, utc));
    }

    TEST(Time, NextTransitionUTC) {
        const auto tz = turbo::TimeZone::utc();
        turbo::TimeZone::CivilTransition trans;

        auto t = turbo::Time::past_infinite();
        EXPECT_FALSE(tz.NextTransition(t, &trans));

        t = turbo::Time::future_infinite();
        EXPECT_FALSE(tz.NextTransition(t, &trans));
    }

    TEST(Time, PrevTransitionUTC) {
        const auto tz = turbo::TimeZone::utc();
        turbo::TimeZone::CivilTransition trans;

        auto t = turbo::Time::future_infinite();
        EXPECT_FALSE(tz.PrevTransition(t, &trans));

        t = turbo::Time::past_infinite();
        EXPECT_FALSE(tz.PrevTransition(t, &trans));
    }

    TEST(Time, NextTransitionNYC) {
        const auto tz = turbo::time_internal::LoadTimeZone("America/New_York");
        turbo::TimeZone::CivilTransition trans;

        auto t = turbo::Time::from_civil(turbo::CivilSecond(2018, 6, 30, 0, 0, 0), tz);
        EXPECT_TRUE(tz.NextTransition(t, &trans));
        EXPECT_EQ(turbo::CivilSecond(2018, 11, 4, 2, 0, 0), trans.from);
        EXPECT_EQ(turbo::CivilSecond(2018, 11, 4, 1, 0, 0), trans.to);

        t = turbo::Time::future_infinite();
        EXPECT_FALSE(tz.NextTransition(t, &trans));

        t = turbo::Time::past_infinite();
        EXPECT_TRUE(tz.NextTransition(t, &trans));
        if (trans.from == turbo::CivilSecond(1918, 03, 31, 2, 0, 0)) {
            // It looks like the tzdata is only 32 bit (probably macOS),
            // which bottoms out at 1901-12-13T20:45:52+00:00.
            EXPECT_EQ(turbo::CivilSecond(1918, 3, 31, 3, 0, 0), trans.to);
        } else {
            EXPECT_EQ(turbo::CivilSecond(1883, 11, 18, 12, 3, 58), trans.from);
            EXPECT_EQ(turbo::CivilSecond(1883, 11, 18, 12, 0, 0), trans.to);
        }
    }

    TEST(Time, PrevTransitionNYC) {
        const auto tz = turbo::time_internal::LoadTimeZone("America/New_York");
        turbo::TimeZone::CivilTransition trans;

        auto t = turbo::Time::from_civil(turbo::CivilSecond(2018, 6, 30, 0, 0, 0), tz);
        EXPECT_TRUE(tz.PrevTransition(t, &trans));
        EXPECT_EQ(turbo::CivilSecond(2018, 3, 11, 2, 0, 0), trans.from);
        EXPECT_EQ(turbo::CivilSecond(2018, 3, 11, 3, 0, 0), trans.to);

        t = turbo::Time::past_infinite();
        EXPECT_FALSE(tz.PrevTransition(t, &trans));

        t = turbo::Time::future_infinite();
        EXPECT_TRUE(tz.PrevTransition(t, &trans));
        // We have a transition but we don't know which one.
    }

    TEST(Time, turbo_stringify) {
        // Time::format is already well tested, so just use one test case here to
        // verify that StrFormat("%v", t) works as expected.
        turbo::Time t = turbo::Time::current_time();
        EXPECT_EQ(turbo::StrFormat("%v", t), turbo::Time::format(t));
    }

}  // namespace
