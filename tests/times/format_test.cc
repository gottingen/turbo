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

#include <cstdint>
#include <limits>
#include <string>

#include <gmock/gmock.h>
#include <gtest/gtest.h>
#include <tests/times/test_util.h>
#include <turbo/times/time.h>

using testing::HasSubstr;

namespace {

// A helper that tests the given format specifier by itself, and with leading
// and trailing characters.  For example: TestFormatSpecifier(t, "%a", "Thu").
    void TestFormatSpecifier(turbo::Time t, turbo::TimeZone tz,
                             const std::string &fmt, const std::string &ans) {
        EXPECT_EQ(ans, turbo::Time::format(fmt, t, tz));
        EXPECT_EQ("xxx " + ans, turbo::Time::format("xxx " + fmt, t, tz));
        EXPECT_EQ(ans + " yyy", turbo::Time::format(fmt + " yyy", t, tz));
        EXPECT_EQ("xxx " + ans + " yyy",
                  turbo::Time::format("xxx " + fmt + " yyy", t, tz));
    }

    //
    // Testing Time::format()
    //

    TEST(FormatTime, Basics) {
        turbo::TimeZone tz = turbo::UTCTimeZone();
        turbo::Time t = turbo::Time::from_time_t(0);

        // Starts with a couple basic edge cases.
        EXPECT_EQ("", turbo::Time::format("", t, tz));
        EXPECT_EQ(" ", turbo::Time::format(" ", t, tz));
        EXPECT_EQ("  ", turbo::Time::format("  ", t, tz));
        EXPECT_EQ("xxx", turbo::Time::format("xxx", t, tz));
        std::string big(128, 'x');
        EXPECT_EQ(big, turbo::Time::format(big, t, tz));
        // Cause the 1024-byte buffer to grow.
        std::string bigger(100000, 'x');
        EXPECT_EQ(bigger, turbo::Time::format(bigger, t, tz));

        t += turbo::Hours(13) + turbo::Minutes(4) + turbo::Seconds(5);
        t += turbo::Milliseconds(6) + turbo::Microseconds(7) + turbo::Nanoseconds(8);
        EXPECT_EQ("1970-01-01", turbo::Time::format("%Y-%m-%d", t, tz));
        EXPECT_EQ("13:04:05", turbo::Time::format("%H:%M:%S", t, tz));
        EXPECT_EQ("13:04:05.006", turbo::Time::format("%H:%M:%E3S", t, tz));
        EXPECT_EQ("13:04:05.006007", turbo::Time::format("%H:%M:%E6S", t, tz));
        EXPECT_EQ("13:04:05.006007008", turbo::Time::format("%H:%M:%E9S", t, tz));
    }

    TEST(FormatTime, LocaleSpecific) {
        const turbo::TimeZone tz = turbo::UTCTimeZone();
        turbo::Time t = turbo::Time::from_time_t(0);

        TestFormatSpecifier(t, tz, "%a", "Thu");
        TestFormatSpecifier(t, tz, "%A", "Thursday");
        TestFormatSpecifier(t, tz, "%b", "Jan");
        TestFormatSpecifier(t, tz, "%B", "January");

        // %c should at least produce the numeric year and time-of-day.
        const std::string s =
                turbo::Time::format("%c", turbo::Time::from_time_t(0), turbo::UTCTimeZone());
        EXPECT_THAT(s, HasSubstr("1970"));
        EXPECT_THAT(s, HasSubstr("00:00:00"));

        TestFormatSpecifier(t, tz, "%p", "AM");
        TestFormatSpecifier(t, tz, "%x", "01/01/70");
        TestFormatSpecifier(t, tz, "%X", "00:00:00");
    }

    TEST(FormatTime, ExtendedSeconds) {
        const turbo::TimeZone tz = turbo::UTCTimeZone();

        // No subseconds.
        turbo::Time t = turbo::Time::from_time_t(0) + turbo::Seconds(5);
        EXPECT_EQ("05", turbo::Time::format("%E*S", t, tz));
        EXPECT_EQ("05.000000000000000", turbo::Time::format("%E15S", t, tz));

        // With subseconds.
        t += turbo::Milliseconds(6) + turbo::Microseconds(7) + turbo::Nanoseconds(8);
        EXPECT_EQ("05.006007008", turbo::Time::format("%E*S", t, tz));
        EXPECT_EQ("05", turbo::Time::format("%E0S", t, tz));
        EXPECT_EQ("05.006007008000000", turbo::Time::format("%E15S", t, tz));

        // Times before the Unix epoch.
        t = turbo::Time::from_microseconds(-1);
        EXPECT_EQ("1969-12-31 23:59:59.999999",
                  turbo::Time::format("%Y-%m-%d %H:%M:%E*S", t, tz));

        // Here is a "%E*S" case we got wrong for a while.  While the first
        // instant below is correctly rendered as "...:07.333304", the second
        // one used to appear as "...:07.33330499999999999".
        t = turbo::Time::from_microseconds(1395024427333304);
        EXPECT_EQ("2014-03-17 02:47:07.333304",
                  turbo::Time::format("%Y-%m-%d %H:%M:%E*S", t, tz));
        t += turbo::Microseconds(1);
        EXPECT_EQ("2014-03-17 02:47:07.333305",
                  turbo::Time::format("%Y-%m-%d %H:%M:%E*S", t, tz));
    }

