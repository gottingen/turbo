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

#include <cstdint>

#include <benchmark/benchmark.h>
#include <turbo/log/check.h>
#include <turbo/strings/charset.h>

namespace {

turbo::CharSet MakeBenchmarkMap() {
  turbo::CharSet m;
  uint32_t x[] = {0x0, 0x1, 0x2, 0x3, 0xf, 0xe, 0xd, 0xc};
  for (uint32_t& t : x) t *= static_cast<uint32_t>(0x11111111UL);
  for (uint32_t i = 0; i < 256; ++i) {
    if ((x[i / 32] >> (i % 32)) & 1) m = m | turbo::CharSet::Char(i);
  }
  return m;
}

// Micro-benchmark for Charmap::contains.
static void BM_Contains(benchmark::State& state) {
  // Loop-body replicated 10 times to increase time per iteration.
  // Argument continuously changed to avoid generating common subexpressions.
  // Final CHECK used to discourage unwanted optimization.
  const turbo::CharSet benchmark_map = MakeBenchmarkMap();
  unsigned char c = 0;
  int ops = 0;
  for (auto _ : state) {
    ops += benchmark_map.contains(c++);
    ops += benchmark_map.contains(c++);
    ops += benchmark_map.contains(c++);
    ops += benchmark_map.contains(c++);
    ops += benchmark_map.contains(c++);
    ops += benchmark_map.contains(c++);
    ops += benchmark_map.contains(c++);
    ops += benchmark_map.contains(c++);
    ops += benchmark_map.contains(c++);
    ops += benchmark_map.contains(c++);
  }
  CHECK_NE(ops, -1);
}
BENCHMARK(BM_Contains);

}  // namespace
