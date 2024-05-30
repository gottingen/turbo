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

#ifndef _WIN32

#include <sys/types.h>

#endif

#include <algorithm>
#include <chrono>  // NOLINT(build/c++11)
#include <cstdint>
#include <cstdlib>
#include <cstring>
#include <ctime>
#include <limits>

#include <turbo/base/attributes.h>
#include <turbo/base/call_once.h>
#include <turbo/base/config.h>
#include <turbo/times/time.h>

namespace turbo {
    TURBO_NAMESPACE_BEGIN
    namespace synchronization_internal {

#ifdef TURBO_INTERNAL_NEED_REDUNDANT_CONSTEXPR_DECL
        constexpr uint64_t KernelTimeout::kNoTimeout;
        constexpr int64_t KernelTimeout::kMaxNanos;
#endif

        int64_t KernelTimeout::SteadyClockNow() {
            if (!SupportsSteadyClock()) {
                return turbo::GetCurrentTimeNanos();
            }
            return std::chrono::duration_cast<std::chrono::nanoseconds>(
                    std::chrono::steady_clock::now().time_since_epoch())
                    .count();
        }

        KernelTimeout::KernelTimeout(turbo::Time t) {
            // `turbo::Time::future_infinite()` is a common "no timeout" value and cheaper to
            // compare than convert.
            if (t == turbo::Time::future_infinite()) {
                rep_ = kNoTimeout;
                return;
            }

            int64_t unix_nanos = turbo::Time::to_nanoseconds(t);

            // A timeout that lands before the unix epoch is converted to 0.
            // In theory implementations should expire these timeouts immediately.
            if (unix_nanos < 0) {
                unix_nanos = 0;
            }

            // Values greater than or equal to kMaxNanos are converted to infinite.
            if (unix_nanos >= kMaxNanos) {
                rep_ = kNoTimeout;
                return;
            }

            rep_ = static_cast<uint64_t>(unix_nanos) << 1;
        }

        KernelTimeout::KernelTimeout(turbo::Duration d) {
            // `turbo::InfiniteDuration()` is a common "no timeout" value and cheaper to
            // compare than convert.
            if (d == turbo::InfiniteDuration()) {
                rep_ = kNoTimeout;
                return;
            }

            int64_t nanos = turbo::ToInt64Nanoseconds(d);

            // Negative durations are normalized to 0.
            // In theory implementations should expire these timeouts immediately.
            if (nanos < 0) {
                nanos = 0;
            }

            int64_t now = SteadyClockNow();
            if (nanos > kMaxNanos - now) {
                // Durations that would be greater than kMaxNanos are converted to infinite.
                rep_ = kNoTimeout;
                return;
            }

            nanos += now;
            rep_ = (static_cast<uint64_t>(nanos) << 1) | uint64_t{1};
        }

        int64_t KernelTimeout::MakeAbsNanos() const {
            if (!has_timeout()) {
                return kMaxNanos;
            }

            int64_t nanos = RawAbsNanos();

            if (is_relative_timeout()) {
                // We need to change epochs, because the relative timeout might be
                // represented by an absolute timestamp from another clock.
                nanos = std::max<int64_t>(nanos - SteadyClockNow(), 0);
                int64_t now = turbo::GetCurrentTimeNanos();
                if (nanos > kMaxNanos - now) {
                    // Overflow.
                    nanos = kMaxNanos;
                } else {
                    nanos += now;
                }
            } else if (nanos == 0) {
                // Some callers have assumed that 0 means no timeout, so instead we return a
                // time of 1 nanosecond after the epoch.
                nanos = 1;
            }

            return nanos;
        }

        int64_t KernelTimeout::InNanosecondsFromNow() const {
            if (!has_timeout()) {
                return kMaxNanos;
            }

            int64_t nanos = RawAbsNanos();
            if (is_absolute_timeout()) {
                return std::max<int64_t>(nanos - turbo::GetCurrentTimeNanos(), 0);
            }
            return std::max<int64_t>(nanos - SteadyClockNow(), 0);
        }

        struct timespec KernelTimeout::MakeAbsTimespec() const {
            return turbo::ToTimespec(turbo::Nanoseconds(MakeAbsNanos()));
        }

        struct timespec KernelTimeout::MakeRelativeTimespec() const {
            return turbo::ToTimespec(turbo::Nanoseconds(InNanosecondsFromNow()));
        }

#ifndef _WIN32

        struct timespec KernelTimeout::MakeClockAbsoluteTimespec(clockid_t c) const {
            if (!has_timeout()) {
                return turbo::ToTimespec(turbo::Nanoseconds(kMaxNanos));
            }

            int64_t nanos = RawAbsNanos();
            if (is_absolute_timeout()) {
                nanos -= turbo::GetCurrentTimeNanos();
            } else {
                nanos -= SteadyClockNow();
            }

            struct timespec now;
            TURBO_RAW_CHECK(clock_gettime(c, &now) == 0, "clock_gettime() failed");
            turbo::Duration from_clock_epoch =
                    turbo::DurationFromTimespec(now) + turbo::Nanoseconds(nanos);
            if (from_clock_epoch <= turbo::ZeroDuration()) {
                // Some callers have assumed that 0 means no timeout, so instead we return a
                // time of 1 nanosecond after the epoch. For safety we also do not return
                // negative values.
                return turbo::ToTimespec(turbo::Nanoseconds(1));
            }
            return turbo::ToTimespec(from_clock_epoch);
        }

#endif

        KernelTimeout::DWord KernelTimeout::InMillisecondsFromNow() const {
            constexpr DWord kInfinite = std::numeric_limits<DWord>::max();

            if (!has_timeout()) {
                return kInfinite;
            }

            constexpr uint64_t kNanosInMillis = uint64_t{1'000'000};
            constexpr uint64_t kMaxValueNanos =
                    std::numeric_limits<int64_t>::max() - kNanosInMillis + 1;

            uint64_t ns_from_now = static_cast<uint64_t>(InNanosecondsFromNow());
            if (ns_from_now >= kMaxValueNanos) {
                // Rounding up would overflow.
                return kInfinite;
            }
            // Convert to milliseconds, always rounding up.
            uint64_t ms_from_now = (ns_from_now + kNanosInMillis - 1) / kNanosInMillis;
            if (ms_from_now > kInfinite) {
                return kInfinite;
            }
            return static_cast<DWord>(ms_from_now);
        }

        std::chrono::time_point<std::chrono::system_clock>
        KernelTimeout::ToChronoTimePoint() const {
            if (!has_timeout()) {
                return std::chrono::time_point<std::chrono::system_clock>::max();
            }

            // The cast to std::microseconds is because (on some platforms) the
            // std::ratio used by std::chrono::steady_clock doesn't convert to
            // std::nanoseconds, so it doesn't compile.
            auto micros = std::chrono::duration_cast<std::chrono::microseconds>(
                    std::chrono::nanoseconds(MakeAbsNanos()));
            return std::chrono::system_clock::from_time_t(0) + micros;
        }

        std::chrono::nanoseconds KernelTimeout::ToChronoDuration() const {
            if (!has_timeout()) {
                return std::chrono::nanoseconds::max();
            }
            return std::chrono::nanoseconds(InNanosecondsFromNow());
        }

    }  // namespace synchronization_internal
    TURBO_NAMESPACE_END
}  // namespace turbo
