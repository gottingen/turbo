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

#include <turbo/log/sinks/ansicolor_sink.h>
#include <turbo/strings/string_view.h>
#include <turbo/log/internal/fs_helper.h>

namespace turbo {

    AnsiColorSink::AnsiColorSink(FILE *file) : _file(file) {
        _color_active = log_internal::in_terminal(file) && log_internal::is_color_terminal();
    }

    std::array<std::string_view, static_cast<int>(LogSeverity::kFatal) + 1> colors_map = {
            AnsiColorSink::green,  // kInfo
            AnsiColorSink::yellow_bold, // kWarning
            AnsiColorSink::red_bold, // kError
            AnsiColorSink::bold_on_red, // kFatal
    };

    void AnsiColorSink::set_level_color(const LogSeverity severity, const std::string_view color) {
        colors_map[static_cast<int>(severity)] = color;
    }

    void AnsiColorSink::Send(const LogEntry &entry) {
        std::lock_guard<std::mutex> lock(_mutex);
        if (!_color_active) {
            if (entry.log_severity() != LogSeverity::kFatal) {
                ::fwrite(entry.text_message_with_prefix_and_newline().data(), 1,
                         entry.text_message_with_prefix_and_newline().size(), _file);
            } else {
                if (entry.stacktrace().size() > 0) {
                    ::fwrite(entry.text_message_with_prefix_and_newline().data(), 1,
                             entry.text_message_with_prefix_and_newline().size(), _file);
                    ::fwrite(entry.stacktrace().data(), 1, entry.stacktrace().size(), _file);
                }
            }
            return;
        }

        if (entry.log_severity() != LogSeverity::kFatal) {
            auto color = colors_map[static_cast<int>(entry.log_severity())];
            ::fwrite(color.data(), 1, color.size(), _file);
            ::fwrite(entry.text_message_with_prefix_and_newline().data(), 1, entry.text_message_with_prefix_and_newline().size() - entry.text_message_with_newline().size(), _file);
            ::fwrite(reset.data(), 1, reset.size(), _file);
            ::fwrite(entry.text_message_with_newline().data(), 1,
                     entry.text_message_with_newline().size(), _file);
        } else {
            if (entry.stacktrace().size() > 0) {
                auto color = colors_map[static_cast<int>(entry.log_severity())];
                ::fwrite(color.data(), 1, color.size(), _file);
                ::fwrite(entry.text_message_with_prefix_and_newline().data(), 1, entry.text_message_with_prefix_and_newline().size() - entry.text_message_with_newline().size(), _file);
                ::fwrite(reset.data(), 1, reset.size(), _file);
                ::fwrite(entry.text_message_with_newline().data(), 1,
                         entry.text_message_with_newline().size(), _file);
                ::fwrite(entry.stacktrace().data(), 1, entry.stacktrace().size(), _file);
            }
        }
    }

    void AnsiColorSink::Flush() {
        std::lock_guard<std::mutex> lock(_mutex);
        ::fflush(_file);
    }

}  // namespace turbo
