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
#include <turbo/time/time.h>
#include <turbo/container/circular_queue.h>
#include <turbo/log/log_sink_registry.h>

namespace turbo {

    class RotatingFileSink : public LogSink {
    public:
        RotatingFileSink(turbo::string_view base_filename,
                         std::size_t max_size,
                         std::size_t max_files = 0,
                         int check_interval_s = 60);

        ~RotatingFileSink() override;

        void Send(const LogEntry &entry) override;

        void Flush() override;

        static std::string calc_filename(const std::string &filename, std::size_t index);

    private:
        void do_rotate(turbo::Time stmp);

        bool rename_file(const std::string &src_filename, const std::string &target_filename);

    private:
        std::string _base_filename;
        std::size_t max_size_;
        std::size_t max_files_;
        int _check_interval_s;
        turbo::Time _next_check_time;
        std::unique_ptr<FileWriter> _file_writer;
        std::mutex _mutex;

    };
}  // namespace  turbo
