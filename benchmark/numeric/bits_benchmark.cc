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
#include <vector>

#include <benchmark/benchmark.h>
#include <turbo/base/optimization.h>
#include <turbo/numeric/bits.h>
#include <turbo/random/random.h>

namespace turbo {
namespace {

template <typename T>
static void BM_bit_width(benchmark::State& state) {
  const auto count = static_cast<size_t>(state.range(0));

  turbo::BitGen rng;
  std::vector<T> values;
  values.reserve(count);
  for (size_t i = 0; i < count; ++i) {
    values.push_back(turbo::Uniform<T>(rng, 0, std::numeric_limits<T>::max()));
  }

  while (state.KeepRunningBatch(static_cast<int64_t>(count))) {
    for (size_t i = 0; i < count; ++i) {
      benchmark::DoNotOptimize(turbo::bit_width(values[i]));
    }
  }
}
BENCHMARK_TEMPLATE(BM_bit_width, uint8_t)->Range(1, 1 << 20);
BENCHMARK_TEMPLATE(BM_bit_width, uint16_t)->Range(1, 1 << 20);
BENCHMARK_TEMPLATE(BM_bit_width, uint32_t)->Range(1, 1 << 20);
BENCHMARK_TEMPLATE(BM_bit_width, uint64_t)->Range(1, 1 << 20);

template <typename T>
static void BM_bit_width_nonzero(benchmark::State& state) {
  const auto count = static_cast<size_t>(state.range(0));

  turbo::BitGen rng;
  std::vector<T> values;
  values.reserve(count);
  for (size_t i = 0; i < count; ++i) {
    values.push_back(turbo::Uniform<T>(rng, 1, std::numeric_limits<T>::max()));
  }

  while (state.KeepRunningBatch(static_cast<int64_t>(count))) {
    for (size_t i = 0; i < count; ++i) {
      const T value = values[i];
      TURBO_ASSUME(value > 0);
      benchmark::DoNotOptimize(turbo::bit_width(value));
    }
  }
}
BENCHMARK_TEMPLATE(BM_bit_width_nonzero, uint8_t)->Range(1, 1 << 20);
BENCHMARK_TEMPLATE(BM_bit_width_nonzero, uint16_t)->Range(1, 1 << 20);
BENCHMARK_TEMPLATE(BM_bit_width_nonzero, uint32_t)->Range(1, 1 << 20);
BENCHMARK_TEMPLATE(BM_bit_width_nonzero, uint64_t)->Range(1, 1 << 20);

}  // namespace
}  // namespace turbo
