//
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
//
#pragma once

#include <turbo/log/log_sink.h>
#include <deque>
#include <memory>
#include <mutex>
#include <turbo/log/internal/append_file.h>
#include <turbo/times/time.h>
#include <turbo/container/circular_queue.h>
#include <turbo/log/log_sink_registry.h>

namespace turbo {

    class AnsiColorSink : public LogSink {
    public:
        AnsiColorSink(FILE *file);

        ~AnsiColorSink() override = default;

        void Send(const LogEntry& entry) override;

        void Flush() override;

        static void set_level_color(const LogSeverity severity, const std::string_view color);
    public:
        // Formatting codes
        static constexpr std::string_view reset = "\033[m";
        static constexpr std::string_view bold = "\033[1m";
        static constexpr std::string_view dark = "\033[2m";
        static constexpr std::string_view underline = "\033[4m";
        static constexpr std::string_view blink = "\033[5m";
        static constexpr std::string_view reverse = "\033[7m";
        static constexpr std::string_view concealed = "\033[8m";
        static constexpr std::string_view clear_line = "\033[K";

        // Foreground colors
        static constexpr std::string_view black = "\033[30m";
        static constexpr std::string_view red = "\033[31m";
        static constexpr std::string_view green = "\033[32m";
        static constexpr std::string_view yellow = "\033[33m";
        static constexpr std::string_view blue = "\033[34m";
        static constexpr std::string_view magenta = "\033[35m";
        static constexpr std::string_view cyan = "\033[36m";
        static constexpr std::string_view white = "\033[37m";

        /// Background colors
        static constexpr std::string_view on_black = "\033[40m";
        static constexpr std::string_view on_red = "\033[41m";
        static constexpr std::string_view on_green = "\033[42m";
        static constexpr std::string_view on_yellow = "\033[43m";
        static constexpr std::string_view on_blue = "\033[44m";
        static constexpr std::string_view on_magenta = "\033[45m";
        static constexpr std::string_view on_cyan = "\033[46m";
        static constexpr std::string_view on_white = "\033[47m";

        /// Bold colors
        static constexpr std::string_view yellow_bold = "\033[33m\033[1m";
        static constexpr std::string_view red_bold = "\033[31m\033[1m";
        static constexpr std::string_view bold_on_red = "\033[1m\033[41m";
    private:
        FILE *_file{nullptr};
        std::mutex _mutex;
        bool _color_active{false};
    };

    class AnsiColorStdoutSink : public AnsiColorSink {
    public:
        static AnsiColorStdoutSink *instance() {
            static AnsiColorStdoutSink sink;
            return &sink;
        }
    private:
        AnsiColorStdoutSink() : AnsiColorSink(stdout) {}

    };

    class AnsiColorStderrSink : public AnsiColorSink {
    public:
        static AnsiColorStderrSink *instance() {
            static AnsiColorStderrSink sink;
            return &sink;
        }
    private:
        AnsiColorStderrSink() : AnsiColorSink(stderr) {}
    };
}  // namespace turbo

