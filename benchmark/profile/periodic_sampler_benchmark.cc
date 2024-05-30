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

#include <turbo/profiling/internal/periodic_sampler.h>
#include <benchmark/benchmark.h>

namespace turbo {
TURBO_NAMESPACE_BEGIN
namespace profiling_internal {
namespace {

template <typename Sampler>
void BM_Sample(Sampler* sampler, benchmark::State& state) {
  for (auto _ : state) {
    benchmark::DoNotOptimize(sampler);
    benchmark::DoNotOptimize(sampler->Sample());
  }
}

template <typename Sampler>
void BM_SampleMinunumInlined(Sampler* sampler, benchmark::State& state) {
  for (auto _ : state) {
    benchmark::DoNotOptimize(sampler);
    if (TURBO_UNLIKELY(sampler->SubtleMaybeSample())) {
      benchmark::DoNotOptimize(sampler->SubtleConfirmSample());
    }
  }
}

void BM_PeriodicSampler_TinySample(benchmark::State& state) {
  struct Tag {};
  PeriodicSampler<Tag, 10> sampler;
  BM_Sample(&sampler, state);
}
BENCHMARK(BM_PeriodicSampler_TinySample);

void BM_PeriodicSampler_ShortSample(benchmark::State& state) {
  struct Tag {};
  PeriodicSampler<Tag, 1024> sampler;
  BM_Sample(&sampler, state);
}
BENCHMARK(BM_PeriodicSampler_ShortSample);

void BM_PeriodicSampler_LongSample(benchmark::State& state) {
  struct Tag {};
  PeriodicSampler<Tag, 1024 * 1024> sampler;
  BM_Sample(&sampler, state);
}
BENCHMARK(BM_PeriodicSampler_LongSample);

void BM_PeriodicSampler_LongSampleMinunumInlined(benchmark::State& state) {
  struct Tag {};
  PeriodicSampler<Tag, 1024 * 1024> sampler;
  BM_SampleMinunumInlined(&sampler, state);
}
BENCHMARK(BM_PeriodicSampler_LongSampleMinunumInlined);

void BM_PeriodicSampler_Disabled(benchmark::State& state) {
  struct Tag {};
  PeriodicSampler<Tag, 0> sampler;
  BM_Sample(&sampler, state);
}
BENCHMARK(BM_PeriodicSampler_Disabled);

}  // namespace
}  // namespace profiling_internal
TURBO_NAMESPACE_END
}  // namespace turbo
