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
#include "turbo/files/sequential_read_file.h"
#include "turbo/files/fio.h"
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
            auto rs = Fio::file_open_read(_file_path, "rb", _option);
            if (rs.ok()) {
                _fp = rs.value();
                if (_listener.after_open) {
                    _listener.after_open(_file_path, _fp);
                }
                return turbo::ok_status();
            }
            if (_option.open_interval > 0) {
                turbo::SleepFor(turbo::Milliseconds(_option.open_interval));
            }
        }
        return turbo::errno_to_status(errno, turbo::Format("Failed opening file {} for reading", _file_path.c_str()));
    }

    turbo::ResultStatus<size_t> SequentialReadFile::read(void *buff, size_t len) {
        if (_fp == nullptr) {
            return turbo::unavailable_error("file not open for read yet");
        }
        if(len == 0) {
            return 0;
        }
        size_t has_read = 0;
        char *pdata = static_cast<char *>(buff);
        while (has_read < len && !std::feof(_fp)) {
            auto n = std::fread(pdata + static_cast<std::ptrdiff_t>(has_read), 1, len - has_read, _fp);
            if (std::ferror(_fp)) {
                return turbo::errno_to_status(errno, "");
            }
            has_read += n;
        };
        if(has_read == 0) {
            return turbo::reach_file_end_error("");
        }
        return has_read;
    }

    turbo::ResultStatus<size_t> SequentialReadFile::read(std::string *content, size_t n) {
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
        auto rs = read(pdata, len);
        if(!rs.ok()) {
            content->resize(pre_len);
            return rs;
        }
        content->resize(pre_len + rs.value());
        return rs.value();
    }

    turbo::ResultStatus<size_t> SequentialReadFile::read(turbo::Cord *buf, size_t n) {
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
        bool first = true;
        size_t has_read = 0;
        while(has_read < len) {
            CordBuffer buffer = first ? buf->get_append_buffer(len) : CordBuffer::CreateWithDefaultLimit(len - has_read);
            turbo::Span<char> slice = buffer.available_up_to(len - has_read);
            auto rs = read(slice.data(), slice.size());
            if(!rs.ok()) {
                if(turbo::is_reach_file_end(rs.status())) {
                    break;
                }
                return rs;
            }
            first = false;
            buf->append(std::move(buffer));
            has_read += rs.value();
        }
        // any data read from file yet, it already reach end last time
        if(has_read == 0) {
            return turbo::reach_file_end_error("");
        }

        return has_read;
    }

    void SequentialReadFile::close() {
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

    turbo::Status SequentialReadFile::skip(off_t n) {
        if (_fp != nullptr) {
            std::fseek(_fp, implicit_cast<off_t>(n), SEEK_CUR);
        }
        return turbo::ok_status();
    }

    turbo::ResultStatus<bool> SequentialReadFile::is_eof() {
        auto ret = std::feof(_fp);
        if(ret < 0) {
            return turbo::errno_to_status(errno, turbo::Format("test file eof {}", _file_path.c_str()));
        }
        return ret == 0;
    }

} // namespace turbo