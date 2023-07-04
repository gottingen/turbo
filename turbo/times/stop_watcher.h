// Copyright 2023 The Turbo Authors.
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
//

#ifndef TURBO_TIME_STOP_WATCHER_H_
#define TURBO_TIME_STOP_WATCHER_H_

#include "turbo/times/clock.h"
#include "turbo/format/format.h"

namespace turbo {

    class StopWatcher {
    public:

        StopWatcher();

        // Start this timer
        void reset();

        // Get the elapse from start() to stop(), in various units.
        [[nodiscard]] turbo::Duration elapsed() const;

        [[nodiscard]] int64_t elapsed_nano() { return turbo::ToInt64Nanoseconds(elapsed()); }

        [[nodiscard]] int64_t elapsed_micro() { return turbo::ToInt64Microseconds(elapsed()); }

        [[nodiscard]] int64_t elapsed_mill() { return turbo::ToInt64Milliseconds(elapsed()); }

        [[nodiscard]] int64_t elapsed_sec() { return turbo::ToInt64Seconds(elapsed());; }

        [[nodiscard]] double elapsed_nano(double) { return turbo::ToDoubleNanoseconds(elapsed()); }

        [[nodiscard]] double elapsed_micro(double) { return turbo::ToDoubleMicroseconds(elapsed()); }

        [[nodiscard]] double elapsed_mill(double) { return turbo::ToDoubleMilliseconds(elapsed()); }

        [[nodiscard]] double elapsed_sec(double) { return turbo::ToDoubleSeconds(elapsed()); }
        [[nodiscard]] std::chrono::duration<double> elapsed_chrono() const { return turbo::ToChronoNanoseconds(elapsed()); }

    private:
        turbo::Time _start;
        turbo::Time _stop;
    };

}  // namespace turbo

namespace fmt {

    // Support for fmt formatting  (e.g. "{:012.9}" or just "{}")
    template<>
    struct formatter<turbo::StopWatcher> : formatter<double> {
        template<typename FormatContext>
        auto format(const turbo::StopWatcher &sw, FormatContext &ctx) -> decltype(ctx.out()) {
            return formatter<double>::format(sw.elapsed_chrono().count(), ctx);
        }
    };
} // namespace std


#endif  // TURBO_TIME_STOP_WATCHER_H_