    TEST(FormatTime, RFC1123FormatPadsYear) {  // locale specific
        turbo::TimeZone tz = turbo::UTCTimeZone();

        // A year of 77 should be padded to 0077.
        turbo::Time t = turbo::Time::from_civil(turbo::CivilSecond(77, 6, 28, 9, 8, 7), tz);
        EXPECT_EQ("Mon, 28 Jun 0077 09:08:07 +0000",
                  turbo::Time::format(turbo::RFC1123_full, t, tz));
        EXPECT_EQ("28 Jun 0077 09:08:07 +0000",
                  turbo::Time::format(turbo::RFC1123_no_wday, t, tz));
    }

    TEST(FormatTime, InfiniteTime) {
        turbo::TimeZone tz = turbo::time_internal::LoadTimeZone("America/Los_Angeles");

        // The format and timezone are ignored.
        EXPECT_EQ("infinite-future",
                  turbo::Time::format("%H:%M blah", turbo::Time::future_infinite(), tz));
        EXPECT_EQ("infinite-past",
                  turbo::Time::format("%H:%M blah", turbo::Time::past_infinite(), tz));
    }

//
// Testing Time::parse()
//

    TEST(parse_time, Basics) {
        turbo::Time t = turbo::Time::from_time_t(1234567890);
        std::string err;

        // Simple edge cases.
        EXPECT_TRUE(turbo::Time::parse("", "", &t, &err)) << err;
        EXPECT_EQ(turbo::Time::from_unix_epoch(), t);  // everything defaulted
        EXPECT_TRUE(turbo::Time::parse(" ", " ", &t, &err)) << err;
        EXPECT_TRUE(turbo::Time::parse("  ", "  ", &t, &err)) << err;
        EXPECT_TRUE(turbo::Time::parse("x", "x", &t, &err)) << err;
        EXPECT_TRUE(turbo::Time::parse("xxx", "xxx", &t, &err)) << err;

        EXPECT_TRUE(turbo::Time::parse("%Y-%m-%d %H:%M:%S %z",
                                     "2013-06-28 19:08:09 -0800", &t, &err))
                            << err;
        const auto ci = turbo::FixedTimeZone(-8 * 60 * 60).At(t);
        EXPECT_EQ(turbo::CivilSecond(2013, 6, 28, 19, 8, 9), ci.cs);
        EXPECT_EQ(turbo::ZeroDuration(), ci.subsecond);
    }

    TEST(parse_time, NullErrorString) {
        turbo::Time t;
        EXPECT_FALSE(turbo::Time::parse("%Q", "invalid format", &t, nullptr));
        EXPECT_FALSE(turbo::Time::parse("%H", "12 trailing data", &t, nullptr));
        EXPECT_FALSE(
                turbo::Time::parse("%H out of range", "42 out of range", &t, nullptr));
    }

    TEST(ParseTime, WithTimeZone) {
        const turbo::TimeZone tz =
                turbo::time_internal::LoadTimeZone("America/Los_Angeles");
        turbo::Time t;
        std::string e;

        // We can parse a string without a UTC offset if we supply a timezone.
        EXPECT_TRUE(
                turbo::Time::parse("%Y-%m-%d %H:%M:%S", "2013-06-28 19:08:09", tz, &t, &e))
                            << e;
        auto ci = tz.At(t);
        EXPECT_EQ(turbo::CivilSecond(2013, 6, 28, 19, 8, 9), ci.cs);
        EXPECT_EQ(turbo::ZeroDuration(), ci.subsecond);

        // But the timezone is ignored when a UTC offset is present.
        EXPECT_TRUE(turbo::Time::parse("%Y-%m-%d %H:%M:%S %z",
                                     "2013-06-28 19:08:09 +0800", tz, &t, &e))
                            << e;
        ci = turbo::FixedTimeZone(8 * 60 * 60).At(t);
        EXPECT_EQ(turbo::CivilSecond(2013, 6, 28, 19, 8, 9), ci.cs);
        EXPECT_EQ(turbo::ZeroDuration(), ci.subsecond);
    }

