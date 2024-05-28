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
#include <turbo/time/time.h>
#include <turbo/log/internal/append_file.h>
#include <turbo/container/circular_queue.h>
#include <turbo/log/log_sink_registry.h>

namespace turbo {

    class DailyFileSink : public LogSink {
    public:
        DailyFileSink(turbo::string_view base_filename,
                      int rotation_hour = 0,
                      int rotation_minute = 0,
                      int check_interval_s = 60,
                      bool truncate = false,
                      uint16_t max_files = 0);

        ~DailyFileSink() override;

        void Send(const LogEntry& entry) override;

        void Flush() override;
    private:
        turbo::Time next_rotation_time(turbo::Time stamp) const;
        void init_file_queue();
        void rotate_file(turbo::Time stamp);
    private:
        std::string _base_filename;
        int _rotation_hour;
        int _rotation_minute;
        bool _truncate;
        uint16_t _max_files;
        int  _check_interval_s;
        turbo::Time _next_check_time;
        turbo::Time _next_rotation_time;
        circular_queue<std::string> _files;
        std::unique_ptr<FileWriter> _file_writer;
        std::mutex _mutex;

    };
}  // namespace  turbo
