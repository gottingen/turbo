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

#include "turbo/files/sequential_write_file.h"
#include <unistd.h>
#include <fcntl.h>
#include <cerrno>
#include <cstring>
#include "turbo/base/casts.h"
#include "turbo/files/fio.h"
#include <iostream>
#include "turbo/platform/port.h"

namespace turbo {

    SequentialWriteFile::SequentialWriteFile(const FileEventListener &listener) : _listener(listener) {

    }

    SequentialWriteFile::~SequentialWriteFile() {
        close();
    }

    void SequentialWriteFile::set_option(const FileOption &option) {
        _option = option;
    }

    turbo::Status SequentialWriteFile::open(const turbo::filesystem::path &fname, bool truncate) {
        close();
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
                if(!pdir.empty()) {
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
                auto rs = Fio::file_open_write(_file_path, trunc_mode, _option);
                if (!rs.ok()) {
                    continue;
                }
                std::fclose(rs.value());
            }
            auto rs = Fio::file_open_write(_file_path, mode, _option);
            if (rs.ok()) {
                _fd = rs.value();
                if (_listener.after_open) {
                    _listener.after_open(_file_path, _fd);
                }
                return turbo::OkStatus();
            }
            if (_option.open_interval > 0) {
                turbo::SleepFor(turbo::Milliseconds(_option.open_interval));
            }
        }
        return turbo::ErrnoToStatus(errno, turbo::Format("Failed opening file {} for writing", _file_path.c_str()));
    }

    turbo::Status SequentialWriteFile::reopen(bool truncate) {
        close();
        if (_file_path.empty()) {
            return turbo::InvalidArgumentError("file name empty");
        }
        return open(_file_path, truncate);
    }

    turbo::Status SequentialWriteFile::write(const char *data, size_t size) {
        TURBO_ASSERT(_fd != nullptr);
        if (std::fwrite(data, 1, size, _fd) != size) {
            return turbo::ErrnoToStatus(errno, turbo::Format("Failed writing to file {}", _file_path.c_str()));
        }
        return turbo::OkStatus();
    }

    turbo::ResultStatus<size_t> SequentialWriteFile::size() const {
        if (_fd == nullptr) {
            return turbo::InvalidArgumentError("Failed getting file size. fp is null");
        }
        auto rs = Fio::file_size(_fd);
        return rs;
    }

    void SequentialWriteFile::close() {
        if (_fd != nullptr) {
            if (_listener.before_close) {
                _listener.before_close(_file_path, _fd);
            }

            std::fclose(_fd);
            _fd = nullptr;

            if (_listener.after_close) {
                _listener.after_close(_file_path);
            }
        }
    }

    turbo::Status SequentialWriteFile::flush() {
        if (std::fflush(_fd) != 0) {
            return turbo::ErrnoToStatus(errno,
                                        turbo::Format("Failed flush to file {}", _file_path.c_str()));
        }
        return turbo::OkStatus();
    }

    const turbo::filesystem::path &SequentialWriteFile::file_path() const {
        return _file_path;
    }

}  // namespace turbo
