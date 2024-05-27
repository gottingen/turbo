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
#include <turbo/time/time.h>

namespace turbo {

    class DailyFileSink : public LogSink {
    public:
        DailyFileSink(turbo::string_view base_filename,
                      int rotation_hour,
                      int rotation_minute,
                      bool truncate = false,
                      uint16_t max_files = 0);

        ~DailyFileSink() override;

        void Send(const LogEntry& entry) override;

        void Flush() override;
    private:
        turbo::Time next_rotation_time() const;
    private:
        std::string _base_filename;
        int _rotation_hour;
        int _rotation_minute;
        bool _truncate;
        uint16_t _max_files;
        turbo::Time _next_rotation_time;
        std::deque<std::string> _files;

    };
}  // namespace  turbo
