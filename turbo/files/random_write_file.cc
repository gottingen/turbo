// Copyright 2023 The Elastic-AI Authors.
// part of Elastic AI Search
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
//
// Created by jeff on 23-11-28.
//

#include "turbo/files/random_write_file.h"

namespace turbo {

    RandomWriteFile::RandomWriteFile(const FileEventListener &listener) : _listener(listener) {

    }

    RandomWriteFile::~RandomWriteFile() {
        close();
    }

    void RandomWriteFile::set_option(const FileOption &option) {
        _option = option;
    }

    turbo::Status RandomWriteFile::open(const turbo::filesystem::path &fname, bool truncate) {
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
                _fp = rs.value();
                if (_listener.after_open) {
                    _listener.after_open(_file_path, _fp);
                }
                _fd = fileno(_fp);
                return turbo::ok_status();
            }
            if (_option.open_interval > 0) {
                turbo::SleepFor(turbo::Milliseconds(_option.open_interval));
            }
        }
        return turbo::errno_to_status(errno, turbo::Format("Failed opening file {} for writing", _file_path.c_str()));
    }

    turbo::Status RandomWriteFile::reopen(bool truncate) {
        close();
        if (_file_path.empty()) {
            return turbo::invalid_argument_error("file name empty");
        }
        return open(_file_path, truncate);
    }

    turbo::Status RandomWriteFile::write(size_t offset, const char *data, size_t size, bool truncate) {
        if (_fd == -1) {
            return turbo::unavailable_error("file not open for read yet");
        }

        ssize_t write_size = ::pwrite(_fd, data, size, static_cast<off_t>(offset));
        if(write_size < 0 ) {
            return turbo::errno_to_status(errno, _file_path.c_str());
        }
        if(truncate) {
            if(::ftruncate(_fd, static_cast<off_t>(offset + size)) != 0) {
                return turbo::errno_to_status(errno, turbo::Format("Failed truncate file {} for size:{} ", _file_path.c_str(), static_cast<off_t>(offset + size)));
            }
        }
        return turbo::ok_status();
    }

    turbo::Status RandomWriteFile::truncate(size_t size) {
        if(::ftruncate(_fd, static_cast<off_t>(size)) != 0) {
            return turbo::errno_to_status(errno, turbo::Format("Failed truncate file {} for size:{} ", _file_path.c_str(), static_cast<off_t>(size)));
        }
        return turbo::ok_status();
    }

    turbo::ResultStatus<size_t> RandomWriteFile::size() const {
        if (_fp == nullptr) {
            return turbo::invalid_argument_error("Failed getting file size. fp is null");
        }
        auto rs = Fio::file_size(_fp);
        return rs;
    }

    void RandomWriteFile::close() {
        _fd = -1;
        if (_fp != nullptr) {
            if (_listener.before_close) {
                _listener.before_close(_file_path, _fp);
            }

            std::fclose(_fp);
            _fp = nullptr;

            if (_listener.after_close) {
                _listener.after_close(_file_path);
            }
        }
    }

    turbo::Status RandomWriteFile::flush() {
        if (std::fflush(_fp) != 0) {
            return turbo::errno_to_status(errno,
                                        turbo::Format("Failed flush to file {}", _file_path.c_str()));
        }
        return turbo::ok_status();
    }

    const turbo::filesystem::path &RandomWriteFile::file_path() const {
        return _file_path;
    }

}  // namespace turbo