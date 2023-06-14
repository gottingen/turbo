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

namespace turbo {

    class StopWatcher {
    public:

        StopWatcher();

        // Start this timer
        void Start();

        // Stop this timer
        void Stop();

        // Get the elapse from start() to stop(), in various units.
        [[nodiscard]] turbo::Duration Elapsed() const { return _stop - _start; }

        [[nodiscard]] int64_t ElapsedNano() const { return turbo::ToInt64Nanoseconds(_stop - _start); }

        [[nodiscard]] int64_t ElapsedMicro() const { return turbo::ToInt64Microseconds(_stop - _start); }

        [[nodiscard]] int64_t ElapsedMill() const { return turbo::ToInt64Milliseconds(_stop - _start); }

        [[nodiscard]] int64_t ElapsedSec() const { return turbo::ToInt64Seconds(_stop - _start);; }

        [[nodiscard]] double ElapsedNano(double) const { return turbo::ToDoubleNanoseconds(_stop - _start); }

        [[nodiscard]] double ElapsedMicro(double) const { return turbo::ToDoubleMicroseconds(_stop - _start); }

        [[nodiscard]] double ElapsedMill(double) const { return turbo::ToDoubleMilliseconds(_stop - _start); }

        [[nodiscard]] double ElapsedSec(double) const { return turbo::ToDoubleSeconds(_stop - _start); }

    private:
        turbo::Time _start;
        turbo::Time _stop;
    };

}  // namespace turbo

#endif  // TURBO_TIME_STOP_WATCHER_H_