    TEST(ParseTime, ErrorCases) {
        turbo::Time t = turbo::Time::from_time_t(0);
        std::string err;

        EXPECT_FALSE(turbo::Time::parse("%S", "123", &t, &err)) << err;
        EXPECT_THAT(err, HasSubstr("Illegal trailing data"));

        // Can't parse an illegal format specifier.
        err.clear();
        EXPECT_FALSE(turbo::Time::parse("%Q", "x", &t, &err)) << err;
        // Exact contents of "err" are platform-dependent because of
        // differences in the strptime implementation between macOS and Linux.
        EXPECT_FALSE(err.empty());

        // Fails because of trailing, unparsed data "blah".
        EXPECT_FALSE(turbo::Time::parse("%m-%d", "2-3 blah", &t, &err)) << err;
        EXPECT_THAT(err, HasSubstr("Illegal trailing data"));

        // Feb 31 requires normalization.
        EXPECT_FALSE(turbo::Time::parse("%m-%d", "2-31", &t, &err)) << err;
        EXPECT_THAT(err, HasSubstr("Out-of-range"));

        // Check that we cannot have spaces in UTC offsets.
        EXPECT_TRUE(turbo::Time::parse("%z", "-0203", &t, &err)) << err;
        EXPECT_FALSE(turbo::Time::parse("%z", "- 2 3", &t, &err)) << err;
        EXPECT_THAT(err, HasSubstr("Failed to parse"));
        EXPECT_TRUE(turbo::Time::parse("%Ez", "-02:03", &t, &err)) << err;
        EXPECT_FALSE(turbo::Time::parse("%Ez", "- 2: 3", &t, &err)) << err;
        EXPECT_THAT(err, HasSubstr("Failed to parse"));

        // Check that we reject other malformed UTC offsets.
        EXPECT_FALSE(turbo::Time::parse("%Ez", "+-08:00", &t, &err)) << err;
        EXPECT_THAT(err, HasSubstr("Failed to parse"));
        EXPECT_FALSE(turbo::Time::parse("%Ez", "-+08:00", &t, &err)) << err;
        EXPECT_THAT(err, HasSubstr("Failed to parse"));

        // Check that we do not accept "-0" in fields that allow zero.
        EXPECT_FALSE(turbo::Time::parse("%Y", "-0", &t, &err)) << err;
        EXPECT_THAT(err, HasSubstr("Failed to parse"));
        EXPECT_FALSE(turbo::Time::parse("%E4Y", "-0", &t, &err)) << err;
        EXPECT_THAT(err, HasSubstr("Failed to parse"));
        EXPECT_FALSE(turbo::Time::parse("%H", "-0", &t, &err)) << err;
        EXPECT_THAT(err, HasSubstr("Failed to parse"));
        EXPECT_FALSE(turbo::Time::parse("%M", "-0", &t, &err)) << err;
        EXPECT_THAT(err, HasSubstr("Failed to parse"));
        EXPECT_FALSE(turbo::Time::parse("%S", "-0", &t, &err)) << err;
        EXPECT_THAT(err, HasSubstr("Failed to parse"));
        EXPECT_FALSE(turbo::Time::parse("%z", "+-000", &t, &err)) << err;
        EXPECT_THAT(err, HasSubstr("Failed to parse"));
        EXPECT_FALSE(turbo::Time::parse("%Ez", "+-0:00", &t, &err)) << err;
        EXPECT_THAT(err, HasSubstr("Failed to parse"));
        EXPECT_FALSE(turbo::Time::parse("%z", "-00-0", &t, &err)) << err;
        EXPECT_THAT(err, HasSubstr("Illegal trailing data"));
        EXPECT_FALSE(turbo::Time::parse("%Ez", "-00:-0", &t, &err)) << err;
        EXPECT_THAT(err, HasSubstr("Illegal trailing data"));
    }

