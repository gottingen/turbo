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

#include <turbo/time/clock.h>

#if !defined(_WIN32)
#include <sys/time.h>
#else
#include <winsock2.h>
#endif  // _WIN32
#include <cstdio>

#include <turbo/base/internal/cycleclock.h>
#include <benchmark/benchmark.h>

namespace {

void BM_Clock_Now_TurboTime(benchmark::State& state) {
  while (state.KeepRunning()) {
    benchmark::DoNotOptimize(turbo::Now());
  }
}
BENCHMARK(BM_Clock_Now_TurboTime);

void BM_Clock_Now_GetCurrentTimeNanos(benchmark::State& state) {
  while (state.KeepRunning()) {
    benchmark::DoNotOptimize(turbo::GetCurrentTimeNanos());
  }
}
BENCHMARK(BM_Clock_Now_GetCurrentTimeNanos);

void BM_Clock_Now_TurboTime_ToUnixNanos(benchmark::State& state) {
  while (state.KeepRunning()) {
    benchmark::DoNotOptimize(turbo::ToUnixNanos(turbo::Now()));
  }
}
BENCHMARK(BM_Clock_Now_TurboTime_ToUnixNanos);

void BM_Clock_Now_CycleClock(benchmark::State& state) {
  while (state.KeepRunning()) {
    benchmark::DoNotOptimize(turbo::base_internal::CycleClock::Now());
  }
}
BENCHMARK(BM_Clock_Now_CycleClock);

#if !defined(_WIN32)
static void BM_Clock_Now_gettimeofday(benchmark::State& state) {
  struct timeval tv;
  while (state.KeepRunning()) {
    benchmark::DoNotOptimize(gettimeofday(&tv, nullptr));
  }
}
BENCHMARK(BM_Clock_Now_gettimeofday);

static void BM_Clock_Now_clock_gettime(benchmark::State& state) {
  struct timespec ts;
  while (state.KeepRunning()) {
    benchmark::DoNotOptimize(clock_gettime(CLOCK_REALTIME, &ts));
  }
}
BENCHMARK(BM_Clock_Now_clock_gettime);
#endif  // _WIN32

}  // namespace
