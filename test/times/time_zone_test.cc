
/****************************************************************
 * Copyright (c) 2022, liyinbin
 * All rights reserved.
 * Author by liyinbin (jeff.li) lijippy@163.com
 *****************************************************************/

#include "turbo/times/internal/time_zone.h"

#include "testing/gtest_wrap.h"
#include "testing/time_util.h"
#include "turbo/times/time.h"

namespace {

    TEST(time_zone, ValueSemantics) {
        turbo::time_zone tz;
        turbo::time_zone tz2 = tz;  // Copy-construct
        EXPECT_EQ(tz, tz2);
        tz2 = tz;  // Copy-assign
        EXPECT_EQ(tz, tz2);
    }

    TEST(time_zone, Equality) {
        turbo::time_zone a, b;
        EXPECT_EQ(a, b);
        EXPECT_EQ(a.name(), b.name());

        turbo::time_zone implicit_utc;
        turbo::time_zone explicit_utc = turbo::utc_time_zone();
        EXPECT_EQ(implicit_utc, explicit_utc);
        EXPECT_EQ(implicit_utc.name(), explicit_utc.name());

        turbo::time_zone la = turbo::times_internal::load_time_zone("America/Los_Angeles");
        turbo::time_zone nyc = turbo::times_internal::load_time_zone("America/New_York");
        EXPECT_NE(la, nyc);
    }

    TEST(time_zone, CCTZConversion) {
        const turbo::times_internal::time_zone cz = turbo::times_internal::utc_time_zone();
        const turbo::time_zone tz(cz);
        EXPECT_EQ(cz, turbo::times_internal::time_zone(tz));
    }

    TEST(time_zone, DefaultTimeZones) {
        turbo::time_zone tz;
        EXPECT_EQ("UTC", turbo::time_zone().name());
        EXPECT_EQ("UTC", turbo::utc_time_zone().name());
    }

    TEST(time_zone, fixed_time_zone) {
        const turbo::time_zone tz = turbo::fixed_time_zone(123);
        const turbo::times_internal::time_zone
                cz = turbo::times_internal::fixed_time_zone(turbo::times_internal::seconds(123));
        EXPECT_EQ(tz, turbo::time_zone(cz));
    }

    TEST(time_zone, NamedTimeZones) {
        turbo::time_zone nyc = turbo::times_internal::load_time_zone("America/New_York");
        EXPECT_EQ("America/New_York", nyc.name());
        turbo::time_zone syd = turbo::times_internal::load_time_zone("Australia/Sydney");
        EXPECT_EQ("Australia/Sydney", syd.name());
        turbo::time_zone fixed = turbo::fixed_time_zone((((3 * 60) + 25) * 60) + 45);
        EXPECT_EQ("Fixed/UTC+03:25:45", fixed.name());
    }

    TEST(time_zone, Failures) {
        turbo::time_zone tz(turbo::times_internal::load_time_zone("America/Los_Angeles"));
        EXPECT_FALSE(load_time_zone("Invalid/time_zone", &tz));
        EXPECT_EQ(turbo::utc_time_zone(), tz);  // guaranteed fallback to UTC

        // Ensures that the load still fails on a subsequent attempt.
        tz = turbo::times_internal::load_time_zone("America/Los_Angeles");
        EXPECT_FALSE(load_time_zone("Invalid/time_zone", &tz));
        EXPECT_EQ(turbo::utc_time_zone(), tz);  // guaranteed fallback to UTC

        // Loading an empty std::string timezone should fail.
        tz = turbo::times_internal::load_time_zone("America/Los_Angeles");
        EXPECT_FALSE(load_time_zone("", &tz));
        EXPECT_EQ(turbo::utc_time_zone(), tz);  // guaranteed fallback to UTC


    }

/*
TEST(time_zone, local_time_zone) {
  const turbo::time_zone local_tz = turbo::local_time_zone();
  turbo::time_zone tz = turbo::times_internal::load_time_zone("localtime");
  EXPECT_EQ(tz, local_tz);
}
*/

}  // namespace
