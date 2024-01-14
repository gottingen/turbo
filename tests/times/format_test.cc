// Copyright 2020 The Turbo Authors.
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

#include <cstdint>
#include <limits>
#include <string>

#include "turbo/testing/test.h"
#include "tests/times/test_util.h"
#include "turbo/times/time.h"
#include "turbo/strings/match.h"
namespace {

    // A helper that tests the given format specifier by itself, and with leading
    // and trailing characters.  For example: TestFormatSpecifier(t, "%a", "Thu").
    void TestFormatSpecifier(turbo::Time t, turbo::TimeZone tz,
                             const std::string &fmt, const std::string &ans) {
        REQUIRE_EQ(ans, turbo::format_time(fmt, t, tz));
        REQUIRE_EQ("xxx " + ans, turbo::format_time("xxx " + fmt, t, tz));
        REQUIRE_EQ(ans + " yyy", turbo::format_time(fmt + " yyy", t, tz));
        REQUIRE_EQ("xxx " + ans + " yyy",
                  turbo::format_time("xxx " + fmt + " yyy", t, tz));
    }

    //
    // Testing format_time()
    //

    TEST_CASE("format_time, Basics") {
        turbo::TimeZone tz = turbo::utc_time_zone();
        turbo::Time t = turbo::from_time_t(0);

        // Starts with a couple basic edge cases.
        REQUIRE_EQ("", turbo::format_time("", t, tz));
        REQUIRE_EQ(" ", turbo::format_time(" ", t, tz));
        REQUIRE_EQ("  ", turbo::format_time("  ", t, tz));
        REQUIRE_EQ("xxx", turbo::format_time("xxx", t, tz));
        std::string big(128, 'x');
        REQUIRE_EQ(big, turbo::format_time(big, t, tz));
        // Cause the 1024-byte buffer to grow.
        std::string bigger(100000, 'x');
        REQUIRE_EQ(bigger, turbo::format_time(bigger, t, tz));

        t += turbo::hours(13) + turbo::minutes(4) + turbo::seconds(5);
        t += turbo::milliseconds(6) + turbo::microseconds(7) + turbo::nanoseconds(8);
        REQUIRE_EQ("1970-01-01", turbo::format_time("%Y-%m-%d", t, tz));
        REQUIRE_EQ("13:04:05", turbo::format_time("%H:%M:%S", t, tz));
        REQUIRE_EQ("13:04:05.006", turbo::format_time("%H:%M:%E3S", t, tz));
        REQUIRE_EQ("13:04:05.006007", turbo::format_time("%H:%M:%E6S", t, tz));
        REQUIRE_EQ("13:04:05.006007008", turbo::format_time("%H:%M:%E9S", t, tz));
    }

    TEST_CASE("format_time, LocaleSpecific") {
        const turbo::TimeZone tz = turbo::utc_time_zone();
        turbo::Time t = turbo::from_time_t(0);

        TestFormatSpecifier(t, tz, "%a", "Thu");
        TestFormatSpecifier(t, tz, "%A", "Thursday");
        TestFormatSpecifier(t, tz, "%b", "Jan");
        TestFormatSpecifier(t, tz, "%B", "January");

        // %c should at least produce the numeric year and time-of-day.
        const std::string s =
                turbo::format_time("%c", turbo::from_time_t(0), turbo::utc_time_zone());
        REQUIRE(turbo::str_contains(s, "1970"));
        REQUIRE(turbo::str_contains(s, "00:00:00"));

        TestFormatSpecifier(t, tz, "%p", "AM");
        TestFormatSpecifier(t, tz, "%x", "01/01/70");
        TestFormatSpecifier(t, tz, "%X", "00:00:00");
    }

