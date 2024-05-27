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

#include <turbo/strings/internal/ostringstream.h>

#include <sstream>
#include <string>

#include <benchmark/benchmark.h>

namespace {

enum StringType {
  kNone,
  kStdString,
};

// Benchmarks for std::ostringstream.
template <StringType kOutput>
void BM_StdStream(benchmark::State& state) {
  const int num_writes = state.range(0);
  const int bytes_per_write = state.range(1);
  const std::string payload(bytes_per_write, 'x');
  for (auto _ : state) {
    std::ostringstream strm;
    benchmark::DoNotOptimize(strm);
    for (int i = 0; i != num_writes; ++i) {
      strm << payload;
    }
    switch (kOutput) {
      case kNone: {
        break;
      }
      case kStdString: {
        std::string s = strm.str();
        benchmark::DoNotOptimize(s);
        break;
      }
    }
  }
}

// Create the stream, optionally write to it, then destroy it.
BENCHMARK_TEMPLATE(BM_StdStream, kNone)
    ->ArgPair(0, 0)
    ->ArgPair(1, 16)   // 16 bytes is small enough for SSO
    ->ArgPair(1, 256)  // 256 bytes requires heap allocation
    ->ArgPair(1024, 256);
// Create the stream, write to it, get std::string out, then destroy.
BENCHMARK_TEMPLATE(BM_StdStream, kStdString)
    ->ArgPair(1, 16)   // 16 bytes is small enough for SSO
    ->ArgPair(1, 256)  // 256 bytes requires heap allocation
    ->ArgPair(1024, 256);

// Benchmarks for OStringStream.
template <StringType kOutput>
void BM_CustomStream(benchmark::State& state) {
  const int num_writes = state.range(0);
  const int bytes_per_write = state.range(1);
  const std::string payload(bytes_per_write, 'x');
  for (auto _ : state) {
    std::string out;
    turbo::strings_internal::OStringStream strm(&out);
    benchmark::DoNotOptimize(strm);
    for (int i = 0; i != num_writes; ++i) {
      strm << payload;
    }
    switch (kOutput) {
      case kNone: {
        break;
      }
      case kStdString: {
        std::string s = out;
        benchmark::DoNotOptimize(s);
        break;
      }
    }
  }
}

// Create the stream, optionally write to it, then destroy it.
BENCHMARK_TEMPLATE(BM_CustomStream, kNone)
    ->ArgPair(0, 0)
    ->ArgPair(1, 16)   // 16 bytes is small enough for SSO
    ->ArgPair(1, 256)  // 256 bytes requires heap allocation
    ->ArgPair(1024, 256);
// Create the stream, write to it, get std::string out, then destroy.
// It's not useful in practice to extract std::string from OStringStream; we
// measure it for completeness.
BENCHMARK_TEMPLATE(BM_CustomStream, kStdString)
    ->ArgPair(1, 16)   // 16 bytes is small enough for SSO
    ->ArgPair(1, 256)  // 256 bytes requires heap allocation
    ->ArgPair(1024, 256);

}  // namespace
