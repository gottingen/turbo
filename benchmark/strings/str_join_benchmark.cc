//
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

#include <turbo/strings/str_join.h>

#include <string>
#include <tuple>
#include <vector>
#include <utility>

#include <benchmark/benchmark.h>

namespace {

void BM_Join2_Strings(benchmark::State& state) {
  const int string_len = state.range(0);
  const int num_strings = state.range(1);
  const std::string s(string_len, 'x');
  const std::vector<std::string> v(num_strings, s);
  for (auto _ : state) {
    std::string s = turbo::StrJoin(v, "-");
    benchmark::DoNotOptimize(s);
  }
}
BENCHMARK(BM_Join2_Strings)
    ->ArgPair(1 << 0, 1 << 3)
    ->ArgPair(1 << 10, 1 << 3)
    ->ArgPair(1 << 13, 1 << 3)
    ->ArgPair(1 << 0, 1 << 10)
    ->ArgPair(1 << 10, 1 << 10)
    ->ArgPair(1 << 13, 1 << 10)
    ->ArgPair(1 << 0, 1 << 13)
    ->ArgPair(1 << 10, 1 << 13)
    ->ArgPair(1 << 13, 1 << 13);

void BM_Join2_Ints(benchmark::State& state) {
  const int num_ints = state.range(0);
  const std::vector<int> v(num_ints, 42);
  for (auto _ : state) {
    std::string s = turbo::StrJoin(v, "-");
    benchmark::DoNotOptimize(s);
  }
}
BENCHMARK(BM_Join2_Ints)->Range(0, 1 << 13);

void BM_Join2_KeysAndValues(benchmark::State& state) {
  const int string_len = state.range(0);
  const int num_pairs = state.range(1);
  const std::string s(string_len, 'x');
  const std::vector<std::pair<std::string, int>> v(num_pairs,
                                                   std::make_pair(s, 42));
  for (auto _ : state) {
    std::string s = turbo::StrJoin(v, ",", turbo::PairFormatter("="));
    benchmark::DoNotOptimize(s);
  }
}
BENCHMARK(BM_Join2_KeysAndValues)
    ->ArgPair(1 << 0, 1 << 3)
    ->ArgPair(1 << 10, 1 << 3)
    ->ArgPair(1 << 13, 1 << 3)
    ->ArgPair(1 << 0, 1 << 10)
    ->ArgPair(1 << 10, 1 << 10)
    ->ArgPair(1 << 13, 1 << 10)
    ->ArgPair(1 << 0, 1 << 13)
    ->ArgPair(1 << 10, 1 << 13)
    ->ArgPair(1 << 13, 1 << 13);

void BM_JoinStreamable(benchmark::State& state) {
  const int string_len = state.range(0);
  const int num_strings = state.range(1);
  const std::vector<std::string> v(num_strings, std::string(string_len, 'x'));
  for (auto _ : state) {
    std::string s = turbo::StrJoin(v, "", turbo::StreamFormatter());
    benchmark::DoNotOptimize(s);
  }
}
BENCHMARK(BM_JoinStreamable)
    ->ArgPair(0, 0)
    ->ArgPair(16, 1)
    ->ArgPair(256, 1)
    ->ArgPair(16, 16)
    ->ArgPair(256, 16)
    ->ArgPair(16, 256)
    ->ArgPair(256, 256);

void BM_JoinTuple(benchmark::State& state) {
  for (auto _ : state) {
    std::string s =
        turbo::StrJoin(std::make_tuple(123456789, 987654321, 24680, 13579), "/");
    benchmark::DoNotOptimize(s);
  }
}
BENCHMARK(BM_JoinTuple);

}  // namespace
