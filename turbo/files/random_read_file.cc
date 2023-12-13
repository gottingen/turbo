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
#include "turbo/files/random_read_file.h"
#include "turbo/files/fio.h"
#include "turbo/base/casts.h"
#include "turbo/log/logging.h"
#include <cerrno>
#include <cstdio>
#include <cstring>
#include <fcntl.h>
#include <unistd.h>

namespace turbo {

    RandomReadFile::RandomReadFile(const FileEventListener &listener)
            : _listener(listener) {

    }

    RandomReadFile::~RandomReadFile() {
        close();
    }

    turbo::Status
    RandomReadFile::open(const turbo::filesystem::path &path, const turbo::FileOption &option) noexcept {
        close();
        _option = option;
        _file_path = path;
        if(_file_path.empty()) {
            return turbo::invalid_argument_error("file path is empty");
        }
        if (_listener.before_open) {
            _listener.before_open(_file_path);
        }

        for (int tries = 0; tries < _option.open_tries; ++tries) {
            auto rs = Fio::file_open_read(_file_path, "rb", _option);
            if (rs.ok()) {
                _fp = rs.value();
                if (_listener.after_open) {
                    _listener.after_open(_file_path, _fp);
                }
                _fd = fileno(_fp);
                return turbo::ok_status();
            }
            if (_option.open_interval > 0) {
                turbo::sleep_for(turbo::milliseconds(_option.open_interval));
            }
        }
        return turbo::errno_to_status(errno, turbo::format("Failed opening file {} for reading", _file_path.c_str()));
    }

    turbo::ResultStatus<size_t> RandomReadFile::read(size_t offset, void *buff, size_t len) {
        if (_fp == nullptr) {
            return turbo::unavailable_error("file not open for read yet");
        }
        size_t has_read = 0;
        /// _fd may > 0 with _fp valid
        ssize_t read_size = ::pread(_fd, buff, len, static_cast<off_t>(offset));
        if(read_size < 0 ) {
            return turbo::errno_to_status(errno, _file_path.c_str());
        }
        // read_size > 0 means read the end of file
        return has_read;
    }

    turbo::ResultStatus<size_t> RandomReadFile::read(size_t offset, std::string *content, size_t n) {
        if (_fp == nullptr) {
            return turbo::unavailable_error("file not open for read yet");
        }
        size_t len = n;
        if(len == npos) {
            auto r = Fio::file_size(_fp);
            if(!r.ok()) {
                return r;
            }
            len = r.value();
        }
        auto pre_len = content->size();
        content->resize(pre_len + len);
        char* pdata = content->data() + pre_len;
        auto rs = read(offset, pdata, len);
        if(!rs.ok()) {
            content->resize(pre_len);
            return rs;
        }
        content->resize(pre_len + rs.value());
        return rs.value();
    }

    turbo::ResultStatus<size_t> RandomReadFile::read(size_t offset, turbo::Cord *buf, size_t n) {
        if (_fp == nullptr) {
            return turbo::unavailable_error("file not open for read yet");
        }
        size_t len = n;
        if(len == npos) {
            auto r = Fio::file_size(_fp);
            if(!r.ok()) {
                return r;
            }
            len = r.value();
        }
        auto slice = buf->get_append_buffer(len);
        auto rs = read(offset, slice.data(), len);
        if(!rs.ok()) {
            return rs;
        }
        slice.SetLength(rs.value());
        buf->append(std::move(slice));

        return rs.value();
    }

    void RandomReadFile::close() {
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


} // namespace turbo