    TEST_CASE("format_time, ExtendedSeconds") {
        const turbo::TimeZone tz = turbo::utc_time_zone();

        // No subseconds.
        turbo::Time t = turbo::from_time_t(0) + turbo::seconds(5);
        REQUIRE_EQ("05", turbo::format_time("%E*S", t, tz));
        REQUIRE_EQ("05.000000000000000", turbo::format_time("%E15S", t, tz));

        // With subseconds.
        t += turbo::milliseconds(6) + turbo::microseconds(7) + turbo::nanoseconds(8);
        REQUIRE_EQ("05.006007008", turbo::format_time("%E*S", t, tz));
        REQUIRE_EQ("05", turbo::format_time("%E0S", t, tz));
        REQUIRE_EQ("05.006007008000000", turbo::format_time("%E15S", t, tz));

        // Times before the Unix epoch.
        t = turbo::from_unix_micros(-1);
        REQUIRE_EQ("1969-12-31 23:59:59.999999",
                  turbo::format_time("%Y-%m-%d %H:%M:%E*S", t, tz));

        // Here is a "%E*S" case we got wrong for a while.  While the first
        // instant below is correctly rendered as "...:07.333304", the second
        // one used to appear as "...:07.33330499999999999".
        t = turbo::from_unix_micros(1395024427333304);
        REQUIRE_EQ("2014-03-17 02:47:07.333304",
                  turbo::format_time("%Y-%m-%d %H:%M:%E*S", t, tz));
        t += turbo::microseconds(1);
        REQUIRE_EQ("2014-03-17 02:47:07.333305",
                  turbo::format_time("%Y-%m-%d %H:%M:%E*S", t, tz));
    }

    TEST_CASE("format_time, RFC1123FormatPadsYear") {  // locale specific
        turbo::TimeZone tz = turbo::utc_time_zone();

        // A year of 77 should be padded to 0077.
        turbo::Time t = turbo::from_civil(turbo::CivilSecond(77, 6, 28, 9, 8, 7), tz);
        REQUIRE_EQ("Mon, 28 Jun 0077 09:08:07 +0000",
                  turbo::format_time(turbo::RFC1123_full, t, tz));
        REQUIRE_EQ("28 Jun 0077 09:08:07 +0000",
                  turbo::format_time(turbo::RFC1123_no_wday, t, tz));
    }

    TEST_CASE("format_time, InfiniteTime") {
        turbo::TimeZone tz = turbo::time_internal::load_time_zone("America/Los_Angeles");

        // The format and timezone are ignored.
        REQUIRE_EQ("infinite-future",
                  turbo::format_time("%H:%M blah", turbo::infinite_future(), tz));
        REQUIRE_EQ("infinite-past",
                  turbo::format_time("%H:%M blah", turbo::infinite_past(), tz));
    }

    //
    // Testing parse_time()
    //

    TEST_CASE("parse_time, Basics") {
        turbo::Time t = turbo::from_time_t(1234567890);
        std::string err;

        // Simple edge cases.
        REQUIRE(turbo::parse_time("", "", &t, &err)) ;
        REQUIRE_EQ(turbo::unix_epoch(), t);  // everything defaulted
        REQUIRE(turbo::parse_time(" ", " ", &t, &err)) ;
        REQUIRE(turbo::parse_time("  ", "  ", &t, &err)) ;
        REQUIRE(turbo::parse_time("x", "x", &t, &err)) ;
        REQUIRE(turbo::parse_time("xxx", "xxx", &t, &err)) ;

        REQUIRE(turbo::parse_time("%Y-%m-%d %H:%M:%S %z",
                                     "2013-06-28 19:08:09 -0800", &t, &err));
        const auto ci = turbo::fixed_time_zone(-8 * 60 * 60).at(t);
        REQUIRE_EQ(turbo::CivilSecond(2013, 6, 28, 19, 8, 9), ci.cs);
        REQUIRE_EQ(turbo::zero_duration(), ci.subsecond);
    }

    TEST_CASE("parse_time, NullErrorString") {
        turbo::Time t;
        REQUIRE_FALSE(turbo::parse_time("%Q", "invalid format", &t, nullptr));
        REQUIRE_FALSE(turbo::parse_time("%H", "12 trailing data", &t, nullptr));
        REQUIRE_FALSE(
                turbo::parse_time("%H out of range", "42 out of range", &t, nullptr));
    }

    TEST_CASE("parse_time, WithTimeZone") {
        const turbo::TimeZone tz =
                turbo::time_internal::load_time_zone("America/Los_Angeles");
        turbo::Time t;
        std::string e;

        // We can parse a string without a UTC offset if we supply a timezone.
        REQUIRE(
                turbo::parse_time("%Y-%m-%d %H:%M:%S", "2013-06-28 19:08:09", tz, &t, &e))
                            ;
        auto ci = tz.at(t);
        REQUIRE_EQ(turbo::CivilSecond(2013, 6, 28, 19, 8, 9), ci.cs);
        REQUIRE_EQ(turbo::zero_duration(), ci.subsecond);

        // But the timezone is ignored when a UTC offset is present.
        REQUIRE(turbo::parse_time("%Y-%m-%d %H:%M:%S %z",
                                     "2013-06-28 19:08:09 +0800", tz, &t, &e))
                            ;
        ci = turbo::fixed_time_zone(8 * 60 * 60).at(t);
        REQUIRE_EQ(turbo::CivilSecond(2013, 6, 28, 19, 8, 9), ci.cs);
        REQUIRE_EQ(turbo::zero_duration(), ci.subsecond);
    }

