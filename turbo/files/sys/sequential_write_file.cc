// Copyright 2023 The Turbo Authors.
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

#include "turbo/files/sys/sequential_write_file.h"
#include "turbo/files/sys/sys_io.h"
#include <unistd.h>
#include <fcntl.h>
#include <cerrno>
#include <cstring>
#include "turbo/base/casts.h"
#include <iostream>
#include "turbo/platform/port.h"
#include "turbo/log/logging.h"

namespace turbo {

    SequentialWriteFile::SequentialWriteFile(const FileEventListener &listener) : _listener(listener) {

    }

    SequentialWriteFile::~SequentialWriteFile() {
        close();
    }

    turbo::Status SequentialWriteFile::open(const turbo::filesystem::path &fname, bool truncate, const turbo::FileOption &option)  noexcept {
        close();
        _option = option;
        _file_path = fname;
        TURBO_ASSERT(!_file_path.empty());
        auto *mode = "ab";
        auto *trunc_mode = "wb";

        if (_listener.before_open) {
            _listener.before_open(_file_path);
        }
        for (int tries = 0; tries < _option.open_tries; ++tries) {
            // create containing folder if not exists already.
            if (_option.create_dir_if_miss) {
                auto pdir = _file_path.parent_path();
                if (!pdir.empty()) {
                    std::error_code ec;
                    if (!turbo::filesystem::exists(pdir, ec)) {
                        if (ec) {
                            continue;
                        }
                        if (!turbo::filesystem::create_directories(pdir, ec)) {
                            continue;
                        }
                    }
                }
            }
            if (truncate) {
                // Truncate by opening-and-closing a tmp file in "wb" mode, always
                // opening the actual log-we-write-to in "ab" mode, since that
                // interacts more politely with eternal processes that might
                // rotate/truncate the file underneath us.
                auto rs = turbo::sys_io::open_write(_file_path, trunc_mode, _option);
                if (!rs.ok()) {
                    continue;
                }
                ::close(rs.value());
            }
            auto rs = turbo::sys_io::open_write(_file_path, mode, _option);
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
        return turbo::errno_to_status(errno, turbo::format("Failed opening file {} for writing", _file_path.c_str()));
    }

    turbo::Status SequentialWriteFile::reopen(bool truncate) {
        close();
        if (_file_path.empty()) {
            return turbo::invalid_argument_error("file name empty");
        }
        return open(_file_path, truncate);
    }

    turbo::Status SequentialWriteFile::write(const void *data, size_t size) {
        INVALID_FD_RETURN(_fd);
        if (::write(_fd, data, size) != size) {
            return turbo::errno_to_status(errno, turbo::format("Failed writing to file {}", _file_path.c_str()));
        }
        return turbo::ok_status();
    }

    [[nodiscard]] turbo::Status SequentialWriteFile::write(const turbo::IOBuf &buff)  {
        size_t size = buff.size();
        IOBuf piece_data(buff);
        ssize_t left = size;
        while (left > 0) {
            auto wrs = piece_data.cut_into_file_descriptor(_fd, left);
            if (wrs.ok() && wrs.value() > 0) {
                left -= wrs.value();
            } else if (is_unavailable(wrs.status())) {
                continue;
            } else {
                TLOG_WARN("write falied, err: {} fd: {} size: {}", wrs.status().to_string(), _fd, size);
                return wrs.status();
            }
        }

        return turbo::ok_status();
    }

    turbo::ResultStatus<size_t> SequentialWriteFile::size() const {
        INVALID_FD_RETURN(_fd);
        auto rs = turbo::sys_io::file_size(_fd);
        return rs;
    }

    void SequentialWriteFile::close() {
        if (_fd != INVALID_FILE_HANDLER) {
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

    turbo::Status SequentialWriteFile::truncate(size_t size) {
        INVALID_FD_RETURN(_fd);
        if (::ftruncate(_fd, static_cast<off_t>(size)) != 0) {
            return turbo::errno_to_status(errno, turbo::format("Failed truncate file {} for size:{} ", _file_path.c_str(),
                                                             static_cast<off_t>(size)));
        }
        if(::lseek(_fd, static_cast<off_t>(size), SEEK_SET) != 0) {
            return turbo::errno_to_status(errno, turbo::format("Failed seek file end {} for size:{} ", _file_path.c_str(),
                                                             static_cast<off_t>(size)));
        }
        return turbo::ok_status();
    }

    turbo::Status SequentialWriteFile::flush() {
        INVALID_FD_RETURN(_fd);
        if (::fsync(_fd) != 0) {
            return turbo::errno_to_status(errno,
                                        turbo::format("Failed flush to file {}", _file_path.c_str()));
        }
        return turbo::ok_status();
    }

    const turbo::filesystem::path &SequentialWriteFile::file_path() const {
        return _file_path;
    }

}  // namespace turbo