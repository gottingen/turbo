// Copyright 2020 The Turbo Authors.
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//      https://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.

#include "benchmark/benchmark.h"
#include "turbo/platform/internal/thread_identity.h"
#include "turbo/synchronization/internal/create_thread_identity.h"
#include "turbo/synchronization/internal/per_thread_sem.h"

namespace {

void BM_SafeCurrentThreadIdentity(benchmark::State& state) {
  for (auto _ : state) {
    benchmark::DoNotOptimize(
        turbo::synchronization_internal::GetOrCreateCurrentThreadIdentity());
  }
}
BENCHMARK(BM_SafeCurrentThreadIdentity);

void BM_UnsafeCurrentThreadIdentity(benchmark::State& state) {
  for (auto _ : state) {
    benchmark::DoNotOptimize(
        turbo::base_internal::CurrentThreadIdentityIfPresent());
  }
}
BENCHMARK(BM_UnsafeCurrentThreadIdentity);

}  // namespace
