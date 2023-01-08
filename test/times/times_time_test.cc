
/****************************************************************
 * Copyright (c) 2022, liyinbin
 * All rights reserved.
 * Author by liyinbin (jeff.li) lijippy@163.com
 *****************************************************************/

#include "flare/times/time.h"

#if defined(_MSC_VER)
#include <winsock2.h>  // for timeval
#endif

#include <chrono>  // NOLINT(build/c++11)
#include <cstring>
#include <ctime>
#include <iomanip>
#include <limits>
#include <string>


#include "testing/gtest_wrap.h"
#include "flare/base/int128.h"
#include "testing/time_util.h"

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
        if (ts.tv_sec == arg.tv_sec && ts.tv_nsec == arg.tv_nsec)
            return true;
        *result_listener << "expected: {" << ts.tv_sec << ", " << ts.tv_nsec << "} ";
        *result_listener << "actual: {" << arg.tv_sec << ", " << arg.tv_nsec << "}";
        return false;
    }

// A gMock matcher to match timeval values. Use this matcher like:
// timeval tv1, tv2;
// EXPECT_THAT(tv1, TimevalMatcher(tv2));
    MATCHER_P(TimevalMatcher, tv, "") {
        if (tv.tv_sec == arg.tv_sec && tv.tv_usec == arg.tv_usec)
            return true;
        *result_listener << "expected: {" << tv.tv_sec << ", " << tv.tv_usec << "} ";
        *result_listener << "actual: {" << arg.tv_sec << ", " << arg.tv_usec << "}";
        return false;
    }

    TEST(time_point, ConstExpr) {
        constexpr flare::time_point t0 = flare::time_point::unix_epoch();
        static_assert(t0 == flare::time_point(), "unix_epoch");
        constexpr flare::time_point t1 = flare::time_point::infinite_future();
        static_assert(t1 != flare::time_point(), "infinite_future");
        constexpr flare::time_point t2 = flare::time_point::infinite_past();
        static_assert(t2 != flare::time_point(), "infinite_past");
        constexpr flare::time_point t3 = flare::time_point::from_unix_nanos(0);
        static_assert(t3 == flare::time_point(), "from_unix_nanos");
        constexpr flare::time_point t4 = flare::time_point::from_unix_micros(0);
        static_assert(t4 == flare::time_point(), "from_unix_micros");
        constexpr flare::time_point t5 = flare::time_point::from_unix_millis(0);
        static_assert(t5 == flare::time_point(), "from_unix_millis");
        constexpr flare::time_point t6 = flare::time_point::from_unix_seconds(0);
        static_assert(t6 == flare::time_point(), "from_unix_seconds");
        constexpr flare::time_point t7 = flare::time_point::from_time_t(0);
        static_assert(t7 == flare::time_point(), "from_time_t");
    }

    TEST(time_point, ValueSemantics) {
        flare::time_point a;      // Default construction
        flare::time_point b = a;  // Copy construction
        EXPECT_EQ(a, b);
        flare::time_point c(a);  // Copy construction (again)
        EXPECT_EQ(a, b);
        EXPECT_EQ(a, c);
        EXPECT_EQ(b, c);
        b = c;       // Assignment
        EXPECT_EQ(a, b);
        EXPECT_EQ(a, c);
        EXPECT_EQ(b, c);
    }

    TEST(time_point, unix_epoch) {
        const auto ci = flare::utc_time_zone().at(flare::time_point::unix_epoch());
        EXPECT_EQ(flare::chrono_second(1970, 1, 1, 0, 0, 0), ci.cs);
        EXPECT_EQ(flare::zero_duration(), ci.subsecond);
        EXPECT_EQ(flare::chrono_weekday::thursday, flare::get_weekday(ci.cs));
    }

    TEST(time_point, breakdown) {
        flare::time_zone tz = flare::times_internal::load_time_zone("America/New_York");
        flare::time_point t = flare::time_point::unix_epoch();

        // The Unix epoch as seen in NYC.
        auto ci = tz.at(t);
        EXPECT_CIVIL_INFO(ci, 1969, 12, 31, 19, 0, 0, -18000, false);
        EXPECT_EQ(flare::zero_duration(), ci.subsecond);
        EXPECT_EQ(flare::chrono_weekday::wednesday, flare::get_weekday(ci.cs));

        // Just before the epoch.
        t -= flare::duration::nanoseconds(1);
        ci = tz.at(t);
        EXPECT_CIVIL_INFO(ci, 1969, 12, 31, 18, 59, 59, -18000, false);
        EXPECT_EQ(flare::duration::nanoseconds(999999999), ci.subsecond);
        EXPECT_EQ(flare::chrono_weekday::wednesday, flare::get_weekday(ci.cs));

        // Some time later.
        t += flare::duration::hours(24) * 2735;
        t += flare::duration::hours(18) + flare::duration::minutes(30) + flare::duration::seconds(15) +
             flare::duration::nanoseconds(9);
        ci = tz.at(t);
        EXPECT_CIVIL_INFO(ci, 1977, 6, 28, 14, 30, 15, -14400, true);
        EXPECT_EQ(8, ci.subsecond / flare::duration::nanoseconds(1));
        EXPECT_EQ(flare::chrono_weekday::tuesday, flare::get_weekday(ci.cs));
    }

    TEST(time_point, AdditiveOperators) {
        const flare::duration d = flare::duration::nanoseconds(1);
        const flare::time_point t0;
        const flare::time_point t1 = t0 + d;

        EXPECT_EQ(d, t1 - t0);
        EXPECT_EQ(-d, t0 - t1);
        EXPECT_EQ(t0, t1 - d);

        flare::time_point t(t0);
        EXPECT_EQ(t0, t);
        t += d;
        EXPECT_EQ(t0 + d, t);
        EXPECT_EQ(d, t - t0);
        t -= d;
        EXPECT_EQ(t0, t);

        // Tests overflow between subseconds and seconds.
        t = flare::time_point::unix_epoch();
        t += flare::duration::milliseconds(500);
        EXPECT_EQ(flare::time_point::unix_epoch() + flare::duration::milliseconds(500), t);
        t += flare::duration::milliseconds(600);
        EXPECT_EQ(flare::time_point::unix_epoch() + flare::duration::milliseconds(1100), t);
        t -= flare::duration::milliseconds(600);
        EXPECT_EQ(flare::time_point::unix_epoch() + flare::duration::milliseconds(500), t);
        t -= flare::duration::milliseconds(500);
        EXPECT_EQ(flare::time_point::unix_epoch(), t);
    }

    TEST(time_point, RelationalOperators) {
        constexpr flare::time_point t1 = flare::time_point::from_unix_nanos(0);
        constexpr flare::time_point t2 = flare::time_point::from_unix_nanos(1);
        constexpr flare::time_point t3 = flare::time_point::from_unix_nanos(2);

        static_assert(flare::time_point() == t1, "");
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

    TEST(time_point, Infinity) {
        constexpr flare::time_point ifuture = flare::time_point::infinite_future();
        constexpr flare::time_point ipast = flare::time_point::infinite_past();

        static_assert(ifuture == ifuture, "");
        static_assert(ipast == ipast, "");
        static_assert(ipast < ifuture, "");
        static_assert(ifuture > ipast, "");

        // Arithmetic saturates
        EXPECT_EQ(ifuture, ifuture + flare::duration::seconds(1));
        EXPECT_EQ(ifuture, ifuture - flare::duration::seconds(1));
        EXPECT_EQ(ipast, ipast + flare::duration::seconds(1));
        EXPECT_EQ(ipast, ipast - flare::duration::seconds(1));

        EXPECT_EQ(flare::infinite_duration(), ifuture - ifuture);
        EXPECT_EQ(flare::infinite_duration(), ifuture - ipast);
        EXPECT_EQ(-flare::infinite_duration(), ipast - ifuture);
        EXPECT_EQ(-flare::infinite_duration(), ipast - ipast);

        constexpr flare::time_point t = flare::time_point::unix_epoch();  // Any finite time.
        static_assert(t < ifuture, "");
        static_assert(t > ipast, "");
    }

    TEST(time_point, FloorConversion) {
#define TEST_FLOOR_CONVERSION(TO, FROM) \
  EXPECT_EQ(1, FROM(1001).TO());         \
  EXPECT_EQ(1, FROM(1000).TO());         \
  EXPECT_EQ(0, FROM(999).TO());          \
  EXPECT_EQ(0, FROM(1).TO());            \
  EXPECT_EQ(0, FROM(0).TO());            \
  EXPECT_EQ(-1, FROM(-1).TO());          \
  EXPECT_EQ(-1, FROM(-999).TO());        \
  EXPECT_EQ(-1, FROM(-1000).TO());       \
  EXPECT_EQ(-2, FROM(-1001).TO());

        TEST_FLOOR_CONVERSION(to_unix_micros, flare::time_point::from_unix_nanos);
        TEST_FLOOR_CONVERSION(to_unix_millis, flare::time_point::from_unix_micros);
        TEST_FLOOR_CONVERSION(to_unix_seconds, flare::time_point::from_unix_millis);
        TEST_FLOOR_CONVERSION(to_time_t, flare::time_point::from_unix_millis);

#undef TEST_FLOOR_CONVERSION

        // Tests to_unix_nanos.
        EXPECT_EQ(1, (flare::time_point::unix_epoch() + flare::duration::nanoseconds(3) / 2).to_unix_nanos());
        EXPECT_EQ(1, (flare::time_point::unix_epoch() + flare::duration::nanoseconds(1)).to_unix_nanos());
        EXPECT_EQ(0, (flare::time_point::unix_epoch() + flare::duration::nanoseconds(1) / 2).to_unix_nanos());
        EXPECT_EQ(0, (flare::time_point::unix_epoch() + flare::duration::nanoseconds(0)).to_unix_nanos());
        EXPECT_EQ(-1,
                  (flare::time_point::unix_epoch() - flare::duration::nanoseconds(1) / 2).to_unix_nanos());
        EXPECT_EQ(-1, (flare::time_point::unix_epoch() - flare::duration::nanoseconds(1)).to_unix_nanos());
        EXPECT_EQ(-2,
                  (flare::time_point::unix_epoch() - flare::duration::nanoseconds(3) / 2).to_unix_nanos());

        // Tests to_universal, which uses a different epoch than the tests above.
        EXPECT_EQ(1,
                  (flare::time_point::universal_epoch() + flare::duration::nanoseconds(101)).to_universal());
        EXPECT_EQ(1,
                  (flare::time_point::universal_epoch() + flare::duration::nanoseconds(100)).to_universal());
        EXPECT_EQ(0,
                  (flare::time_point::universal_epoch() + flare::duration::nanoseconds(99)).to_universal());
        EXPECT_EQ(0,
                  (flare::time_point::universal_epoch() + flare::duration::nanoseconds(1)).to_universal());
        EXPECT_EQ(0,
                  (flare::time_point::universal_epoch() + flare::duration::nanoseconds(0)).to_universal());
        EXPECT_EQ(-1,
                  (flare::time_point::universal_epoch() + flare::duration::nanoseconds(-1)).to_universal());
        EXPECT_EQ(-1,
                  (flare::time_point::universal_epoch() + flare::duration::nanoseconds(-99)).to_universal());
        EXPECT_EQ(
                -1, (flare::time_point::universal_epoch() + flare::duration::nanoseconds(-100)).to_universal());
        EXPECT_EQ(
                -2, (flare::time_point::universal_epoch() + flare::duration::nanoseconds(-101)).to_universal());

        // Tests to_timespec()/time_from_timespec()
        const struct {
            flare::time_point t;
            timespec ts;
        } to_ts[] = {
                {flare::time_point::from_unix_seconds(1) + flare::duration::nanoseconds(1),      {1,  1}},
                {flare::time_point::from_unix_seconds(1) + flare::duration::nanoseconds(1) / 2,  {1,  0}},
                {flare::time_point::from_unix_seconds(1) + flare::duration::nanoseconds(0),      {1,  0}},
                {flare::time_point::from_unix_seconds(0) + flare::duration::nanoseconds(0),      {0,  0}},
                {flare::time_point::from_unix_seconds(0) - flare::duration::nanoseconds(1) / 2,  {-1, 999999999}},
                {flare::time_point::from_unix_seconds(0) - flare::duration::nanoseconds(1),      {-1, 999999999}},
                {flare::time_point::from_unix_seconds(-1) + flare::duration::nanoseconds(1),     {-1, 1}},
                {flare::time_point::from_unix_seconds(-1) + flare::duration::nanoseconds(1) / 2, {-1, 0}},
                {flare::time_point::from_unix_seconds(-1) + flare::duration::nanoseconds(0),     {-1, 0}},
                {flare::time_point::from_unix_seconds(-1) - flare::duration::nanoseconds(1) / 2, {-2, 999999999}},
        };
        for (const auto &test : to_ts) {
            EXPECT_THAT(test.t.to_timespec(), TimespecMatcher(test.ts));
        }
        const struct {
            timespec ts;
            flare::time_point t;
        } from_ts[] = {
                {{1,  1},         flare::time_point::from_unix_seconds(1) + flare::duration::nanoseconds(1)},
                {{1,  0},         flare::time_point::from_unix_seconds(1) + flare::duration::nanoseconds(0)},
                {{0,  0},         flare::time_point::from_unix_seconds(0) + flare::duration::nanoseconds(0)},
                {{0,  -1},        flare::time_point::from_unix_seconds(0) - flare::duration::nanoseconds(1)},
                {{-1, 999999999}, flare::time_point::from_unix_seconds(0) - flare::duration::nanoseconds(1)},
                {{-1, 1},         flare::time_point::from_unix_seconds(-1) + flare::duration::nanoseconds(1)},
                {{-1, 0},         flare::time_point::from_unix_seconds(-1) + flare::duration::nanoseconds(0)},
                {{-1, -1},        flare::time_point::from_unix_seconds(-1) - flare::duration::nanoseconds(1)},
                {{-2, 999999999}, flare::time_point::from_unix_seconds(-1) - flare::duration::nanoseconds(1)},
        };
        for (const auto &test : from_ts) {
            EXPECT_EQ(test.t, flare::time_point::from_timespec(test.ts));
        }

        // Tests to_timeval()/time_from_timeval() (same as timespec above)
        const struct {
            flare::time_point t;
            timeval tv;
        } to_tv[] = {
                {flare::time_point::from_unix_seconds(1) + flare::duration::microseconds(1),      {1,  1}},
                {flare::time_point::from_unix_seconds(1) + flare::duration::microseconds(1) / 2,  {1,  0}},
                {flare::time_point::from_unix_seconds(1) + flare::duration::microseconds(0),      {1,  0}},
                {flare::time_point::from_unix_seconds(0) + flare::duration::microseconds(0),      {0,  0}},
                {flare::time_point::from_unix_seconds(0) - flare::duration::microseconds(1) / 2,  {-1, 999999}},
                {flare::time_point::from_unix_seconds(0) - flare::duration::microseconds(1),      {-1, 999999}},
                {flare::time_point::from_unix_seconds(-1) + flare::duration::microseconds(1),     {-1, 1}},
                {flare::time_point::from_unix_seconds(-1) + flare::duration::microseconds(1) / 2, {-1, 0}},
                {flare::time_point::from_unix_seconds(-1) + flare::duration::microseconds(0),     {-1, 0}},
                {flare::time_point::from_unix_seconds(-1) - flare::duration::microseconds(1) / 2, {-2, 999999}},
        };
        for (const auto &test : to_tv) {
            EXPECT_THAT(test.t.to_timeval(), TimevalMatcher(test.tv));
        }
        const struct {
            timeval tv;
            flare::time_point t;
        } from_tv[] = {
                {{1,  1},      flare::time_point::from_unix_seconds(1) + flare::duration::microseconds(1)},
                {{1,  0},      flare::time_point::from_unix_seconds(1) + flare::duration::microseconds(0)},
                {{0,  0},      flare::time_point::from_unix_seconds(0) + flare::duration::microseconds(0)},
                {{0,  -1},     flare::time_point::from_unix_seconds(0) - flare::duration::microseconds(1)},
                {{-1, 999999}, flare::time_point::from_unix_seconds(0) - flare::duration::microseconds(1)},
                {{-1, 1},      flare::time_point::from_unix_seconds(-1) + flare::duration::microseconds(1)},
                {{-1, 0},      flare::time_point::from_unix_seconds(-1) + flare::duration::microseconds(0)},
                {{-1, -1},     flare::time_point::from_unix_seconds(-1) - flare::duration::microseconds(1)},
                {{-2, 999999}, flare::time_point::from_unix_seconds(-1) - flare::duration::microseconds(1)},
        };
        for (const auto &test : from_tv) {
            EXPECT_EQ(test.t, flare::time_point::from_timeval(test.tv));
        }

        // Tests flooring near negative infinity.
        const int64_t min_plus_1 = std::numeric_limits<int64_t>::min() + 1;
        EXPECT_EQ(min_plus_1, flare::time_point::from_unix_seconds(min_plus_1).to_unix_seconds());
        EXPECT_EQ(std::numeric_limits<int64_t>::min(),
                  (flare::time_point::from_unix_seconds(min_plus_1) - flare::duration::nanoseconds(1) / 2).to_unix_seconds());

        // Tests flooring near positive infinity.
        EXPECT_EQ(std::numeric_limits<int64_t>::max(),
                  (flare::time_point::from_unix_seconds(
                          std::numeric_limits<int64_t>::max()) + flare::duration::nanoseconds(1) / 2).to_unix_seconds());
        EXPECT_EQ(std::numeric_limits<int64_t>::max(),
                  (flare::time_point::from_unix_seconds(std::numeric_limits<int64_t>::max())).to_unix_seconds());
        EXPECT_EQ(std::numeric_limits<int64_t>::max() - 1,
                  (flare::time_point::from_unix_seconds(
                          std::numeric_limits<int64_t>::max()) - flare::duration::nanoseconds(1) / 2).to_unix_seconds());
    }

    TEST(time_point, RoundtripConversion) {
#define TEST_CONVERSION_ROUND_TRIP(SOURCE, FROM, TO, MATCHER) \
  EXPECT_THAT(FROM(SOURCE).TO(), MATCHER(SOURCE))

        // from_unix_nanos() and to_unix_nanos()
        int64_t now_ns = flare::get_current_time_nanos();
        TEST_CONVERSION_ROUND_TRIP(-1, flare::time_point::from_unix_nanos, to_unix_nanos,
                                   testing::Eq);
        TEST_CONVERSION_ROUND_TRIP(0, flare::time_point::from_unix_nanos, to_unix_nanos,
                                   testing::Eq);
        TEST_CONVERSION_ROUND_TRIP(1, flare::time_point::from_unix_nanos, to_unix_nanos,
                                   testing::Eq);
        TEST_CONVERSION_ROUND_TRIP(now_ns, flare::time_point::from_unix_nanos, to_unix_nanos,
                                   testing::Eq)
                            << now_ns;

        // from_unix_micros() and to_unix_micros()
        int64_t now_us = flare::get_current_time_nanos() / 1000;
        TEST_CONVERSION_ROUND_TRIP(-1, flare::time_point::from_unix_micros, to_unix_micros,
                                   testing::Eq);
        TEST_CONVERSION_ROUND_TRIP(0, flare::time_point::from_unix_micros, to_unix_micros,
                                   testing::Eq);
        TEST_CONVERSION_ROUND_TRIP(1, flare::time_point::from_unix_micros, to_unix_micros,
                                   testing::Eq);
        TEST_CONVERSION_ROUND_TRIP(now_us, flare::time_point::from_unix_micros, to_unix_micros,
                                   testing::Eq)
                            << now_us;

        // from_unix_millis() and to_unix_millis()
        int64_t now_ms = flare::get_current_time_nanos() / 1000000;
        TEST_CONVERSION_ROUND_TRIP(-1, flare::time_point::from_unix_millis, to_unix_millis,
                                   testing::Eq);
        TEST_CONVERSION_ROUND_TRIP(0, flare::time_point::from_unix_millis, to_unix_millis,
                                   testing::Eq);
        TEST_CONVERSION_ROUND_TRIP(1, flare::time_point::from_unix_millis, to_unix_millis,
                                   testing::Eq);
        TEST_CONVERSION_ROUND_TRIP(now_ms, flare::time_point::from_unix_millis, to_unix_millis,
                                   testing::Eq)
                            << now_ms;

        // from_unix_seconds() and to_unix_seconds()
        int64_t now_s = std::time(nullptr);
        TEST_CONVERSION_ROUND_TRIP(-1, flare::time_point::from_unix_seconds, to_unix_seconds,
                                   testing::Eq);
        TEST_CONVERSION_ROUND_TRIP(0, flare::time_point::from_unix_seconds, to_unix_seconds,
                                   testing::Eq);
        TEST_CONVERSION_ROUND_TRIP(1, flare::time_point::from_unix_seconds, to_unix_seconds,
                                   testing::Eq);
        TEST_CONVERSION_ROUND_TRIP(now_s, flare::time_point::from_unix_seconds, to_unix_seconds,
                                   testing::Eq)
                            << now_s;

        // from_time_t() and to_time_t()
        time_t now_time_t = std::time(nullptr);
        TEST_CONVERSION_ROUND_TRIP(-1, flare::time_point::from_time_t, to_time_t, testing::Eq);
        TEST_CONVERSION_ROUND_TRIP(0, flare::time_point::from_time_t, to_time_t, testing::Eq);
        TEST_CONVERSION_ROUND_TRIP(1, flare::time_point::from_time_t, to_time_t, testing::Eq);
        TEST_CONVERSION_ROUND_TRIP(now_time_t, flare::time_point::from_time_t, to_time_t,
                                   testing::Eq)
                            << now_time_t;

        // time_from_timeval() and to_timeval()
        timeval tv;
        tv.tv_sec = -1;
        tv.tv_usec = 0;
        TEST_CONVERSION_ROUND_TRIP(tv, flare::time_point::from_timeval, to_timeval,
                                   TimevalMatcher);
        tv.tv_sec = -1;
        tv.tv_usec = 999999;
        TEST_CONVERSION_ROUND_TRIP(tv, flare::time_point::from_timeval, to_timeval,
                                   TimevalMatcher);
        tv.tv_sec = 0;
        tv.tv_usec = 0;
        TEST_CONVERSION_ROUND_TRIP(tv, flare::time_point::from_timeval, to_timeval,
                                   TimevalMatcher);
        tv.tv_sec = 0;
        tv.tv_usec = 1;
        TEST_CONVERSION_ROUND_TRIP(tv, flare::time_point::from_timeval, to_timeval,
                                   TimevalMatcher);
        tv.tv_sec = 1;
        tv.tv_usec = 0;
        TEST_CONVERSION_ROUND_TRIP(tv, flare::time_point::from_timeval, to_timeval,
                                   TimevalMatcher);

        // time_from_timespec() and to_timespec()
        timespec ts;
        ts.tv_sec = -1;
        ts.tv_nsec = 0;
        TEST_CONVERSION_ROUND_TRIP(ts, flare::time_point::from_timespec, to_timespec,
                                   TimespecMatcher);
        ts.tv_sec = -1;
        ts.tv_nsec = 999999999;
        TEST_CONVERSION_ROUND_TRIP(ts, flare::time_point::from_timespec, to_timespec,
                                   TimespecMatcher);
        ts.tv_sec = 0;
        ts.tv_nsec = 0;
        TEST_CONVERSION_ROUND_TRIP(ts, flare::time_point::from_timespec, to_timespec,
                                   TimespecMatcher);
        ts.tv_sec = 0;
        ts.tv_nsec = 1;
        TEST_CONVERSION_ROUND_TRIP(ts, flare::time_point::from_timespec, to_timespec,
                                   TimespecMatcher);
        ts.tv_sec = 1;
        ts.tv_nsec = 0;
        TEST_CONVERSION_ROUND_TRIP(ts, flare::time_point::from_timespec, to_timespec,
                                   TimespecMatcher);

        // from_date() and to_date()
        double now_ud = flare::get_current_time_nanos() / 1000000;
        TEST_CONVERSION_ROUND_TRIP(-1.5, flare::time_point::from_date, to_date,
                                   testing::DoubleEq);
        TEST_CONVERSION_ROUND_TRIP(-1, flare::time_point::from_date, to_date,
                                   testing::DoubleEq);
        TEST_CONVERSION_ROUND_TRIP(-0.5, flare::time_point::from_date, to_date,
                                   testing::DoubleEq);
        TEST_CONVERSION_ROUND_TRIP(0, flare::time_point::from_date, to_date,
                                   testing::DoubleEq);
        TEST_CONVERSION_ROUND_TRIP(0.5, flare::time_point::from_date, to_date,
                                   testing::DoubleEq);
        TEST_CONVERSION_ROUND_TRIP(1, flare::time_point::from_date, to_date,
                                   testing::DoubleEq);
        TEST_CONVERSION_ROUND_TRIP(1.5, flare::time_point::from_date, to_date,
                                   testing::DoubleEq);
        TEST_CONVERSION_ROUND_TRIP(now_ud, flare::time_point::from_date, to_date,
                                   testing::DoubleEq)
                            << std::fixed << std::setprecision(17) << now_ud;

        // from_universal() and to_universal()
        int64_t now_uni = ((719162LL * (24 * 60 * 60)) * (1000 * 1000 * 10)) +
                          (flare::get_current_time_nanos() / 100);
        TEST_CONVERSION_ROUND_TRIP(-1, flare::time_point::from_universal, to_universal,
                                   testing::Eq);
        TEST_CONVERSION_ROUND_TRIP(0, flare::time_point::from_universal, to_universal,
                                   testing::Eq);
        TEST_CONVERSION_ROUND_TRIP(1, flare::time_point::from_universal, to_universal,
                                   testing::Eq);
        TEST_CONVERSION_ROUND_TRIP(now_uni, flare::time_point::from_universal, to_universal,
                                   testing::Eq)
                            << now_uni;

#undef TEST_CONVERSION_ROUND_TRIP
    }

    template<typename duration>
    std::chrono::system_clock::time_point MakeChronoUnixTime(const duration &d) {
        return std::chrono::system_clock::from_time_t(0) + d;
    }

    TEST(time_point, from_chrono) {
        std::cout<<__LINE__<<std::endl;
        EXPECT_EQ(flare::time_point::from_time_t(-1),
                  flare::time_point::from_chrono(std::chrono::system_clock::from_time_t(-1)));
        std::cout<<__LINE__<<std::endl;
        EXPECT_EQ(flare::time_point::from_time_t(0),
                  flare::time_point::from_chrono(std::chrono::system_clock::from_time_t(0)));
        EXPECT_EQ(flare::time_point::from_time_t(1),
                  flare::time_point::from_chrono(std::chrono::system_clock::from_time_t(1)));

        EXPECT_EQ(
                flare::time_point::from_unix_millis(-1),
                flare::time_point::from_chrono(MakeChronoUnixTime(std::chrono::milliseconds(-1))));
        EXPECT_EQ(flare::time_point::from_unix_millis(0),
                  flare::time_point::from_chrono(MakeChronoUnixTime(std::chrono::milliseconds(0))));
        EXPECT_EQ(flare::time_point::from_unix_millis(1),
                  flare::time_point::from_chrono(MakeChronoUnixTime(std::chrono::milliseconds(1))));
        std::cout<<__LINE__<<std::endl;
        // Chrono doesn't define exactly its range and precision (neither does
        // flare::time_point), so let's simply test +/- ~100 years to make sure things work.
        const auto century_sec = 60 * 60 * 24 * 365 * int64_t{100};
        const auto century = std::chrono::seconds(century_sec);
        const auto chrono_future = MakeChronoUnixTime(century);
        const auto chrono_past = MakeChronoUnixTime(-century);
        EXPECT_EQ(flare::time_point::from_unix_seconds(century_sec),
                  flare::time_point::from_chrono(chrono_future));
        EXPECT_EQ(flare::time_point::from_unix_seconds(-century_sec), flare::time_point::from_chrono(chrono_past));

        // Roundtrip them both back to chrono.
        EXPECT_EQ(chrono_future,
                  flare::time_point::from_unix_seconds(century_sec).to_chrono_time());
        EXPECT_EQ(chrono_past,
                  flare::time_point::from_unix_seconds(-century_sec).to_chrono_time());
    }

    TEST(time_point, to_chrono_time) {
        EXPECT_EQ(std::chrono::system_clock::from_time_t(-1),
                  flare::time_point::from_time_t(-1).to_chrono_time());
        EXPECT_EQ(std::chrono::system_clock::from_time_t(0),
                  flare::time_point::from_time_t(0).to_chrono_time());
        EXPECT_EQ(std::chrono::system_clock::from_time_t(1),
                  flare::time_point::from_time_t(1).to_chrono_time());

        EXPECT_EQ(MakeChronoUnixTime(std::chrono::milliseconds(-1)),
                  flare::time_point::from_unix_millis(-1).to_chrono_time());
        EXPECT_EQ(MakeChronoUnixTime(std::chrono::milliseconds(0)),
                  flare::time_point::from_unix_millis(0).to_chrono_time());
        EXPECT_EQ(MakeChronoUnixTime(std::chrono::milliseconds(1)),
                  flare::time_point::from_unix_millis(1).to_chrono_time());

        // time_point before the Unix epoch should floor, not trunc.
        const auto tick = flare::duration::nanoseconds(1) / 4;
        EXPECT_EQ(std::chrono::system_clock::from_time_t(0) -
                  std::chrono::system_clock::duration(1),
                  (flare::time_point::unix_epoch() - tick).to_chrono_time());
    }

// Check that flare::int128 works as a std::chrono::duration representation.
    TEST(time_point, Chrono128) {
        // Define a std::chrono::time_point type whose time[sic]_since_epoch() is
        // a signed 128-bit count of attoseconds. This has a range and resolution
        // (currently) beyond those of flare::time_point, and undoubtedly also beyond those
        // of std::chrono::system_clock::time_point.
        //
        // Note: The to/from-chrono support should probably be updated to handle
        // such wide representations.
        using Timestamp =
        std::chrono::time_point<std::chrono::system_clock,
                std::chrono::duration<flare::int128, std::atto>>;

        // Expect that we can round-trip the std::chrono::system_clock::time_point
        // extremes through both flare::time_point and Timestamp, and that Timestamp can
        // handle the (current) flare::time_point extremes.
        //
        // Note: We should use std::chrono::floor() instead of time_point_cast(),
        // but floor() is only available since c++17.
        for (const auto& tp : {std::chrono::system_clock::time_point::min(),
                              std::chrono::system_clock::time_point::max()}) {
            EXPECT_EQ(tp, flare::time_point::from_chrono(tp).to_chrono_time());
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

    TEST(time_point, TimeZoneAt) {
      std::cout<<__LINE__<<std::endl;
        const flare::time_zone nyc =
                flare::times_internal::load_time_zone("America/New_York");
        const std::string fmt = "%a, %e %b %Y %H:%M:%S %z (%Z)";

        std::cout<<__LINE__<<std::endl;
        // A non-transition where the civil time is unique.
        flare::chrono_second nov01(2013, 11, 1, 8, 30, 0);
        const auto nov01_ci = nyc.at(nov01);
        EXPECT_EQ(flare::time_zone::time_info::UNIQUE, nov01_ci.kind)<<__LINE__;
        EXPECT_EQ("Fri,  1 Nov 2013 08:30:00 -0400 (EDT)",
                  flare::format_time(fmt, nov01_ci.pre, nyc));
        EXPECT_EQ(nov01_ci.pre, nov01_ci.trans);
        EXPECT_EQ(nov01_ci.pre, nov01_ci.post);
        EXPECT_EQ(nov01_ci.pre, flare::from_chrono(nov01, nyc));
        std::cout<<__LINE__<<std::endl;
        // A Spring DST transition, when there is a gap in civil time
        // and we prefer the later of the possible interpretations of a
        // non-existent time.
        flare::chrono_second mar13(2011, 3, 13, 2, 15, 0);
        const auto mar_ci = nyc.at(mar13);
        EXPECT_EQ(flare::time_zone::time_info::SKIPPED, mar_ci.kind);
        EXPECT_EQ("Sun, 13 Mar 2011 03:15:00 -0400 (EDT)",
                  flare::format_time(fmt, mar_ci.pre, nyc));
        EXPECT_EQ("Sun, 13 Mar 2011 03:00:00 -0400 (EDT)",
                  flare::format_time(fmt, mar_ci.trans, nyc));
        EXPECT_EQ("Sun, 13 Mar 2011 01:15:00 -0500 (EST)",
                  flare::format_time(fmt, mar_ci.post, nyc));
        EXPECT_EQ(mar_ci.trans, flare::from_chrono(mar13, nyc));

        // A Fall DST transition, when civil times are repeated and
        // we prefer the earlier of the possible interpretations of an
        // ambiguous time.
        flare::chrono_second nov06(2011, 11, 6, 1, 15, 0);
        const auto nov06_ci = nyc.at(nov06);
        EXPECT_EQ(flare::time_zone::time_info::REPEATED, nov06_ci.kind);
        EXPECT_EQ("Sun,  6 Nov 2011 01:15:00 -0400 (EDT)",
                  flare::format_time(fmt, nov06_ci.pre, nyc));
        EXPECT_EQ("Sun,  6 Nov 2011 01:00:00 -0500 (EST)",
                  flare::format_time(fmt, nov06_ci.trans, nyc));
        EXPECT_EQ("Sun,  6 Nov 2011 01:15:00 -0500 (EST)",
                  flare::format_time(fmt, nov06_ci.post, nyc));
        EXPECT_EQ(nov06_ci.pre, flare::from_chrono(nov06, nyc));

        // Check that (time_t) -1 is handled correctly.
        flare::chrono_second minus1(1969, 12, 31, 18, 59, 59);
        const auto minus1_cl = nyc.at(minus1);
        EXPECT_EQ(flare::time_zone::time_info::UNIQUE, minus1_cl.kind);
        EXPECT_EQ(-1, minus1_cl.pre.to_time_t());
        EXPECT_EQ("Wed, 31 Dec 1969 18:59:59 -0500 (EST)",
                  flare::format_time(fmt, minus1_cl.pre, nyc));
        EXPECT_EQ("Wed, 31 Dec 1969 23:59:59 +0000 (UTC)",
                  flare::format_time(fmt, minus1_cl.pre, flare::utc_time_zone()));
    }

// from_chrono(chrono_second(year, mon, day, hour, min, sec), utc_time_zone())
// has a specialized fastpath implementation, which we exercise here.
    TEST(time_point, FromCivilUTC) {
        const flare::time_zone utc = flare::utc_time_zone();
        const std::string fmt = "%a, %e %b %Y %H:%M:%S %z (%Z)";
        const int kMax = std::numeric_limits<int>::max();
        const int kMin = std::numeric_limits<int>::min();
        flare::time_point t;

        // 292091940881 is the last positive year to use the fastpath.
        t = flare::from_chrono(
                flare::chrono_second(292091940881, kMax, kMax, kMax, kMax, kMax), utc);
        EXPECT_EQ("Fri, 25 Nov 292277026596 12:21:07 +0000 (UTC)",
                  flare::format_time(fmt, t, utc));
        t = flare::from_chrono(
                flare::chrono_second(292091940882, kMax, kMax, kMax, kMax, kMax), utc);
        EXPECT_EQ("infinite-future", flare::format_time(fmt, t, utc));  // no overflow

        // -292091936940 is the last negative year to use the fastpath.
        t = flare::from_chrono(
                flare::chrono_second(-292091936940, kMin, kMin, kMin, kMin, kMin), utc);
        EXPECT_EQ("Fri,  1 Nov -292277022657 10:37:52 +0000 (UTC)",
                  flare::format_time(fmt, t, utc));
        t = flare::from_chrono(
                flare::chrono_second(-292091936941, kMin, kMin, kMin, kMin, kMin), utc);
        EXPECT_EQ("infinite-past", flare::format_time(fmt, t, utc));  // no underflow

        // Check that we're counting leap years correctly.
        t = flare::from_chrono(flare::chrono_second(1900, 2, 28, 23, 59, 59), utc);
        EXPECT_EQ("Wed, 28 Feb 1900 23:59:59 +0000 (UTC)",
                  flare::format_time(fmt, t, utc));
        t = flare::from_chrono(flare::chrono_second(1900, 3, 1, 0, 0, 0), utc);
        EXPECT_EQ("Thu,  1 Mar 1900 00:00:00 +0000 (UTC)",
                  flare::format_time(fmt, t, utc));
        t = flare::from_chrono(flare::chrono_second(2000, 2, 29, 23, 59, 59), utc);
        EXPECT_EQ("Tue, 29 Feb 2000 23:59:59 +0000 (UTC)",
                  flare::format_time(fmt, t, utc));
        t = flare::from_chrono(flare::chrono_second(2000, 3, 1, 0, 0, 0), utc);
        EXPECT_EQ("Wed,  1 Mar 2000 00:00:00 +0000 (UTC)",
                  flare::format_time(fmt, t, utc));
    }

    TEST(time_point, to_tm) {
        const flare::time_zone utc = flare::utc_time_zone();

        // Compares the results of to_tm() to gmtime_r() for lots of times over the
        // course of a few days.
        const flare::time_point start =
                flare::from_chrono(flare::chrono_second(2014, 1, 2, 3, 4, 5), utc);
        const flare::time_point end =
                flare::from_chrono(flare::chrono_second(2014, 1, 5, 3, 4, 5), utc);
        for (flare::time_point t = start; t < end; t += flare::duration::seconds(30)) {
            const struct tm tm_bt = to_tm(t, utc);
            const time_t tt = t.to_time_t();
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
        const flare::time_zone nyc =
                flare::times_internal::load_time_zone("America/New_York");
        flare::time_point t = flare::from_chrono(flare::chrono_second(2014, 3, 1, 0, 0, 0), nyc);
        struct tm tm = to_tm(t, nyc);
        EXPECT_FALSE(tm.tm_isdst);

        // Checks that the tm_isdst field is correct when in daylight time.
        t = flare::from_chrono(flare::chrono_second(2014, 4, 1, 0, 0, 0), nyc);
        tm = to_tm(t, nyc);
        EXPECT_TRUE(tm.tm_isdst);

        // Checks overflow.
        tm = to_tm(flare::time_point::infinite_future(), nyc);
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
        tm = to_tm(flare::time_point::infinite_past(), nyc);
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

    TEST(time_point, from_tm) {
        const flare::time_zone nyc =
                flare::times_internal::load_time_zone("America/New_York");

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
        flare::time_point t = from_tm(tm, nyc);
        EXPECT_EQ("2014-06-28T01:02:03-04:00", flare::format_time(t, nyc));  // DST
        tm.tm_isdst = 0;
        t = from_tm(tm, nyc);
        EXPECT_EQ("2014-06-28T01:02:03-04:00", flare::format_time(t, nyc));  // DST
        tm.tm_isdst = 1;
        t = from_tm(tm, nyc);
        EXPECT_EQ("2014-06-28T01:02:03-04:00", flare::format_time(t, nyc));  // DST

        // Adjusts tm to refer to an ambiguous time.
        tm.tm_year = 2014 - 1900;
        tm.tm_mon = 11 - 1;
        tm.tm_mday = 2;
        tm.tm_hour = 1;
        tm.tm_min = 30;
        tm.tm_sec = 42;
        tm.tm_isdst = -1;
        t = from_tm(tm, nyc);
        EXPECT_EQ("2014-11-02T01:30:42-04:00", flare::format_time(t, nyc));  // DST
        tm.tm_isdst = 0;
        t = from_tm(tm, nyc);
        EXPECT_EQ("2014-11-02T01:30:42-05:00", flare::format_time(t, nyc));  // STD
        tm.tm_isdst = 1;
        t = from_tm(tm, nyc);
        EXPECT_EQ("2014-11-02T01:30:42-04:00", flare::format_time(t, nyc));  // DST

        // Adjusts tm to refer to a skipped time.
        tm.tm_year = 2014 - 1900;
        tm.tm_mon = 3 - 1;
        tm.tm_mday = 9;
        tm.tm_hour = 2;
        tm.tm_min = 30;
        tm.tm_sec = 42;
        tm.tm_isdst = -1;
        t = from_tm(tm, nyc);
        EXPECT_EQ("2014-03-09T03:30:42-04:00", flare::format_time(t, nyc));  // DST
        tm.tm_isdst = 0;
        t = from_tm(tm, nyc);
        EXPECT_EQ("2014-03-09T01:30:42-05:00", flare::format_time(t, nyc));  // STD
        tm.tm_isdst = 1;
        t = from_tm(tm, nyc);
        EXPECT_EQ("2014-03-09T03:30:42-04:00", flare::format_time(t, nyc));  // DST

        // Adjusts tm to refer to a time with a year larger than 2147483647.
        tm.tm_year = 2147483647 - 1900 + 1;
        tm.tm_mon = 6 - 1;
        tm.tm_mday = 28;
        tm.tm_hour = 1;
        tm.tm_min = 2;
        tm.tm_sec = 3;
        tm.tm_isdst = -1;
        t = from_tm(tm, flare::utc_time_zone());
        EXPECT_EQ("2147483648-06-28T01:02:03+00:00",
                  flare::format_time(t, flare::utc_time_zone()));

        // Adjusts tm to refer to a time with a very large month.
        tm.tm_year = 2019 - 1900;
        tm.tm_mon = 2147483647;
        tm.tm_mday = 28;
        tm.tm_hour = 1;
        tm.tm_min = 2;
        tm.tm_sec = 3;
        tm.tm_isdst = -1;
        t = from_tm(tm, flare::utc_time_zone());
        EXPECT_EQ("178958989-08-28T01:02:03+00:00",
                  flare::format_time(t, flare::utc_time_zone()));
    }

    TEST(time_point, TMRoundTrip) {
        const flare::time_zone nyc =
                flare::times_internal::load_time_zone("America/New_York");

        // Test round-tripping across a skipped transition
        flare::time_point start = flare::from_chrono(flare::chrono_hour(2014, 3, 9, 0), nyc);
        flare::time_point end = flare::from_chrono(flare::chrono_hour(2014, 3, 9, 4), nyc);
        for (flare::time_point t = start; t < end; t += flare::duration::minutes(1)) {
            struct tm tm = to_tm(t, nyc);
            flare::time_point rt = from_tm(tm, nyc);
            EXPECT_EQ(rt, t);
        }

        // Test round-tripping across an ambiguous transition
        start = flare::from_chrono(flare::chrono_hour(2014, 11, 2, 0), nyc);
        end = flare::from_chrono(flare::chrono_hour(2014, 11, 2, 4), nyc);
        for (flare::time_point t = start; t < end; t += flare::duration::minutes(1)) {
            struct tm tm = to_tm(t, nyc);
            flare::time_point rt = from_tm(tm, nyc);
            EXPECT_EQ(rt, t);
        }

        // Test round-tripping of unique instants crossing a day boundary
        start = flare::from_chrono(flare::chrono_hour(2014, 6, 27, 22), nyc);
        end = flare::from_chrono(flare::chrono_hour(2014, 6, 28, 4), nyc);
        for (flare::time_point t = start; t < end; t += flare::duration::minutes(1)) {
            struct tm tm = to_tm(t, nyc);
            flare::time_point rt = from_tm(tm, nyc);
            EXPECT_EQ(rt, t);
        }
    }

    TEST(time_point, Range) {
        // The API's documented range is +/- 100 billion years.
        const flare::duration range = flare::duration::hours(24) * 365.2425 * 100000000000;

        // Arithmetic and comparison still works at +/-range around base values.
        flare::time_point bases[2] = {flare::time_point::unix_epoch(), flare::time_now()};
        for (const auto& base : bases) {
            flare::time_point bottom = base - range;
            EXPECT_GT(bottom, bottom - flare::duration::nanoseconds(1));
            EXPECT_LT(bottom, bottom + flare::duration::nanoseconds(1));
            flare::time_point top = base + range;
            EXPECT_GT(top, top - flare::duration::nanoseconds(1));
            EXPECT_LT(top, top + flare::duration::nanoseconds(1));
            flare::duration full_range = 2 * range;
            EXPECT_EQ(full_range, top - bottom);
            EXPECT_EQ(-full_range, bottom - top);
        }
    }

    TEST(time_point, Limits) {
        // It is an implementation detail that time_point().rep_ == zero_duration(),
        // and that the resolution of a duration is 1/4 of a nanosecond.
        const flare::time_point zero;
        const flare::time_point max =
                zero + flare::duration::seconds(std::numeric_limits<int64_t>::max()) +
                flare::duration::nanoseconds(999999999) + flare::duration::nanoseconds(3) / 4;
        const flare::time_point min =
                zero + flare::duration::seconds(std::numeric_limits<int64_t>::min());

        // Some simple max/min bounds checks.
        EXPECT_LT(max, flare::time_point::infinite_future());
        EXPECT_GT(min, flare::time_point::infinite_past());
        EXPECT_LT(zero, max);
        EXPECT_GT(zero, min);
        EXPECT_GE(flare::time_point::unix_epoch(), min);
        EXPECT_LT(flare::time_point::unix_epoch(), max);

        // Check sign of time_point differences.
        EXPECT_LT(flare::zero_duration(), max - zero);
        EXPECT_LT(flare::zero_duration(),
                  zero - flare::duration::nanoseconds(1) / 4 - min);  // avoid zero - min

        // Arithmetic works at max - 0.25ns and min + 0.25ns.
        EXPECT_GT(max, max - flare::duration::nanoseconds(1) / 4);
        EXPECT_LT(min, min + flare::duration::nanoseconds(1) / 4);
    }

    TEST(time_point, ConversionSaturation) {
        const flare::time_zone utc = flare::utc_time_zone();
        flare::time_point t;

        const auto max_time_t = std::numeric_limits<time_t>::max();
        const auto min_time_t = std::numeric_limits<time_t>::min();
        time_t tt = max_time_t - 1;
        t = flare::time_point::from_time_t(tt);
        tt = t.to_time_t();
        EXPECT_EQ(max_time_t - 1, tt);
        t += flare::duration::seconds(1);
        tt = t.to_time_t();
        EXPECT_EQ(max_time_t, tt);
        t += flare::duration::seconds(1);  // no effect
        tt = t.to_time_t();
        EXPECT_EQ(max_time_t, tt);

        tt = min_time_t + 1;
        t = flare::time_point::from_time_t(tt);
        tt = t.to_time_t();
        EXPECT_EQ(min_time_t + 1, tt);
        t -= flare::duration::seconds(1);
        tt = t.to_time_t();
        EXPECT_EQ(min_time_t, tt);
        t -= flare::duration::seconds(1);  // no effect
        tt = t.to_time_t();
        EXPECT_EQ(min_time_t, tt);

        const auto max_timeval_sec =
                std::numeric_limits<decltype(timeval::tv_sec)>::max();
        const auto min_timeval_sec =
                std::numeric_limits<decltype(timeval::tv_sec)>::min();
        timeval tv;
        tv.tv_sec = max_timeval_sec;
        tv.tv_usec = 999998;
        t = flare::time_point::from_timeval(tv);
        tv = t.to_timeval();
        EXPECT_EQ(max_timeval_sec, tv.tv_sec);
        EXPECT_EQ(999998, tv.tv_usec);
        t += flare::duration::microseconds(1);
        tv = t.to_timeval();
        EXPECT_EQ(max_timeval_sec, tv.tv_sec);
        EXPECT_EQ(999999, tv.tv_usec);
        t += flare::duration::microseconds(1);  // no effect
        tv = t.to_timeval();
        EXPECT_EQ(max_timeval_sec, tv.tv_sec);
        EXPECT_EQ(999999, tv.tv_usec);

        tv.tv_sec = min_timeval_sec;
        tv.tv_usec = 1;
        t = flare::time_point::from_timeval(tv);
        tv = t.to_timeval();
        EXPECT_EQ(min_timeval_sec, tv.tv_sec);
        EXPECT_EQ(1, tv.tv_usec);
        t -= flare::duration::microseconds(1);
        tv = t.to_timeval();
        EXPECT_EQ(min_timeval_sec, tv.tv_sec);
        EXPECT_EQ(0, tv.tv_usec);
        t -= flare::duration::microseconds(1);  // no effect
        tv = t.to_timeval();
        EXPECT_EQ(min_timeval_sec, tv.tv_sec);
        EXPECT_EQ(0, tv.tv_usec);

        const auto max_timespec_sec =
                std::numeric_limits<decltype(timespec::tv_sec)>::max();
        const auto min_timespec_sec =
                std::numeric_limits<decltype(timespec::tv_sec)>::min();
        timespec ts;
        ts.tv_sec = max_timespec_sec;
        ts.tv_nsec = 999999998;
        t = flare::time_point::from_timespec(ts);
        ts = t.to_timespec();
        EXPECT_EQ(max_timespec_sec, ts.tv_sec);
        EXPECT_EQ(999999998, ts.tv_nsec);
        t += flare::duration::nanoseconds(1);
        ts = t.to_timespec();
        EXPECT_EQ(max_timespec_sec, ts.tv_sec);
        EXPECT_EQ(999999999, ts.tv_nsec);
        t += flare::duration::nanoseconds(1);  // no effect
        ts = t.to_timespec();
        EXPECT_EQ(max_timespec_sec, ts.tv_sec);
        EXPECT_EQ(999999999, ts.tv_nsec);

        ts.tv_sec = min_timespec_sec;
        ts.tv_nsec = 1;
        t = flare::time_point::from_timespec(ts);
        ts = t.to_timespec();
        EXPECT_EQ(min_timespec_sec, ts.tv_sec);
        EXPECT_EQ(1, ts.tv_nsec);
        t -= flare::duration::nanoseconds(1);
        ts = t.to_timespec();
        EXPECT_EQ(min_timespec_sec, ts.tv_sec);
        EXPECT_EQ(0, ts.tv_nsec);
        t -= flare::duration::nanoseconds(1);  // no effect
        ts = t.to_timespec();
        EXPECT_EQ(min_timespec_sec, ts.tv_sec);
        EXPECT_EQ(0, ts.tv_nsec);

        // Checks how time_zone::At() saturates on infinities.
        auto ci = utc.at(flare::time_point::infinite_future());
        EXPECT_CIVIL_INFO(ci, std::numeric_limits<int64_t>::max(), 12, 31, 23,
                          59, 59, 0, false);
        EXPECT_EQ(flare::infinite_duration(), ci.subsecond);
        EXPECT_EQ(flare::chrono_weekday::thursday, flare::get_weekday(ci.cs));
        EXPECT_EQ(365, flare::get_yearday(ci.cs));
        EXPECT_STREQ("-00", ci.zone_abbr);  // artifact of time_zone::At()
        ci = utc.at(flare::time_point::infinite_past());
        EXPECT_CIVIL_INFO(ci, std::numeric_limits<int64_t>::min(), 1, 1, 0, 0,
                          0, 0, false);
        EXPECT_EQ(-flare::infinite_duration(), ci.subsecond);
        EXPECT_EQ(flare::chrono_weekday::sunday, flare::get_weekday(ci.cs));
        EXPECT_EQ(1, flare::get_yearday(ci.cs));
        EXPECT_STREQ("-00", ci.zone_abbr);  // artifact of time_zone::At()

        // Approach the maximal time_point value from below.
        t = flare::from_chrono(flare::chrono_second(292277026596, 12, 4, 15, 30, 6), utc);
        EXPECT_EQ("292277026596-12-04T15:30:06+00:00",
                  flare::format_time(flare::RFC3339_full, t, utc));
        t = flare::from_chrono(flare::chrono_second(292277026596, 12, 4, 15, 30, 7), utc);
        EXPECT_EQ("292277026596-12-04T15:30:07+00:00",
                  flare::format_time(flare::RFC3339_full, t, utc));
        EXPECT_EQ(
                flare::time_point::unix_epoch() + flare::duration::seconds(std::numeric_limits<int64_t>::max()), t);

        // Checks that we can also get the maximal time_point value for a far-east zone.
        const flare::time_zone plus14 = flare::fixed_time_zone(14 * 60 * 60);
        t = flare::from_chrono(flare::chrono_second(292277026596, 12, 5, 5, 30, 7), plus14);
        EXPECT_EQ("292277026596-12-05T05:30:07+14:00",
                  flare::format_time(flare::RFC3339_full, t, plus14));
        EXPECT_EQ(
                flare::time_point::unix_epoch() + flare::duration::seconds(std::numeric_limits<int64_t>::max()), t);

        // One second later should push us to infinity.
        t = flare::from_chrono(flare::chrono_second(292277026596, 12, 4, 15, 30, 8), utc);
        EXPECT_EQ("infinite-future", flare::format_time(flare::RFC3339_full, t, utc));

        // Approach the minimal time_point value from above.
        t = flare::from_chrono(flare::chrono_second(-292277022657, 1, 27, 8, 29, 53), utc);
        EXPECT_EQ("-292277022657-01-27T08:29:53+00:00",
                  flare::format_time(flare::RFC3339_full, t, utc));
        t = flare::from_chrono(flare::chrono_second(-292277022657, 1, 27, 8, 29, 52), utc);
        EXPECT_EQ("-292277022657-01-27T08:29:52+00:00",
                  flare::format_time(flare::RFC3339_full, t, utc));
        EXPECT_EQ(
                flare::time_point::unix_epoch() + flare::duration::seconds(std::numeric_limits<int64_t>::min()), t);

        // Checks that we can also get the minimal time_point value for a far-west zone.
        const flare::time_zone minus12 = flare::fixed_time_zone(-12 * 60 * 60);
        t = flare::from_chrono(flare::chrono_second(-292277022657, 1, 26, 20, 29, 52),
                              minus12);
        EXPECT_EQ("-292277022657-01-26T20:29:52-12:00",
                  flare::format_time(flare::RFC3339_full, t, minus12));
        EXPECT_EQ(
                flare::time_point::unix_epoch() + flare::duration::seconds(std::numeric_limits<int64_t>::min()), t);

        // One second before should push us to -infinity.
        t = flare::from_chrono(flare::chrono_second(-292277022657, 1, 27, 8, 29, 51), utc);
        EXPECT_EQ("infinite-past", flare::format_time(flare::RFC3339_full, t, utc));
    }

// In zones with POSIX-style recurring rules we use special logic to
// handle conversions in the distant future.  Here we check the limits
// of those conversions, particularly with respect to integer overflow.
    TEST(time_point, ExtendedConversionSaturation) {
        const flare::time_zone syd =
                flare::times_internal::load_time_zone("Australia/Sydney");
        const flare::time_zone nyc =
                flare::times_internal::load_time_zone("America/New_York");
        const flare::time_point max =
                flare::time_point::from_unix_seconds(std::numeric_limits<int64_t>::max());
        flare::time_zone::chrono_info ci;
        flare::time_point t;

        // The maximal time converted in each zone.
        ci = syd.at(max);
        EXPECT_CIVIL_INFO(ci, 292277026596, 12, 5, 2, 30, 7, 39600, true);
        t = flare::from_chrono(flare::chrono_second(292277026596, 12, 5, 2, 30, 7), syd);
        EXPECT_EQ(max, t);
        ci = nyc.at(max);
        EXPECT_CIVIL_INFO(ci, 292277026596, 12, 4, 10, 30, 7, -18000, false);
        t = flare::from_chrono(flare::chrono_second(292277026596, 12, 4, 10, 30, 7), nyc);
        EXPECT_EQ(max, t);

        // One second later should push us to infinity.
        t = flare::from_chrono(flare::chrono_second(292277026596, 12, 5, 2, 30, 8), syd);
        EXPECT_EQ(flare::time_point::infinite_future(), t);
        t = flare::from_chrono(flare::chrono_second(292277026596, 12, 4, 10, 30, 8), nyc);
        EXPECT_EQ(flare::time_point::infinite_future(), t);

        // And we should stick there.
        t = flare::from_chrono(flare::chrono_second(292277026596, 12, 5, 2, 30, 9), syd);
        EXPECT_EQ(flare::time_point::infinite_future(), t);
        t = flare::from_chrono(flare::chrono_second(292277026596, 12, 4, 10, 30, 9), nyc);
        EXPECT_EQ(flare::time_point::infinite_future(), t);

        // All the way up to a saturated date/time, without overflow.
        t = flare::from_chrono(flare::chrono_second::max(), syd);
        EXPECT_EQ(flare::time_point::infinite_future(), t);
        t = flare::from_chrono(flare::chrono_second::max(), nyc);
        EXPECT_EQ(flare::time_point::infinite_future(), t);
    }

    TEST(time_point, FromCivilAlignment) {
        const flare::time_zone utc = flare::utc_time_zone();
        const flare::chrono_second cs(2015, 2, 3, 4, 5, 6);
        flare::time_point t = flare::from_chrono(cs, utc);
        EXPECT_EQ("2015-02-03T04:05:06+00:00", flare::format_time(t, utc));
        t = flare::from_chrono(flare::chrono_minute(cs), utc);
        EXPECT_EQ("2015-02-03T04:05:00+00:00", flare::format_time(t, utc));
        t = flare::from_chrono(flare::chrono_hour(cs), utc);
        EXPECT_EQ("2015-02-03T04:00:00+00:00", flare::format_time(t, utc));
        t = flare::from_chrono(flare::chrono_day(cs), utc);
        EXPECT_EQ("2015-02-03T00:00:00+00:00", flare::format_time(t, utc));
        t = flare::from_chrono(flare::chrono_month(cs), utc);
        EXPECT_EQ("2015-02-01T00:00:00+00:00", flare::format_time(t, utc));
        t = flare::from_chrono(flare::chrono_year(cs), utc);
        EXPECT_EQ("2015-01-01T00:00:00+00:00", flare::format_time(t, utc));
    }

    TEST(time_point, LegacyDateTime) {
        const flare::time_zone utc = flare::utc_time_zone();
        const std::string ymdhms = "%Y-%m-%d %H:%M:%S";
        const int kMax = std::numeric_limits<int>::max();
        const int kMin = std::numeric_limits<int>::min();
        flare::time_point t;

        t = flare::format_date_time(std::numeric_limits<flare::chrono_year_t>::max(),
                                   kMax, kMax, kMax, kMax, kMax, utc);
        EXPECT_EQ("infinite-future",
                  flare::format_time(ymdhms, t, utc));  // no overflow
        t = flare::format_date_time(std::numeric_limits<flare::chrono_year_t>::min(),
                                   kMin, kMin, kMin, kMin, kMin, utc);
        EXPECT_EQ("infinite-past",
                  flare::format_time(ymdhms, t, utc));  // no overflow

        // Check normalization.
        EXPECT_TRUE(flare::convert_date_time(2013, 10, 32, 8, 30, 0, utc).normalized);
        t = flare::format_date_time(2015, 1, 1, 0, 0, 60, utc);
        EXPECT_EQ("2015-01-01 00:01:00", flare::format_time(ymdhms, t, utc));
        t = flare::format_date_time(2015, 1, 1, 0, 60, 0, utc);
        EXPECT_EQ("2015-01-01 01:00:00", flare::format_time(ymdhms, t, utc));
        t = flare::format_date_time(2015, 1, 1, 24, 0, 0, utc);
        EXPECT_EQ("2015-01-02 00:00:00", flare::format_time(ymdhms, t, utc));
        t = flare::format_date_time(2015, 1, 32, 0, 0, 0, utc);
        EXPECT_EQ("2015-02-01 00:00:00", flare::format_time(ymdhms, t, utc));
        t = flare::format_date_time(2015, 13, 1, 0, 0, 0, utc);
        EXPECT_EQ("2016-01-01 00:00:00", flare::format_time(ymdhms, t, utc));
        t = flare::format_date_time(2015, 13, 32, 60, 60, 60, utc);
        EXPECT_EQ("2016-02-03 13:01:00", flare::format_time(ymdhms, t, utc));
        t = flare::format_date_time(2015, 1, 1, 0, 0, -1, utc);
        EXPECT_EQ("2014-12-31 23:59:59", flare::format_time(ymdhms, t, utc));
        t = flare::format_date_time(2015, 1, 1, 0, -1, 0, utc);
        EXPECT_EQ("2014-12-31 23:59:00", flare::format_time(ymdhms, t, utc));
        t = flare::format_date_time(2015, 1, 1, -1, 0, 0, utc);
        EXPECT_EQ("2014-12-31 23:00:00", flare::format_time(ymdhms, t, utc));
        t = flare::format_date_time(2015, 1, -1, 0, 0, 0, utc);
        EXPECT_EQ("2014-12-30 00:00:00", flare::format_time(ymdhms, t, utc));
        t = flare::format_date_time(2015, -1, 1, 0, 0, 0, utc);
        EXPECT_EQ("2014-11-01 00:00:00", flare::format_time(ymdhms, t, utc));
        t = flare::format_date_time(2015, -1, -1, -1, -1, -1, utc);
        EXPECT_EQ("2014-10-29 22:58:59", flare::format_time(ymdhms, t, utc));
    }

    TEST(time_point, NextTransitionUTC) {
        const auto tz = flare::utc_time_zone();
        flare::time_zone::chrono_transition trans;

        auto t = flare::time_point::infinite_past();
        EXPECT_FALSE(tz.next_transition(t, &trans));

        t = flare::time_point::infinite_future();
        EXPECT_FALSE(tz.next_transition(t, &trans));
    }

    TEST(time_point, PrevTransitionUTC) {
        const auto tz = flare::utc_time_zone();
        flare::time_zone::chrono_transition trans;

        auto t = flare::time_point::infinite_future();
        EXPECT_FALSE(tz.prev_transition(t, &trans));

        t = flare::time_point::infinite_past();
        EXPECT_FALSE(tz.prev_transition(t, &trans));
    }

    TEST(time_point, NextTransitionNYC) {
        const auto tz = flare::times_internal::load_time_zone("America/New_York");
        flare::time_zone::chrono_transition trans;

        auto t = flare::from_chrono(flare::chrono_second(2018, 6, 30, 0, 0, 0), tz);
        EXPECT_TRUE(tz.next_transition(t, &trans));
        EXPECT_EQ(flare::chrono_second(2018, 11, 4, 2, 0, 0), trans.from);
        EXPECT_EQ(flare::chrono_second(2018, 11, 4, 1, 0, 0), trans.to);

        t = flare::time_point::infinite_future();
        EXPECT_FALSE(tz.next_transition(t, &trans));

        t = flare::time_point::infinite_past();
        EXPECT_TRUE(tz.next_transition(t, &trans));
        if (trans.from == flare::chrono_second(1918, 03, 31, 2, 0, 0)) {
            // It looks like the tzdata is only 32 bit (probably macOS),
            // which bottoms out at 1901-12-13T20:45:52+00:00.
            EXPECT_EQ(flare::chrono_second(1918, 3, 31, 3, 0, 0), trans.to);
        } else {
            EXPECT_EQ(flare::chrono_second(1883, 11, 18, 12, 3, 58), trans.from);
            EXPECT_EQ(flare::chrono_second(1883, 11, 18, 12, 0, 0), trans.to);
        }
    }

    TEST(time_point, PrevTransitionNYC) {
        const auto tz = flare::times_internal::load_time_zone("America/New_York");
        flare::time_zone::chrono_transition trans;

        auto t = flare::from_chrono(flare::chrono_second(2018, 6, 30, 0, 0, 0), tz);
        EXPECT_TRUE(tz.prev_transition(t, &trans));
        EXPECT_EQ(flare::chrono_second(2018, 3, 11, 2, 0, 0), trans.from);
        EXPECT_EQ(flare::chrono_second(2018, 3, 11, 3, 0, 0), trans.to);

        t = flare::time_point::infinite_past();
        EXPECT_FALSE(tz.prev_transition(t, &trans));

        t = flare::time_point::infinite_future();
        EXPECT_TRUE(tz.prev_transition(t, &trans));
        // We have a transition but we don't know which one.
    }

}  // namespace
