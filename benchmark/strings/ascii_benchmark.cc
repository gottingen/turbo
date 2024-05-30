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

#include <turbo/strings/ascii.h>

#include <algorithm>
#include <cctype>
#include <cstddef>
#include <string>
#include <array>
#include <random>

#include <benchmark/benchmark.h>

namespace {

std::array<unsigned char, 256> MakeShuffledBytes() {
  std::array<unsigned char, 256> bytes;
  for (size_t i = 0; i < 256; ++i) bytes[i] = static_cast<unsigned char>(i);
  std::random_device rd;
  std::seed_seq seed({rd(), rd(), rd(), rd(), rd(), rd(), rd(), rd()});
  std::mt19937 g(seed);
  std::shuffle(bytes.begin(), bytes.end(), g);
  return bytes;
}

template <typename Function>
void AsciiBenchmark(benchmark::State& state, Function f) {
  std::array<unsigned char, 256> bytes = MakeShuffledBytes();
  size_t sum = 0;
  for (auto _ : state) {
    for (unsigned char b : bytes) sum += f(b) ? 1 : 0;
  }
  // Make a copy of `sum` before calling `DoNotOptimize` to make sure that `sum`
  // can be put in a CPU register and not degrade performance in the loop above.
  size_t sum2 = sum;
  benchmark::DoNotOptimize(sum2);
  state.SetBytesProcessed(state.iterations() * bytes.size());
}

using StdAsciiFunction = int (*)(int);
template <StdAsciiFunction f>
void BM_Ascii(benchmark::State& state) {
  AsciiBenchmark(state, f);
}

using TurboAsciiIsFunction = bool (*)(unsigned char);
template <TurboAsciiIsFunction f>
void BM_Ascii(benchmark::State& state) {
  AsciiBenchmark(state, f);
}

using TurboAsciiToFunction = char (*)(unsigned char);
template <TurboAsciiToFunction f>
void BM_Ascii(benchmark::State& state) {
  AsciiBenchmark(state, f);
}

inline char Noop(unsigned char b) { return static_cast<char>(b); }

BENCHMARK_TEMPLATE(BM_Ascii, Noop);
BENCHMARK_TEMPLATE(BM_Ascii, std::isalpha);
BENCHMARK_TEMPLATE(BM_Ascii, turbo::ascii_isalpha);
BENCHMARK_TEMPLATE(BM_Ascii, std::isdigit);
BENCHMARK_TEMPLATE(BM_Ascii, turbo::ascii_isdigit);
BENCHMARK_TEMPLATE(BM_Ascii, std::isalnum);
BENCHMARK_TEMPLATE(BM_Ascii, turbo::ascii_isalnum);
BENCHMARK_TEMPLATE(BM_Ascii, std::isspace);
BENCHMARK_TEMPLATE(BM_Ascii, turbo::ascii_isspace);
BENCHMARK_TEMPLATE(BM_Ascii, std::ispunct);
BENCHMARK_TEMPLATE(BM_Ascii, turbo::ascii_ispunct);
BENCHMARK_TEMPLATE(BM_Ascii, std::isblank);
BENCHMARK_TEMPLATE(BM_Ascii, turbo::ascii_isblank);
BENCHMARK_TEMPLATE(BM_Ascii, std::iscntrl);
BENCHMARK_TEMPLATE(BM_Ascii, turbo::ascii_iscntrl);
BENCHMARK_TEMPLATE(BM_Ascii, std::isxdigit);
BENCHMARK_TEMPLATE(BM_Ascii, turbo::ascii_isxdigit);
BENCHMARK_TEMPLATE(BM_Ascii, std::isprint);
BENCHMARK_TEMPLATE(BM_Ascii, turbo::ascii_isprint);
BENCHMARK_TEMPLATE(BM_Ascii, std::isgraph);
BENCHMARK_TEMPLATE(BM_Ascii, turbo::ascii_isgraph);
BENCHMARK_TEMPLATE(BM_Ascii, std::isupper);
BENCHMARK_TEMPLATE(BM_Ascii, turbo::ascii_isupper);
BENCHMARK_TEMPLATE(BM_Ascii, std::islower);
BENCHMARK_TEMPLATE(BM_Ascii, turbo::ascii_islower);
BENCHMARK_TEMPLATE(BM_Ascii, isascii);
BENCHMARK_TEMPLATE(BM_Ascii, turbo::ascii_isascii);
BENCHMARK_TEMPLATE(BM_Ascii, std::tolower);
BENCHMARK_TEMPLATE(BM_Ascii, turbo::ascii_tolower);
BENCHMARK_TEMPLATE(BM_Ascii, std::toupper);
BENCHMARK_TEMPLATE(BM_Ascii, turbo::ascii_toupper);

static void BM_StrToLower(benchmark::State& state) {
  const int size = state.range(0);
  std::string s(size, 'X');
  for (auto _ : state) {
    benchmark::DoNotOptimize(s);
    std::string res = turbo::str_to_lower(s);
    benchmark::DoNotOptimize(res);
  }
}
BENCHMARK(BM_StrToLower)
    ->DenseRange(0, 32)
    ->RangeMultiplier(2)
    ->Range(64, 1 << 26);

static void BM_StrToUpper(benchmark::State& state) {
  const int size = state.range(0);
  std::string s(size, 'x');
  for (auto _ : state) {
    benchmark::DoNotOptimize(s);
    std::string res = turbo::str_to_upper(s);
    benchmark::DoNotOptimize(res);
  }
}
BENCHMARK(BM_StrToUpper)
    ->DenseRange(0, 32)
    ->RangeMultiplier(2)
    ->Range(64, 1 << 26);

}  // namespace