    TEST(ParseTime, ExtendedSeconds) {
        std::string err;
        turbo::Time t;

        // Here is a "%E*S" case we got wrong for a while.  The fractional
        // part of the first instant is less than 2^31 and was correctly
        // parsed, while the second (and any subsecond field >=2^31) failed.
        t = turbo::Time::from_unix_epoch();
        EXPECT_TRUE(turbo::Time::parse("%E*S", "0.2147483647", &t, &err)) << err;
        EXPECT_EQ(turbo::Time::from_unix_epoch() + turbo::Nanoseconds(214748364) +
                  turbo::Nanoseconds(1) / 2,
                  t);
        t = turbo::Time::from_unix_epoch();
        EXPECT_TRUE(turbo::Time::parse("%E*S", "0.2147483648", &t, &err)) << err;
        EXPECT_EQ(turbo::Time::from_unix_epoch() + turbo::Nanoseconds(214748364) +
                  turbo::Nanoseconds(3) / 4,
                  t);

        // We should also be able to specify long strings of digits far
        // beyond the current resolution and have them convert the same way.
        t = turbo::Time::from_unix_epoch();
        EXPECT_TRUE(turbo::Time::parse(
                "%E*S", "0.214748364801234567890123456789012345678901234567890123456789",
                &t, &err))
                            << err;
        EXPECT_EQ(turbo::Time::from_unix_epoch() + turbo::Nanoseconds(214748364) +
                  turbo::Nanoseconds(3) / 4,
                  t);
    }

    TEST(ParseTime, ExtendedOffsetErrors) {
        std::string err;
        turbo::Time t;

        // %z against +-HHMM.
        EXPECT_FALSE(turbo::Time::parse("%z", "-123", &t, &err)) << err;
        EXPECT_THAT(err, HasSubstr("Illegal trailing data"));

        // %z against +-HH.
        EXPECT_FALSE(turbo::Time::parse("%z", "-1", &t, &err)) << err;
        EXPECT_THAT(err, HasSubstr("Failed to parse"));

        // %Ez against +-HH:MM.
        EXPECT_FALSE(turbo::Time::parse("%Ez", "-12:3", &t, &err)) << err;
        EXPECT_THAT(err, HasSubstr("Illegal trailing data"));

        // %Ez against +-HHMM.
        EXPECT_FALSE(turbo::Time::parse("%Ez", "-123", &t, &err)) << err;
        EXPECT_THAT(err, HasSubstr("Illegal trailing data"));

        // %Ez against +-HH.
        EXPECT_FALSE(turbo::Time::parse("%Ez", "-1", &t, &err)) << err;
        EXPECT_THAT(err, HasSubstr("Failed to parse"));
    }

    TEST(ParseTime, InfiniteTime) {
        turbo::Time t;
        std::string err;
        EXPECT_TRUE(turbo::Time::parse("%H:%M blah", "infinite-future", &t, &err));
        EXPECT_EQ(turbo::Time::future_infinite(), t);

        // Surrounding whitespace.
        EXPECT_TRUE(turbo::Time::parse("%H:%M blah", "  infinite-future", &t, &err));
        EXPECT_EQ(turbo::Time::future_infinite(), t);
        EXPECT_TRUE(turbo::Time::parse("%H:%M blah", "infinite-future  ", &t, &err));
        EXPECT_EQ(turbo::Time::future_infinite(), t);
        EXPECT_TRUE(turbo::Time::parse("%H:%M blah", "  infinite-future  ", &t, &err));
        EXPECT_EQ(turbo::Time::future_infinite(), t);

        EXPECT_TRUE(turbo::Time::parse("%H:%M blah", "infinite-past", &t, &err));
        EXPECT_EQ(turbo::Time::past_infinite(), t);

        // Surrounding whitespace.
        EXPECT_TRUE(turbo::Time::parse("%H:%M blah", "  infinite-past", &t, &err));
        EXPECT_EQ(turbo::Time::past_infinite(), t);
        EXPECT_TRUE(turbo::Time::parse("%H:%M blah", "infinite-past  ", &t, &err));
        EXPECT_EQ(turbo::Time::past_infinite(), t);
        EXPECT_TRUE(turbo::Time::parse("%H:%M blah", "  infinite-past  ", &t, &err));
        EXPECT_EQ(turbo::Time::past_infinite(), t);

        // "infinite-future" as literal string
        turbo::TimeZone tz = turbo::UTCTimeZone();
        EXPECT_TRUE(turbo::Time::parse("infinite-future %H:%M", "infinite-future 03:04",
                                     &t, &err));
        EXPECT_NE(turbo::Time::future_infinite(), t);
        EXPECT_EQ(3, tz.At(t).cs.hour());
        EXPECT_EQ(4, tz.At(t).cs.minute());

        // "infinite-past" as literal string
        EXPECT_TRUE(
                turbo::Time::parse("infinite-past %H:%M", "infinite-past 03:04", &t, &err));
        EXPECT_NE(turbo::Time::past_infinite(), t);
        EXPECT_EQ(3, tz.At(t).cs.hour());
        EXPECT_EQ(4, tz.At(t).cs.minute());

        // The input doesn't match the format.
        EXPECT_FALSE(turbo::Time::parse("infinite-future %H:%M", "03:04", &t, &err));
        EXPECT_FALSE(turbo::Time::parse("infinite-past %H:%M", "03:04", &t, &err));
    }

