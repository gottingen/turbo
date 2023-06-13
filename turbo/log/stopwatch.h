// Copyright 2023 The titan-search Authors.
// Copyright(c) 2015-present, Gabi Melman & spdlog contributors.
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
//

#pragma once

#include <turbo/format/str_format.h>
#include <chrono>

// Stopwatch support for spdlog  (using std::chrono::steady_clock).
// Displays elapsed seconds since construction as double.
//
// Usage:
//
// turbo::tlog::stopwatch sw;
// ...
// turbo::tlog::debug("Elapsed: {} seconds", sw);    =>  "Elapsed 0.005116733 seconds"
// turbo::tlog::info("Elapsed: {:.6} seconds", sw);  =>  "Elapsed 0.005163 seconds"
//
//
// If other units are needed (e.g. millis instead of double), include "fmt/chrono.h" and use "duration_cast<..>(sw.elapsed())":
//
// #include <turbo/log/fmt/chrono.h>
//..
// using std::chrono::duration_cast;
// using std::chrono::milliseconds;
// turbo::tlog::info("Elapsed {}", duration_cast<milliseconds>(sw.elapsed())); => "Elapsed 5ms"

namespace turbo::tlog {
    class stopwatch {
        using clock = std::chrono::steady_clock;
        std::chrono::time_point<clock> start_tp_;

    public:
        stopwatch()
                : start_tp_{clock::now()} {}

        std::chrono::duration<double> elapsed() const {
            return std::chrono::duration<double>(clock::now() - start_tp_);
        }

        void reset() {
            start_tp_ = clock::now();
        }
    };
} // namespace turbo::tlog

// Support for fmt formatting  (e.g. "{:012.9}" or just "{}")
namespace fmt {

    template<>
    struct formatter<turbo::tlog::stopwatch> : formatter<double> {
        template<typename FormatContext>
        auto format(const turbo::tlog::stopwatch &sw, FormatContext &ctx) -> decltype(ctx.out()) {
            return formatter<double>::format(sw.elapsed().count(), ctx);
        }
    };
} // namespace std
