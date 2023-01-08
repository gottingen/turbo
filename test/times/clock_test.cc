
/****************************************************************
 * Copyright (c) 2022, liyinbin
 * All rights reserved.
 * Author by liyinbin (jeff.li) lijippy@163.com
 *****************************************************************/
#include "flare/times/time.h"
#include "flare/base/profile.h"
#include <signal.h>
#include <unistd.h>
#include "testing/gtest_wrap.h"

namespace {

    TEST(time_point, Now) {
        const flare::time_point before = flare::time_point::from_unix_nanos(flare::get_current_time_nanos());
        const flare::time_point now = flare::time_now();
        const flare::time_point after = flare::time_point::from_unix_nanos(flare::get_current_time_nanos());
        EXPECT_GE(now, before);
        EXPECT_GE(after, now);
    }

    enum class AlarmPolicy {
        kWithoutAlarm, kWithAlarm
    };

    bool alarm_handler_invoked = false;

    void AlarmHandler(int signo) {
        ASSERT_EQ(signo, SIGALRM);
        alarm_handler_invoked = true;
    }


    // Does sleep_for(d) take between lower_bound and upper_bound at least
    // once between now and (now + timeout)?  If requested (and supported),
    // add an alarm for the middle of the sleep period and expect it to fire.
    bool SleepForBounded(flare::duration d, flare::duration lower_bound,
                         flare::duration upper_bound, flare::duration timeout,
                         AlarmPolicy alarm_policy, int *attempts) {
        const flare::time_point deadline = flare::time_now() + timeout;
        while (flare::time_now() < deadline) {
            sig_t old_alarm = SIG_DFL;
            if (alarm_policy == AlarmPolicy::kWithAlarm) {
                alarm_handler_invoked = false;
                old_alarm = signal(SIGALRM, AlarmHandler);
                alarm((d / 2).to_int64_seconds());
            }
            ++*attempts;
            flare::time_point start = flare::time_now();
            flare::sleep_for(d);
            flare::duration actual = flare::time_now() - start;
            if (alarm_policy == AlarmPolicy::kWithAlarm) {
                signal(SIGALRM, old_alarm);
                if (!alarm_handler_invoked)
                    continue;
            }
            if (lower_bound <= actual && actual <= upper_bound) {
                return true;  // yes, the sleep_for() was correctly bounded
            }
        }
        return false;
    }

    testing::AssertionResult AssertSleepForBounded(flare::duration d,
                                                   flare::duration early,
                                                   flare::duration late,
                                                   flare::duration timeout,
                                                   AlarmPolicy alarm_policy) {
        const flare::duration lower_bound = d - early;
        const flare::duration upper_bound = d + late;
        int attempts = 0;
        if (SleepForBounded(d, lower_bound, upper_bound, timeout, alarm_policy,
                            &attempts)) {
            return testing::AssertionSuccess();
        }
        return testing::AssertionFailure()
                << "sleep_for(" << d << ") did not return within [" << lower_bound
                << ":" << upper_bound << "] in " << attempts << " attempt"
                << (attempts == 1 ? "" : "s") << " over " << timeout
                << (alarm_policy == AlarmPolicy::kWithAlarm ? " with" : " without")
                << " an alarm";
    }

    // Tests that sleep_for() returns neither too early nor too late.
    TEST(sleep_for, Bounded) {
        const flare::duration d = flare::duration::milliseconds(2500);
        const flare::duration early = flare::duration::milliseconds(100);
        const flare::duration late = flare::duration::milliseconds(300);
        const flare::duration timeout = 48 * d;
        EXPECT_TRUE(AssertSleepForBounded(d, early, late, timeout,
                                          AlarmPolicy::kWithoutAlarm));
        EXPECT_TRUE(AssertSleepForBounded(d, early, late, timeout,
                                          AlarmPolicy::kWithAlarm));
    }

}  // namespace
