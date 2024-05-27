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

#include <turbo/log/internal/append_file.h>
#include <turbo/base/internal/strerror.h>
#include <cerrno>
#include <fstream>
#include <cstring>

namespace turbo::log_internal {

    AppendFile::~AppendFile() {
        if (_file != nullptr) {
            ::fclose(_file);
        }
    }

    int AppendFile::initialize(turbo::string_view path) {
        if (_file != nullptr) {
            return 0;
        }
        _path.assign(path.data(), path.size());
        _file = fopen(_path.c_str(), "ab");
        if (_file == nullptr) {
            return errno;
        }
        ::setbuffer(_file, _buffer, sizeof(_buffer));
        return 0;
    }

    int AppendFile::reopen() {
        if (_file != nullptr) {
            ::fclose(_file);
            _file = nullptr;
        }
        _file = fopen(_path.c_str(), "ab");
        if (_file == nullptr) {
            return errno;
        }
        ::setbuffer(_file, _buffer, sizeof(_buffer));
        return 0;
    }

    ssize_t AppendFile::write(turbo::string_view message) {
        if (_file == nullptr) {
            return -1;
        }
        ssize_t written = 0;
        auto len = message.size();
        auto logline = message.data();
        while (written != len) {
            size_t remain = len - written;
            auto n = ::fwrite_unlocked(logline, 1, remain, _file);
            if (n != remain) {
                int err = ferror(_file);
                if (err) {
                    fprintf(stderr, "AppendFile::append() failed %d\n", err);
                    break;
                }
            }
            written += n;
        }

        return written;
    }

    void AppendFile::flush() {
        if (_file == nullptr) {
            return;
        }
        ::fflush(_file);
    }

    void AppendFile::close() {
        if (_file != nullptr) {
            ::fclose(_file);
            _file = nullptr;
        }
    }


}  // namespace turbo::log_internal
