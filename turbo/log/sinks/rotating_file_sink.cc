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

#include <turbo/log/sinks/rotating_file_sink.h>
#include <turbo/log/internal/fs_helper.h>
#include <turbo/strings/str_format.h>
#include <turbo/time/clock.h>

namespace turbo {

    std::string RotatingFileSink::calc_filename(const std::string &filename, std::size_t index) {
        if (index == 0u) {
            return filename;
        }

        std::string basename, ext;
        char buff[256];
        std::tie(basename, ext) = log_internal::split_by_extension(filename);
        turbo::SNPrintF(buff, sizeof(buff), "%s.%zu%s", basename.c_str(), index, ext.c_str());
        return buff;
    }

    RotatingFileSink::RotatingFileSink(turbo::string_view base_filename,std::size_t max_size,
            std::size_t max_files,int check_interval_s) : _base_filename(base_filename), max_size_(max_size), max_files_(max_files), _check_interval_s(check_interval_s), _next_check_time(turbo::Now() + turbo::Seconds(check_interval_s)) {
        _file_writer = std::make_unique<log_internal::AppendFile>();
        _file_writer->initialize(_base_filename);
        do_rotate(turbo::Now());
    }

    RotatingFileSink::~RotatingFileSink() {
        if(_file_writer) {
            _file_writer->close();
        }
    }

    void RotatingFileSink::Send(const LogEntry &entry) {
        std::unique_lock<std::mutex> lock(_mutex);
        do_rotate(entry.timestamp());
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

    void RotatingFileSink::Flush() {
        std::lock_guard<std::mutex> lock(_mutex);
        _file_writer->flush();
    }

    void RotatingFileSink::do_rotate(turbo::Time stmp) {
        if (stmp >= _next_check_time) {
            _file_writer->close();
            _file_writer->reopen();
            _next_check_time = stmp + turbo::Seconds(_check_interval_s);

        }

        if(max_size_ == 0) {
            return;
        }

        if(_file_writer->file_size() < max_size_) {
            return;
        }
        _file_writer->close();
        for (auto i = max_files_; i > 0; --i) {
            std::string src = calc_filename(_base_filename, i - 1);
            if (!log_internal::path_exists(src)) {
                continue;
            }
            std::string target = calc_filename(_base_filename, i);

            rename_file(src, target);
        }
        _file_writer->reopen();
    }
    bool RotatingFileSink::rename_file(const std::string &src_filename, const std::string &target_filename) {
        (void)log_internal::remove(target_filename);
        return log_internal::rename(src_filename, target_filename) == 0;
    }
}  // namespace turbo
