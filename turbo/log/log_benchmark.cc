// Copyright 2022 The Turbo Authors.
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

#include "turbo/platform/attributes.h"
#include "turbo/base/log_severity.h"
#include "turbo/flags/flag.h"
#include "turbo/log/check.h"
#include "turbo/log/globals.h"
#include "turbo/log/log.h"
#include "turbo/log/log_entry.h"
#include "turbo/log/log_sink.h"
#include "turbo/log/log_sink_registry.h"
#include "benchmark/benchmark.h"

namespace {

class NullLogSink : public turbo::LogSink {
 public:
  NullLogSink() { turbo::AddLogSink(this); }

  ~NullLogSink() override { turbo::RemoveLogSink(this); }

  void Send(const turbo::LogEntry&) override {}
};

constexpr int x = -1;

void BM_SuccessfulBinaryCheck(benchmark::State& state) {
  int n = 0;
  while (state.KeepRunningBatch(8)) {
    CHECK_GE(n, x);
    CHECK_GE(n, x);
    CHECK_GE(n, x);
    CHECK_GE(n, x);
    CHECK_GE(n, x);
    CHECK_GE(n, x);
    CHECK_GE(n, x);
    CHECK_GE(n, x);
    ++n;
  }
  benchmark::DoNotOptimize(n);
}
BENCHMARK(BM_SuccessfulBinaryCheck);

static void BM_SuccessfulUnaryCheck(benchmark::State& state) {
  int n = 0;
  while (state.KeepRunningBatch(8)) {
    CHECK(n >= x);
    CHECK(n >= x);
    CHECK(n >= x);
    CHECK(n >= x);
    CHECK(n >= x);
    CHECK(n >= x);
    CHECK(n >= x);
    CHECK(n >= x);
    ++n;
  }
  benchmark::DoNotOptimize(n);
}
BENCHMARK(BM_SuccessfulUnaryCheck);

static void BM_DisabledLogOverhead(benchmark::State& state) {
  turbo::ScopedStderrThreshold disable_stderr_logging(
      turbo::LogSeverityAtLeast::kInfinity);
  turbo::log_internal::ScopedMinLogLevel scoped_min_log_level(
      turbo::LogSeverityAtLeast::kInfinity);
  for (auto _ : state) {
    LOG(INFO);
  }
}
BENCHMARK(BM_DisabledLogOverhead);

static void BM_EnabledLogOverhead(benchmark::State& state) {
  turbo::ScopedStderrThreshold stderr_logging(
      turbo::LogSeverityAtLeast::kInfinity);
  turbo::log_internal::ScopedMinLogLevel scoped_min_log_level(
      turbo::LogSeverityAtLeast::kInfo);
  TURBO_ATTRIBUTE_UNUSED NullLogSink null_sink;
  for (auto _ : state) {
    LOG(INFO);
  }
}
BENCHMARK(BM_EnabledLogOverhead);

}  // namespace

