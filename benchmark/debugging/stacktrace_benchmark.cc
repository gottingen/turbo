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

#include <turbo/base/attributes.h>
#include <turbo/base/config.h>
#include <turbo/base/optimization.h>
#include <turbo/debugging/stacktrace.h>
#include <benchmark/benchmark.h>

namespace turbo {
TURBO_NAMESPACE_BEGIN
namespace {

static constexpr int kMaxStackDepth = 100;
static constexpr int kCacheSize = (1 << 16);
void* pcs[kMaxStackDepth];

TURBO_ATTRIBUTE_NOINLINE void func(benchmark::State& state, int x, int depth) {
  if (x <= 0) {
    // Touch a significant amount of memory so that the stack is likely to be
    // not cached in the L1 cache.
    state.PauseTiming();
    int* arr = new int[kCacheSize];
    for (int i = 0; i < kCacheSize; ++i) benchmark::DoNotOptimize(arr[i] = 100);
    delete[] arr;
    state.ResumeTiming();
    benchmark::DoNotOptimize(turbo::GetStackTrace(pcs, depth, 0));
    return;
  }
  TURBO_BLOCK_TAIL_CALL_OPTIMIZATION();
  func(state, --x, depth);
}

void BM_GetStackTrace(benchmark::State& state) {
  int depth = state.range(0);
  for (auto s : state) {
    func(state, depth, depth);
  }
}

BENCHMARK(BM_GetStackTrace)->DenseRange(10, kMaxStackDepth, 10);
}  // namespace
TURBO_NAMESPACE_END
}  // namespace turbo