    TEST_CASE("parse_time, ErrorCases") {
        turbo::Time t = turbo::from_time_t(0);
        std::string err;

        REQUIRE_FALSE(turbo::parse_time("%S", "123", &t, &err)) ;
        REQUIRE(turbo::str_contains(err, "Illegal trailing data"));

        // Can't parse an illegal format specifier.
        err.clear();
        REQUIRE_FALSE(turbo::parse_time("%Q", "x", &t, &err)) ;
        // Exact contents of "err" are platform-dependent because of
        // differences in the strptime implementation between macOS and Linux.
        REQUIRE_FALSE(err.empty());

        // Fails because of trailing, unparsed data "blah".
        REQUIRE_FALSE(turbo::parse_time("%m-%d", "2-3 blah", &t, &err)) ;
        REQUIRE(turbo::str_contains(err, "Illegal trailing data"));

        // Feb 31 requires normalization.
        REQUIRE_FALSE(turbo::parse_time("%m-%d", "2-31", &t, &err)) ;
        REQUIRE(turbo::str_contains(err, "Out-of-range"));

        // Check that we cannot have spaces in UTC offsets.
        REQUIRE(turbo::parse_time("%z", "-0203", &t, &err)) ;
        REQUIRE_FALSE(turbo::parse_time("%z", "- 2 3", &t, &err)) ;
        REQUIRE(turbo::str_contains(err, "Failed to parse"));
        REQUIRE(turbo::parse_time("%Ez", "-02:03", &t, &err)) ;
        REQUIRE_FALSE(turbo::parse_time("%Ez", "- 2: 3", &t, &err)) ;
        REQUIRE(turbo::str_contains(err, "Failed to parse"));

        // Check that we reject other malformed UTC offsets.
        REQUIRE_FALSE(turbo::parse_time("%Ez", "+-08:00", &t, &err)) ;
        REQUIRE(turbo::str_contains(err, "Failed to parse"));
        REQUIRE_FALSE(turbo::parse_time("%Ez", "-+08:00", &t, &err)) ;
        REQUIRE(turbo::str_contains(err, "Failed to parse"));

        // Check that we do not accept "-0" in fields that allow zero.
        REQUIRE_FALSE(turbo::parse_time("%Y", "-0", &t, &err)) ;
        REQUIRE(turbo::str_contains(err, "Failed to parse"));
        REQUIRE_FALSE(turbo::parse_time("%E4Y", "-0", &t, &err)) ;
        REQUIRE(turbo::str_contains(err, "Failed to parse"));
        REQUIRE_FALSE(turbo::parse_time("%H", "-0", &t, &err)) ;
        REQUIRE(turbo::str_contains(err, "Failed to parse"));
        REQUIRE_FALSE(turbo::parse_time("%M", "-0", &t, &err)) ;
        REQUIRE(turbo::str_contains(err, "Failed to parse"));
        REQUIRE_FALSE(turbo::parse_time("%S", "-0", &t, &err)) ;
        REQUIRE(turbo::str_contains(err, "Failed to parse"));
        REQUIRE_FALSE(turbo::parse_time("%z", "+-000", &t, &err)) ;
        REQUIRE(turbo::str_contains(err, "Failed to parse"));
        REQUIRE_FALSE(turbo::parse_time("%Ez", "+-0:00", &t, &err)) ;
        REQUIRE(turbo::str_contains(err, "Failed to parse"));
        REQUIRE_FALSE(turbo::parse_time("%z", "-00-0", &t, &err)) ;
        REQUIRE(turbo::str_contains(err, "Illegal trailing data"));
        REQUIRE_FALSE(turbo::parse_time("%Ez", "-00:-0", &t, &err)) ;
        REQUIRE(turbo::str_contains(err, "Illegal trailing data"));
    }

