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

#include "turbo/times/clock.h"

#include "turbo/platform/port.h"

#if defined(TURBO_HAVE_ALARM)

#include <signal.h>
#include <unistd.h>

#ifdef _AIX
// sig_t is not defined in AIX.
typedef void (*sig_t)(int);
#endif
#elif defined(__linux__) || defined(__APPLE__)
#error all known Linux and Apple targets have alarm
#endif

#include "gtest/gtest.h"
#include "turbo/times/time.h"

namespace {

    TEST(Time, Now) {
        const turbo::Time before = turbo::from_unix_nanos(turbo::get_current_time_nanos());
        const turbo::Time now = turbo::time_now();
        const turbo::Time after = turbo::from_unix_nanos(turbo::get_current_time_nanos());
        EXPECT_GE(now, before);
        EXPECT_GE(after, now);
    }

    enum class AlarmPolicy {
        kWithoutAlarm, kWithAlarm
    };

#if defined(TURBO_HAVE_ALARM)
    bool alarm_handler_invoked = false;

    void AlarmHandler(int signo) {
        ASSERT_EQ(signo, SIGALRM);
        alarm_handler_invoked = true;
    }

#endif

// Does sleep_for(d) take between lower_bound and upper_bound at least
// once between now and (now + timeout)?  If requested (and supported),
// add an alarm for the middle of the sleep period and expect it to fire.
    bool SleepForBounded(turbo::Duration d, turbo::Duration lower_bound,
                         turbo::Duration upper_bound, turbo::Duration timeout,
                         AlarmPolicy alarm_policy, int *attempts) {
        const turbo::Time deadline = turbo::time_now() + timeout;
        while (turbo::time_now() < deadline) {
#if defined(TURBO_HAVE_ALARM)
            sig_t old_alarm = SIG_DFL;
            if (alarm_policy == AlarmPolicy::kWithAlarm) {
                alarm_handler_invoked = false;
                old_alarm = signal(SIGALRM, AlarmHandler);
                alarm(turbo::to_int64_seconds(d / 2));
            }
#else
            EXPECT_EQ(alarm_policy, AlarmPolicy::kWithoutAlarm);
#endif
            ++*attempts;
            turbo::Time start = turbo::time_now();
            turbo::sleep_for(d);
            turbo::Duration actual = turbo::time_now() - start;
#if defined(TURBO_HAVE_ALARM)
            if (alarm_policy == AlarmPolicy::kWithAlarm) {
                signal(SIGALRM, old_alarm);
                if (!alarm_handler_invoked) continue;
            }
#endif
            if (lower_bound <= actual && actual <= upper_bound) {
                return true;  // yes, the sleep_for() was correctly bounded
            }
        }
        return false;
    }

    testing::AssertionResult AssertSleepForBounded(turbo::Duration d,
                                                   turbo::Duration early,
                                                   turbo::Duration late,
                                                   turbo::Duration timeout,
                                                   AlarmPolicy alarm_policy) {
        const turbo::Duration lower_bound = d - early;
        const turbo::Duration upper_bound = d + late;
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
        const turbo::Duration d = turbo::milliseconds(2500);
        const turbo::Duration early = turbo::milliseconds(100);
        const turbo::Duration late = turbo::milliseconds(300);
        const turbo::Duration timeout = 48 * d;
        EXPECT_TRUE(AssertSleepForBounded(d, early, late, timeout,
                                          AlarmPolicy::kWithoutAlarm));
#if defined(TURBO_HAVE_ALARM)
        EXPECT_TRUE(AssertSleepForBounded(d, early, late, timeout,
                                          AlarmPolicy::kWithAlarm));
#endif
    }

}  // namespace
