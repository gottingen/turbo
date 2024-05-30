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

#include <turbo/times/time.h>

#if !defined(_WIN32)

#include <sys/time.h>

#endif  // _WIN32

#include <algorithm>
#include <cmath>
#include <cstddef>
#include <cstring>
#include <ctime>
#include <memory>
#include <string>

#include <turbo/times/clock.h>
#include <tests/times/test_util.h>
#include <benchmark/benchmark.h>

namespace {

//
// Addition/Subtraction of a duration
//

    void BM_Time_Arithmetic(benchmark::State &state) {
        const turbo::Duration nano = turbo::Nanoseconds(1);
        const turbo::Duration sec = turbo::Seconds(1);
        turbo::Time t = turbo::Time::from_unix_epoch();
        while (state.KeepRunning()) {
            benchmark::DoNotOptimize(t += nano);
            benchmark::DoNotOptimize(t -= sec);
        }
    }

    BENCHMARK(BM_Time_Arithmetic);

//
// Time difference
//

    void BM_Time_Difference(benchmark::State &state) {
        turbo::Time start = turbo::Time::current_time();
        turbo::Time end = start + turbo::Nanoseconds(1);
        turbo::Duration diff;
        while (state.KeepRunning()) {
            benchmark::DoNotOptimize(diff += end - start);
        }
    }

    BENCHMARK(BM_Time_Difference);

//
// ToDateTime
//
// In each "ToDateTime" benchmark we switch between two instants
// separated by at least one transition in order to defeat any
// internal caching of previous results (e.g., see local_time_hint_).
//
// The "UTC" variants use UTC instead of the Google/local time zone.
//

    void BM_Time_ToDateTime_Turbo(benchmark::State &state) {
        const turbo::TimeZone tz =
                turbo::time_internal::LoadTimeZone("America/Los_Angeles");
        turbo::Time t = turbo::Time::from_seconds(1384569027);
        turbo::Time t2 = turbo::Time::from_seconds(1418962578);
        while (state.KeepRunning()) {
            std::swap(t, t2);
            t += turbo::Seconds(1);
            benchmark::DoNotOptimize(tz.At(t));
        }
    }

    BENCHMARK(BM_Time_ToDateTime_Turbo);

    void BM_Time_ToDateTime_Libc(benchmark::State &state) {
        // No timezone support, so just use localtime.
        time_t t = 1384569027;
        time_t t2 = 1418962578;
        while (state.KeepRunning()) {
            std::swap(t, t2);
            t += 1;
            struct tm tm;
#if !defined(_WIN32)
            benchmark::DoNotOptimize(localtime_r(&t, &tm));
#else   // _WIN32
            benchmark::DoNotOptimize(localtime_s(&tm, &t));
#endif  // _WIN32
        }
    }

    BENCHMARK(BM_Time_ToDateTime_Libc);

    void BM_Time_ToDateTimeUTC_Turbo(benchmark::State &state) {
        const turbo::TimeZone tz = turbo::TimeZone::utc();
        turbo::Time t = turbo::Time::from_seconds(1384569027);
        while (state.KeepRunning()) {
            t += turbo::Seconds(1);
            benchmark::DoNotOptimize(tz.At(t));
        }
    }

    BENCHMARK(BM_Time_ToDateTimeUTC_Turbo);

    void BM_Time_ToDateTimeUTC_Libc(benchmark::State &state) {
        time_t t = 1384569027;
        while (state.KeepRunning()) {
            t += 1;
            struct tm tm;
#if !defined(_WIN32)
            benchmark::DoNotOptimize(gmtime_r(&t, &tm));
#else   // _WIN32
            benchmark::DoNotOptimize(gmtime_s(&tm, &t));
#endif  // _WIN32
        }
    }

    BENCHMARK(BM_Time_ToDateTimeUTC_Libc);

//
// Time::from_microseconds
//

    void BM_Time_FromUnixMicros(benchmark::State &state) {
        int i = 0;
        while (state.KeepRunning()) {
            benchmark::DoNotOptimize(turbo::Time::from_microseconds(i));
            ++i;
        }
    }

    BENCHMARK(BM_Time_FromUnixMicros);

    void BM_Time_ToUnixNanos(benchmark::State &state) {
        const turbo::Time t = turbo::Time::from_unix_epoch() + turbo::Seconds(123);
        while (state.KeepRunning()) {
            benchmark::DoNotOptimize(turbo::Time::to_nanoseconds(t));
        }
    }

    BENCHMARK(BM_Time_ToUnixNanos);

    void BM_Time_ToUnixMicros(benchmark::State &state) {
        const turbo::Time t = turbo::Time::from_unix_epoch() + turbo::Seconds(123);
        while (state.KeepRunning()) {
            benchmark::DoNotOptimize(turbo::Time::to_microseconds(t));
        }
    }

    BENCHMARK(BM_Time_ToUnixMicros);

    void BM_Time_ToUnixMillis(benchmark::State &state) {
        const turbo::Time t = turbo::Time::from_unix_epoch() + turbo::Seconds(123);
        while (state.KeepRunning()) {
            benchmark::DoNotOptimize(turbo::Time::to_milliseconds(t));
        }
    }

    BENCHMARK(BM_Time_ToUnixMillis);

    void BM_Time_ToUnixSeconds(benchmark::State &state) {
        const turbo::Time t = turbo::Time::from_unix_epoch() + turbo::Seconds(123);
        while (state.KeepRunning()) {
            benchmark::DoNotOptimize(turbo::Time::to_seconds(t));
        }
    }