    TEST_CASE("parse_time, ExtendedSeconds") {
        std::string err;
        turbo::Time t;

        // Here is a "%E*S" case we got wrong for a while.  The fractional
        // part of the first instant is less than 2^31 and was correctly
        // parsed, while the second (and any subsecond field >=2^31) failed.
        t = turbo::unix_epoch();
        REQUIRE(turbo::parse_time("%E*S", "0.2147483647", &t, &err)) ;
        REQUIRE_EQ(turbo::unix_epoch() + turbo::nanoseconds(214748364) +
                  turbo::nanoseconds(1) / 2,
                  t);
        t = turbo::unix_epoch();
        REQUIRE(turbo::parse_time("%E*S", "0.2147483648", &t, &err)) ;
        REQUIRE_EQ(turbo::unix_epoch() + turbo::nanoseconds(214748364) +
                  turbo::nanoseconds(3) / 4,
                  t);

        // We should also be able to specify long strings of digits far
        // beyond the current resolution and have them convert the same way.
        t = turbo::unix_epoch();
        REQUIRE(turbo::parse_time(
                "%E*S", "0.214748364801234567890123456789012345678901234567890123456789",
                &t, &err))
                            ;
        REQUIRE_EQ(turbo::unix_epoch() + turbo::nanoseconds(214748364) +
                  turbo::nanoseconds(3) / 4,
                  t);
    }

    TEST_CASE("parse_time, ExtendedOffsetErrors") {
        std::string err;
        turbo::Time t;

        // %z against +-HHMM.
        REQUIRE_FALSE(turbo::parse_time("%z", "-123", &t, &err)) ;
        REQUIRE(turbo::str_contains(err, "Illegal trailing data"));

        // %z against +-HH.
        REQUIRE_FALSE(turbo::parse_time("%z", "-1", &t, &err)) ;
        REQUIRE(turbo::str_contains(err, "Failed to parse"));

        // %Ez against +-HH:MM.
        REQUIRE_FALSE(turbo::parse_time("%Ez", "-12:3", &t, &err)) ;
        REQUIRE(turbo::str_contains(err, "Illegal trailing data"));

        // %Ez against +-HHMM.
        REQUIRE_FALSE(turbo::parse_time("%Ez", "-123", &t, &err)) ;
        REQUIRE(turbo::str_contains(err, "Illegal trailing data"));

        // %Ez against +-HH.
        REQUIRE_FALSE(turbo::parse_time("%Ez", "-1", &t, &err)) ;
        REQUIRE(turbo::str_contains(err, "Failed to parse"));
    }

    TEST_CASE("parse_time, InfiniteTime") {
        turbo::Time t;
        std::string err;
        REQUIRE(turbo::parse_time("%H:%M blah", "infinite-future", &t, &err));
        REQUIRE_EQ(turbo::infinite_future(), t);

        // Surrounding whitespace.
        REQUIRE(turbo::parse_time("%H:%M blah", "  infinite-future", &t, &err));
        REQUIRE_EQ(turbo::infinite_future(), t);
        REQUIRE(turbo::parse_time("%H:%M blah", "infinite-future  ", &t, &err));
        REQUIRE_EQ(turbo::infinite_future(), t);
        REQUIRE(turbo::parse_time("%H:%M blah", "  infinite-future  ", &t, &err));
        REQUIRE_EQ(turbo::infinite_future(), t);

        REQUIRE(turbo::parse_time("%H:%M blah", "infinite-past", &t, &err));
        REQUIRE_EQ(turbo::infinite_past(), t);

        // Surrounding whitespace.
        REQUIRE(turbo::parse_time("%H:%M blah", "  infinite-past", &t, &err));
        REQUIRE_EQ(turbo::infinite_past(), t);
        REQUIRE(turbo::parse_time("%H:%M blah", "infinite-past  ", &t, &err));
        REQUIRE_EQ(turbo::infinite_past(), t);
        REQUIRE(turbo::parse_time("%H:%M blah", "  infinite-past  ", &t, &err));
        REQUIRE_EQ(turbo::infinite_past(), t);

        // "infinite-future" as literal string
        turbo::TimeZone tz = turbo::utc_time_zone();
        REQUIRE(turbo::parse_time("infinite-future %H:%M", "infinite-future 03:04",
                                     &t, &err));
        REQUIRE_NE(turbo::infinite_future(), t);
        REQUIRE_EQ(3, tz.at(t).cs.hour());
        REQUIRE_EQ(4, tz.at(t).cs.minute());

        // "infinite-past" as literal string
        REQUIRE(
                turbo::parse_time("infinite-past %H:%M", "infinite-past 03:04", &t, &err));
        REQUIRE_NE(turbo::infinite_past(), t);
        REQUIRE_EQ(3, tz.at(t).cs.hour());
        REQUIRE_EQ(4, tz.at(t).cs.minute());

        // The input doesn't match the format.
        REQUIRE_FALSE(turbo::parse_time("infinite-future %H:%M", "03:04", &t, &err));
        REQUIRE_FALSE(turbo::parse_time("infinite-past %H:%M", "03:04", &t, &err));
    }

