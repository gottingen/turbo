// Copyright 2022 The Turbo Authors.
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//      https://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.
//
#include "turbo/files/sys/sequential_read_file.h"
#include "turbo/files/sys/sys_io.h"
#include "turbo/base/casts.h"
#include "turbo/log/logging.h"
#include <cerrno>
#include <cstdio>
#include <cstring>
#include <fcntl.h>
#include <unistd.h>

namespace turbo {

    SequentialReadFile::SequentialReadFile(const FileEventListener &listener)
            : _listener(listener) {

    }

    SequentialReadFile::~SequentialReadFile() {
        close();
    }

    turbo::Status
    SequentialReadFile::open(const turbo::filesystem::path &path, const turbo::FileOption &option) noexcept {
        close();
        _option = option;
        _file_path = path;
        TURBO_ASSERT(!_file_path.empty());
        if (_listener.before_open) {
            _listener.before_open(_file_path);
        }

        for (int tries = 0; tries < _option.open_tries; ++tries) {
            auto rs = turbo::sys_io::open_read(_file_path, "rb", _option);
            if (rs.ok()) {
                _fd = rs.value();
                if (_listener.after_open) {
                    _listener.after_open(_file_path, _fd);
                }
                return turbo::ok_status();
            }
            if (_option.open_interval > 0) {
                turbo::sleep_for(turbo::milliseconds(_option.open_interval));
            }
        }
        return turbo::errno_to_status(errno, turbo::format("Failed opening file {} for reading", _file_path.c_str()));
    }

    turbo::ResultStatus<size_t> SequentialReadFile::read(void *buff, size_t len) {
        INVALID_FD_RETURN(_fd);
        if (len == 0) {
            return 0;
        }
        size_t has_read = 0;
        char *pdata = static_cast<char *>(buff);
        while (has_read < len) {
            auto left = len - has_read;
            auto n = ::read(_fd, pdata + static_cast<std::ptrdiff_t>(has_read), left);
            if (n < 0) {
                return turbo::errno_to_status(errno, "");
            }
            _position += n;
            has_read += n;
            if (n < left) {
                break;
            }

        };
        return has_read;
    }

    turbo::ResultStatus<size_t> SequentialReadFile::read(std::string *content, size_t n) {
        INVALID_FD_RETURN(_fd);
        size_t len = n;
        if (len == kInfiniteFileSize) {
            auto r = turbo::sys_io::file_size(_fd);
            if (!r.ok()) {
                return r;
            }
            len = r.value();
        }
        auto pre_len = content->size();
        content->resize(pre_len + len);
        char *pdata = content->data() + pre_len;
        auto rs = read(pdata, len);
        if (!rs.ok()) {
            content->resize(pre_len);
            return rs;
        }
        content->resize(pre_len + rs.value());
        _position += rs.value();
        return rs.value();
    }

    turbo::ResultStatus<size_t> SequentialReadFile::read(turbo::IOBuf *buf, size_t n) {
        INVALID_FD_RETURN(_fd);
        size_t len = n;
        if (len == kInfiniteFileSize) {
            auto r = turbo::sys_io::file_size(_fd);
            if (!r.ok()) {
                return r;
            }
            len = r.value();
        }
        IOPortal portal;
        auto r = portal.append_from_file_descriptor(_fd, len);
        if (!r.ok()) {
            return r;
        }
        _position += r.value();
        buf->append(IOBuf::Movable(portal));
        return r.value();
    }

    void SequentialReadFile::close() {
        if (_fd > 0) {
            if (_listener.before_close) {
                _listener.before_close(_file_path, _fd);
            }

            ::close(_fd);
            _fd = INVALID_FILE_HANDLER;

            if (_listener.after_close) {
                _listener.after_close(_file_path);
            }
        }
    }

    turbo::Status SequentialReadFile::skip(off_t n) {
        INVALID_FD_RETURN(_fd);

        ::lseek(_fd, implicit_cast<off_t>(n), SEEK_CUR);
        return turbo::ok_status();
    }

    turbo::ResultStatus<bool> SequentialReadFile::is_eof() const {
        INVALID_FD_RETURN(_fd);
        auto fp = fdopen(_fd, "rb");
        if (fp == nullptr) {
            return turbo::errno_to_status(errno, turbo::format("test file eof {}", _file_path.c_str()));
        }
        auto ret = ::feof(fp);
        if (ret < 0) {
            return turbo::errno_to_status(errno, turbo::format("test file eof {}", _file_path.c_str()));
        }
        return ret;
    }

} // namespace turbo