    BENCHMARK(BM_Time_ToUnixSeconds);

//
// Time::from_civil
//
// In each "Time::from_civil" benchmark we switch between two YMDhms values
// separated by at least one transition in order to defeat any internal
// caching of previous results (e.g., see time_local_hint_).
//
// The "UTC" variants use UTC instead of the Google/local time zone.
// The "Day0" variants require normalization of the day of month.
//

    void BM_Time_FromCivil_Turbo(benchmark::State &state) {
        const turbo::TimeZone tz =
                turbo::time_internal::LoadTimeZone("America/Los_Angeles");
        int i = 0;
        while (state.KeepRunning()) {
            if ((i & 1) == 0) {
                benchmark::DoNotOptimize(
                        turbo::Time::from_civil(turbo::CivilSecond(2014, 12, 18, 20, 16, 18), tz));
            } else {
                benchmark::DoNotOptimize(
                        turbo::Time::from_civil(turbo::CivilSecond(2013, 11, 15, 18, 30, 27), tz));
            }
            ++i;
        }
    }

    BENCHMARK(BM_Time_FromCivil_Turbo);

    void BM_Time_FromCivil_Libc(benchmark::State &state) {
        // No timezone support, so just use localtime.
        int i = 0;
        while (state.KeepRunning()) {
            struct tm tm;
            if ((i & 1) == 0) {
                tm.tm_year = 2014 - 1900;
                tm.tm_mon = 12 - 1;
                tm.tm_mday = 18;
                tm.tm_hour = 20;
                tm.tm_min = 16;
                tm.tm_sec = 18;
            } else {
                tm.tm_year = 2013 - 1900;
                tm.tm_mon = 11 - 1;
                tm.tm_mday = 15;
                tm.tm_hour = 18;
                tm.tm_min = 30;
                tm.tm_sec = 27;
            }
            tm.tm_isdst = -1;
            mktime(&tm);
            ++i;
        }
    }

    BENCHMARK(BM_Time_FromCivil_Libc);

    void BM_Time_FromCivilUTC_Turbo(benchmark::State &state) {
        const turbo::TimeZone tz = turbo::TimeZone::utc();
        while (state.KeepRunning()) {
            benchmark::DoNotOptimize(
                    turbo::Time::from_civil(turbo::CivilSecond(2014, 12, 18, 20, 16, 18), tz));
        }
    }

    BENCHMARK(BM_Time_FromCivilUTC_Turbo);

    void BM_Time_FromCivilDay0_Turbo(benchmark::State &state) {
        const turbo::TimeZone tz =
                turbo::time_internal::LoadTimeZone("America/Los_Angeles");
        int i = 0;
        while (state.KeepRunning()) {
            if ((i & 1) == 0) {
                benchmark::DoNotOptimize(
                        turbo::Time::from_civil(turbo::CivilSecond(2014, 12, 0, 20, 16, 18), tz));
            } else {
                benchmark::DoNotOptimize(
                        turbo::Time::from_civil(turbo::CivilSecond(2013, 11, 0, 18, 30, 27), tz));
            }
            ++i;
        }
    }

    BENCHMARK(BM_Time_FromCivilDay0_Turbo);

    void BM_Time_FromCivilDay0_Libc(benchmark::State &state) {
        // No timezone support, so just use localtime.
        int i = 0;
        while (state.KeepRunning()) {
            struct tm tm;
            if ((i & 1) == 0) {
                tm.tm_year = 2014 - 1900;
                tm.tm_mon = 12 - 1;
                tm.tm_mday = 0;
                tm.tm_hour = 20;
                tm.tm_min = 16;
                tm.tm_sec = 18;
            } else {
                tm.tm_year = 2013 - 1900;
                tm.tm_mon = 11 - 1;
                tm.tm_mday = 0;
                tm.tm_hour = 18;
                tm.tm_min = 30;
                tm.tm_sec = 27;
            }
            tm.tm_isdst = -1;
            mktime(&tm);
            ++i;
        }
    }

    BENCHMARK(BM_Time_FromCivilDay0_Libc);

    //
    // To/FromTimespec
    //

    void BM_Time_ToTimespec(benchmark::State &state) {
        turbo::Time now = turbo::Time::current_time();
        while (state.KeepRunning()) {
            benchmark::DoNotOptimize(turbo::Time::to_timespec(now));
        }
    }

    BENCHMARK(BM_Time_ToTimespec);

    void BM_Time_FromTimespec(benchmark::State &state) {
        timespec ts = turbo::Time::to_timespec(turbo::Time::current_time());
        while (state.KeepRunning()) {
            if (++ts.tv_nsec == 1000 * 1000 * 1000) {
                ++ts.tv_sec;
                ts.tv_nsec = 0;
            }
            benchmark::DoNotOptimize(turbo::Time::from_timespec(ts));
        }
    }

    BENCHMARK(BM_Time_FromTimespec);

//
// Comparison with InfiniteFuture/Past
//

    void BM_Time_InfiniteFuture(benchmark::State &state) {
        while (state.KeepRunning()) {
            benchmark::DoNotOptimize(turbo::Time::future_infinite());
        }
    }

    BENCHMARK(BM_Time_InfiniteFuture);

    void BM_Time_past_infinite(benchmark::State &state) {
        while (state.KeepRunning()) {
            benchmark::DoNotOptimize(turbo::Time::past_infinite());
        }
    }

    BENCHMARK(BM_Time_past_infinite);

}  // namespace
