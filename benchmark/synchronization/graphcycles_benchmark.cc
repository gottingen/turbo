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

#include <turbo/synchronization/internal/graphcycles.h>

#include <algorithm>
#include <cstdint>
#include <vector>

#include <benchmark/benchmark.h>
#include <turbo/base/internal/raw_logging.h>

namespace {

void BM_StressTest(benchmark::State& state) {
  const int num_nodes = state.range(0);
  while (state.KeepRunningBatch(num_nodes)) {
    turbo::synchronization_internal::GraphCycles g;
    std::vector<turbo::synchronization_internal::GraphId> nodes(num_nodes);
    for (int i = 0; i < num_nodes; i++) {
      nodes[i] = g.GetId(reinterpret_cast<void*>(static_cast<uintptr_t>(i)));
    }
    for (int i = 0; i < num_nodes; i++) {
      int end = std::min(num_nodes, i + 5);
      for (int j = i + 1; j < end; j++) {
        TURBO_RAW_CHECK(g.InsertEdge(nodes[i], nodes[j]), "");
      }
    }
  }
}
BENCHMARK(BM_StressTest)->Range(2048, 1048576);

}  // namespace
