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

#include <turbo/log/daily_file_sink.h>
#include <turbo/time/clock.h>

namespace turbo {
    /*
    DailyFileSink::DailyFileSink(turbo::string_view base_filename,
                                 int rotation_hour,
                                 int rotation_minute,
                                 bool truncate,
                                 uint16_t max_files)
        : _base_filename(base_filename),
          _rotation_hour(rotation_hour),
          _rotation_minute(rotation_minute),
          _truncate(truncate),
          _max_files(max_files) {
        _next_rotation_time = turbo::Now();
        _next_rotation_time = _next_rotation_time.RoundDown(turbo::Time::kMicrosecondsPerDay);
        _next_rotation_time += turbo::DuratioFromHours(_rotation_hour);
        _next_rotation_time += turbo::TimeDelta::FromMinutes(_rotation_minute);
    }

    DailyFileSink::~DailyFileSink() {
        Flush();
    }

    void DailyFileSink::Send(const LogEntry& entry) {
        turbo::Time now = turbo::Time::Now();
        if (now >= _next_rotation_time) {
            Flush();
            _next_rotation_time += turbo::TimeDelta::FromDays(1);
        }
        // Write to the current file
    }

    turbo::Time next_rotation_time() const {

    }

    void DailyFileSink::Flush() {
        // Close the current file
        // Rotate the files
    }*/
}  // namespace  turbo