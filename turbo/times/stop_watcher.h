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
#include <string>
#include <iostream>

namespace turbo {

    class StopWatcher {
    protected:
        /// This is the type of a printing function, you can make your own
        using time_print_t = std::function<std::string(std::string_view, std::string_view)>;

        /// Standard print function, this one is set by default
        static std::string Simple(std::string_view title, std::string_view time) {
            return turbo::Format("{}: {}", title, time);
        }

        /// This is a fancy print function with --- headers
        static std::string Big(std::string_view title, std::string_view time) {
            return turbo::Format(
                    "-----------------------------------------\n| {} | time = {}\n-----------------------------------------",
                    title, time);
        }

    public:
        StopWatcher(const std::string &title = "Timer", time_print_t time_print = Simple);

        // Start this timer
        void reset();

        const StopWatcher &stop();

        // Get the elapse from start() to stop(), in various units.
        [[nodiscard]] turbo::Duration elapsed() const;

        [[nodiscard]] int64_t elapsed_nano() const { return turbo::ToInt64Nanoseconds(elapsed()); }

        [[nodiscard]] int64_t elapsed_micro() const { return turbo::ToInt64Microseconds(elapsed()); }

        [[nodiscard]] int64_t elapsed_mill() const { return turbo::ToInt64Milliseconds(elapsed()); }

        [[nodiscard]] int64_t elapsed_sec() const { return turbo::ToInt64Seconds(elapsed());; }

        [[nodiscard]] double elapsed_nano(double) const { return turbo::ToDoubleNanoseconds(elapsed()); }

        [[nodiscard]] double elapsed_micro(double) const { return turbo::ToDoubleMicroseconds(elapsed()); }

        [[nodiscard]] double elapsed_mill(double) const { return turbo::ToDoubleMilliseconds(elapsed()); }

        [[nodiscard]] double elapsed_sec(double) const { return turbo::ToDoubleSeconds(elapsed()); }

        [[nodiscard]] std::chrono::duration<double> elapsed_chrono() const {
            return turbo::ToChronoNanoseconds(elapsed());
        }

        /// This prints out a time string from a time
        std::string make_time_str(turbo::Duration span) const {  // NOLINT(modernize-use-nodiscard)
            if (span < turbo::Microseconds(1))
                return turbo::Format("{}ns", turbo::ToDoubleNanoseconds(span));
            if (span < turbo::Milliseconds(1))
                return turbo::Format("{}us", turbo::ToDoubleMicroseconds(span));
            if (span < turbo::Seconds(1))
                return turbo::Format("{}us", turbo::ToDoubleMilliseconds(span));
            return turbo::Format("{}us", turbo::ToDoubleSeconds(span));
        }

        std::string to_string() const {
            return _time_print(_title, make_time_str(elapsed()));
        }

    private:
        static constexpr turbo::Duration kZero = turbo::Nanoseconds(0);
        turbo::Time _start;
        std::string _title;
        time_print_t _time_print;
        turbo::Duration _duration;
    };

    /// This class prints out the time upon destruction
    class AutoWatcher {
    public:
        /// Reimplementing the constructor is required in GCC 4.7
        explicit AutoWatcher(StopWatcher *w, bool show = false) : _w(w), _show(show) {}
        // GCC 4.7 does not support using inheriting constructors.

        /// This destructor prints the string
        ~AutoWatcher() {
            _w->stop();
            if(_show) {
                std::cout<<_w->to_string()<<std::endl;
            }
        }

    private:
        StopWatcher *_w;
        bool _show;
    };

}  // namespace turbo

inline std::ostream &operator<<(std::ostream &in, const turbo::StopWatcher &timer) { return in << timer.to_string(); }

namespace fmt {

    // Support for fmt formatting  (e.g. "{:012.9}" or just "{}")
    template<>
    struct formatter<turbo::StopWatcher> : formatter<std::string> {
        template<typename FormatContext>
        auto format(const turbo::StopWatcher &sw, FormatContext &ctx) -> decltype(ctx.out()) {
            return formatter<std::string>::format(sw.to_string(), ctx);
        }
    };
} // namespace std


#endif  // TURBO_TIME_STOP_WATCHER_H_
