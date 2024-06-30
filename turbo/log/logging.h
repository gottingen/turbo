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

#include <turbo/log/log.h>
#include <turbo/log/check.h>
#include <turbo/log/die_if_null.h>
#include <turbo/log/vlog_is_on.h>
#include <turbo/log/globals.h>
#include <turbo/log/initialize.h>

namespace turbo {

    enum class LogSinkType {
        kColorStderr,
        kDailyFile,
        kHourlyFile,
        kRotatingFile,
    };

    // verbose log level for all
    static constexpr int V_ALL = 0;

    // verbose log level for important information
    static constexpr int V_IMPORTANT = 100;

    // verbose log level for debug information
    static constexpr int V_DEBUG = 200;

    // verbose log level for trace information
    static constexpr int V_TRACE = 300;


    void setup_daily_file_sink(const std::string& base_filename,
                                   int rotation_hour = 0,
                                   int rotation_minute = 0,
                                   int check_interval_s = 60,
                                   bool truncate = false,
                                   uint16_t max_files = 0);

    void setup_hourly_file_sink(const std::string& base_filename,
                                  int rotation_minute = 0,
                                  int check_interval_s = 60,
                                  bool truncate = false,
                                  uint16_t max_files = 0);

    void setup_rotating_file_sink(const std::string& base_filename,
                                    int max_file_size_mb = 100,
                                    uint16_t max_files = 100,
                                    bool truncate = false,
                                    int check_interval_s = 60);

    void setup_ansi_color_stdout_sink();

    void setup_color_stderr_sink();

    void enable_stderr_logging(LogSeverityAtLeast threshold = LogSeverityAtLeast::kError);

    void disable_stderr_logging();

    void cleanup_log();

    void load_flags_symbol();

    void setup_log_by_flags();

}  // namespace turbo

