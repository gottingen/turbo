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

#if defined(_MSC_VER)
#include <winsock2.h>  // for timeval
#endif

#include <array>
#include <cfloat>
#include <chrono>  // NOLINT(build/c++11)
#include <cmath>
#include <cstdint>
#include <ctime>
#include <iomanip>
#include <limits>
#include <random>
#include <string>

#include <gmock/gmock.h>
#include <gtest/gtest.h>
#include <turbo/strings/str_format.h>
#include <turbo/times/time.h>

namespace {

    constexpr int64_t kint64max = std::numeric_limits<int64_t>::max();
    constexpr int64_t kint64min = std::numeric_limits<int64_t>::min();

    // Approximates the given number of years. This is only used to make some test
    // code more readable.
    turbo::Duration ApproxYears(int64_t n) { return turbo::Duration::hours(n) * 365 * 24; }

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

    TEST(Duration, ConstExpr) {
        constexpr turbo::Duration d0 = turbo::Duration::zero();
        static_assert(d0 == turbo::Duration::zero(), "Duration::zero()");
        constexpr turbo::Duration d1 = turbo::Duration::seconds(1);
        static_assert(d1 == turbo::Duration::seconds(1), "Duration::seconds(1)");
        static_assert(d1 != turbo::Duration::zero(), "Duration::seconds(1)");
        constexpr turbo::Duration d2 = turbo::Duration::max_infinite();
        static_assert(d2 == turbo::Duration::max_infinite(), "Duration::max_infinite()");
        static_assert(d2 != turbo::Duration::zero(), "Duration::max_infinite()");
    }

    TEST(Duration, ValueSemantics) {
        // If this compiles, the test passes.
        constexpr turbo::Duration a;      // Default construction
        constexpr turbo::Duration b = a;  // Copy construction
        constexpr turbo::Duration c(b);   // Copy construction (again)

        turbo::Duration d;
        d = c;  // Assignment
    }

    TEST(Duration, Factories) {
        constexpr turbo::Duration zero = turbo::Duration::zero();
        constexpr turbo::Duration nano = turbo::Duration::nanoseconds(1);
        constexpr turbo::Duration micro = turbo::Duration::microseconds(1);
        constexpr turbo::Duration milli = turbo::Duration::milliseconds(1);
        constexpr turbo::Duration sec = turbo::Duration::seconds(1);
        constexpr turbo::Duration min = turbo::Duration::minutes(1);
        constexpr turbo::Duration hour = turbo::Duration::hours(1);

        EXPECT_EQ(zero, turbo::Duration());
        EXPECT_EQ(zero, turbo::Duration::seconds(0));
        EXPECT_EQ(nano, turbo::Duration::nanoseconds(1));
        EXPECT_EQ(micro, turbo::Duration::nanoseconds(1000));
        EXPECT_EQ(milli, turbo::Duration::microseconds(1000));
        EXPECT_EQ(sec, turbo::Duration::milliseconds(1000));
        EXPECT_EQ(min, turbo::Duration::seconds(60));
        EXPECT_EQ(hour, turbo::Duration::minutes(60));

        // Tests factory limits
        const turbo::Duration inf = turbo::Duration::max_infinite();

        EXPECT_GT(inf, turbo::Duration::seconds(kint64max));
        EXPECT_LT(-inf, turbo::Duration::seconds(kint64min));
        EXPECT_LT(-inf, turbo::Duration::seconds(-kint64max));

        EXPECT_EQ(inf, turbo::Duration::minutes(kint64max));
        EXPECT_EQ(-inf, turbo::Duration::minutes(kint64min));
        EXPECT_EQ(-inf, turbo::Duration::minutes(-kint64max));
        EXPECT_GT(inf, turbo::Duration::minutes(kint64max / 60));
        EXPECT_LT(-inf, turbo::Duration::minutes(kint64min / 60));
        EXPECT_LT(-inf, turbo::Duration::minutes(-kint64max / 60));

        EXPECT_EQ(inf, turbo::Duration::hours(kint64max));
        EXPECT_EQ(-inf, turbo::Duration::hours(kint64min));
        EXPECT_EQ(-inf, turbo::Duration::hours(-kint64max));
        EXPECT_GT(inf, turbo::Duration::hours(kint64max / 3600));
        EXPECT_LT(-inf, turbo::Duration::hours(kint64min / 3600));
        EXPECT_LT(-inf, turbo::Duration::hours(-kint64max / 3600));
    }