    TEST_CASE("parse_time, FailsOnUnrepresentableTime") {
        const turbo::TimeZone utc = turbo::utc_time_zone();
        turbo::Time t;
        REQUIRE_FALSE(
                turbo::parse_time("%Y-%m-%d", "-292277022657-01-27", utc, &t, nullptr));
        REQUIRE(
                turbo::parse_time("%Y-%m-%d", "-292277022657-01-28", utc, &t, nullptr));
        REQUIRE(
                turbo::parse_time("%Y-%m-%d", "292277026596-12-04", utc, &t, nullptr));
        REQUIRE_FALSE(
                turbo::parse_time("%Y-%m-%d", "292277026596-12-05", utc, &t, nullptr));
    }

//
// Roundtrip test for format_time()/parse_time().
//

    TEST_CASE("FormatParse, RoundTrip") {
        const turbo::TimeZone lax =
                turbo::time_internal::load_time_zone("America/Los_Angeles");
        const turbo::Time in =
                turbo::from_civil(turbo::CivilSecond(1977, 6, 28, 9, 8, 7), lax);
        const turbo::Duration subseconds = turbo::nanoseconds(654321);
        std::string err;

        // RFC3339, which renders subseconds.
        {
            turbo::Time out;
            const std::string s =
                    turbo::format_time(turbo::RFC3339_full, in + subseconds, lax);
            REQUIRE(turbo::parse_time(turbo::RFC3339_full, s, &out, &err));
            REQUIRE_EQ(in + subseconds, out);  // RFC3339_full includes %Ez
        }

        // RFC1123, which only does whole seconds.
        {
            turbo::Time out;
            const std::string s = turbo::format_time(turbo::RFC1123_full, in, lax);
            REQUIRE(turbo::parse_time(turbo::RFC1123_full, s, &out, &err));
            REQUIRE_EQ(in, out);  // RFC1123_full includes %z
        }

        // `turbo::format_time()` falls back to strftime() for "%c", which appears to
        // work. On Windows, `turbo::parse_time()` falls back to std::get_time() which
        // appears to fail on "%c" (or at least on the "%c" text produced by
        // `strftime()`). This makes it fail the round-trip test.
        //
        // Under the emscripten compiler `turbo::parse_time() falls back to
        // `strptime()`, but that ends up using a different definition for "%c"
        // compared to `strftime()`, also causing the round-trip test to fail
        // (see https://github.com/kripken/emscripten/pull/7491).
#if !defined(_MSC_VER) && !defined(__EMSCRIPTEN__)
        // Even though we don't know what %c will produce, it should roundtrip,
        // but only in the 0-offset timezone.
        {
            turbo::Time out;
            const std::string s = turbo::format_time("%c", in, turbo::utc_time_zone());
            REQUIRE(turbo::parse_time("%c", s, &out, &err));
            REQUIRE_EQ(in, out);
        }
#endif  // !_MSC_VER && !__EMSCRIPTEN__
    }

    TEST_CASE("FormatParse, RoundTripDistantFuture") {
        const turbo::TimeZone tz = turbo::utc_time_zone();
        const turbo::Time in =
                turbo::from_unix_seconds(std::numeric_limits<int64_t>::max());
        std::string err;

        turbo::Time out;
        const std::string s = turbo::format_time(turbo::RFC3339_full, in, tz);
        REQUIRE(turbo::parse_time(turbo::RFC3339_full, s, &out, &err));
        REQUIRE_EQ(in, out);
    }

    TEST_CASE("FormatParse, RoundTripDistantPast") {
        const turbo::TimeZone tz = turbo::utc_time_zone();
        const turbo::Time in =
                turbo::from_unix_seconds(std::numeric_limits<int64_t>::min());
        std::string err;

        turbo::Time out;
        const std::string s = turbo::format_time(turbo::RFC3339_full, in, tz);
        REQUIRE(turbo::parse_time(turbo::RFC3339_full, s, &out, &err));
        REQUIRE_EQ(in, out);
    }

}  // namespace
