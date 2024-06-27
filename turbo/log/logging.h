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

    struct LogConfig {
        /// format /path/to/log_file.log
        /// or ./logs/log_file.log
        std::string base_filename;
        bool err_to_stderr{true};
        LogSeverityAtLeast stderr_threshold{LogSeverityAtLeast::kError};
        int rotation_hour{0};
        int rotation_minute{0};
        int check_interval_s{60};
        bool truncate{false};
        int max_files{0};
        int max_file_size{0};

        LogConfig &set_base_filename(const std::string &base_filename) {
            this->base_filename = base_filename;
            return *this;
        }
        LogConfig &set_err_to_stderr(bool err_to_stderr) {
            this->err_to_stderr = err_to_stderr;
            return *this;
        }
        LogConfig &set_stderr_threshold(LogSeverityAtLeast stderr_threshold) {
            this->stderr_threshold = stderr_threshold;
            return *this;
        }
        LogConfig &set_rotation_hour(int rotation_hour) {
            this->rotation_hour = rotation_hour;
            return *this;
        }
        LogConfig &set_rotation_minute(int rotation_minute) {
            this->rotation_minute = rotation_minute;
            return *this;
        }
        LogConfig &set_check_interval_s(int check_interval_s) {
            this->check_interval_s = check_interval_s;
            return *this;
        }
        LogConfig &set_truncate(bool truncate) {
            this->truncate = truncate;
            return *this;
        }
        LogConfig &set_max_files(uint16_t max_files) {
            this->max_files = max_files;
            return *this;
        }

        LogConfig &set_max_file_size(int max_file_size) {
            this->max_file_size = max_file_size;
            return *this;
        }
    };

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

}  // namespace turbo

