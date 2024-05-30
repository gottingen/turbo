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

#include <turbo/synchronization/internal/kernel_timeout.h>

#include <ctime>
#include <chrono>  // NOLINT(build/c++11)
#include <limits>

#include <turbo/base/config.h>
#include <turbo/random/random.h>
#include <turbo/times/clock.h>
#include <turbo/times/time.h>
#include <gtest/gtest.h>

// Test go/btm support by randomizing the value of clock_gettime() for
// CLOCK_MONOTONIC. This works by overriding a weak symbol in glibc.
// We should be resistant to this randomization when !SupportsSteadyClock().
#if defined(__GOOGLE_GRTE_VERSION__) && \
    !defined(TURBO_HAVE_ADDRESS_SANITIZER) && \
    !defined(TURBO_HAVE_MEMORY_SANITIZER) && \
    !defined(TURBO_HAVE_THREAD_SANITIZER)
extern "C" int __clock_gettime(clockid_t c, struct timespec* ts);

extern "C" int clock_gettime(clockid_t c, struct timespec* ts) {
  if (c == CLOCK_MONOTONIC &&
      !turbo::synchronization_internal::KernelTimeout::SupportsSteadyClock()) {
    thread_local turbo::BitGen gen;  // NOLINT
    ts->tv_sec = turbo::Uniform(gen, 0, 1'000'000'000);
    ts->tv_nsec = turbo::Uniform(gen, 0, 1'000'000'000);
    return 0;
  }
  return __clock_gettime(c, ts);
}
#endif

namespace {

#if defined(TURBO_HAVE_ADDRESS_SANITIZER) || \
    defined(TURBO_HAVE_MEMORY_SANITIZER) || \
    defined(TURBO_HAVE_THREAD_SANITIZER) || defined(__ANDROID__) || \
    defined(__APPLE__) || defined(_WIN32) || defined(_WIN64)
    constexpr turbo::Duration kTimingBound = turbo::Milliseconds(5);
#else
    constexpr turbo::Duration kTimingBound = turbo::Microseconds(250);
#endif

    using turbo::synchronization_internal::KernelTimeout;

    TEST(KernelTimeout, FiniteTimes) {
        constexpr turbo::Duration kDurationsToTest[] = {
                turbo::Duration::zero(),
                turbo::Nanoseconds(1),
                turbo::Microseconds(1),
                turbo::Milliseconds(1),
                turbo::Seconds(1),
                turbo::Minutes(1),
                turbo::Hours(1),
                turbo::Hours(1000),
                -turbo::Nanoseconds(1),
                -turbo::Microseconds(1),
                -turbo::Milliseconds(1),
                -turbo::Seconds(1),
                -turbo::Minutes(1),
                -turbo::Hours(1),
                -turbo::Hours(1000),
        };

        for (auto duration: kDurationsToTest) {
            const turbo::Time now = turbo::Time::current_time();
            const turbo::Time when = now + duration;
            SCOPED_TRACE(duration);
            KernelTimeout t(when);
            EXPECT_TRUE(t.has_timeout());
            EXPECT_TRUE(t.is_absolute_timeout());
            EXPECT_FALSE(t.is_relative_timeout());
            EXPECT_EQ(turbo::Time::from_timespec(t.MakeAbsTimespec()), when);
#ifndef _WIN32
            EXPECT_LE(
                    turbo::Duration::abs(turbo::Time::current_time() + duration -
                                       turbo::Time::from_timespec(
                                               t.MakeClockAbsoluteTimespec(CLOCK_REALTIME))),
                    turbo::Milliseconds(10));
#endif
            EXPECT_LE(
                    turbo::Duration::abs(turbo::DurationFromTimespec(t.MakeRelativeTimespec()) -
                                       std::max(duration, turbo::Duration::zero())),
                    kTimingBound);
            EXPECT_EQ(turbo::Time::from_nanoseconds(t.MakeAbsNanos()), when);
            EXPECT_LE(turbo::Duration::abs(turbo::Milliseconds(t.InMillisecondsFromNow()) -
                                         std::max(duration, turbo::Duration::zero())),
                      turbo::Milliseconds(5));
            EXPECT_LE(turbo::Duration::abs(turbo::Time::from_chrono(t.ToChronoTimePoint()) - when),
                      turbo::Microseconds(1));
            EXPECT_LE(turbo::Duration::abs(turbo::Duration::from_chrono(t.ToChronoDuration()) -
                                         std::max(duration, turbo::Duration::zero())),
                      kTimingBound);
        }
    }

    TEST(KernelTimeout, InfiniteFuture) {
        KernelTimeout t(turbo::Time::future_infinite());
        EXPECT_FALSE(t.has_timeout());
        // Callers are expected to check has_timeout() instead of using the methods
        // below, but we do try to do something reasonable if they don't. We may not
        // be able to round-trip back to turbo::Duration::max_infinite() or
        // turbo::Time::future_infinite(), but we should return a very large value.
        EXPECT_GT(turbo::Time::from_timespec(t.MakeAbsTimespec()),
                  turbo::Time::current_time() + turbo::Hours(100000));
#ifndef _WIN32
        EXPECT_GT(turbo::Time::from_timespec(t.MakeClockAbsoluteTimespec(CLOCK_REALTIME)),
                  turbo::Time::current_time() + turbo::Hours(100000));
#endif
        EXPECT_GT(turbo::DurationFromTimespec(t.MakeRelativeTimespec()),
                  turbo::Hours(100000));
        EXPECT_GT(turbo::Time::from_nanoseconds(t.MakeAbsNanos()),
                  turbo::Time::current_time() + turbo::Hours(100000));
        EXPECT_EQ(t.InMillisecondsFromNow(),
                  std::numeric_limits<KernelTimeout::DWord>::max());
        EXPECT_EQ(t.ToChronoTimePoint(),
                  std::chrono::time_point<std::chrono::system_clock>::max());
        EXPECT_GE(t.ToChronoDuration(), std::chrono::nanoseconds::max());
    }

    TEST(KernelTimeout, DefaultConstructor) {
        // The default constructor is equivalent to turbo::Time::future_infinite().
        KernelTimeout t;
        EXPECT_FALSE(t.has_timeout());
        // Callers are expected to check has_timeout() instead of using the methods
        // below, but we do try to do something reasonable if they don't. We may not
        // be able to round-trip back to turbo::Duration::max_infinite() or
        // turbo::Time::future_infinite(), but we should return a very large value.
        EXPECT_GT(turbo::Time::from_timespec(t.MakeAbsTimespec()),
                  turbo::Time::current_time() + turbo::Hours(100000));
#ifndef _WIN32
        EXPECT_GT(turbo::Time::from_timespec(t.MakeClockAbsoluteTimespec(CLOCK_REALTIME)),
                  turbo::Time::current_time() + turbo::Hours(100000));
#endif
        EXPECT_GT(turbo::DurationFromTimespec(t.MakeRelativeTimespec()),
                  turbo::Hours(100000));
        EXPECT_GT(turbo::Time::from_nanoseconds(t.MakeAbsNanos()),
                  turbo::Time::current_time() + turbo::Hours(100000));
        EXPECT_EQ(t.InMillisecondsFromNow(),
                  std::numeric_limits<KernelTimeout::DWord>::max());
        EXPECT_EQ(t.ToChronoTimePoint(),
                  std::chrono::time_point<std::chrono::system_clock>::max());
        EXPECT_GE(t.ToChronoDuration(), std::chrono::nanoseconds::max());
    }

    TEST(KernelTimeout, TimeMaxNanos) {
        // Time >= kMaxNanos should behave as no timeout.
        KernelTimeout t(turbo::Time::from_nanoseconds(std::numeric_limits<int64_t>::max()));
        EXPECT_FALSE(t.has_timeout());
        // Callers are expected to check has_timeout() instead of using the methods
        // below, but we do try to do something reasonable if they don't. We may not
        // be able to round-trip back to turbo::Duration::max_infinite() or
        // turbo::Time::future_infinite(), but we should return a very large value.
        EXPECT_GT(turbo::Time::from_timespec(t.MakeAbsTimespec()),
                  turbo::Time::current_time() + turbo::Hours(100000));
#ifndef _WIN32
        EXPECT_GT(turbo::Time::from_timespec(t.MakeClockAbsoluteTimespec(CLOCK_REALTIME)),
                  turbo::Time::current_time() + turbo::Hours(100000));
#endif
        EXPECT_GT(turbo::DurationFromTimespec(t.MakeRelativeTimespec()),
                  turbo::Hours(100000));
        EXPECT_GT(turbo::Time::from_nanoseconds(t.MakeAbsNanos()),
                  turbo::Time::current_time() + turbo::Hours(100000));
        EXPECT_EQ(t.InMillisecondsFromNow(),
                  std::numeric_limits<KernelTimeout::DWord>::max());
        EXPECT_EQ(t.ToChronoTimePoint(),
                  std::chrono::time_point<std::chrono::system_clock>::max());
        EXPECT_GE(t.ToChronoDuration(), std::chrono::nanoseconds::max());
    }

    TEST(KernelTimeout, Never) {
        // KernelTimeout::Never() is equivalent to turbo::Time::future_infinite().
        KernelTimeout t = KernelTimeout::Never();
        EXPECT_FALSE(t.has_timeout());
        // Callers are expected to check has_timeout() instead of using the methods
        // below, but we do try to do something reasonable if they don't. We may not
        // be able to round-trip back to turbo::Duration::max_infinite() or
        // turbo::Time::future_infinite(), but we should return a very large value.
        EXPECT_GT(turbo::Time::from_timespec(t.MakeAbsTimespec()),
                  turbo::Time::current_time() + turbo::Hours(100000));
#ifndef _WIN32
        EXPECT_GT(turbo::Time::from_timespec(t.MakeClockAbsoluteTimespec(CLOCK_REALTIME)),
                  turbo::Time::current_time() + turbo::Hours(100000));
#endif
        EXPECT_GT(turbo::DurationFromTimespec(t.MakeRelativeTimespec()),
                  turbo::Hours(100000));
        EXPECT_GT(turbo::Time::from_nanoseconds(t.MakeAbsNanos()),
                  turbo::Time::current_time() + turbo::Hours(100000));
        EXPECT_EQ(t.InMillisecondsFromNow(),
                  std::numeric_limits<KernelTimeout::DWord>::max());
        EXPECT_EQ(t.ToChronoTimePoint(),
                  std::chrono::time_point<std::chrono::system_clock>::max());
        EXPECT_GE(t.ToChronoDuration(), std::chrono::nanoseconds::max());
    }

    TEST(KernelTimeout, PastInfinite) {
        KernelTimeout t(turbo::Time::past_infinite());
        EXPECT_TRUE(t.has_timeout());
        EXPECT_TRUE(t.is_absolute_timeout());
        EXPECT_FALSE(t.is_relative_timeout());
        EXPECT_LE(turbo::Time::from_timespec(t.MakeAbsTimespec()),
                  turbo::Time::from_nanoseconds(1));
#ifndef _WIN32
        EXPECT_LE(turbo::Time::from_timespec(t.MakeClockAbsoluteTimespec(CLOCK_REALTIME)),
                  turbo::Time::from_seconds(1));
#endif
        EXPECT_EQ(turbo::DurationFromTimespec(t.MakeRelativeTimespec()),
                  turbo::Duration::zero());
        EXPECT_LE(turbo::Time::from_nanoseconds(t.MakeAbsNanos()), turbo::Time::from_nanoseconds(1));
        EXPECT_EQ(t.InMillisecondsFromNow(), KernelTimeout::DWord{0});
        EXPECT_LT(t.ToChronoTimePoint(), std::chrono::system_clock::from_time_t(0) +
                                         std::chrono::seconds(1));
        EXPECT_EQ(t.ToChronoDuration(), std::chrono::nanoseconds(0));
    }

    TEST(KernelTimeout, FiniteDurations) {
        constexpr turbo::Duration kDurationsToTest[] = {
                turbo::Duration::zero(),
                turbo::Nanoseconds(1),
                turbo::Microseconds(1),
                turbo::Milliseconds(1),
                turbo::Seconds(1),
                turbo::Minutes(1),
                turbo::Hours(1),
                turbo::Hours(1000),
        };

        for (auto duration: kDurationsToTest) {
            SCOPED_TRACE(duration);
            KernelTimeout t(duration);
            EXPECT_TRUE(t.has_timeout());
            EXPECT_FALSE(t.is_absolute_timeout());
            EXPECT_TRUE(t.is_relative_timeout());
            EXPECT_LE(turbo::Duration::abs(turbo::Time::current_time() + duration -
                                         turbo::Time::from_timespec(t.MakeAbsTimespec())),
                      turbo::Milliseconds(5));
#ifndef _WIN32
            EXPECT_LE(
                    turbo::Duration::abs(turbo::Time::current_time() + duration -
                                       turbo::Time::from_timespec(
                                               t.MakeClockAbsoluteTimespec(CLOCK_REALTIME))),
                    turbo::Milliseconds(5));
#endif
            EXPECT_LE(
                    turbo::Duration::abs(turbo::DurationFromTimespec(t.MakeRelativeTimespec()) -
                                       duration),
                    kTimingBound);
            EXPECT_LE(turbo::Duration::abs(turbo::Time::current_time() + duration -
                                         turbo::Time::from_nanoseconds(t.MakeAbsNanos())),
                      turbo::Milliseconds(5));
            EXPECT_LE(turbo::Milliseconds(t.InMillisecondsFromNow()) - duration,
                      turbo::Milliseconds(5));
            EXPECT_LE(turbo::Duration::abs(turbo::Time::current_time() + duration -
                                         turbo::Time::from_chrono(t.ToChronoTimePoint())),
                      kTimingBound);
            EXPECT_LE(
                    turbo::Duration::abs(turbo::Duration::from_chrono(t.ToChronoDuration()) - duration),
                    kTimingBound);
        }
    }

    TEST(KernelTimeout, NegativeDurations) {
        constexpr turbo::Duration kDurationsToTest[] = {
                -turbo::Duration::zero(),
                -turbo::Nanoseconds(1),
                -turbo::Microseconds(1),
                -turbo::Milliseconds(1),
                -turbo::Seconds(1),
                -turbo::Minutes(1),
                -turbo::Hours(1),
                -turbo::Hours(1000),
                -turbo::Duration::max_infinite(),
        };

        for (auto duration: kDurationsToTest) {
            // Negative durations should all be converted to zero durations or "now".
            SCOPED_TRACE(duration);
            KernelTimeout t(duration);
            EXPECT_TRUE(t.has_timeout());
            EXPECT_FALSE(t.is_absolute_timeout());
            EXPECT_TRUE(t.is_relative_timeout());
            EXPECT_LE(turbo::Duration::abs(turbo::Time::current_time() -
                                         turbo::Time::from_timespec(t.MakeAbsTimespec())),
                      turbo::Milliseconds(5));
#ifndef _WIN32
            EXPECT_LE(turbo::Duration::abs(turbo::Time::current_time() - turbo::Time::from_timespec(
                    t.MakeClockAbsoluteTimespec(
                            CLOCK_REALTIME))),
                      turbo::Milliseconds(5));
#endif
            EXPECT_EQ(turbo::DurationFromTimespec(t.MakeRelativeTimespec()),
                      turbo::Duration::zero());
            EXPECT_LE(
                    turbo::Duration::abs(turbo::Time::current_time() - turbo::Time::from_nanoseconds(t.MakeAbsNanos())),
                    turbo::Milliseconds(5));
            EXPECT_EQ(t.InMillisecondsFromNow(), KernelTimeout::DWord{0});
            EXPECT_LE(turbo::Duration::abs(turbo::Time::current_time() -
                                         turbo::Time::from_chrono(t.ToChronoTimePoint())),
                      turbo::Milliseconds(5));
            EXPECT_EQ(t.ToChronoDuration(), std::chrono::nanoseconds(0));
        }
    }

    TEST(KernelTimeout, Infinite) {
        KernelTimeout t(turbo::Duration::max_infinite());
        EXPECT_FALSE(t.has_timeout());
        // Callers are expected to check has_timeout() instead of using the methods
        // below, but we do try to do something reasonable if they don't. We may not
        // be able to round-trip back to turbo::Duration::max_infinite() or
        // turbo::Time::future_infinite(), but we should return a very large value.
        EXPECT_GT(turbo::Time::from_timespec(t.MakeAbsTimespec()),
                  turbo::Time::current_time() + turbo::Hours(100000));
#ifndef _WIN32
        EXPECT_GT(turbo::Time::from_timespec(t.MakeClockAbsoluteTimespec(CLOCK_REALTIME)),
                  turbo::Time::current_time() + turbo::Hours(100000));
#endif
        EXPECT_GT(turbo::DurationFromTimespec(t.MakeRelativeTimespec()),
                  turbo::Hours(100000));
        EXPECT_GT(turbo::Time::from_nanoseconds(t.MakeAbsNanos()),
                  turbo::Time::current_time() + turbo::Hours(100000));
        EXPECT_EQ(t.InMillisecondsFromNow(),
                  std::numeric_limits<KernelTimeout::DWord>::max());
        EXPECT_EQ(t.ToChronoTimePoint(),
                  std::chrono::time_point<std::chrono::system_clock>::max());
        EXPECT_GE(t.ToChronoDuration(), std::chrono::nanoseconds::max());
    }

    TEST(KernelTimeout, DurationMaxNanos) {
        // Duration >= kMaxNanos should behave as no timeout.
        KernelTimeout t(turbo::Nanoseconds(std::numeric_limits<int64_t>::max()));
        EXPECT_FALSE(t.has_timeout());
        // Callers are expected to check has_timeout() instead of using the methods
        // below, but we do try to do something reasonable if they don't. We may not
        // be able to round-trip back to turbo::Duration::max_infinite() or
        // turbo::Time::future_infinite(), but we should return a very large value.
        EXPECT_GT(turbo::Time::from_timespec(t.MakeAbsTimespec()),
                  turbo::Time::current_time() + turbo::Hours(100000));
#ifndef _WIN32
        EXPECT_GT(turbo::Time::from_timespec(t.MakeClockAbsoluteTimespec(CLOCK_REALTIME)),
                  turbo::Time::current_time() + turbo::Hours(100000));
#endif
        EXPECT_GT(turbo::DurationFromTimespec(t.MakeRelativeTimespec()),
                  turbo::Hours(100000));
        EXPECT_GT(turbo::Time::from_nanoseconds(t.MakeAbsNanos()),
                  turbo::Time::current_time() + turbo::Hours(100000));
        EXPECT_EQ(t.InMillisecondsFromNow(),
                  std::numeric_limits<KernelTimeout::DWord>::max());
        EXPECT_EQ(t.ToChronoTimePoint(),
                  std::chrono::time_point<std::chrono::system_clock>::max());
        EXPECT_GE(t.ToChronoDuration(), std::chrono::nanoseconds::max());
    }

    TEST(KernelTimeout, OverflowNanos) {
        // Test what happens when KernelTimeout is constructed with an turbo::Duration
        // that would overflow now_nanos + duration.
        int64_t now_nanos = turbo::Time::to_nanoseconds(turbo::Time::current_time());
        int64_t limit = std::numeric_limits<int64_t>::max() - now_nanos;
        turbo::Duration duration = turbo::Nanoseconds(limit) + turbo::Seconds(1);
        KernelTimeout t(duration);
        // Timeouts should still be far in the future.
        EXPECT_GT(turbo::Time::from_timespec(t.MakeAbsTimespec()),
                  turbo::Time::current_time() + turbo::Hours(100000));
#ifndef _WIN32
        EXPECT_GT(turbo::Time::from_timespec(t.MakeClockAbsoluteTimespec(CLOCK_REALTIME)),
                  turbo::Time::current_time() + turbo::Hours(100000));
#endif
        EXPECT_GT(turbo::DurationFromTimespec(t.MakeRelativeTimespec()),
                  turbo::Hours(100000));
        EXPECT_GT(turbo::Time::from_nanoseconds(t.MakeAbsNanos()),
                  turbo::Time::current_time() + turbo::Hours(100000));
        EXPECT_LE(turbo::Milliseconds(t.InMillisecondsFromNow()) - duration,
                  turbo::Milliseconds(5));
        EXPECT_GT(t.ToChronoTimePoint(),
                  std::chrono::system_clock::now() + std::chrono::hours(100000));
        EXPECT_GT(t.ToChronoDuration(), std::chrono::hours(100000));
    }

}  // namespace