    TEST(Duration, ToConversion) {
#define TEST_DURATION_CONVERSION(UNIT, UT)                                  \
  do {                                                                  \
    const turbo::Duration d = turbo::UT(1.5);                           \
    constexpr turbo::Duration z = turbo::Duration::zero();                  \
    constexpr turbo::Duration inf = turbo::Duration::max_infinite();            \
    constexpr double dbl_inf = std::numeric_limits<double>::infinity(); \
    EXPECT_EQ(kint64min, turbo::Duration::to_##UNIT(-inf));                    \
    EXPECT_EQ(-1, turbo::Duration::to_##UNIT(-d));                             \
    EXPECT_EQ(0, turbo::Duration::to_##UNIT(z));                               \
    EXPECT_EQ(1, turbo::Duration::to_##UNIT(d));                               \
    EXPECT_EQ(kint64max, turbo::Duration::to_##UNIT(inf));                     \
    EXPECT_EQ(-dbl_inf, turbo::Duration::to_double_##UNIT(-inf));                    \
    EXPECT_EQ(-1.5, turbo::Duration::to_double_##UNIT(-d));                          \
    EXPECT_EQ(0, turbo::Duration::to_double_##UNIT(z));                              \
    EXPECT_EQ(1.5, turbo::Duration::to_double_##UNIT(d));                            \
    EXPECT_EQ(dbl_inf, turbo::Duration::to_double_##UNIT(inf));                      \
  } while (0)

        TEST_DURATION_CONVERSION(nanoseconds, Duration::nanoseconds);
        TEST_DURATION_CONVERSION(microseconds,Duration::microseconds);
        TEST_DURATION_CONVERSION(milliseconds, Duration::milliseconds);
        TEST_DURATION_CONVERSION(seconds,Duration::seconds);
        TEST_DURATION_CONVERSION(minutes, Duration::minutes);
        TEST_DURATION_CONVERSION(hours, Duration::hours);

#undef TEST_DURATION_CONVERSION
    }

    template<int64_t N>
    void TestToConversion() {
        constexpr turbo::Duration nano = turbo::Duration::nanoseconds(N);
        EXPECT_EQ(N, turbo::Duration::to_nanoseconds(nano));
        EXPECT_EQ(0, turbo::Duration::to_microseconds(nano));
        EXPECT_EQ(0, turbo::Duration::to_milliseconds(nano));
        EXPECT_EQ(0, turbo::Duration::to_seconds(nano));
        EXPECT_EQ(0, turbo::Duration::to_minutes(nano));
        EXPECT_EQ(0, turbo::Duration::to_hours(nano));
        const turbo::Duration micro = turbo::Duration::microseconds(N);
        EXPECT_EQ(N * 1000, turbo::Duration::to_nanoseconds(micro));
        EXPECT_EQ(N, turbo::Duration::to_microseconds(micro));
        EXPECT_EQ(0, turbo::Duration::to_milliseconds(micro));
        EXPECT_EQ(0, turbo::Duration::to_seconds(micro));
        EXPECT_EQ(0, turbo::Duration::to_minutes(micro));
        EXPECT_EQ(0, turbo::Duration::to_hours(micro));
        const turbo::Duration milli = turbo::Duration::milliseconds(N);
        EXPECT_EQ(N * 1000 * 1000, turbo::Duration::to_nanoseconds(milli));
        EXPECT_EQ(N * 1000, turbo::Duration::to_microseconds(milli));
        EXPECT_EQ(N, turbo::Duration::to_milliseconds(milli));
        EXPECT_EQ(0, turbo::Duration::to_seconds(milli));
        EXPECT_EQ(0, turbo::Duration::to_minutes(milli));
        EXPECT_EQ(0, turbo::Duration::to_hours(milli));
        const turbo::Duration sec = turbo::Duration::seconds(N);
        EXPECT_EQ(N * 1000 * 1000 * 1000, turbo::Duration::to_nanoseconds(sec));
        EXPECT_EQ(N * 1000 * 1000, turbo::Duration::to_microseconds(sec));
        EXPECT_EQ(N * 1000, turbo::Duration::to_milliseconds(sec));
        EXPECT_EQ(N, turbo::Duration::to_seconds(sec));
        EXPECT_EQ(0, turbo::Duration::to_minutes(sec));
        EXPECT_EQ(0, turbo::Duration::to_hours(sec));
        const turbo::Duration min = turbo::Duration::minutes(N);
        EXPECT_EQ(N * 60 * 1000 * 1000 * 1000, turbo::Duration::to_nanoseconds(min));
        EXPECT_EQ(N * 60 * 1000 * 1000, turbo::Duration::to_microseconds(min));
        EXPECT_EQ(N * 60 * 1000, turbo::Duration::to_milliseconds(min));
        EXPECT_EQ(N * 60, turbo::Duration::to_seconds(min));
        EXPECT_EQ(N, turbo::Duration::to_minutes(min));
        EXPECT_EQ(0, turbo::Duration::to_hours(min));
        const turbo::Duration hour = turbo::Duration::hours(N);
        EXPECT_EQ(N * 60 * 60 * 1000 * 1000 * 1000, turbo::Duration::to_nanoseconds(hour));
        EXPECT_EQ(N * 60 * 60 * 1000 * 1000, turbo::Duration::to_microseconds(hour));
        EXPECT_EQ(N * 60 * 60 * 1000, turbo::Duration::to_milliseconds(hour));
        EXPECT_EQ(N * 60 * 60, turbo::Duration::to_seconds(hour));
        EXPECT_EQ(N * 60, turbo::Duration::to_minutes(hour));
        EXPECT_EQ(N, turbo::Duration::to_hours(hour));
    }

    TEST(Duration, ToConversionDeprecated) {
        TestToConversion<43>();
        TestToConversion<1>();
        TestToConversion<0>();
        TestToConversion<-1>();
        TestToConversion<-43>();
    }

    template<int64_t N>
    void TestFromChronoBasicEquality() {
        using std::chrono::nanoseconds;
        using std::chrono::microseconds;
        using std::chrono::milliseconds;
        using std::chrono::seconds;
        using std::chrono::minutes;
        using std::chrono::hours;

        static_assert(turbo::Duration::nanoseconds(N) == turbo::Duration::from_chrono(nanoseconds(N)), "");
        static_assert(turbo::Duration::microseconds(N) == turbo::Duration::from_chrono(microseconds(N)), "");
        static_assert(turbo::Duration::milliseconds(N) == turbo::Duration::from_chrono(milliseconds(N)), "");
        static_assert(turbo::Duration::seconds(N) == turbo::Duration::from_chrono(seconds(N)), "");
        static_assert(turbo::Duration::minutes(N) == turbo::Duration::from_chrono(minutes(N)), "");
        static_assert(turbo::Duration::hours(N) == turbo::Duration::from_chrono(hours(N)), "");
    }

    TEST(Duration, FromChrono) {
        TestFromChronoBasicEquality<-123>();
        TestFromChronoBasicEquality<-1>();
        TestFromChronoBasicEquality<0>();
        TestFromChronoBasicEquality<1>();
        TestFromChronoBasicEquality<123>();

        // Minutes (might, depending on the platform) saturate at +inf.
        const auto chrono_minutes_max = std::chrono::minutes::max();
        const auto minutes_max = turbo::Duration::from_chrono(chrono_minutes_max);
        const int64_t minutes_max_count = chrono_minutes_max.count();
        if (minutes_max_count > kint64max / 60) {
            EXPECT_EQ(turbo::Duration::max_infinite(), minutes_max);
        } else {
            EXPECT_EQ(turbo::Duration::minutes(minutes_max_count), minutes_max);
        }

        // Minutes (might, depending on the platform) saturate at -inf.
        const auto chrono_minutes_min = std::chrono::minutes::min();
        const auto minutes_min = turbo::Duration::from_chrono(chrono_minutes_min);
        const int64_t minutes_min_count = chrono_minutes_min.count();
        if (minutes_min_count < kint64min / 60) {
            EXPECT_EQ(-turbo::Duration::max_infinite(), minutes_min);
        } else {
            EXPECT_EQ(turbo::Duration::minutes(minutes_min_count), minutes_min);
        }

        // Hours (might, depending on the platform) saturate at +inf.
        const auto chrono_hours_max = std::chrono::hours::max();
        const auto hours_max = turbo::Duration::from_chrono(chrono_hours_max);
        const int64_t hours_max_count = chrono_hours_max.count();
        if (hours_max_count > kint64max / 3600) {
            EXPECT_EQ(turbo::Duration::max_infinite(), hours_max);
        } else {
            EXPECT_EQ(turbo::Duration::hours(hours_max_count), hours_max);
        }

        // Hours (might, depending on the platform) saturate at -inf.
        const auto chrono_hours_min = std::chrono::hours::min();
        const auto hours_min = turbo::Duration::from_chrono(chrono_hours_min);
        const int64_t hours_min_count = chrono_hours_min.count();
        if (hours_min_count < kint64min / 3600) {
            EXPECT_EQ(-turbo::Duration::max_infinite(), hours_min);
        } else {
            EXPECT_EQ(turbo::Duration::hours(hours_min_count), hours_min);
        }
    }

    template<int64_t N>
    void TestToChrono() {
        using std::chrono::nanoseconds;
        using std::chrono::microseconds;
        using std::chrono::milliseconds;
        using std::chrono::seconds;
        using std::chrono::minutes;
        using std::chrono::hours;

        EXPECT_EQ(nanoseconds(N), turbo::Duration::to_chrono_nanoseconds(turbo::Duration::nanoseconds(N)));
        EXPECT_EQ(microseconds(N), turbo::Duration::to_chrono_microseconds(turbo::Duration::microseconds(N)));
        EXPECT_EQ(milliseconds(N), turbo::Duration::to_chrono_milliseconds(turbo::Duration::milliseconds(N)));
        EXPECT_EQ(seconds(N), turbo::Duration::to_chrono_seconds(turbo::Duration::seconds(N)));

        constexpr auto turbo_minutes = turbo::Duration::minutes(N);
        auto chrono_minutes = minutes(N);
        if (turbo_minutes == -turbo::Duration::max_infinite()) {
            chrono_minutes = minutes::min();
        } else if (turbo_minutes == turbo::Duration::max_infinite()) {
            chrono_minutes = minutes::max();
        }
        EXPECT_EQ(chrono_minutes, turbo::Duration::to_chrono_minutes(turbo_minutes));

        constexpr auto turbo_hours = turbo::Duration::hours(N);
        auto chrono_hours = hours(N);
        if (turbo_hours == -turbo::Duration::max_infinite()) {
            chrono_hours = hours::min();
        } else if (turbo_hours == turbo::Duration::max_infinite()) {
            chrono_hours = hours::max();
        }
        EXPECT_EQ(chrono_hours, turbo::Duration::to_chrono_hours(turbo_hours));
    }

    TEST(Duration, ToChrono) {
        using std::chrono::nanoseconds;
        using std::chrono::microseconds;
        using std::chrono::milliseconds;
        using std::chrono::seconds;
        using std::chrono::minutes;
        using std::chrono::hours;

        TestToChrono<kint64min>();
        TestToChrono<-1>();
        TestToChrono<0>();
        TestToChrono<1>();
        TestToChrono<kint64max>();

        // Verify truncation toward zero.
        const auto tick = turbo::Duration::nanoseconds(1) / 4;
        EXPECT_EQ(nanoseconds(0), turbo::Duration::to_chrono_nanoseconds(tick));
        EXPECT_EQ(nanoseconds(0), turbo::Duration::to_chrono_nanoseconds(-tick));
        EXPECT_EQ(microseconds(0), turbo::Duration::to_chrono_microseconds(tick));
        EXPECT_EQ(microseconds(0), turbo::Duration::to_chrono_microseconds(-tick));
        EXPECT_EQ(milliseconds(0), turbo::Duration::to_chrono_milliseconds(tick));
        EXPECT_EQ(milliseconds(0), turbo::Duration::to_chrono_milliseconds(-tick));
        EXPECT_EQ(seconds(0), turbo::Duration::to_chrono_seconds(tick));
        EXPECT_EQ(seconds(0), turbo::Duration::to_chrono_seconds(-tick));
        EXPECT_EQ(minutes(0), turbo::Duration::to_chrono_minutes(tick));
        EXPECT_EQ(minutes(0), turbo::Duration::to_chrono_minutes(-tick));
        EXPECT_EQ(hours(0), turbo::Duration::to_chrono_hours(tick));
        EXPECT_EQ(hours(0), turbo::Duration::to_chrono_hours(-tick));

        // Verifies +/- infinity saturation at max/min.
        constexpr auto inf = turbo::Duration::max_infinite();
        EXPECT_EQ(nanoseconds::min(), turbo::Duration::to_chrono_nanoseconds(-inf));
        EXPECT_EQ(nanoseconds::max(), turbo::Duration::to_chrono_nanoseconds(inf));
        EXPECT_EQ(microseconds::min(), turbo::Duration::to_chrono_microseconds(-inf));
        EXPECT_EQ(microseconds::max(), turbo::Duration::to_chrono_microseconds(inf));
        EXPECT_EQ(milliseconds::min(), turbo::Duration::to_chrono_milliseconds(-inf));
        EXPECT_EQ(milliseconds::max(), turbo::Duration::to_chrono_milliseconds(inf));
        EXPECT_EQ(seconds::min(), turbo::Duration::to_chrono_seconds(-inf));
        EXPECT_EQ(seconds::max(), turbo::Duration::to_chrono_seconds(inf));
        EXPECT_EQ(minutes::min(), turbo::Duration::to_chrono_minutes(-inf));
        EXPECT_EQ(minutes::max(), turbo::Duration::to_chrono_minutes(inf));
        EXPECT_EQ(hours::min(), turbo::Duration::to_chrono_hours(-inf));
        EXPECT_EQ(hours::max(), turbo::Duration::to_chrono_hours(inf));
    }

    TEST(Duration, FactoryOverloads) {
        enum E {
            kOne = 1
        };
#define TEST_FACTORY_OVERLOADS(NAME)                                          \
  EXPECT_EQ(1, NAME(kOne) / NAME(kOne));                                      \
  EXPECT_EQ(1, NAME(static_cast<int8_t>(1)) / NAME(1));                       \
  EXPECT_EQ(1, NAME(static_cast<int16_t>(1)) / NAME(1));                      \
  EXPECT_EQ(1, NAME(static_cast<int32_t>(1)) / NAME(1));                      \
  EXPECT_EQ(1, NAME(static_cast<int64_t>(1)) / NAME(1));                      \
  EXPECT_EQ(1, NAME(static_cast<uint8_t>(1)) / NAME(1));                      \
  EXPECT_EQ(1, NAME(static_cast<uint16_t>(1)) / NAME(1));                     \
  EXPECT_EQ(1, NAME(static_cast<uint32_t>(1)) / NAME(1));                     \
  EXPECT_EQ(1, NAME(static_cast<uint64_t>(1)) / NAME(1));                     \
  EXPECT_EQ(NAME(1) / 2, NAME(static_cast<float>(0.5)));                      \
  EXPECT_EQ(NAME(1) / 2, NAME(static_cast<double>(0.5)));                     \
  EXPECT_EQ(1.5, turbo::Duration::fdiv(NAME(static_cast<float>(1.5)), NAME(1))); \
  EXPECT_EQ(1.5, turbo::Duration::fdiv(NAME(static_cast<double>(1.5)), NAME(1)));

        TEST_FACTORY_OVERLOADS(turbo::Duration::nanoseconds);
        TEST_FACTORY_OVERLOADS(turbo::Duration::microseconds);
        TEST_FACTORY_OVERLOADS(turbo::Duration::milliseconds);
        TEST_FACTORY_OVERLOADS(turbo::Duration::seconds);
        TEST_FACTORY_OVERLOADS(turbo::Duration::minutes);
        TEST_FACTORY_OVERLOADS(turbo::Duration::hours);

#undef TEST_FACTORY_OVERLOADS

        EXPECT_EQ(turbo::Duration::milliseconds(1500), turbo::Duration::seconds(1.5));
        EXPECT_LT(turbo::Duration::nanoseconds(1), turbo::Duration::nanoseconds(1.5));
        EXPECT_GT(turbo::Duration::nanoseconds(2), turbo::Duration::nanoseconds(1.5));

        const double dbl_inf = std::numeric_limits<double>::infinity();
        EXPECT_EQ(turbo::Duration::max_infinite(), turbo::Duration::nanoseconds(dbl_inf));
        EXPECT_EQ(turbo::Duration::max_infinite(), turbo::Duration::microseconds(dbl_inf));
        EXPECT_EQ(turbo::Duration::max_infinite(), turbo::Duration::milliseconds(dbl_inf));
        EXPECT_EQ(turbo::Duration::max_infinite(), turbo::Duration::seconds(dbl_inf));
        EXPECT_EQ(turbo::Duration::max_infinite(), turbo::Duration::minutes(dbl_inf));
        EXPECT_EQ(turbo::Duration::max_infinite(), turbo::Duration::hours(dbl_inf));
        EXPECT_EQ(-turbo::Duration::max_infinite(), turbo::Duration::nanoseconds(-dbl_inf));
        EXPECT_EQ(-turbo::Duration::max_infinite(), turbo::Duration::microseconds(-dbl_inf));
        EXPECT_EQ(-turbo::Duration::max_infinite(), turbo::Duration::milliseconds(-dbl_inf));
        EXPECT_EQ(-turbo::Duration::max_infinite(), turbo::Duration::seconds(-dbl_inf));
        EXPECT_EQ(-turbo::Duration::max_infinite(), turbo::Duration::minutes(-dbl_inf));
        EXPECT_EQ(-turbo::Duration::max_infinite(), turbo::Duration::hours(-dbl_inf));
    }

    TEST(Duration, InfinityExamples) {
        // These examples are used in the documentation in time.h. They are
        // written so that they can be copy-n-pasted easily.

        constexpr turbo::Duration inf = turbo::Duration::max_infinite();
        constexpr turbo::Duration d = turbo::Duration::seconds(1);  // Any finite duration

        EXPECT_TRUE(inf == inf + inf);
        EXPECT_TRUE(inf == inf + d);
        EXPECT_TRUE(inf == inf - inf);
        EXPECT_TRUE(-inf == d - inf);

        EXPECT_TRUE(inf == d * 1e100);
        EXPECT_TRUE(0 == d / inf);  // NOLINT(readability/check)

        // Division by zero returns infinity, or kint64min/MAX where necessary.
        EXPECT_TRUE(inf == d / 0);
        EXPECT_TRUE(kint64max == d / turbo::Duration::zero());
    }

    TEST(Duration, InfinityComparison) {
        const turbo::Duration inf = turbo::Duration::max_infinite();
        const turbo::Duration any_dur = turbo::Duration::seconds(1);

        // Equality
        EXPECT_EQ(inf, inf);
        EXPECT_EQ(-inf, -inf);
        EXPECT_NE(inf, -inf);
        EXPECT_NE(any_dur, inf);
        EXPECT_NE(any_dur, -inf);

        // Relational
        EXPECT_GT(inf, any_dur);
        EXPECT_LT(-inf, any_dur);
        EXPECT_LT(-inf, inf);
        EXPECT_GT(inf, -inf);
    }

    TEST(Duration, InfinityAddition) {
        const turbo::Duration sec_max = turbo::Duration::seconds(kint64max);
        const turbo::Duration sec_min = turbo::Duration::seconds(kint64min);
        const turbo::Duration any_dur = turbo::Duration::seconds(1);
        const turbo::Duration inf = turbo::Duration::max_infinite();

        // Addition
        EXPECT_EQ(inf, inf + inf);
        EXPECT_EQ(inf, inf + -inf);
        EXPECT_EQ(-inf, -inf + inf);
        EXPECT_EQ(-inf, -inf + -inf);

        EXPECT_EQ(inf, inf + any_dur);
        EXPECT_EQ(inf, any_dur + inf);
        EXPECT_EQ(-inf, -inf + any_dur);
        EXPECT_EQ(-inf, any_dur + -inf);

        // Interesting case
        turbo::Duration almost_inf = sec_max + turbo::Duration::nanoseconds(999999999);
        EXPECT_GT(inf, almost_inf);
        almost_inf += -turbo::Duration::nanoseconds(999999999);
        EXPECT_GT(inf, almost_inf);

        // Addition overflow/underflow
        EXPECT_EQ(inf, sec_max + turbo::Duration::seconds(1));
        EXPECT_EQ(inf, sec_max + sec_max);
        EXPECT_EQ(-inf, sec_min + -turbo::Duration::seconds(1));
        EXPECT_EQ(-inf, sec_min + -sec_max);

        // For reference: IEEE 754 behavior
        const double dbl_inf = std::numeric_limits<double>::infinity();
        EXPECT_TRUE(std::isinf(dbl_inf + dbl_inf));
        EXPECT_TRUE(std::isnan(dbl_inf + -dbl_inf));  // We return inf
        EXPECT_TRUE(std::isnan(-dbl_inf + dbl_inf));  // We return inf
        EXPECT_TRUE(std::isinf(-dbl_inf + -dbl_inf));
    }

    TEST(Duration, InfinitySubtraction) {
        const turbo::Duration sec_max = turbo::Duration::seconds(kint64max);
        const turbo::Duration sec_min = turbo::Duration::seconds(kint64min);
        const turbo::Duration any_dur = turbo::Duration::seconds(1);
        const turbo::Duration inf = turbo::Duration::max_infinite();

        // Subtraction
        EXPECT_EQ(inf, inf - inf);
        EXPECT_EQ(inf, inf - -inf);
        EXPECT_EQ(-inf, -inf - inf);
        EXPECT_EQ(-inf, -inf - -inf);

        EXPECT_EQ(inf, inf - any_dur);
        EXPECT_EQ(-inf, any_dur - inf);
        EXPECT_EQ(-inf, -inf - any_dur);
        EXPECT_EQ(inf, any_dur - -inf);

        // Subtraction overflow/underflow
        EXPECT_EQ(inf, sec_max - -turbo::Duration::seconds(1));
        EXPECT_EQ(inf, sec_max - -sec_max);
        EXPECT_EQ(-inf, sec_min - turbo::Duration::seconds(1));
        EXPECT_EQ(-inf, sec_min - sec_max);

        // Interesting case
        turbo::Duration almost_neg_inf = sec_min;
        EXPECT_LT(-inf, almost_neg_inf);
        almost_neg_inf -= -turbo::Duration::nanoseconds(1);
        EXPECT_LT(-inf, almost_neg_inf);

        // For reference: IEEE 754 behavior
        const double dbl_inf = std::numeric_limits<double>::infinity();
        EXPECT_TRUE(std::isnan(dbl_inf - dbl_inf));  // We return inf
        EXPECT_TRUE(std::isinf(dbl_inf - -dbl_inf));
        EXPECT_TRUE(std::isinf(-dbl_inf - dbl_inf));
        EXPECT_TRUE(std::isnan(-dbl_inf - -dbl_inf));  // We return inf
    }

    TEST(Duration, InfinityMultiplication) {
        const turbo::Duration sec_max = turbo::Duration::seconds(kint64max);
        const turbo::Duration sec_min = turbo::Duration::seconds(kint64min);
        const turbo::Duration inf = turbo::Duration::max_infinite();

#define TEST_INF_MUL_WITH_TYPE(T)                                     \
  EXPECT_EQ(inf, inf * static_cast<T>(2));                            \
  EXPECT_EQ(-inf, inf * static_cast<T>(-2));                          \
  EXPECT_EQ(-inf, -inf * static_cast<T>(2));                          \
  EXPECT_EQ(inf, -inf * static_cast<T>(-2));                          \
  EXPECT_EQ(inf, inf * static_cast<T>(0));                            \
  EXPECT_EQ(-inf, -inf * static_cast<T>(0));                          \
  EXPECT_EQ(inf, sec_max * static_cast<T>(2));                        \
  EXPECT_EQ(inf, sec_min * static_cast<T>(-2));                       \
  EXPECT_EQ(inf, (sec_max / static_cast<T>(2)) * static_cast<T>(3));  \
  EXPECT_EQ(-inf, sec_max * static_cast<T>(-2));                      \
  EXPECT_EQ(-inf, sec_min * static_cast<T>(2));                       \
  EXPECT_EQ(-inf, (sec_min / static_cast<T>(2)) * static_cast<T>(3));

        TEST_INF_MUL_WITH_TYPE(int64_t);  // NOLINT(readability/function)
        TEST_INF_MUL_WITH_TYPE(double);   // NOLINT(readability/function)

#undef TEST_INF_MUL_WITH_TYPE

        const double dbl_inf = std::numeric_limits<double>::infinity();
        EXPECT_EQ(inf, inf * dbl_inf);
        EXPECT_EQ(-inf, -inf * dbl_inf);
        EXPECT_EQ(-inf, inf * -dbl_inf);
        EXPECT_EQ(inf, -inf * -dbl_inf);

        const turbo::Duration any_dur = turbo::Duration::seconds(1);
        EXPECT_EQ(inf, any_dur * dbl_inf);
        EXPECT_EQ(-inf, -any_dur * dbl_inf);
        EXPECT_EQ(-inf, any_dur * -dbl_inf);
        EXPECT_EQ(inf, -any_dur * -dbl_inf);

        // Fixed-point multiplication will produce a finite value, whereas floating
        // point fuzziness will overflow to inf.
        EXPECT_NE(turbo::Duration::max_infinite(), turbo::Duration::seconds(1) * kint64max);
        EXPECT_EQ(inf, turbo::Duration::seconds(1) * static_cast<double>(kint64max));
        EXPECT_NE(-turbo::Duration::max_infinite(), turbo::Duration::seconds(1) * kint64min);
        EXPECT_EQ(-inf, turbo::Duration::seconds(1) * static_cast<double>(kint64min));

        // Note that sec_max * or / by 1.0 overflows to inf due to the 53-bit
        // limitations of double.
        EXPECT_NE(inf, sec_max);
        EXPECT_NE(inf, sec_max / 1);
        EXPECT_EQ(inf, sec_max / 1.0);
        EXPECT_NE(inf, sec_max * 1);
        EXPECT_EQ(inf, sec_max * 1.0);
    }

    TEST(Duration, InfinityDivision) {
        const turbo::Duration sec_max = turbo::Duration::seconds(kint64max);
        const turbo::Duration sec_min = turbo::Duration::seconds(kint64min);
        const turbo::Duration inf = turbo::Duration::max_infinite();

        // Division of Duration by a double
#define TEST_INF_DIV_WITH_TYPE(T)            \
  EXPECT_EQ(inf, inf / static_cast<T>(2));   \
  EXPECT_EQ(-inf, inf / static_cast<T>(-2)); \
  EXPECT_EQ(-inf, -inf / static_cast<T>(2)); \
  EXPECT_EQ(inf, -inf / static_cast<T>(-2));

        TEST_INF_DIV_WITH_TYPE(int64_t);  // NOLINT(readability/function)
        TEST_INF_DIV_WITH_TYPE(double);   // NOLINT(readability/function)

#undef TEST_INF_DIV_WITH_TYPE

        // Division of Duration by a double overflow/underflow
        EXPECT_EQ(inf, sec_max / 0.5);
        EXPECT_EQ(inf, sec_min / -0.5);
        EXPECT_EQ(inf, ((sec_max / 0.5) + turbo::Duration::seconds(1)) / 0.5);
        EXPECT_EQ(-inf, sec_max / -0.5);
        EXPECT_EQ(-inf, sec_min / 0.5);
        EXPECT_EQ(-inf, ((sec_min / 0.5) - turbo::Duration::seconds(1)) / 0.5);

        const double dbl_inf = std::numeric_limits<double>::infinity();
        EXPECT_EQ(inf, inf / dbl_inf);
        EXPECT_EQ(-inf, inf / -dbl_inf);
        EXPECT_EQ(-inf, -inf / dbl_inf);
        EXPECT_EQ(inf, -inf / -dbl_inf);

        const turbo::Duration any_dur = turbo::Duration::seconds(1);
        EXPECT_EQ(turbo::Duration::zero(), any_dur / dbl_inf);
        EXPECT_EQ(turbo::Duration::zero(), any_dur / -dbl_inf);
        EXPECT_EQ(turbo::Duration::zero(), -any_dur / dbl_inf);
        EXPECT_EQ(turbo::Duration::zero(), -any_dur / -dbl_inf);
    }

    TEST(Duration, InfinityModulus) {
        const turbo::Duration sec_max = turbo::Duration::seconds(kint64max);
        const turbo::Duration any_dur = turbo::Duration::seconds(1);
        const turbo::Duration inf = turbo::Duration::max_infinite();

        EXPECT_EQ(inf, inf % inf);
        EXPECT_EQ(inf, inf % -inf);
        EXPECT_EQ(-inf, -inf % -inf);
        EXPECT_EQ(-inf, -inf % inf);

        EXPECT_EQ(any_dur, any_dur % inf);
        EXPECT_EQ(any_dur, any_dur % -inf);
        EXPECT_EQ(-any_dur, -any_dur % inf);
        EXPECT_EQ(-any_dur, -any_dur % -inf);

        EXPECT_EQ(inf, inf % -any_dur);
        EXPECT_EQ(inf, inf % any_dur);
        EXPECT_EQ(-inf, -inf % -any_dur);
        EXPECT_EQ(-inf, -inf % any_dur);

        // Remainder isn't affected by overflow.
        EXPECT_EQ(turbo::Duration::zero(), sec_max % turbo::Duration::seconds(1));
        EXPECT_EQ(turbo::Duration::zero(), sec_max % turbo::Duration::milliseconds(1));
        EXPECT_EQ(turbo::Duration::zero(), sec_max % turbo::Duration::microseconds(1));
        EXPECT_EQ(turbo::Duration::zero(), sec_max % turbo::Duration::nanoseconds(1));
        EXPECT_EQ(turbo::Duration::zero(), sec_max % turbo::Duration::nanoseconds(1) / 4);
    }

    TEST(Duration, InfinityIDiv) {
        const turbo::Duration sec_max = turbo::Duration::seconds(kint64max);
        const turbo::Duration any_dur = turbo::Duration::seconds(1);
        const turbo::Duration inf = turbo::Duration::max_infinite();
        const double dbl_inf = std::numeric_limits<double>::infinity();

        // Duration::idiv (int64_t return value + a remainer)
        turbo::Duration rem = turbo::Duration::zero();
        EXPECT_EQ(kint64max, turbo::Duration::idiv(inf, inf, &rem));
        EXPECT_EQ(inf, rem);

        rem = turbo::Duration::zero();
        EXPECT_EQ(kint64max, turbo::Duration::idiv(-inf, -inf, &rem));
        EXPECT_EQ(-inf, rem);

        rem = turbo::Duration::zero();
        EXPECT_EQ(kint64max, turbo::Duration::idiv(inf, any_dur, &rem));
        EXPECT_EQ(inf, rem);

        rem = turbo::Duration::zero();
        EXPECT_EQ(0, turbo::Duration::idiv(any_dur, inf, &rem));
        EXPECT_EQ(any_dur, rem);

        rem = turbo::Duration::zero();
        EXPECT_EQ(kint64max, turbo::Duration::idiv(-inf, -any_dur, &rem));
        EXPECT_EQ(-inf, rem);

        rem = turbo::Duration::zero();
        EXPECT_EQ(0, turbo::Duration::idiv(-any_dur, -inf, &rem));
        EXPECT_EQ(-any_dur, rem);

        rem = turbo::Duration::zero();
        EXPECT_EQ(kint64min, turbo::Duration::idiv(-inf, inf, &rem));
        EXPECT_EQ(-inf, rem);

        rem = turbo::Duration::zero();
        EXPECT_EQ(kint64min, turbo::Duration::idiv(inf, -inf, &rem));
        EXPECT_EQ(inf, rem);

        rem = turbo::Duration::zero();
        EXPECT_EQ(kint64min, turbo::Duration::idiv(-inf, any_dur, &rem));
        EXPECT_EQ(-inf, rem);

        rem = turbo::Duration::zero();
        EXPECT_EQ(0, turbo::Duration::idiv(-any_dur, inf, &rem));
        EXPECT_EQ(-any_dur, rem);

        rem = turbo::Duration::zero();
        EXPECT_EQ(kint64min, turbo::Duration::idiv(inf, -any_dur, &rem));
        EXPECT_EQ(inf, rem);

        rem = turbo::Duration::zero();
        EXPECT_EQ(0, turbo::Duration::idiv(any_dur, -inf, &rem));
        EXPECT_EQ(any_dur, rem);

        // Duration::idiv overflow/underflow
        rem = any_dur;
        EXPECT_EQ(kint64max,
                  turbo::Duration::idiv(sec_max, turbo::Duration::nanoseconds(1) / 4, &rem));
        EXPECT_EQ(sec_max - turbo::Duration::nanoseconds(kint64max) / 4, rem);

        rem = any_dur;
        EXPECT_EQ(kint64max,
                  turbo::Duration::idiv(sec_max, turbo::Duration::milliseconds(1), &rem));
        EXPECT_EQ(sec_max - turbo::Duration::milliseconds(kint64max), rem);

        rem = any_dur;
        EXPECT_EQ(kint64max,
                  turbo::Duration::idiv(-sec_max, -turbo::Duration::milliseconds(1), &rem));
        EXPECT_EQ(-sec_max + turbo::Duration::milliseconds(kint64max), rem);

        rem = any_dur;
        EXPECT_EQ(kint64min,
                  turbo::Duration::idiv(-sec_max, turbo::Duration::milliseconds(1), &rem));
        EXPECT_EQ(-sec_max - turbo::Duration::milliseconds(kint64min), rem);

        rem = any_dur;
        EXPECT_EQ(kint64min,
                  turbo::Duration::idiv(sec_max, -turbo::Duration::milliseconds(1), &rem));
        EXPECT_EQ(sec_max + turbo::Duration::milliseconds(kint64min), rem);

        //
        // operator/(Duration, Duration) is a wrapper for Duration::idiv().
        //

        // IEEE 754 says inf / inf should be nan, but int64_t doesn't have
        // nan so we'll return kint64max/kint64min instead.
        EXPECT_TRUE(std::isnan(dbl_inf / dbl_inf));
        EXPECT_EQ(kint64max, inf / inf);
        EXPECT_EQ(kint64max, -inf / -inf);
        EXPECT_EQ(kint64min, -inf / inf);
        EXPECT_EQ(kint64min, inf / -inf);

        EXPECT_TRUE(std::isinf(dbl_inf / 2.0));
        EXPECT_EQ(kint64max, inf / any_dur);
        EXPECT_EQ(kint64max, -inf / -any_dur);
        EXPECT_EQ(kint64min, -inf / any_dur);
        EXPECT_EQ(kint64min, inf / -any_dur);

        EXPECT_EQ(0.0, 2.0 / dbl_inf);
        EXPECT_EQ(0, any_dur / inf);
        EXPECT_EQ(0, any_dur / -inf);
        EXPECT_EQ(0, -any_dur / inf);
        EXPECT_EQ(0, -any_dur / -inf);
        EXPECT_EQ(0, turbo::Duration::zero() / inf);

        // Division of Duration by a Duration overflow/underflow
        EXPECT_EQ(kint64max, sec_max / turbo::Duration::milliseconds(1));
        EXPECT_EQ(kint64max, -sec_max / -turbo::Duration::milliseconds(1));
        EXPECT_EQ(kint64min, -sec_max / turbo::Duration::milliseconds(1));
        EXPECT_EQ(kint64min, sec_max / -turbo::Duration::milliseconds(1));
    }

    TEST(Duration, InfinityFDiv) {
        const turbo::Duration any_dur = turbo::Duration::seconds(1);
        const turbo::Duration inf = turbo::Duration::max_infinite();
        const double dbl_inf = std::numeric_limits<double>::infinity();

        EXPECT_EQ(dbl_inf, turbo::Duration::fdiv(inf, inf));
        EXPECT_EQ(dbl_inf, turbo::Duration::fdiv(-inf, -inf));
        EXPECT_EQ(dbl_inf, turbo::Duration::fdiv(inf, any_dur));
        EXPECT_EQ(0.0, turbo::Duration::fdiv(any_dur, inf));
        EXPECT_EQ(dbl_inf, turbo::Duration::fdiv(-inf, -any_dur));
        EXPECT_EQ(0.0, turbo::Duration::fdiv(-any_dur, -inf));

        EXPECT_EQ(-dbl_inf, turbo::Duration::fdiv(-inf, inf));
        EXPECT_EQ(-dbl_inf, turbo::Duration::fdiv(inf, -inf));
        EXPECT_EQ(-dbl_inf, turbo::Duration::fdiv(-inf, any_dur));
        EXPECT_EQ(0.0, turbo::Duration::fdiv(-any_dur, inf));
        EXPECT_EQ(-dbl_inf, turbo::Duration::fdiv(inf, -any_dur));
        EXPECT_EQ(0.0, turbo::Duration::fdiv(any_dur, -inf));
    }

    TEST(Duration, DivisionByZero) {
        const turbo::Duration zero = turbo::Duration::zero();
        const turbo::Duration inf = turbo::Duration::max_infinite();
        const turbo::Duration any_dur = turbo::Duration::seconds(1);
        const double dbl_inf = std::numeric_limits<double>::infinity();
        const double dbl_denorm = std::numeric_limits<double>::denorm_min();

        // Operator/(Duration, double)
        EXPECT_EQ(inf, zero / 0.0);
        EXPECT_EQ(-inf, zero / -0.0);
        EXPECT_EQ(inf, any_dur / 0.0);
        EXPECT_EQ(-inf, any_dur / -0.0);
        EXPECT_EQ(-inf, -any_dur / 0.0);
        EXPECT_EQ(inf, -any_dur / -0.0);

        // Tests dividing by a number very close to, but not quite zero.
        EXPECT_EQ(zero, zero / dbl_denorm);
        EXPECT_EQ(zero, zero / -dbl_denorm);
        EXPECT_EQ(inf, any_dur / dbl_denorm);
        EXPECT_EQ(-inf, any_dur / -dbl_denorm);
        EXPECT_EQ(-inf, -any_dur / dbl_denorm);
        EXPECT_EQ(inf, -any_dur / -dbl_denorm);

        // IDiv
        turbo::Duration rem = zero;
        EXPECT_EQ(kint64max, turbo::Duration::idiv(zero, zero, &rem));
        EXPECT_EQ(inf, rem);

        rem = zero;
        EXPECT_EQ(kint64max, turbo::Duration::idiv(any_dur, zero, &rem));
        EXPECT_EQ(inf, rem);

        rem = zero;
        EXPECT_EQ(kint64min, turbo::Duration::idiv(-any_dur, zero, &rem));
        EXPECT_EQ(-inf, rem);

        // Operator/(Duration, Duration)
        EXPECT_EQ(kint64max, zero / zero);
        EXPECT_EQ(kint64max, any_dur / zero);
        EXPECT_EQ(kint64min, -any_dur / zero);

        // FDiv
        EXPECT_EQ(dbl_inf, turbo::Duration::fdiv(zero, zero));
        EXPECT_EQ(dbl_inf, turbo::Duration::fdiv(any_dur, zero));
        EXPECT_EQ(-dbl_inf, turbo::Duration::fdiv(-any_dur, zero));
    }

    TEST(Duration, NaN) {
        // Note that IEEE 754 does not define the behavior of a nan's sign when it is
        // copied, so the code below allows for either + or - Duration::min_infinite.
#define TEST_NAN_HANDLING(NAME, NAN)           \
  do {                                         \
    const auto inf = turbo::Duration::max_infinite(); \
    auto x = NAME(NAN);                        \
    EXPECT_TRUE(x == inf || x == -inf);        \
    auto y = NAME(42);                         \
    y *= NAN;                                  \
    EXPECT_TRUE(y == inf || y == -inf);        \
    auto z = NAME(42);                         \
    z /= NAN;                                  \
    EXPECT_TRUE(z == inf || z == -inf);        \
  } while (0)

        const double nan = std::numeric_limits<double>::quiet_NaN();
        TEST_NAN_HANDLING(turbo::Duration::nanoseconds, nan);
        TEST_NAN_HANDLING(turbo::Duration::microseconds, nan);
        TEST_NAN_HANDLING(turbo::Duration::milliseconds, nan);
        TEST_NAN_HANDLING(turbo::Duration::seconds, nan);
        TEST_NAN_HANDLING(turbo::Duration::minutes, nan);
        TEST_NAN_HANDLING(turbo::Duration::hours, nan);

        TEST_NAN_HANDLING(turbo::Duration::nanoseconds, -nan);
        TEST_NAN_HANDLING(turbo::Duration::microseconds, -nan);
        TEST_NAN_HANDLING(turbo::Duration::milliseconds, -nan);
        TEST_NAN_HANDLING(turbo::Duration::seconds, -nan);
        TEST_NAN_HANDLING(turbo::Duration::minutes, -nan);
        TEST_NAN_HANDLING(turbo::Duration::hours, -nan);

#undef TEST_NAN_HANDLING
    }

    TEST(Duration, Range) {
        const turbo::Duration range = ApproxYears(100 * 1e9);
        const turbo::Duration range_future = range;
        const turbo::Duration range_past = -range;

        EXPECT_LT(range_future, turbo::Duration::max_infinite());
        EXPECT_GT(range_past, -turbo::Duration::max_infinite());

        const turbo::Duration full_range = range_future - range_past;
        EXPECT_GT(full_range, turbo::Duration::zero());
        EXPECT_LT(full_range, turbo::Duration::max_infinite());

        const turbo::Duration neg_full_range = range_past - range_future;
        EXPECT_LT(neg_full_range, turbo::Duration::zero());
        EXPECT_GT(neg_full_range, -turbo::Duration::max_infinite());

        EXPECT_LT(neg_full_range, full_range);
        EXPECT_EQ(neg_full_range, -full_range);
    }

    TEST(Duration, RelationalOperators) {
#define TEST_REL_OPS(UNIT)               \
  static_assert(UNIT(2) == UNIT(2), ""); \
  static_assert(UNIT(1) != UNIT(2), ""); \
  static_assert(UNIT(1) < UNIT(2), "");  \
  static_assert(UNIT(3) > UNIT(2), "");  \
  static_assert(UNIT(1) <= UNIT(2), ""); \
  static_assert(UNIT(2) <= UNIT(2), ""); \
  static_assert(UNIT(3) >= UNIT(2), ""); \
  static_assert(UNIT(2) >= UNIT(2), "");

        TEST_REL_OPS(turbo::Duration::nanoseconds);
        TEST_REL_OPS(turbo::Duration::microseconds);
        TEST_REL_OPS(turbo::Duration::milliseconds);
        TEST_REL_OPS(turbo::Duration::seconds);
        TEST_REL_OPS(turbo::Duration::minutes);
        TEST_REL_OPS(turbo::Duration::hours);

#undef TEST_REL_OPS
    }

    TEST(Duration, Addition) {
#define TEST_ADD_OPS(UNIT)                  \
  do {                                      \
    EXPECT_EQ(UNIT(2), UNIT(1) + UNIT(1));  \
    EXPECT_EQ(UNIT(1), UNIT(2) - UNIT(1));  \
    EXPECT_EQ(UNIT(0), UNIT(2) - UNIT(2));  \
    EXPECT_EQ(UNIT(-1), UNIT(1) - UNIT(2)); \
    EXPECT_EQ(UNIT(-2), UNIT(0) - UNIT(2)); \
    EXPECT_EQ(UNIT(-2), UNIT(1) - UNIT(3)); \
    turbo::Duration a = UNIT(1);             \
    a += UNIT(1);                           \
    EXPECT_EQ(UNIT(2), a);                  \
    a -= UNIT(1);                           \
    EXPECT_EQ(UNIT(1), a);                  \
  } while (0)

        TEST_ADD_OPS(turbo::Duration::nanoseconds);
        TEST_ADD_OPS(turbo::Duration::microseconds);
        TEST_ADD_OPS(turbo::Duration::milliseconds);
        TEST_ADD_OPS(turbo::Duration::seconds);
        TEST_ADD_OPS(turbo::Duration::minutes);
        TEST_ADD_OPS(turbo::Duration::hours);

#undef TEST_ADD_OPS

        EXPECT_EQ(turbo::Duration::seconds(2), turbo::Duration::seconds(3) - 2 * turbo::Duration::milliseconds(500));
        EXPECT_EQ(turbo::Duration::seconds(2) + turbo::Duration::milliseconds(500),
                  turbo::Duration::seconds(3) - turbo::Duration::milliseconds(500));

        EXPECT_EQ(turbo::Duration::seconds(1) + turbo::Duration::milliseconds(998),
                  turbo::Duration::milliseconds(999) + turbo::Duration::milliseconds(999));

        EXPECT_EQ(turbo::Duration::milliseconds(-1),
                  turbo::Duration::milliseconds(998) - turbo::Duration::milliseconds(999));

        // Tests fractions of a nanoseconds. These are implementation details only.
        EXPECT_GT(turbo::Duration::nanoseconds(1), turbo::Duration::nanoseconds(1) / 2);
        EXPECT_EQ(turbo::Duration::nanoseconds(1),
                  turbo::Duration::nanoseconds(1) / 2 + turbo::Duration::nanoseconds(1) / 2);
        EXPECT_GT(turbo::Duration::nanoseconds(1) / 4, turbo::Duration::nanoseconds(0));
        EXPECT_EQ(turbo::Duration::nanoseconds(1) / 8, turbo::Duration::nanoseconds(0));

        // Tests subtraction that will cause wrap around of the rep_lo_ bits.
        turbo::Duration d_7_5 = turbo::Duration::seconds(7) + turbo::Duration::milliseconds(500);
        turbo::Duration d_3_7 = turbo::Duration::seconds(3) + turbo::Duration::milliseconds(700);
        turbo::Duration ans_3_8 = turbo::Duration::seconds(3) + turbo::Duration::milliseconds(800);
        EXPECT_EQ(ans_3_8, d_7_5 - d_3_7);

        // Subtracting min_duration
        turbo::Duration min_dur = turbo::Duration::seconds(kint64min);
        EXPECT_EQ(turbo::Duration::seconds(0), min_dur - min_dur);
        EXPECT_EQ(turbo::Duration::seconds(kint64max), turbo::Duration::seconds(-1) - min_dur);
    }

    TEST(Duration, Negation) {
        // By storing negations of various values in constexpr variables we
        // verify that the initializers are constant expressions.
        constexpr turbo::Duration negated_zero_duration = -turbo::Duration::zero();
        EXPECT_EQ(negated_zero_duration, turbo::Duration::zero());

        constexpr turbo::Duration negated_infinite_duration =
                -turbo::Duration::max_infinite();
        EXPECT_NE(negated_infinite_duration, turbo::Duration::max_infinite());
        EXPECT_EQ(-negated_infinite_duration, turbo::Duration::max_infinite());

        // The public APIs to check if a duration is infinite depend on using
        // Duration::min_infinite(), but we're trying to test operator- here, so we
        // need to use the lower-level internal query IsInfiniteDuration.
        EXPECT_TRUE(
                turbo::time_internal::IsInfiniteDuration(negated_infinite_duration));

        // The largest Duration is kint64max seconds and kTicksPerSecond - 1 ticks.
        // Using the turbo::time_internal::MakeDuration API is the cleanest way to
        // construct that Duration.
        constexpr turbo::Duration max_duration = turbo::time_internal::MakeDuration(
                kint64max, turbo::time_internal::kTicksPerSecond - 1);
        constexpr turbo::Duration negated_max_duration = -max_duration;
        // The largest negatable value is one tick above the minimum representable;
        // it's the negation of max_duration.
        constexpr turbo::Duration nearly_min_duration =
                turbo::time_internal::MakeDuration(kint64min, int64_t{1});
        constexpr turbo::Duration negated_nearly_min_duration = -nearly_min_duration;

        EXPECT_EQ(negated_max_duration, nearly_min_duration);
        EXPECT_EQ(negated_nearly_min_duration, max_duration);
        EXPECT_EQ(-(-max_duration), max_duration);

        constexpr turbo::Duration min_duration =
                turbo::time_internal::MakeDuration(kint64min);
        constexpr turbo::Duration negated_min_duration = -min_duration;
        EXPECT_EQ(negated_min_duration, turbo::Duration::max_infinite());
    }

    TEST(Duration, AbsoluteValue) {
        EXPECT_EQ(turbo::Duration::zero(), turbo::Duration::abs(turbo::Duration::zero()));
        EXPECT_EQ(turbo::Duration::seconds(1),  turbo::Duration::abs(turbo::Duration::seconds(1)));
        EXPECT_EQ(turbo::Duration::seconds(1),  turbo::Duration::abs(turbo::Duration::seconds(-1)));

        EXPECT_EQ(turbo::Duration::max_infinite(),  turbo::Duration::abs(turbo::Duration::max_infinite()));
        EXPECT_EQ(turbo::Duration::max_infinite(),  turbo::Duration::abs(-turbo::Duration::max_infinite()));

        turbo::Duration max_dur =
                turbo::Duration::seconds(kint64max) + (turbo::Duration::seconds(1) - turbo::Duration::nanoseconds(1) / 4);
        EXPECT_EQ(max_dur,  turbo::Duration::abs(max_dur));

        turbo::Duration min_dur = turbo::Duration::seconds(kint64min);
        EXPECT_EQ(turbo::Duration::max_infinite(),  turbo::Duration::abs(min_dur));
        EXPECT_EQ(max_dur,  turbo::Duration::abs(min_dur + turbo::Duration::nanoseconds(1) / 4));
    }

    TEST(Duration, Multiplication) {
#define TEST_MUL_OPS(UNIT)                                    \
  do {                                                        \
    EXPECT_EQ(UNIT(5), UNIT(2) * 2.5);                        \
    EXPECT_EQ(UNIT(2), UNIT(5) / 2.5);                        \
    EXPECT_EQ(UNIT(-5), UNIT(-2) * 2.5);                      \
    EXPECT_EQ(UNIT(-5), -UNIT(2) * 2.5);                      \
    EXPECT_EQ(UNIT(-5), UNIT(2) * -2.5);                      \
    EXPECT_EQ(UNIT(-2), UNIT(-5) / 2.5);                      \
    EXPECT_EQ(UNIT(-2), -UNIT(5) / 2.5);                      \
    EXPECT_EQ(UNIT(-2), UNIT(5) / -2.5);                      \
    EXPECT_EQ(UNIT(2), UNIT(11) % UNIT(3));                   \
    turbo::Duration a = UNIT(2);                               \
    a *= 2.5;                                                 \
    EXPECT_EQ(UNIT(5), a);                                    \
    a /= 2.5;                                                 \
    EXPECT_EQ(UNIT(2), a);                                    \
    a %= UNIT(1);                                             \
    EXPECT_EQ(UNIT(0), a);                                    \
    turbo::Duration big = UNIT(1000000000);                    \
    big *= 3;                                                 \
    big /= 3;                                                 \
    EXPECT_EQ(UNIT(1000000000), big);                         \
    EXPECT_EQ(-UNIT(2), -UNIT(2));                            \
    EXPECT_EQ(-UNIT(2), UNIT(2) * -1);                        \
    EXPECT_EQ(-UNIT(2), -1 * UNIT(2));                        \
    EXPECT_EQ(-UNIT(-2), UNIT(2));                            \
    EXPECT_EQ(2, UNIT(2) / UNIT(1));                          \
    turbo::Duration rem;                                       \
    EXPECT_EQ(2, turbo::Duration::idiv(UNIT(2), UNIT(1), &rem)); \
    EXPECT_EQ(2.0, turbo::Duration::fdiv(UNIT(2), UNIT(1)));     \
  } while (0)

        TEST_MUL_OPS(turbo::Duration::nanoseconds);
        TEST_MUL_OPS(turbo::Duration::microseconds);
        TEST_MUL_OPS(turbo::Duration::milliseconds);
        TEST_MUL_OPS(turbo::Duration::seconds);
        TEST_MUL_OPS(turbo::Duration::minutes);
        TEST_MUL_OPS(turbo::Duration::hours);

#undef TEST_MUL_OPS

        // Ensures that multiplication and division by 1 with a maxed-out durations
        // doesn't lose precision.
        turbo::Duration max_dur =
                turbo::Duration::seconds(kint64max) + (turbo::Duration::seconds(1) - turbo::Duration::nanoseconds(1) / 4);
        turbo::Duration min_dur = turbo::Duration::seconds(kint64min);
        EXPECT_EQ(max_dur, max_dur * 1);
        EXPECT_EQ(max_dur, max_dur / 1);
        EXPECT_EQ(min_dur, min_dur * 1);
        EXPECT_EQ(min_dur, min_dur / 1);

        // Tests division on a Duration with a large number of significant digits.
        // Tests when the digits span hi and lo as well as only in hi.
        turbo::Duration sigfigs = turbo::Duration::seconds(2000000000) + turbo::Duration::nanoseconds(3);
        EXPECT_EQ(turbo::Duration::seconds(666666666) + turbo::Duration::nanoseconds(666666667) +
                  turbo::Duration::nanoseconds(1) / 2,
                  sigfigs / 3);
        sigfigs = turbo::Duration::seconds(int64_t{7000000000});
        EXPECT_EQ(turbo::Duration::seconds(2333333333) + turbo::Duration::nanoseconds(333333333) +
                  turbo::Duration::nanoseconds(1) / 4,
                  sigfigs / 3);

        EXPECT_EQ(turbo::Duration::seconds(7) + turbo::Duration::milliseconds(500), turbo::Duration::seconds(3) * 2.5);
        EXPECT_EQ(turbo::Duration::seconds(8) * -1 + turbo::Duration::milliseconds(300),
                  (turbo::Duration::seconds(2) + turbo::Duration::milliseconds(200)) * -3.5);
        EXPECT_EQ(-turbo::Duration::seconds(8) + turbo::Duration::milliseconds(300),
                  (turbo::Duration::seconds(2) + turbo::Duration::milliseconds(200)) * -3.5);
        EXPECT_EQ(turbo::Duration::seconds(1) + turbo::Duration::milliseconds(875),
                  (turbo::Duration::seconds(7) + turbo::Duration::milliseconds(500)) / 4);
        EXPECT_EQ(turbo::Duration::seconds(30),
                  (turbo::Duration::seconds(7) + turbo::Duration::milliseconds(500)) / 0.25);
        EXPECT_EQ(turbo::Duration::seconds(3),
                  (turbo::Duration::seconds(7) + turbo::Duration::milliseconds(500)) / 2.5);

        // Tests division remainder.
        EXPECT_EQ(turbo::Duration::nanoseconds(0), turbo::Duration::nanoseconds(7) % turbo::Duration::nanoseconds(1));
        EXPECT_EQ(turbo::Duration::nanoseconds(0), turbo::Duration::nanoseconds(0) % turbo::Duration::nanoseconds(10));
        EXPECT_EQ(turbo::Duration::nanoseconds(2), turbo::Duration::nanoseconds(7) % turbo::Duration::nanoseconds(5));
        EXPECT_EQ(turbo::Duration::nanoseconds(2), turbo::Duration::nanoseconds(2) % turbo::Duration::nanoseconds(5));

        EXPECT_EQ(turbo::Duration::nanoseconds(1), turbo::Duration::nanoseconds(10) % turbo::Duration::nanoseconds(3));
        EXPECT_EQ(turbo::Duration::nanoseconds(1),
                  turbo::Duration::nanoseconds(10) % turbo::Duration::nanoseconds(-3));
        EXPECT_EQ(turbo::Duration::nanoseconds(-1),
                  turbo::Duration::nanoseconds(-10) % turbo::Duration::nanoseconds(3));
        EXPECT_EQ(turbo::Duration::nanoseconds(-1),
                  turbo::Duration::nanoseconds(-10) % turbo::Duration::nanoseconds(-3));

        EXPECT_EQ(turbo::Duration::milliseconds(100),
                  turbo::Duration::seconds(1) % turbo::Duration::milliseconds(300));
        EXPECT_EQ(
                turbo::Duration::milliseconds(300),
                (turbo::Duration::seconds(3) + turbo::Duration::milliseconds(800)) % turbo::Duration::milliseconds(500));

        EXPECT_EQ(turbo::Duration::nanoseconds(1), turbo::Duration::nanoseconds(1) % turbo::Duration::seconds(1));
        EXPECT_EQ(turbo::Duration::nanoseconds(-1), turbo::Duration::nanoseconds(-1) % turbo::Duration::seconds(1));
        EXPECT_EQ(0, turbo::Duration::nanoseconds(-1) / turbo::Duration::seconds(1));  // Actual -1e-9

        // Tests identity a = (a/b)*b + a%b
#define TEST_MOD_IDENTITY(a, b) \
  EXPECT_EQ((a), ((a) / (b))*(b) + ((a)%(b)))

        TEST_MOD_IDENTITY(turbo::Duration::seconds(0), turbo::Duration::seconds(2));
        TEST_MOD_IDENTITY(turbo::Duration::seconds(1), turbo::Duration::seconds(1));
        TEST_MOD_IDENTITY(turbo::Duration::seconds(1), turbo::Duration::seconds(2));
        TEST_MOD_IDENTITY(turbo::Duration::seconds(2), turbo::Duration::seconds(1));

        TEST_MOD_IDENTITY(turbo::Duration::seconds(-2), turbo::Duration::seconds(1));
        TEST_MOD_IDENTITY(turbo::Duration::seconds(2), turbo::Duration::seconds(-1));
        TEST_MOD_IDENTITY(turbo::Duration::seconds(-2), turbo::Duration::seconds(-1));

        TEST_MOD_IDENTITY(turbo::Duration::nanoseconds(0), turbo::Duration::nanoseconds(2));
        TEST_MOD_IDENTITY(turbo::Duration::nanoseconds(1), turbo::Duration::nanoseconds(1));
        TEST_MOD_IDENTITY(turbo::Duration::nanoseconds(1), turbo::Duration::nanoseconds(2));
        TEST_MOD_IDENTITY(turbo::Duration::nanoseconds(2), turbo::Duration::nanoseconds(1));

        TEST_MOD_IDENTITY(turbo::Duration::nanoseconds(-2), turbo::Duration::nanoseconds(1));
        TEST_MOD_IDENTITY(turbo::Duration::nanoseconds(2), turbo::Duration::nanoseconds(-1));
        TEST_MOD_IDENTITY(turbo::Duration::nanoseconds(-2), turbo::Duration::nanoseconds(-1));

        // Mixed seconds + subseconds
        turbo::Duration mixed_a = turbo::Duration::seconds(1) + turbo::Duration::nanoseconds(2);
        turbo::Duration mixed_b = turbo::Duration::seconds(1) + turbo::Duration::nanoseconds(3);

        TEST_MOD_IDENTITY(turbo::Duration::seconds(0), mixed_a);
        TEST_MOD_IDENTITY(mixed_a, mixed_a);
        TEST_MOD_IDENTITY(mixed_a, mixed_b);
        TEST_MOD_IDENTITY(mixed_b, mixed_a);

        TEST_MOD_IDENTITY(-mixed_a, mixed_b);
        TEST_MOD_IDENTITY(mixed_a, -mixed_b);
        TEST_MOD_IDENTITY(-mixed_a, -mixed_b);

#undef TEST_MOD_IDENTITY
    }

    TEST(Duration, Truncation) {
        const turbo::Duration d = turbo::Duration::nanoseconds(1234567890);
        const turbo::Duration inf = turbo::Duration::max_infinite();
        for (int unit_sign: {1, -1}) {  // sign shouldn't matter
            EXPECT_EQ(turbo::Duration::nanoseconds(1234567890),
                      turbo::Duration::trunc(d, unit_sign * turbo::Duration::nanoseconds(1)));
            EXPECT_EQ(turbo::Duration::microseconds(1234567),
                      turbo::Duration::trunc(d, unit_sign * turbo::Duration::microseconds(1)));
            EXPECT_EQ(turbo::Duration::milliseconds(1234),
                      turbo::Duration::trunc(d, unit_sign * turbo::Duration::milliseconds(1)));
            EXPECT_EQ(turbo::Duration::seconds(1), turbo::Duration::trunc(d, unit_sign * turbo::Duration::seconds(1)));
            EXPECT_EQ(inf, turbo::Duration::trunc(inf, unit_sign * turbo::Duration::seconds(1)));

            EXPECT_EQ(turbo::Duration::nanoseconds(-1234567890),
                      turbo::Duration::trunc(-d, unit_sign * turbo::Duration::nanoseconds(1)));
            EXPECT_EQ(turbo::Duration::microseconds(-1234567),
                      turbo::Duration::trunc(-d, unit_sign * turbo::Duration::microseconds(1)));
            EXPECT_EQ(turbo::Duration::milliseconds(-1234),
                      turbo::Duration::trunc(-d, unit_sign * turbo::Duration::milliseconds(1)));
            EXPECT_EQ(turbo::Duration::seconds(-1), turbo::Duration::trunc(-d, unit_sign * turbo::Duration::seconds(1)));
            EXPECT_EQ(-inf, turbo::Duration::trunc(-inf, unit_sign * turbo::Duration::seconds(1)));
        }
    }

    TEST(Duration, Flooring) {
        const turbo::Duration d = turbo::Duration::nanoseconds(1234567890);
        const turbo::Duration inf = turbo::Duration::max_infinite();
        for (int unit_sign: {1, -1}) {  // sign shouldn't matter
            EXPECT_EQ(turbo::Duration::nanoseconds(1234567890),
                      turbo::Duration::floor(d, unit_sign * turbo::Duration::nanoseconds(1)));
            EXPECT_EQ(turbo::Duration::microseconds(1234567),
                      turbo::Duration::floor(d, unit_sign * turbo::Duration::microseconds(1)));
            EXPECT_EQ(turbo::Duration::milliseconds(1234),
                      turbo::Duration::floor(d, unit_sign * turbo::Duration::milliseconds(1)));
            EXPECT_EQ(turbo::Duration::seconds(1), turbo::Duration::floor(d, unit_sign * turbo::Duration::seconds(1)));
            EXPECT_EQ(inf, turbo::Duration::floor(inf, unit_sign * turbo::Duration::seconds(1)));

            EXPECT_EQ(turbo::Duration::nanoseconds(-1234567890),
                      turbo::Duration::floor(-d, unit_sign * turbo::Duration::nanoseconds(1)));
            EXPECT_EQ(turbo::Duration::microseconds(-1234568),
                      turbo::Duration::floor(-d, unit_sign * turbo::Duration::microseconds(1)));
            EXPECT_EQ(turbo::Duration::milliseconds(-1235),
                      turbo::Duration::floor(-d, unit_sign * turbo::Duration::milliseconds(1)));
            EXPECT_EQ(turbo::Duration::seconds(-2), turbo::Duration::floor(-d, unit_sign * turbo::Duration::seconds(1)));
            EXPECT_EQ(-inf, turbo::Duration::floor(-inf, unit_sign * turbo::Duration::seconds(1)));
        }
    }

    TEST(Duration, Ceiling) {
        const turbo::Duration d = turbo::Duration::nanoseconds(1234567890);
        const turbo::Duration inf = turbo::Duration::max_infinite();
        for (int unit_sign: {1, -1}) {  // // sign shouldn't matter
            EXPECT_EQ(turbo::Duration::nanoseconds(1234567890),
                      turbo::Duration::ceil(d, unit_sign * turbo::Duration::nanoseconds(1)));
            EXPECT_EQ(turbo::Duration::microseconds(1234568),
                      turbo::Duration::ceil(d, unit_sign * turbo::Duration::microseconds(1)));
            EXPECT_EQ(turbo::Duration::milliseconds(1235),
                      turbo::Duration::ceil(d, unit_sign * turbo::Duration::milliseconds(1)));
            EXPECT_EQ(turbo::Duration::seconds(2), turbo::Duration::ceil(d, unit_sign * turbo::Duration::seconds(1)));
            EXPECT_EQ(inf, turbo::Duration::ceil(inf, unit_sign * turbo::Duration::seconds(1)));

            EXPECT_EQ(turbo::Duration::nanoseconds(-1234567890),
                      turbo::Duration::ceil(-d, unit_sign * turbo::Duration::nanoseconds(1)));
            EXPECT_EQ(turbo::Duration::microseconds(-1234567),
                      turbo::Duration::ceil(-d, unit_sign * turbo::Duration::microseconds(1)));
            EXPECT_EQ(turbo::Duration::milliseconds(-1234),
                      turbo::Duration::ceil(-d, unit_sign * turbo::Duration::milliseconds(1)));
            EXPECT_EQ(turbo::Duration::seconds(-1), turbo::Duration::ceil(-d, unit_sign * turbo::Duration::seconds(1)));
            EXPECT_EQ(-inf, turbo::Duration::ceil(-inf, unit_sign * turbo::Duration::seconds(1)));
        }
    }

    TEST(Duration, RoundTripUnits) {
        const int kRange = 100000;

#define ROUND_TRIP_UNIT(U, LOW, HIGH)          \
  do {                                         \
    for (int64_t i = LOW; i < HIGH; ++i) {     \
      turbo::Duration d = turbo::U(i);           \
      if (d == turbo::Duration::max_infinite())       \
        EXPECT_EQ(kint64max, d / turbo::U(1));  \
      else if (d == -turbo::Duration::max_infinite()) \
        EXPECT_EQ(kint64min, d / turbo::U(1));  \
      else                                     \
        EXPECT_EQ(i, turbo::U(i) / turbo::U(1)); \
    }                                          \
  } while (0)

        ROUND_TRIP_UNIT(Duration::nanoseconds, kint64min, kint64min + kRange);
        ROUND_TRIP_UNIT(Duration::nanoseconds, -kRange, kRange);
        ROUND_TRIP_UNIT(Duration::nanoseconds, kint64max - kRange, kint64max);

        ROUND_TRIP_UNIT(Duration::microseconds, kint64min, kint64min + kRange);
        ROUND_TRIP_UNIT(Duration::microseconds, -kRange, kRange);
        ROUND_TRIP_UNIT(Duration::microseconds, kint64max - kRange, kint64max);

        ROUND_TRIP_UNIT(Duration::milliseconds, kint64min, kint64min + kRange);
        ROUND_TRIP_UNIT(Duration::milliseconds, -kRange, kRange);
        ROUND_TRIP_UNIT(Duration::milliseconds, kint64max - kRange, kint64max);

        ROUND_TRIP_UNIT(Duration::seconds, kint64min, kint64min + kRange);
        ROUND_TRIP_UNIT(Duration::seconds, -kRange, kRange);
        ROUND_TRIP_UNIT(Duration::seconds, kint64max - kRange, kint64max);

        ROUND_TRIP_UNIT(Duration::minutes, kint64min / 60, kint64min / 60 + kRange);
        ROUND_TRIP_UNIT(Duration::minutes, -kRange, kRange);
        ROUND_TRIP_UNIT(Duration::minutes, kint64max / 60 - kRange, kint64max / 60);

        ROUND_TRIP_UNIT(Duration::hours, kint64min / 3600, kint64min / 3600 + kRange);
        ROUND_TRIP_UNIT(Duration::hours, -kRange, kRange);
        ROUND_TRIP_UNIT(Duration::hours, kint64max / 3600 - kRange, kint64max / 3600);

#undef ROUND_TRIP_UNIT
    }

    TEST(Duration, TruncConversions) {
        // Tests turbo::Duration::to_timespec()/DurationFromTimespec()
        const struct {
            turbo::Duration d;
            timespec ts;
        } to_ts[] = {
                {turbo::Duration::seconds(1) + turbo::Duration::nanoseconds(1),      {1,  1}},
                {turbo::Duration::seconds(1) + turbo::Duration::nanoseconds(1) / 2,  {1,  0}},
                {turbo::Duration::seconds(1) + turbo::Duration::nanoseconds(0),      {1,  0}},
                {turbo::Duration::seconds(0) + turbo::Duration::nanoseconds(0),      {0,  0}},
                {turbo::Duration::seconds(0) - turbo::Duration::nanoseconds(1) / 2,  {0,  0}},
                {turbo::Duration::seconds(0) - turbo::Duration::nanoseconds(1),      {-1, 999999999}},
                {turbo::Duration::seconds(-1) + turbo::Duration::nanoseconds(1),     {-1, 1}},
                {turbo::Duration::seconds(-1) + turbo::Duration::nanoseconds(1) / 2, {-1, 1}},
                {turbo::Duration::seconds(-1) + turbo::Duration::nanoseconds(0),     {-1, 0}},
                {turbo::Duration::seconds(-1) - turbo::Duration::nanoseconds(1) / 2, {-1, 0}},
        };
        for (const auto &test: to_ts) {
            EXPECT_THAT(turbo::Duration::to_timespec(test.d), TimespecMatcher(test.ts));
        }
        const struct {
            timespec ts;
            turbo::Duration d;
        } from_ts[] = {
                {{1,  1},         turbo::Duration::seconds(1) + turbo::Duration::nanoseconds(1)},
                {{1,  0},         turbo::Duration::seconds(1) + turbo::Duration::nanoseconds(0)},
                {{0,  0},         turbo::Duration::seconds(0) + turbo::Duration::nanoseconds(0)},
                {{0,  -1},        turbo::Duration::seconds(0) - turbo::Duration::nanoseconds(1)},
                {{-1, 999999999}, turbo::Duration::seconds(0) - turbo::Duration::nanoseconds(1)},
                {{-1, 1},         turbo::Duration::seconds(-1) + turbo::Duration::nanoseconds(1)},
                {{-1, 0},         turbo::Duration::seconds(-1) + turbo::Duration::nanoseconds(0)},
                {{-1, -1},        turbo::Duration::seconds(-1) - turbo::Duration::nanoseconds(1)},
                {{-2, 999999999}, turbo::Duration::seconds(-1) - turbo::Duration::nanoseconds(1)},
        };
        for (const auto &test: from_ts) {
            EXPECT_EQ(test.d, turbo::Duration::from_timespec(test.ts));
        }

        // Tests turbo::Duration::to_timeval()/DurationFromTimeval() (same as timespec above)
        const struct {
            turbo::Duration d;
            timeval tv;
        } to_tv[] = {
                {turbo::Duration::seconds(1) + turbo::Duration::microseconds(1),      {1,  1}},
                {turbo::Duration::seconds(1) + turbo::Duration::microseconds(1) / 2,  {1,  0}},
                {turbo::Duration::seconds(1) + turbo::Duration::microseconds(0),      {1,  0}},
                {turbo::Duration::seconds(0) + turbo::Duration::microseconds(0),      {0,  0}},
                {turbo::Duration::seconds(0) - turbo::Duration::microseconds(1) / 2,  {0,  0}},
                {turbo::Duration::seconds(0) - turbo::Duration::microseconds(1),      {-1, 999999}},
                {turbo::Duration::seconds(-1) + turbo::Duration::microseconds(1),     {-1, 1}},
                {turbo::Duration::seconds(-1) + turbo::Duration::microseconds(1) / 2, {-1, 1}},
                {turbo::Duration::seconds(-1) + turbo::Duration::microseconds(0),     {-1, 0}},
                {turbo::Duration::seconds(-1) - turbo::Duration::microseconds(1) / 2, {-1, 0}},
        };
        for (const auto &test: to_tv) {
            EXPECT_THAT(turbo::Duration::to_timeval(test.d), TimevalMatcher(test.tv));
        }
        const struct {
            timeval tv;
            turbo::Duration d;
        } from_tv[] = {
                {{1,  1},      turbo::Duration::seconds(1) + turbo::Duration::microseconds(1)},
                {{1,  0},      turbo::Duration::seconds(1) + turbo::Duration::microseconds(0)},
                {{0,  0},      turbo::Duration::seconds(0) + turbo::Duration::microseconds(0)},
                {{0,  -1},     turbo::Duration::seconds(0) - turbo::Duration::microseconds(1)},
                {{-1, 999999}, turbo::Duration::seconds(0) - turbo::Duration::microseconds(1)},
                {{-1, 1},      turbo::Duration::seconds(-1) + turbo::Duration::microseconds(1)},
                {{-1, 0},      turbo::Duration::seconds(-1) + turbo::Duration::microseconds(0)},
                {{-1, -1},     turbo::Duration::seconds(-1) - turbo::Duration::microseconds(1)},
                {{-2, 999999}, turbo::Duration::seconds(-1) - turbo::Duration::microseconds(1)},
        };
        for (const auto &test: from_tv) {
            EXPECT_EQ(test.d, turbo::Duration::from_timeval(test.tv));
        }
    }

    TEST(Duration, SmallConversions) {
        // Special tests for conversions of small durations.

        EXPECT_EQ(turbo::Duration::zero(), turbo::Duration::seconds(0));
        // TODO(bww): Is the next one OK?
        EXPECT_EQ(turbo::Duration::zero(), turbo::Duration::seconds(std::nextafter(0.125e-9, 0)));
        EXPECT_EQ(turbo::Duration::nanoseconds(1) / 4, turbo::Duration::seconds(0.125e-9));
        EXPECT_EQ(turbo::Duration::nanoseconds(1) / 4, turbo::Duration::seconds(0.250e-9));
        EXPECT_EQ(turbo::Duration::nanoseconds(1) / 2, turbo::Duration::seconds(0.375e-9));
        EXPECT_EQ(turbo::Duration::nanoseconds(1) / 2, turbo::Duration::seconds(0.500e-9));
        EXPECT_EQ(turbo::Duration::nanoseconds(3) / 4, turbo::Duration::seconds(0.625e-9));
        EXPECT_EQ(turbo::Duration::nanoseconds(3) / 4, turbo::Duration::seconds(0.750e-9));
        EXPECT_EQ(turbo::Duration::nanoseconds(1), turbo::Duration::seconds(0.875e-9));
        EXPECT_EQ(turbo::Duration::nanoseconds(1), turbo::Duration::seconds(1.000e-9));

        EXPECT_EQ(turbo::Duration::zero(), turbo::Duration::seconds(std::nextafter(-0.125e-9, 0)));
        EXPECT_EQ(-turbo::Duration::nanoseconds(1) / 4, turbo::Duration::seconds(-0.125e-9));
        EXPECT_EQ(-turbo::Duration::nanoseconds(1) / 4, turbo::Duration::seconds(-0.250e-9));
        EXPECT_EQ(-turbo::Duration::nanoseconds(1) / 2, turbo::Duration::seconds(-0.375e-9));
        EXPECT_EQ(-turbo::Duration::nanoseconds(1) / 2, turbo::Duration::seconds(-0.500e-9));
        EXPECT_EQ(-turbo::Duration::nanoseconds(3) / 4, turbo::Duration::seconds(-0.625e-9));
        EXPECT_EQ(-turbo::Duration::nanoseconds(3) / 4, turbo::Duration::seconds(-0.750e-9));
        EXPECT_EQ(-turbo::Duration::nanoseconds(1), turbo::Duration::seconds(-0.875e-9));
        EXPECT_EQ(-turbo::Duration::nanoseconds(1), turbo::Duration::seconds(-1.000e-9));

        timespec ts;
        ts.tv_sec = 0;
        ts.tv_nsec = 0;
        EXPECT_THAT(turbo::Duration::to_timespec(turbo::Duration::nanoseconds(0)), TimespecMatcher(ts));
        // TODO(bww): Are the next three OK?
        EXPECT_THAT(turbo::Duration::to_timespec(turbo::Duration::nanoseconds(1) / 4), TimespecMatcher(ts));
        EXPECT_THAT(turbo::Duration::to_timespec(turbo::Duration::nanoseconds(2) / 4), TimespecMatcher(ts));
        EXPECT_THAT(turbo::Duration::to_timespec(turbo::Duration::nanoseconds(3) / 4), TimespecMatcher(ts));
        ts.tv_nsec = 1;
        EXPECT_THAT(turbo::Duration::to_timespec(turbo::Duration::nanoseconds(4) / 4), TimespecMatcher(ts));
        EXPECT_THAT(turbo::Duration::to_timespec(turbo::Duration::nanoseconds(5) / 4), TimespecMatcher(ts));
        EXPECT_THAT(turbo::Duration::to_timespec(turbo::Duration::nanoseconds(6) / 4), TimespecMatcher(ts));
        EXPECT_THAT(turbo::Duration::to_timespec(turbo::Duration::nanoseconds(7) / 4), TimespecMatcher(ts));
        ts.tv_nsec = 2;
        EXPECT_THAT(turbo::Duration::to_timespec(turbo::Duration::nanoseconds(8) / 4), TimespecMatcher(ts));

        timeval tv;
        tv.tv_sec = 0;
        tv.tv_usec = 0;
        EXPECT_THAT(turbo::Duration::to_timeval(turbo::Duration::nanoseconds(0)), TimevalMatcher(tv));
        // TODO(bww): Is the next one OK?
        EXPECT_THAT(turbo::Duration::to_timeval(turbo::Duration::nanoseconds(999)), TimevalMatcher(tv));
        tv.tv_usec = 1;
        EXPECT_THAT(turbo::Duration::to_timeval(turbo::Duration::nanoseconds(1000)), TimevalMatcher(tv));
        EXPECT_THAT(turbo::Duration::to_timeval(turbo::Duration::nanoseconds(1999)), TimevalMatcher(tv));
        tv.tv_usec = 2;
        EXPECT_THAT(turbo::Duration::to_timeval(turbo::Duration::nanoseconds(2000)), TimevalMatcher(tv));
    }

    void VerifyApproxSameAsMul(double time_as_seconds, int *const misses) {
        auto direct_seconds = turbo::Duration::seconds(time_as_seconds);
        auto mul_by_one_second = time_as_seconds * turbo::Duration::seconds(1);
        // These are expected to differ by up to one tick due to fused multiply/add
        // contraction.
        if (turbo::Duration::abs(direct_seconds - mul_by_one_second) >
            turbo::time_internal::MakeDuration(0, 1u)) {
            if (*misses > 10) return;
            ASSERT_LE(++(*misses), 10) << "Too many errors, not reporting more.";
            EXPECT_EQ(direct_seconds, mul_by_one_second)
                                << "given double time_as_seconds = " << std::setprecision(17)
                                << time_as_seconds;
        }
    }

// For a variety of interesting durations, we find the exact point
// where one double converts to that duration, and the very next double
// converts to the next duration.  For both of those points, verify that
// Duration::seconds(point) returns a duration near point * Duration::seconds(1.0). (They may
// not be exactly equal due to fused multiply/add contraction.)
    TEST(Duration, ToDoubleSecondsCheckEdgeCases) {
#if (defined(__i386__) || defined(_M_IX86)) && FLT_EVAL_METHOD != 0
        // We're using an x87-compatible FPU, and intermediate operations can be
        // performed with 80-bit floats. This means the edge cases are different than
        // what we expect here, so just skip this test.
        GTEST_SKIP()
            << "Skipping the test because we detected x87 floating-point semantics";
#endif

        constexpr uint32_t kTicksPerSecond = turbo::time_internal::kTicksPerSecond;
        constexpr auto duration_tick = turbo::time_internal::MakeDuration(0, 1u);
        int misses = 0;
        for (int64_t seconds = 0; seconds < 99; ++seconds) {
            uint32_t tick_vals[] = {0, +999, +999999, +999999999, kTicksPerSecond - 1,
                                    0, 1000, 1000000, 1000000000, kTicksPerSecond,
                                    1, 1001, 1000001, 1000000001, kTicksPerSecond + 1,
                                    2, 1002, 1000002, 1000000002, kTicksPerSecond + 2,
                                    3, 1003, 1000003, 1000000003, kTicksPerSecond + 3,
                                    4, 1004, 1000004, 1000000004, kTicksPerSecond + 4,
                                    5, 6, 7, 8, 9};
            for (uint32_t ticks: tick_vals) {
                turbo::Duration s_plus_t = turbo::Duration::seconds(seconds) + ticks * duration_tick;
                for (turbo::Duration d: {s_plus_t, -s_plus_t}) {
                    turbo::Duration after_d = d + duration_tick;
                    EXPECT_NE(d, after_d);
                    EXPECT_EQ(after_d - d, duration_tick);

                    double low_edge = turbo::Duration::to_double_seconds(d);
                    EXPECT_EQ(d, turbo::Duration::seconds(low_edge));

                    double high_edge = turbo::Duration::to_double_seconds(after_d);
                    EXPECT_EQ(after_d, turbo::Duration::seconds(high_edge));

                    for (;;) {
                        double midpoint = low_edge + (high_edge - low_edge) / 2;
                        if (midpoint == low_edge || midpoint == high_edge) break;
                        turbo::Duration mid_duration = turbo::Duration::seconds(midpoint);
                        if (mid_duration == d) {
                            low_edge = midpoint;
                        } else {
                            EXPECT_EQ(mid_duration, after_d);
                            high_edge = midpoint;
                        }
                    }
                    // Now low_edge is the highest double that converts to Duration d,
                    // and high_edge is the lowest double that converts to Duration after_d.
                    VerifyApproxSameAsMul(low_edge, &misses);
                    VerifyApproxSameAsMul(high_edge, &misses);
                }
            }
        }
    }

    TEST(Duration, ToDoubleSecondsCheckRandom) {
        std::random_device rd;
        std::seed_seq seed({rd(), rd(), rd(), rd(), rd(), rd(), rd(), rd()});
        std::mt19937_64 gen(seed);
        // We want doubles distributed from 1/8ns up to 2^63, where
        // as many values are tested from 1ns to 2ns as from 1sec to 2sec,
        // so even distribute along a log-scale of those values, and
        // exponentiate before using them.  (9.223377e+18 is just slightly
        // out of bounds for turbo::Duration.)
        std::uniform_real_distribution<double> uniform(std::log(0.125e-9),
                                                       std::log(9.223377e+18));
        int misses = 0;
        for (int i = 0; i < 1000000; ++i) {
            double d = std::exp(uniform(gen));
            VerifyApproxSameAsMul(d, &misses);
            VerifyApproxSameAsMul(-d, &misses);
        }
    }

    TEST(Duration, ConversionSaturation) {
        turbo::Duration d;

        const auto max_timeval_sec =
                std::numeric_limits<decltype(timeval::tv_sec)>::max();
        const auto min_timeval_sec =
                std::numeric_limits<decltype(timeval::tv_sec)>::min();
        timeval tv;
        tv.tv_sec = max_timeval_sec;
        tv.tv_usec = 999998;
        d = turbo::Duration::from_timeval(tv);
        tv = turbo::Duration::to_timeval(d);
        EXPECT_EQ(max_timeval_sec, tv.tv_sec);
        EXPECT_EQ(999998, tv.tv_usec);
        d += turbo::Duration::microseconds(1);
        tv = turbo::Duration::to_timeval(d);
        EXPECT_EQ(max_timeval_sec, tv.tv_sec);
        EXPECT_EQ(999999, tv.tv_usec);
        d += turbo::Duration::microseconds(1);  // no effect
        tv = turbo::Duration::to_timeval(d);
        EXPECT_EQ(max_timeval_sec, tv.tv_sec);
        EXPECT_EQ(999999, tv.tv_usec);

        tv.tv_sec = min_timeval_sec;
        tv.tv_usec = 1;
        d = turbo::Duration::from_timeval(tv);
        tv = turbo::Duration::to_timeval(d);
        EXPECT_EQ(min_timeval_sec, tv.tv_sec);
        EXPECT_EQ(1, tv.tv_usec);
        d -= turbo::Duration::microseconds(1);
        tv = turbo::Duration::to_timeval(d);
        EXPECT_EQ(min_timeval_sec, tv.tv_sec);
        EXPECT_EQ(0, tv.tv_usec);
        d -= turbo::Duration::microseconds(1);  // no effect
        tv = turbo::Duration::to_timeval(d);
        EXPECT_EQ(min_timeval_sec, tv.tv_sec);
        EXPECT_EQ(0, tv.tv_usec);

        const auto max_timespec_sec =
                std::numeric_limits<decltype(timespec::tv_sec)>::max();
        const auto min_timespec_sec =
                std::numeric_limits<decltype(timespec::tv_sec)>::min();
        timespec ts;
        ts.tv_sec = max_timespec_sec;
        ts.tv_nsec = 999999998;
        d = turbo::Duration::from_timespec(ts);
        ts = turbo::Duration::to_timespec(d);
        EXPECT_EQ(max_timespec_sec, ts.tv_sec);
        EXPECT_EQ(999999998, ts.tv_nsec);
        d += turbo::Duration::nanoseconds(1);
        ts = turbo::Duration::to_timespec(d);
        EXPECT_EQ(max_timespec_sec, ts.tv_sec);
        EXPECT_EQ(999999999, ts.tv_nsec);
        d += turbo::Duration::nanoseconds(1);  // no effect
        ts = turbo::Duration::to_timespec(d);
        EXPECT_EQ(max_timespec_sec, ts.tv_sec);
        EXPECT_EQ(999999999, ts.tv_nsec);

        ts.tv_sec = min_timespec_sec;
        ts.tv_nsec = 1;
        d = turbo::Duration::from_timespec(ts);
        ts = turbo::Duration::to_timespec(d);
        EXPECT_EQ(min_timespec_sec, ts.tv_sec);
        EXPECT_EQ(1, ts.tv_nsec);
        d -= turbo::Duration::nanoseconds(1);
        ts = turbo::Duration::to_timespec(d);
        EXPECT_EQ(min_timespec_sec, ts.tv_sec);
        EXPECT_EQ(0, ts.tv_nsec);
        d -= turbo::Duration::nanoseconds(1);  // no effect
        ts = turbo::Duration::to_timespec(d);
        EXPECT_EQ(min_timespec_sec, ts.tv_sec);
        EXPECT_EQ(0, ts.tv_nsec);
    }

    TEST(Duration, Duration_format) {
        // Example from Go's docs.
        EXPECT_EQ("72h3m0.5s",
                  turbo::Duration::format(turbo::Duration::hours(72) + turbo::Duration::minutes(3) +
                                        turbo::Duration::milliseconds(500)));
        // Go's largest time: 2540400h10m10.000000000s
        EXPECT_EQ("2540400h10m10s",
                  turbo::Duration::format(turbo::Duration::hours(2540400) + turbo::Duration::minutes(10) +
                                        turbo::Duration::seconds(10)));

        EXPECT_EQ("0", turbo::Duration::format(turbo::Duration::zero()));
        EXPECT_EQ("0", turbo::Duration::format(turbo::Duration::seconds(0)));
        EXPECT_EQ("0", turbo::Duration::format(turbo::Duration::nanoseconds(0)));

        EXPECT_EQ("1ns", turbo::Duration::format(turbo::Duration::nanoseconds(1)));
        EXPECT_EQ("1us", turbo::Duration::format(turbo::Duration::microseconds(1)));
        EXPECT_EQ("1ms", turbo::Duration::format(turbo::Duration::milliseconds(1)));
        EXPECT_EQ("1s", turbo::Duration::format(turbo::Duration::seconds(1)));
        EXPECT_EQ("1m", turbo::Duration::format(turbo::Duration::minutes(1)));
        EXPECT_EQ("1h", turbo::Duration::format(turbo::Duration::hours(1)));

        EXPECT_EQ("1h1m", turbo::Duration::format(turbo::Duration::hours(1) + turbo::Duration::minutes(1)));
        EXPECT_EQ("1h1s", turbo::Duration::format(turbo::Duration::hours(1) + turbo::Duration::seconds(1)));
        EXPECT_EQ("1m1s", turbo::Duration::format(turbo::Duration::minutes(1) + turbo::Duration::seconds(1)));

        EXPECT_EQ("1h0.25s",
                  turbo::Duration::format(turbo::Duration::hours(1) + turbo::Duration::milliseconds(250)));
        EXPECT_EQ("1m0.25s",
                  turbo::Duration::format(turbo::Duration::minutes(1) + turbo::Duration::milliseconds(250)));
        EXPECT_EQ("1h1m0.25s",
                  turbo::Duration::format(turbo::Duration::hours(1) + turbo::Duration::minutes(1) +
                                        turbo::Duration::milliseconds(250)));
        EXPECT_EQ("1h0.0005s",
                  turbo::Duration::format(turbo::Duration::hours(1) + turbo::Duration::microseconds(500)));
        EXPECT_EQ("1h0.0000005s",
                  turbo::Duration::format(turbo::Duration::hours(1) + turbo::Duration::nanoseconds(500)));

        // Subsecond special case.
        EXPECT_EQ("1.5ns", turbo::Duration::format(turbo::Duration::nanoseconds(1) +
                                                 turbo::Duration::nanoseconds(1) / 2));
        EXPECT_EQ("1.25ns", turbo::Duration::format(turbo::Duration::nanoseconds(1) +
                                                  turbo::Duration::nanoseconds(1) / 4));
        EXPECT_EQ("1ns", turbo::Duration::format(turbo::Duration::nanoseconds(1) +
                                               turbo::Duration::nanoseconds(1) / 9));
        EXPECT_EQ("1.2us", turbo::Duration::format(turbo::Duration::microseconds(1) +
                                                 turbo::Duration::nanoseconds(200)));
        EXPECT_EQ("1.2ms", turbo::Duration::format(turbo::Duration::milliseconds(1) +
                                                 turbo::Duration::microseconds(200)));
        EXPECT_EQ("1.0002ms", turbo::Duration::format(turbo::Duration::milliseconds(1) +
                                                    turbo::Duration::nanoseconds(200)));
        EXPECT_EQ("1.00001ms", turbo::Duration::format(turbo::Duration::milliseconds(1) +
                                                     turbo::Duration::nanoseconds(10)));
        EXPECT_EQ("1.000001ms",
                  turbo::Duration::format(turbo::Duration::milliseconds(1) + turbo::Duration::nanoseconds(1)));

        // Negative durations.
        EXPECT_EQ("-1ns", turbo::Duration::format(turbo::Duration::nanoseconds(-1)));
        EXPECT_EQ("-1us", turbo::Duration::format(turbo::Duration::microseconds(-1)));
        EXPECT_EQ("-1ms", turbo::Duration::format(turbo::Duration::milliseconds(-1)));
        EXPECT_EQ("-1s", turbo::Duration::format(turbo::Duration::seconds(-1)));
        EXPECT_EQ("-1m", turbo::Duration::format(turbo::Duration::minutes(-1)));
        EXPECT_EQ("-1h", turbo::Duration::format(turbo::Duration::hours(-1)));

        EXPECT_EQ("-1h1m",
                  turbo::Duration::format(-(turbo::Duration::hours(1) + turbo::Duration::minutes(1))));
        EXPECT_EQ("-1h1s",
                  turbo::Duration::format(-(turbo::Duration::hours(1) + turbo::Duration::seconds(1))));
        EXPECT_EQ("-1m1s",
                  turbo::Duration::format(-(turbo::Duration::minutes(1) + turbo::Duration::seconds(1))));

        EXPECT_EQ("-1ns", turbo::Duration::format(turbo::Duration::nanoseconds(-1)));
        EXPECT_EQ("-1.2us", turbo::Duration::format(
                -(turbo::Duration::microseconds(1) + turbo::Duration::nanoseconds(200))));
        EXPECT_EQ("-1.2ms", turbo::Duration::format(
                -(turbo::Duration::milliseconds(1) + turbo::Duration::microseconds(200))));
        EXPECT_EQ("-1.0002ms", turbo::Duration::format(-(turbo::Duration::milliseconds(1) +
                                                       turbo::Duration::nanoseconds(200))));
        EXPECT_EQ("-1.00001ms", turbo::Duration::format(-(turbo::Duration::milliseconds(1) +
                                                        turbo::Duration::nanoseconds(10))));
        EXPECT_EQ("-1.000001ms", turbo::Duration::format(-(turbo::Duration::milliseconds(1) +
                                                         turbo::Duration::nanoseconds(1))));

        //
        // Interesting corner cases.
        //

        const turbo::Duration qns = turbo::Duration::nanoseconds(1) / 4;
        const turbo::Duration max_dur =
                turbo::Duration::seconds(kint64max) + (turbo::Duration::seconds(1) - qns);
        const turbo::Duration min_dur = turbo::Duration::seconds(kint64min);

        EXPECT_EQ("0.25ns", turbo::Duration::format(qns));
        EXPECT_EQ("-0.25ns", turbo::Duration::format(-qns));
        EXPECT_EQ("2562047788015215h30m7.99999999975s",
                  turbo::Duration::format(max_dur));
        EXPECT_EQ("-2562047788015215h30m8s", turbo::Duration::format(min_dur));

        // Tests printing full precision from units that print using Duration::fdiv
        EXPECT_EQ("55.00000000025s", turbo::Duration::format(turbo::Duration::seconds(55) + qns));
        EXPECT_EQ("55.00000025ms",
                  turbo::Duration::format(turbo::Duration::milliseconds(55) + qns));
        EXPECT_EQ("55.00025us", turbo::Duration::format(turbo::Duration::microseconds(55) + qns));
        EXPECT_EQ("55.25ns", turbo::Duration::format(turbo::Duration::nanoseconds(55) + qns));

        // Formatting infinity
        EXPECT_EQ("inf", turbo::Duration::format(turbo::Duration::max_infinite()));
        EXPECT_EQ("-inf", turbo::Duration::format(-turbo::Duration::max_infinite()));

        // Formatting approximately +/- 100 billion years
        const turbo::Duration huge_range = ApproxYears(100000000000);
        EXPECT_EQ("876000000000000h", turbo::Duration::format(huge_range));
        EXPECT_EQ("-876000000000000h", turbo::Duration::format(-huge_range));

        EXPECT_EQ("876000000000000h0.999999999s",
                  turbo::Duration::format(huge_range +
                                        (turbo::Duration::seconds(1) - turbo::Duration::nanoseconds(1))));
        EXPECT_EQ("876000000000000h0.9999999995s",
                  turbo::Duration::format(
                          huge_range + (turbo::Duration::seconds(1) - turbo::Duration::nanoseconds(1) / 2)));
        EXPECT_EQ("876000000000000h0.99999999975s",
                  turbo::Duration::format(
                          huge_range + (turbo::Duration::seconds(1) - turbo::Duration::nanoseconds(1) / 4)));

        EXPECT_EQ("-876000000000000h0.999999999s",
                  turbo::Duration::format(-huge_range -
                                        (turbo::Duration::seconds(1) - turbo::Duration::nanoseconds(1))));
        EXPECT_EQ("-876000000000000h0.9999999995s",
                  turbo::Duration::format(
                          -huge_range - (turbo::Duration::seconds(1) - turbo::Duration::nanoseconds(1) / 2)));
        EXPECT_EQ("-876000000000000h0.99999999975s",
                  turbo::Duration::format(
                          -huge_range - (turbo::Duration::seconds(1) - turbo::Duration::nanoseconds(1) / 4)));
    }

    TEST(Duration, Duration_parse) {
        turbo::Duration d;

        // No specified unit. Should only work for zero and infinity.
        EXPECT_TRUE(turbo::Duration::parse("0", &d));
        EXPECT_EQ(turbo::Duration::zero(), d);
        EXPECT_TRUE(turbo::Duration::parse("+0", &d));
        EXPECT_EQ(turbo::Duration::zero(), d);
        EXPECT_TRUE(turbo::Duration::parse("-0", &d));
        EXPECT_EQ(turbo::Duration::zero(), d);

        EXPECT_TRUE(turbo::Duration::parse("inf", &d));
        EXPECT_EQ(turbo::Duration::max_infinite(), d);
        EXPECT_TRUE(turbo::Duration::parse("+inf", &d));
        EXPECT_EQ(turbo::Duration::max_infinite(), d);
        EXPECT_TRUE(turbo::Duration::parse("-inf", &d));
        EXPECT_EQ(-turbo::Duration::max_infinite(), d);
        EXPECT_FALSE(turbo::Duration::parse("infBlah", &d));

        // Illegal input forms.
        EXPECT_FALSE(turbo::Duration::parse("", &d));
        EXPECT_FALSE(turbo::Duration::parse("0.0", &d));
        EXPECT_FALSE(turbo::Duration::parse(".0", &d));
        EXPECT_FALSE(turbo::Duration::parse(".", &d));
        EXPECT_FALSE(turbo::Duration::parse("01", &d));
        EXPECT_FALSE(turbo::Duration::parse("1", &d));
        EXPECT_FALSE(turbo::Duration::parse("-1", &d));
        EXPECT_FALSE(turbo::Duration::parse("2", &d));
        EXPECT_FALSE(turbo::Duration::parse("2 s", &d));
        EXPECT_FALSE(turbo::Duration::parse(".s", &d));
        EXPECT_FALSE(turbo::Duration::parse("-.s", &d));
        EXPECT_FALSE(turbo::Duration::parse("s", &d));
        EXPECT_FALSE(turbo::Duration::parse(" 2s", &d));
        EXPECT_FALSE(turbo::Duration::parse("2s ", &d));
        EXPECT_FALSE(turbo::Duration::parse(" 2s ", &d));
        EXPECT_FALSE(turbo::Duration::parse("2mt", &d));
        EXPECT_FALSE(turbo::Duration::parse("1e3s", &d));

        // One unit type.
        EXPECT_TRUE(turbo::Duration::parse("1ns", &d));
        EXPECT_EQ(turbo::Duration::nanoseconds(1), d);
        EXPECT_TRUE(turbo::Duration::parse("1us", &d));
        EXPECT_EQ(turbo::Duration::microseconds(1), d);
        EXPECT_TRUE(turbo::Duration::parse("1ms", &d));
        EXPECT_EQ(turbo::Duration::milliseconds(1), d);
        EXPECT_TRUE(turbo::Duration::parse("1s", &d));
        EXPECT_EQ(turbo::Duration::seconds(1), d);
        EXPECT_TRUE(turbo::Duration::parse("2m", &d));
        EXPECT_EQ(turbo::Duration::minutes(2), d);
        EXPECT_TRUE(turbo::Duration::parse("2h", &d));
        EXPECT_EQ(turbo::Duration::hours(2), d);

        // Huge counts of a unit.
        EXPECT_TRUE(turbo::Duration::parse("9223372036854775807us", &d));
        EXPECT_EQ(turbo::Duration::microseconds(9223372036854775807), d);
        EXPECT_TRUE(turbo::Duration::parse("-9223372036854775807us", &d));
        EXPECT_EQ(turbo::Duration::microseconds(-9223372036854775807), d);

        // Multiple units.
        EXPECT_TRUE(turbo::Duration::parse("2h3m4s", &d));
        EXPECT_EQ(turbo::Duration::hours(2) + turbo::Duration::minutes(3) + turbo::Duration::seconds(4), d);
        EXPECT_TRUE(turbo::Duration::parse("3m4s5us", &d));
        EXPECT_EQ(turbo::Duration::minutes(3) + turbo::Duration::seconds(4) + turbo::Duration::microseconds(5), d);
        EXPECT_TRUE(turbo::Duration::parse("2h3m4s5ms6us7ns", &d));
        EXPECT_EQ(turbo::Duration::hours(2) + turbo::Duration::minutes(3) + turbo::Duration::seconds(4) +
                  turbo::Duration::milliseconds(5) + turbo::Duration::microseconds(6) +
                  turbo::Duration::nanoseconds(7),
                  d);

        // Multiple units out of order.
        EXPECT_TRUE(turbo::Duration::parse("2us3m4s5h", &d));
        EXPECT_EQ(turbo::Duration::hours(5) + turbo::Duration::minutes(3) + turbo::Duration::seconds(4) +
                  turbo::Duration::microseconds(2),
                  d);

        // Fractional values of units.
        EXPECT_TRUE(turbo::Duration::parse("1.5ns", &d));
        EXPECT_EQ(1.5 * turbo::Duration::nanoseconds(1), d);
        EXPECT_TRUE(turbo::Duration::parse("1.5us", &d));
        EXPECT_EQ(1.5 * turbo::Duration::microseconds(1), d);
        EXPECT_TRUE(turbo::Duration::parse("1.5ms", &d));
        EXPECT_EQ(1.5 * turbo::Duration::milliseconds(1), d);
        EXPECT_TRUE(turbo::Duration::parse("1.5s", &d));
        EXPECT_EQ(1.5 * turbo::Duration::seconds(1), d);
        EXPECT_TRUE(turbo::Duration::parse("1.5m", &d));
        EXPECT_EQ(1.5 * turbo::Duration::minutes(1), d);
        EXPECT_TRUE(turbo::Duration::parse("1.5h", &d));
        EXPECT_EQ(1.5 * turbo::Duration::hours(1), d);

        // Huge fractional counts of a unit.
        EXPECT_TRUE(turbo::Duration::parse("0.4294967295s", &d));
        EXPECT_EQ(turbo::Duration::nanoseconds(429496729) + turbo::Duration::nanoseconds(1) / 2, d);
        EXPECT_TRUE(turbo::Duration::parse("0.429496729501234567890123456789s", &d));
        EXPECT_EQ(turbo::Duration::nanoseconds(429496729) + turbo::Duration::nanoseconds(1) / 2, d);

        // Negative durations.
        EXPECT_TRUE(turbo::Duration::parse("-1s", &d));
        EXPECT_EQ(turbo::Duration::seconds(-1), d);
        EXPECT_TRUE(turbo::Duration::parse("-1m", &d));
        EXPECT_EQ(turbo::Duration::minutes(-1), d);
        EXPECT_TRUE(turbo::Duration::parse("-1h", &d));
        EXPECT_EQ(turbo::Duration::hours(-1), d);

        EXPECT_TRUE(turbo::Duration::parse("-1h2s", &d));
        EXPECT_EQ(-(turbo::Duration::hours(1) + turbo::Duration::seconds(2)), d);
        EXPECT_FALSE(turbo::Duration::parse("1h-2s", &d));
        EXPECT_FALSE(turbo::Duration::parse("-1h-2s", &d));
        EXPECT_FALSE(turbo::Duration::parse("-1h -2s", &d));
    }

    TEST(Duration, FormatParseRoundTrip) {
#define TEST_PARSE_ROUNDTRIP(d)                \
  do {                                         \
    std::string s = turbo::Duration::format(d);   \
    turbo::Duration dur;                        \
    EXPECT_TRUE(turbo::Duration::parse(s, &dur)); \
    EXPECT_EQ(d, dur);                         \
  } while (0)

        TEST_PARSE_ROUNDTRIP(turbo::Duration::nanoseconds(1));
        TEST_PARSE_ROUNDTRIP(turbo::Duration::microseconds(1));
        TEST_PARSE_ROUNDTRIP(turbo::Duration::milliseconds(1));
        TEST_PARSE_ROUNDTRIP(turbo::Duration::seconds(1));
        TEST_PARSE_ROUNDTRIP(turbo::Duration::minutes(1));
        TEST_PARSE_ROUNDTRIP(turbo::Duration::hours(1));
        TEST_PARSE_ROUNDTRIP(turbo::Duration::hours(1) + turbo::Duration::nanoseconds(2));

        TEST_PARSE_ROUNDTRIP(turbo::Duration::nanoseconds(-1));
        TEST_PARSE_ROUNDTRIP(turbo::Duration::microseconds(-1));
        TEST_PARSE_ROUNDTRIP(turbo::Duration::milliseconds(-1));
        TEST_PARSE_ROUNDTRIP(turbo::Duration::seconds(-1));
        TEST_PARSE_ROUNDTRIP(turbo::Duration::minutes(-1));
        TEST_PARSE_ROUNDTRIP(turbo::Duration::hours(-1));

        TEST_PARSE_ROUNDTRIP(turbo::Duration::hours(-1) + turbo::Duration::nanoseconds(2));
        TEST_PARSE_ROUNDTRIP(turbo::Duration::hours(1) + turbo::Duration::nanoseconds(-2));
        TEST_PARSE_ROUNDTRIP(turbo::Duration::hours(-1) + turbo::Duration::nanoseconds(-2));

        TEST_PARSE_ROUNDTRIP(turbo::Duration::nanoseconds(1) +
                             turbo::Duration::nanoseconds(1) / 4);  // 1.25ns

        const turbo::Duration huge_range = ApproxYears(100000000000);
        TEST_PARSE_ROUNDTRIP(huge_range);
        TEST_PARSE_ROUNDTRIP(huge_range + (turbo::Duration::seconds(1) - turbo::Duration::nanoseconds(1)));

#undef TEST_PARSE_ROUNDTRIP
    }

    TEST(Duration, turbo_stringify) {
        // Duration::format is already well tested, so just use one test case here to
        // verify that str_format("%v", d) works as expected.
        turbo::Duration d = turbo::Duration::seconds(1);
        EXPECT_EQ(turbo::str_format("%v", d), turbo::Duration::format(d));
    }

    TEST(Duration, NoPadding) {
        // Should match the size of a struct with uint32_t alignment and no padding.
        using NoPadding = std::array<uint32_t, 3>;
        EXPECT_EQ(sizeof(NoPadding), sizeof(turbo::Duration));
        EXPECT_EQ(alignof(NoPadding), alignof(turbo::Duration));
    }

}  // namespace
