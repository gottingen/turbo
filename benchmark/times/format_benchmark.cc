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

#include <cstddef>
#include <string>

#include <tests/times/test_util.h>
#include <turbo/times/time.h>
#include <benchmark/benchmark.h>

namespace {

    namespace {
        const char *const kFormats[] = {
                turbo::RFC1123_full,     // 0
                turbo::RFC1123_no_wday,  // 1
                turbo::RFC3339_full,     // 2
                turbo::RFC3339_sec,      // 3
                "%Y-%m-%d%ET%H:%M:%S",  // 4
                "%Y-%m-%d",             // 5
        };
        const int kNumFormats = sizeof(kFormats) / sizeof(kFormats[0]);
    }  // namespace

    void BM_Format_FormatTime(benchmark::State &state) {
        const std::string fmt = kFormats[state.range(0)];
        state.SetLabel(fmt);
        const turbo::TimeZone lax =
                turbo::time_internal::LoadTimeZone("America/Los_Angeles");
        const turbo::Time t =
                turbo::Time::from_civil(turbo::CivilSecond(1977, 6, 28, 9, 8, 7), lax) +
                turbo::Nanoseconds(1);
        while (state.KeepRunning()) {
            benchmark::DoNotOptimize(turbo::Time::format(fmt, t, lax).length());
        }
    }

    BENCHMARK(BM_Format_FormatTime)->DenseRange(0, kNumFormats - 1);

    void BM_Format_ParseTime(benchmark::State &state) {
        const std::string fmt = kFormats[state.range(0)];
        state.SetLabel(fmt);
        const turbo::TimeZone lax =
                turbo::time_internal::LoadTimeZone("America/Los_Angeles");
        turbo::Time t = turbo::Time::from_civil(turbo::CivilSecond(1977, 6, 28, 9, 8, 7), lax) +
                        turbo::Nanoseconds(1);
        const std::string when = turbo::Time::format(fmt, t, lax);
        std::string err;
        while (state.KeepRunning()) {
            benchmark::DoNotOptimize(turbo::Time::parse(fmt, when, lax, &t, &err));
        }
    }

    BENCHMARK(BM_Format_ParseTime)->DenseRange(0, kNumFormats - 1);

}  // namespace
