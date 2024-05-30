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

#include <turbo/times/clock.h>

#include <turbo/base/config.h>

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

#include <gtest/gtest.h>
#include <turbo/times/time.h>

namespace {

    TEST(Time, Now) {
        const turbo::Time before = turbo::Time::from_nanoseconds(turbo::GetCurrentTimeNanos());
        const turbo::Time now = turbo::Time::current_time();
        const turbo::Time after = turbo::Time::from_nanoseconds(turbo::GetCurrentTimeNanos());
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
        const turbo::Time deadline = turbo::Time::current_time() + timeout;
        while (turbo::Time::current_time() < deadline) {
#if defined(TURBO_HAVE_ALARM)
            sig_t old_alarm = SIG_DFL;
            if (alarm_policy == AlarmPolicy::kWithAlarm) {
                alarm_handler_invoked = false;
                old_alarm = signal(SIGALRM, AlarmHandler);
                alarm(turbo::Duration::to_seconds(d / 2));
            }
#else
            EXPECT_EQ(alarm_policy, AlarmPolicy::kWithoutAlarm);
#endif
            ++*attempts;
            turbo::Time start = turbo::Time::current_time();
            turbo::sleep_for(d);
            turbo::Duration actual = turbo::Time::current_time() - start;
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
        const turbo::Duration d = turbo::Duration::milliseconds(2500);
        const turbo::Duration early = turbo::Duration::milliseconds(100);
        const turbo::Duration late = turbo::Duration::milliseconds(300);
        const turbo::Duration timeout = 48 * d;
        EXPECT_TRUE(AssertSleepForBounded(d, early, late, timeout,
                                          AlarmPolicy::kWithoutAlarm));
#if defined(TURBO_HAVE_ALARM)
        EXPECT_TRUE(AssertSleepForBounded(d, early, late, timeout,
                                          AlarmPolicy::kWithAlarm));
#endif
    }

}  // namespace
