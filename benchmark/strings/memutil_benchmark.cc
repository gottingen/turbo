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

#include <turbo/strings/internal/memutil.h>

#include <algorithm>
#include <cstdlib>

#include <benchmark/benchmark.h>
#include <turbo/strings/ascii.h>

// We fill the haystack with aaaaaaaaaaaaaaaaaa...aaaab.
// That gives us:
// - an easy search: 'b'
// - a medium search: 'ab'.  That means every letter is a possible match.
// - a pathological search: 'aaaaaa.......aaaaab' (half as many a's as haytack)

namespace {

constexpr int kHaystackSize = 10000;
constexpr int64_t kHaystackSize64 = kHaystackSize;
const char* MakeHaystack() {
  char* haystack = new char[kHaystackSize];
  for (int i = 0; i < kHaystackSize - 1; ++i) haystack[i] = 'a';
  haystack[kHaystackSize - 1] = 'b';
  return haystack;
}
const char* const kHaystack = MakeHaystack();

bool case_eq(const char a, const char b) {
  return turbo::ascii_tolower(a) == turbo::ascii_tolower(b);
}

void BM_Searchcase(benchmark::State& state) {
  for (auto _ : state) {
    benchmark::DoNotOptimize(std::search(kHaystack, kHaystack + kHaystackSize,
                                         kHaystack + kHaystackSize - 1,
                                         kHaystack + kHaystackSize, case_eq));
  }
  state.SetBytesProcessed(kHaystackSize64 * state.iterations());
}
BENCHMARK(BM_Searchcase);

void BM_SearchcaseMedium(benchmark::State& state) {
  for (auto _ : state) {
    benchmark::DoNotOptimize(std::search(kHaystack, kHaystack + kHaystackSize,
                                         kHaystack + kHaystackSize - 2,
                                         kHaystack + kHaystackSize, case_eq));
  }
  state.SetBytesProcessed(kHaystackSize64 * state.iterations());
}
BENCHMARK(BM_SearchcaseMedium);

void BM_SearchcasePathological(benchmark::State& state) {
  for (auto _ : state) {
    benchmark::DoNotOptimize(std::search(kHaystack, kHaystack + kHaystackSize,
                                         kHaystack + kHaystackSize / 2,
                                         kHaystack + kHaystackSize, case_eq));
  }
  state.SetBytesProcessed(kHaystackSize64 * state.iterations());
}
BENCHMARK(BM_SearchcasePathological);

char* memcasechr(const char* s, int c, size_t slen) {
  c = turbo::ascii_tolower(c);
  for (; slen; ++s, --slen) {
    if (turbo::ascii_tolower(*s) == c) return const_cast<char*>(s);
  }
  return nullptr;
}

const char* memcasematch(const char* phaystack, size_t haylen,
                         const char* pneedle, size_t neelen) {
  if (0 == neelen) {
    return phaystack;  // even if haylen is 0
  }
  if (haylen < neelen) return nullptr;

  const char* match;
  const char* hayend = phaystack + haylen - neelen + 1;
  while ((match = static_cast<char*>(
              memcasechr(phaystack, pneedle[0], hayend - phaystack)))) {
    if (turbo::strings_internal::memcasecmp(match, pneedle, neelen) == 0)
      return match;
    else
      phaystack = match + 1;
  }
  return nullptr;
}

void BM_Memcasematch(benchmark::State& state) {
  for (auto _ : state) {
    benchmark::DoNotOptimize(memcasematch(kHaystack, kHaystackSize, "b", 1));
  }
  state.SetBytesProcessed(kHaystackSize64 * state.iterations());
}
BENCHMARK(BM_Memcasematch);

void BM_MemcasematchMedium(benchmark::State& state) {
  for (auto _ : state) {
    benchmark::DoNotOptimize(memcasematch(kHaystack, kHaystackSize, "ab", 2));
  }
  state.SetBytesProcessed(kHaystackSize64 * state.iterations());
}
BENCHMARK(BM_MemcasematchMedium);

void BM_MemcasematchPathological(benchmark::State& state) {
  for (auto _ : state) {
    benchmark::DoNotOptimize(memcasematch(kHaystack, kHaystackSize,
                                          kHaystack + kHaystackSize / 2,
                                          kHaystackSize - kHaystackSize / 2));
  }
  state.SetBytesProcessed(kHaystackSize64 * state.iterations());
}
BENCHMARK(BM_MemcasematchPathological);

}  // namespace
