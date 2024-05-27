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

#include <limits>

#include <turbo/base/no_destructor.h>
#include <turbo/synchronization/blocking_counter.h>
#include <turbo/synchronization/internal/thread_pool.h>
#include <benchmark/benchmark.h>

namespace {

void BM_BlockingCounter_SingleThread(benchmark::State& state) {
  for (auto _ : state) {
    int iterations = state.range(0);
    turbo::BlockingCounter counter{iterations};
    for (int i = 0; i < iterations; ++i) {
      counter.DecrementCount();
    }
    counter.Wait();
  }
}
BENCHMARK(BM_BlockingCounter_SingleThread)
    ->ArgName("iterations")
    ->Arg(2)
    ->Arg(4)
    ->Arg(16)
    ->Arg(64)
    ->Arg(256);

void BM_BlockingCounter_DecrementCount(benchmark::State& state) {
  static turbo::NoDestructor<turbo::BlockingCounter> counter(
      std::numeric_limits<int>::max());
  for (auto _ : state) {
    counter->DecrementCount();
  }
}
BENCHMARK(BM_BlockingCounter_DecrementCount)
    ->Threads(2)
    ->Threads(4)
    ->Threads(6)
    ->Threads(8)
    ->Threads(10)
    ->Threads(12)
    ->Threads(16)
    ->Threads(32)
    ->Threads(64)
    ->Threads(128);

void BM_BlockingCounter_Wait(benchmark::State& state) {
  int num_threads = state.range(0);
  turbo::synchronization_internal::ThreadPool pool(num_threads);
  for (auto _ : state) {
    turbo::BlockingCounter counter{num_threads};
    pool.Schedule([num_threads, &counter, &pool]() {
      for (int i = 0; i < num_threads; ++i) {
        pool.Schedule([&counter]() { counter.DecrementCount(); });
      }
    });
    counter.Wait();
  }
}
BENCHMARK(BM_BlockingCounter_Wait)
    ->ArgName("threads")
    ->Arg(2)
    ->Arg(4)
    ->Arg(8)
    ->Arg(16)
    ->Arg(32)
    ->Arg(64)
    ->Arg(128);

}  // namespace
