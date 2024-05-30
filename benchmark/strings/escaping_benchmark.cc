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

#include <turbo/strings/escaping.h>

#include <cstdint>
#include <cstdio>
#include <cstring>
#include <memory>
#include <random>
#include <string>

#include <benchmark/benchmark.h>
#include <turbo/base/internal/raw_logging.h>
#include <turbo/strings/internal/escaping_test_common.h>
#include <turbo/strings/str_cat.h>

namespace {

void BM_CUnescapeHexString(benchmark::State& state) {
  std::string src;
  for (int i = 0; i < 50; i++) {
    src += "\\x55";
  }
  std::string dest;
  for (auto _ : state) {
    turbo::c_decode(src, &dest);
  }
}
BENCHMARK(BM_CUnescapeHexString);

void BM_WebSafeBase64Escape_string(benchmark::State& state) {
  std::string raw;
  for (int i = 0; i < 10; ++i) {
    for (const auto& test_set : turbo::strings_internal::base64_strings()) {
      raw += std::string(test_set.plaintext);
    }
  }

  // The actual benchmark loop is tiny...
  std::string escaped;
  for (auto _ : state) {
    turbo::web_safe_base64_encode(raw, &escaped);
  }

  // We want to be sure the compiler doesn't throw away the loop above,
  // and the easiest way to ensure that is to round-trip the results and verify
  // them.
  std::string round_trip;
  turbo::web_safe_base64_decode(escaped, &round_trip);
  TURBO_RAW_CHECK(round_trip == raw, "");
}
BENCHMARK(BM_WebSafeBase64Escape_string);

// Used for the c_encode benchmarks
const char kStringValueNoEscape[] = "1234567890";
const char kStringValueSomeEscaped[] = "123\n56789\xA1";
const char kStringValueMostEscaped[] = "\xA1\xA2\ny\xA4\xA5\xA6z\b\r";

void CEscapeBenchmarkHelper(benchmark::State& state, const char* string_value,
                            int max_len) {
  std::string src;
  while (src.size() < max_len) {
    turbo::str_append(&src, string_value);
  }

  for (auto _ : state) {
    turbo::c_encode(src);
  }
}

void BM_CEscape_NoEscape(benchmark::State& state) {
  CEscapeBenchmarkHelper(state, kStringValueNoEscape, state.range(0));
}
BENCHMARK(BM_CEscape_NoEscape)->Range(1, 1 << 14);

void BM_CEscape_SomeEscaped(benchmark::State& state) {
  CEscapeBenchmarkHelper(state, kStringValueSomeEscaped, state.range(0));
}
BENCHMARK(BM_CEscape_SomeEscaped)->Range(1, 1 << 14);

void BM_CEscape_MostEscaped(benchmark::State& state) {
  CEscapeBenchmarkHelper(state, kStringValueMostEscaped, state.range(0));
}
BENCHMARK(BM_CEscape_MostEscaped)->Range(1, 1 << 14);

}  // namespace