    TEST(ParseTime, FailsOnUnrepresentableTime) {
        const turbo::TimeZone utc = turbo::UTCTimeZone();
        turbo::Time t;
        EXPECT_FALSE(
                turbo::Time::parse("%Y-%m-%d", "-292277022657-01-27", utc, &t, nullptr));
        EXPECT_TRUE(
                turbo::Time::parse("%Y-%m-%d", "-292277022657-01-28", utc, &t, nullptr));
        EXPECT_TRUE(
                turbo::Time::parse("%Y-%m-%d", "292277026596-12-04", utc, &t, nullptr));
        EXPECT_FALSE(
                turbo::Time::parse("%Y-%m-%d", "292277026596-12-05", utc, &t, nullptr));
    }

//
// Roundtrip test for Time::format()/Time::parse().
//

    TEST(FormatParse, RoundTrip) {
        const turbo::TimeZone lax =
                turbo::time_internal::LoadTimeZone("America/Los_Angeles");
        const turbo::Time in =
                turbo::Time::from_civil(turbo::CivilSecond(1977, 6, 28, 9, 8, 7), lax);
        const turbo::Duration subseconds = turbo::Nanoseconds(654321);
        std::string err;

        // RFC3339, which renders subseconds.
        {
            turbo::Time out;
            const std::string s =
                    turbo::Time::format(turbo::RFC3339_full, in + subseconds, lax);
            EXPECT_TRUE(turbo::Time::parse(turbo::RFC3339_full, s, &out, &err))
                                << s << ": " << err;
            EXPECT_EQ(in + subseconds, out);  // RFC3339_full includes %Ez
        }

        // RFC1123, which only does whole seconds.
        {
            turbo::Time out;
            const std::string s = turbo::Time::format(turbo::RFC1123_full, in, lax);
            EXPECT_TRUE(turbo::Time::parse(turbo::RFC1123_full, s, &out, &err))
                                << s << ": " << err;
            EXPECT_EQ(in, out);  // RFC1123_full includes %z
        }

        // `turbo::Time::format()` falls back to strftime() for "%c", which appears to
        // work. On Windows, `turbo::Time::parse()` falls back to std::get_time() which
        // appears to fail on "%c" (or at least on the "%c" text produced by
        // `strftime()`). This makes it fail the round-trip test.
        //
        // Under the emscripten compiler `turbo::Time::parse() falls back to
        // `strptime()`, but that ends up using a different definition for "%c"
        // compared to `strftime()`, also causing the round-trip test to fail
        // (see https://github.com/kripken/emscripten/pull/7491).
#if !defined(_MSC_VER) && !defined(__EMSCRIPTEN__)
        // Even though we don't know what %c will produce, it should roundtrip,
        // but only in the 0-offset timezone.
        {
            turbo::Time out;
            const std::string s = turbo::Time::format("%c", in, turbo::UTCTimeZone());
            EXPECT_TRUE(turbo::Time::parse("%c", s, &out, &err)) << s << ": " << err;
            EXPECT_EQ(in, out);
        }
#endif  // !_MSC_VER && !__EMSCRIPTEN__
    }

    TEST(FormatParse, RoundTripDistantFuture) {
        const turbo::TimeZone tz = turbo::UTCTimeZone();
        const turbo::Time in =
                turbo::Time::from_seconds(std::numeric_limits<int64_t>::max());
        std::string err;

        turbo::Time out;
        const std::string s = turbo::Time::format(turbo::RFC3339_full, in, tz);
        EXPECT_TRUE(turbo::Time::parse(turbo::RFC3339_full, s, &out, &err))
                            << s << ": " << err;
        EXPECT_EQ(in, out);
    }

    TEST(FormatParse, RoundTripDistantPast) {
        const turbo::TimeZone tz = turbo::UTCTimeZone();
        const turbo::Time in =
                turbo::Time::from_seconds(std::numeric_limits<int64_t>::min());
        std::string err;

        turbo::Time out;
        const std::string s = turbo::Time::format(turbo::RFC3339_full, in, tz);
        EXPECT_TRUE(turbo::Time::parse(turbo::RFC3339_full, s, &out, &err))
                            << s << ": " << err;
        EXPECT_EQ(in, out);
    }

}  // namespace
