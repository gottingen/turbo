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

#include <stddef.h>

#include <string>

#include <turbo/container/fixed_array.h>
#include <benchmark/benchmark.h>

namespace {

// For benchmarking -- simple class with constructor and destructor that
// set an int to a constant..
class SimpleClass {
 public:
  SimpleClass() : i(3) {}
  ~SimpleClass() { i = 0; }

 private:
  int i;
};

template <typename C, size_t stack_size>
void BM_FixedArray(benchmark::State& state) {
  const int size = state.range(0);
  for (auto _ : state) {
    turbo::FixedArray<C, stack_size> fa(size);
    benchmark::DoNotOptimize(fa.data());
  }
}
BENCHMARK_TEMPLATE(BM_FixedArray, char, turbo::kFixedArrayUseDefault)
    ->Range(0, 1 << 16);
BENCHMARK_TEMPLATE(BM_FixedArray, char, 0)->Range(0, 1 << 16);
BENCHMARK_TEMPLATE(BM_FixedArray, char, 1)->Range(0, 1 << 16);
BENCHMARK_TEMPLATE(BM_FixedArray, char, 16)->Range(0, 1 << 16);
BENCHMARK_TEMPLATE(BM_FixedArray, char, 256)->Range(0, 1 << 16);
BENCHMARK_TEMPLATE(BM_FixedArray, char, 65536)->Range(0, 1 << 16);

BENCHMARK_TEMPLATE(BM_FixedArray, SimpleClass, turbo::kFixedArrayUseDefault)
    ->Range(0, 1 << 16);
BENCHMARK_TEMPLATE(BM_FixedArray, SimpleClass, 0)->Range(0, 1 << 16);
BENCHMARK_TEMPLATE(BM_FixedArray, SimpleClass, 1)->Range(0, 1 << 16);
BENCHMARK_TEMPLATE(BM_FixedArray, SimpleClass, 16)->Range(0, 1 << 16);
BENCHMARK_TEMPLATE(BM_FixedArray, SimpleClass, 256)->Range(0, 1 << 16);
BENCHMARK_TEMPLATE(BM_FixedArray, SimpleClass, 65536)->Range(0, 1 << 16);

BENCHMARK_TEMPLATE(BM_FixedArray, std::string, turbo::kFixedArrayUseDefault)
    ->Range(0, 1 << 16);
BENCHMARK_TEMPLATE(BM_FixedArray, std::string, 0)->Range(0, 1 << 16);
BENCHMARK_TEMPLATE(BM_FixedArray, std::string, 1)->Range(0, 1 << 16);
BENCHMARK_TEMPLATE(BM_FixedArray, std::string, 16)->Range(0, 1 << 16);
BENCHMARK_TEMPLATE(BM_FixedArray, std::string, 256)->Range(0, 1 << 16);
BENCHMARK_TEMPLATE(BM_FixedArray, std::string, 65536)->Range(0, 1 << 16);

}  // namespace
