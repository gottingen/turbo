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

#include <turbo/log/logging.h>
#include <turbo/log/log_sink_registry.h>
#include <turbo/log/initialize.h>
#include <turbo/log/sinks/ansicolor_sink.h>
#include <turbo/log/sinks/daily_file_sink.h>
#include <turbo/log/sinks/hourly_file_sink.h>
#include <turbo/log/sinks/rotating_file_sink.h>
#include <iostream>

namespace turbo {

    std::mutex g_sink_mutex;
    std::shared_ptr<turbo::LogSink> g_sink;

    std::string g_multi_register_die_message = "Sink already registered you can only register one sink"
                                               "if you want to register multiple sinks please use the log_sink_registry.h, "
                                               "and call add_log_sink() to add the sink to the registry";

    void setup_daily_file_sink(const std::string &base_filename,
                               int rotation_hour,
                               int rotation_minute,
                               int check_interval_s,
                               bool truncate,
                               uint16_t max_files) {
        std::unique_lock lock(g_sink_mutex);
        if (g_sink != nullptr) {
            std::cerr << g_multi_register_die_message << std::endl;
            return;
        }

        g_sink = std::make_shared<DailyFileSink>(base_filename, rotation_hour, rotation_minute, check_interval_s,
                                                 truncate, max_files);
        initialize_log();
        add_log_sink(g_sink.get());
    }

    void setup_hourly_file_sink(const std::string &base_filename,
                                int rotation_minute,
                                int check_interval_s,
                                bool truncate,
                                uint16_t max_files) {
        std::unique_lock lock(g_sink_mutex);
        if (g_sink != nullptr) {
            std::cerr << g_multi_register_die_message << std::endl;
            return;
        }

        g_sink = std::make_shared<HourlyFileSink>(base_filename, rotation_minute, check_interval_s, truncate,
                                                  max_files);
        initialize_log();
        add_log_sink(g_sink.get());
    }

    void setup_rotating_file_sink(const std::string& base_filename,
                                  int max_file_size_mb,
                                  uint16_t max_files,
                                  bool truncate,
                                  int check_interval_s) {
        std::unique_lock lock(g_sink_mutex);
        if (g_sink != nullptr) {
            std::cerr << g_multi_register_die_message << std::endl;
            return;
        }
        static const int one_mb = 1024 * 1024;
        g_sink = std::make_shared<RotatingFileSink>(base_filename, max_file_size_mb * one_mb, check_interval_s, max_files);
        initialize_log();
        add_log_sink(g_sink.get());
    }

    void setup_ansi_color_stdout_sink() {
        std::unique_lock lock(g_sink_mutex);
        if (g_sink != nullptr) {
            std::cerr << g_multi_register_die_message << std::endl;
            return;
        }

        g_sink = std::make_shared<AnsiColorSink>(stdout);
        initialize_log();
        set_stderr_threshold(LogSeverityAtLeast::kInfinity);
        add_log_sink(g_sink.get());
    }

    void setup_color_stderr_sink() {
        std::unique_lock lock(g_sink_mutex);
        if (g_sink != nullptr) {
            std::cerr << g_multi_register_die_message << std::endl;
            return;
        }

        g_sink = std::make_shared<AnsiColorSink>(stderr);
        initialize_log();
        set_stderr_threshold(LogSeverityAtLeast::kInfinity);
        add_log_sink(g_sink.get());
    }

    void cleanup_log() {
        std::unique_lock lock(g_sink_mutex);
        if (g_sink != nullptr) {
            remove_log_sink(g_sink.get());
            g_sink = nullptr;
        }

    }

    void enable_stderr_logging(LogSeverityAtLeast threshold) {
        set_stderr_threshold(threshold);
    }

    void disable_stderr_logging() {
        set_stderr_threshold(LogSeverityAtLeast::kInfinity);
    }
}  // namespace turbo