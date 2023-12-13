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

#include <turbo/log/details/console_globals.h>
#include "turbo/log/details/null_mutex.h"
#include <turbo/log/sinks/sink.h>
#include <memory>
#include <mutex>
#include <string>
#include <array>

namespace turbo::tlog::sinks {

/**
 * This sink prefixes the output with an ANSI escape sequence color code
 * depending on the severity
 * of the message.
 * If no color terminal detected, omit the escape codes.
 */

    template<typename ConsoleMutex>
    class ansicolor_sink : public sink {
    public:
        using mutex_t = typename ConsoleMutex::mutex_t;

        ansicolor_sink(FILE *target_file, color_mode mode);

        ~ansicolor_sink() override = default;

        ansicolor_sink(const ansicolor_sink &other) = delete;

        ansicolor_sink(ansicolor_sink &&other) = delete;

        ansicolor_sink &operator=(const ansicolor_sink &other) = delete;

        ansicolor_sink &operator=(ansicolor_sink &&other) = delete;

        void set_color(level::level_enum color_level, std::string_view color);

        void set_color_mode(color_mode mode);

        bool should_color();

        void log(const details::log_msg &msg) override;

        void flush() override;

        void set_pattern(const std::string &pattern) final;

        void set_formatter(std::unique_ptr<turbo::tlog::formatter> sink_formatter) override;

        // Formatting codes
        const std::string_view reset = "\033[m";
        const std::string_view bold = "\033[1m";
        const std::string_view dark = "\033[2m";
        const std::string_view underline = "\033[4m";
        const std::string_view blink = "\033[5m";
        const std::string_view reverse = "\033[7m";
        const std::string_view concealed = "\033[8m";
        const std::string_view clear_line = "\033[K";

        // Foreground colors
        const std::string_view black = "\033[30m";
        const std::string_view red = "\033[31m";
        const std::string_view green = "\033[32m";
        const std::string_view yellow = "\033[33m";
        const std::string_view blue = "\033[34m";
        const std::string_view magenta = "\033[35m";
        const std::string_view cyan = "\033[36m";
        const std::string_view white = "\033[37m";

        /// Background colors
        const std::string_view on_black = "\033[40m";
        const std::string_view on_red = "\033[41m";
        const std::string_view on_green = "\033[42m";
        const std::string_view on_yellow = "\033[43m";
        const std::string_view on_blue = "\033[44m";
        const std::string_view on_magenta = "\033[45m";
        const std::string_view on_cyan = "\033[46m";
        const std::string_view on_white = "\033[47m";

        /// Bold colors
        const std::string_view yellow_bold = "\033[33m\033[1m";
        const std::string_view red_bold = "\033[31m\033[1m";
        const std::string_view bold_on_red = "\033[1m\033[41m";

    private:
        FILE *target_file_;
        mutex_t &mutex_;
        bool should_do_colors_;
        std::unique_ptr<turbo::tlog::formatter> formatter_;
        std::array<std::string, level::n_levels> colors_;

        void print_ccode_(const std::string_view &color_code);

        void print_range_(const memory_buf_t &formatted, size_t start, size_t end);

        static std::string to_string_(const std::string_view &sv);
    };

    template<typename ConsoleMutex>
    class ansicolor_stdout_sink : public ansicolor_sink<ConsoleMutex> {
    public:
        explicit ansicolor_stdout_sink(color_mode mode = color_mode::automatic);
    };

    template<typename ConsoleMutex>
    class ansicolor_stderr_sink : public ansicolor_sink<ConsoleMutex> {
    public:
        explicit ansicolor_stderr_sink(color_mode mode = color_mode::automatic);
    };

    using ansicolor_stdout_sink_mt = ansicolor_stdout_sink<details::console_mutex>;
    using ansicolor_stdout_sink_st = ansicolor_stdout_sink<details::console_nullmutex>;

    using ansicolor_stderr_sink_mt = ansicolor_stderr_sink<details::console_mutex>;
    using ansicolor_stderr_sink_st = ansicolor_stderr_sink<details::console_nullmutex>;

} // namespace turbo::tlog::sinks

