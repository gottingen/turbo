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


#include <turbo/log/sinks/hourly_file_sink.h>
#include <turbo/times/clock.h>

#include <algorithm>
#include <array>
#include <chrono>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <ctime>
#include <string>
#include <sys/stat.h>
#include <sys/types.h>
#include <iostream>
#include <thread>
#include <turbo/strings/str_format.h>
#include <turbo/log/internal/fs_helper.h>

namespace turbo {

    static std::string calc_hourly_filename(const std::string &filename, const tm &now_tm) {
        std::string basename, ext;
        std::tie(basename, ext) = log_internal::split_by_extension(filename);
        char buff[256];
        turbo::SNPrintF(buff, sizeof(buff), "%s_%04d-%02d-%02d%s", basename.c_str(),
                        now_tm.tm_year + 1900, now_tm.tm_mon + 1, now_tm.tm_mday, ext.c_str());
        return buff;
    }

    HourlyFileSink::HourlyFileSink(turbo::string_view base_filename,
                                 int rotation_minute,
                                 int check_interval_s,
                                 bool truncate,
                                 uint16_t max_files)
            : _base_filename(base_filename),
              _rotation_minute(rotation_minute),
              _truncate(truncate),
              _max_files(max_files),
              _check_interval_s(check_interval_s),
              _next_check_time(turbo::Time::current_time() + turbo::Duration::seconds(check_interval_s)) {
        _next_rotation_time = next_rotation_time(turbo::Time::current_time());
        if (_max_files > 0) {
            init_file_queue();
        }
        auto now = turbo::Time::current_time();
        auto filename = calc_hourly_filename(_base_filename, turbo::Time::to_tm(now, turbo::TimeZone::local()));
        _file_writer = std::make_unique<log_internal::AppendFile>();
        if (_truncate) {
            ::remove(filename.c_str());
        }
        _file_writer->initialize(filename);
    }

    HourlyFileSink::~HourlyFileSink() {
        if (_file_writer != nullptr) {
            _file_writer->close();
        }
    }

    void HourlyFileSink::Send(const LogEntry &entry) {
        std::unique_lock lock(_mutex);
        rotate_file(entry.timestamp());
        if (_file_writer == nullptr) {
            return;
        }
        // Write to the current file
        if(TURBO_PREDICT_TRUE(entry.log_severity() != LogSeverity::kFatal)) {
            _file_writer->write(entry.text_message_with_prefix_and_newline());
        } else {
            if(!entry.stacktrace().empty()) {
                _file_writer->write(entry.text_message_with_prefix_and_newline());
                _file_writer->write(entry.stacktrace());
            }

        }
    }

    void HourlyFileSink::init_file_queue() {
        std::vector<std::string> filenames;
        _files = circular_queue<std::string>(static_cast<size_t>(_max_files));
        auto now = turbo::Time::current_time();
        while (filenames.size() < _max_files) {
            auto filename = calc_hourly_filename(_base_filename, turbo::Time::to_tm(now, turbo::TimeZone::local()));
            if (!log_internal::path_exists(filename)) {
                break;
            }
            filenames.emplace_back(filename);
            now -= turbo::Duration::hours(1);
        }
        for (auto iter = filenames.rbegin(); iter != filenames.rend(); ++iter) {
            _files.push_back(std::move(*iter));
        }
    }

    void HourlyFileSink::rotate_file(turbo::Time stamp) {
        if (stamp >= _next_check_time) {
            _next_check_time = stamp + turbo::Duration::seconds(_check_interval_s);
            if(_file_writer != nullptr)
                _file_writer->reopen();
        }
        if (stamp < _next_rotation_time) {
            return;
        }
        _next_rotation_time = next_rotation_time(stamp);
        auto filename = calc_hourly_filename(_base_filename, turbo::Time::to_tm(stamp, turbo::TimeZone::local()));
        _file_writer->close();
        _file_writer.reset();
        _file_writer = std::make_unique<log_internal::AppendFile>();
        _file_writer->initialize(filename);
        if (_max_files == 0) {
            return;
        }

        auto current_file = _file_writer->file_path();
        if (_files.full()) {
            auto old_filename = std::move(_files.front());
            _files.pop_front();
            bool ok = log_internal::remove_if_exists(old_filename) == 0;
            if (!ok) {
                _files.push_back(std::move(current_file));
                std::cerr << "Failed removing daily file " + old_filename << std::endl;
            }
        }
        _files.push_back(std::move(current_file));
    }

    turbo::Time HourlyFileSink::next_rotation_time(turbo::Time stamp) const {
        auto tm = turbo::Time::to_tm(stamp, turbo::TimeZone::local());
        tm.tm_min = _rotation_minute;
        tm.tm_sec = 0;
        auto rotation_time = turbo::Time::from_tm(tm, turbo::TimeZone::local());
        if (rotation_time > stamp) {
            return rotation_time;
        }
        return rotation_time + turbo::Duration::hours(1);
    }

    void HourlyFileSink::Flush() {
        std::unique_lock lock(_mutex);
        // Close the current file
        // Rotate the files
        if (_file_writer != nullptr) {
            _file_writer->flush();
        }
    }
}  // namespace  turbo
