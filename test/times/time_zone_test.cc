
/****************************************************************
 * Copyright (c) 2022, liyinbin
 * All rights reserved.
 * Author by liyinbin (jeff.li) lijippy@163.com
 *****************************************************************/

#include "flare/times/internal/time_zone.h"

#include "testing/gtest_wrap.h"
#include "testing/time_util.h"
#include "flare/times/time.h"

namespace {

    TEST(time_zone, ValueSemantics) {
        flare::time_zone tz;
        flare::time_zone tz2 = tz;  // Copy-construct
        EXPECT_EQ(tz, tz2);
        tz2 = tz;  // Copy-assign
        EXPECT_EQ(tz, tz2);
    }

    TEST(time_zone, Equality) {
        flare::time_zone a, b;
        EXPECT_EQ(a, b);
        EXPECT_EQ(a.name(), b.name());

        flare::time_zone implicit_utc;
        flare::time_zone explicit_utc = flare::utc_time_zone();
        EXPECT_EQ(implicit_utc, explicit_utc);
        EXPECT_EQ(implicit_utc.name(), explicit_utc.name());

        flare::time_zone la = flare::times_internal::load_time_zone("America/Los_Angeles");
        flare::time_zone nyc = flare::times_internal::load_time_zone("America/New_York");
        EXPECT_NE(la, nyc);
    }

    TEST(time_zone, CCTZConversion) {
        const flare::times_internal::time_zone cz = flare::times_internal::utc_time_zone();
        const flare::time_zone tz(cz);
        EXPECT_EQ(cz, flare::times_internal::time_zone(tz));
    }

    TEST(time_zone, DefaultTimeZones) {
        flare::time_zone tz;
        EXPECT_EQ("UTC", flare::time_zone().name());
        EXPECT_EQ("UTC", flare::utc_time_zone().name());
    }

    TEST(time_zone, fixed_time_zone) {
        const flare::time_zone tz = flare::fixed_time_zone(123);
        const flare::times_internal::time_zone
                cz = flare::times_internal::fixed_time_zone(flare::times_internal::seconds(123));
        EXPECT_EQ(tz, flare::time_zone(cz));
    }

    TEST(time_zone, NamedTimeZones) {
        flare::time_zone nyc = flare::times_internal::load_time_zone("America/New_York");
        EXPECT_EQ("America/New_York", nyc.name());
        flare::time_zone syd = flare::times_internal::load_time_zone("Australia/Sydney");
        EXPECT_EQ("Australia/Sydney", syd.name());
        flare::time_zone fixed = flare::fixed_time_zone((((3 * 60) + 25) * 60) + 45);
        EXPECT_EQ("Fixed/UTC+03:25:45", fixed.name());
    }

    TEST(time_zone, Failures) {
        flare::time_zone tz(flare::times_internal::load_time_zone("America/Los_Angeles"));
        EXPECT_FALSE(load_time_zone("Invalid/time_zone", &tz));
        EXPECT_EQ(flare::utc_time_zone(), tz);  // guaranteed fallback to UTC

        // Ensures that the load still fails on a subsequent attempt.
        tz = flare::times_internal::load_time_zone("America/Los_Angeles");
        EXPECT_FALSE(load_time_zone("Invalid/time_zone", &tz));
        EXPECT_EQ(flare::utc_time_zone(), tz);  // guaranteed fallback to UTC

        // Loading an empty std::string timezone should fail.
        tz = flare::times_internal::load_time_zone("America/Los_Angeles");
        EXPECT_FALSE(load_time_zone("", &tz));
        EXPECT_EQ(flare::utc_time_zone(), tz);  // guaranteed fallback to UTC


    }

/*
TEST(time_zone, local_time_zone) {
  const flare::time_zone local_tz = flare::local_time_zone();
  flare::time_zone tz = flare::times_internal::load_time_zone("localtime");
  EXPECT_EQ(tz, local_tz);
}
*/

}  // namespace
