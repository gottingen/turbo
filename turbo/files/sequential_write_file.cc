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

namespace turbo {

SequentialWriteFile::~SequentialWriteFile() {
    close();
}

turbo::Status SequentialWriteFile::open(const turbo::filesystem::path&fname, bool truncate) {
  if(fname.empty()) {
    return InvalidArgumentError("invalid file name");
  }
  _path = fname;
  int flag = O_WRONLY | O_CLOEXEC | O_CREAT;
  if(truncate) {
    flag |= O_TRUNC;
  }
  _fd = ::open(_path.c_str(), flag, 0644);
  if (_fd < 0) {
    return ErrnoToStatus(errno, "{}");
  }
  return OkStatus();
}

turbo::Status SequentialWriteFile::reopen(const turbo::filesystem::path&fname, bool truncate) {
  close();
  _has_write = 0;
  return  open(fname, truncate);
}

turbo::ResultStatus<ssize_t> SequentialWriteFile::append(const char *data, size_t size) {
  size_t wr = size;
  while (size > 0) {
    ssize_t write_result = ::write(_fd, data, size);
    if (write_result < 0) {
      if (errno == EINTR) {
        continue;  // Retry
      }
      return ErrnoToStatus( errno, _path.c_str());
    }
    data += write_result;
    size -= static_cast<size_t>(write_result);
  }
  _has_write += wr;
  return static_cast<ssize_t>(wr);
}

size_t SequentialWriteFile::have_write() const {
    return _has_write;
}

turbo::ResultStatus<size_t> SequentialWriteFile::size() const {
    std::error_code ec;
    auto size = turbo::filesystem::file_size(_path, ec);
    if(ec) {
      return turbo::ErrnoToStatus(errno, "");
    }
    return size;
}

void SequentialWriteFile::close() {
    if (_fd > 0) {
      ::close(_fd);
      _fd = -1;
    }
}

void SequentialWriteFile::flush() {
#ifdef TURBO_PLATFORM_OSX
    if(_fd > 0) {
      ::fsync(_fd);
    }
#elif TURBO_PLATFORM_LINUX
    if(_fd > 0) {
      ::fdatasync(_fd);
    }
#else
#error unkown how to work
#endif
}

const turbo::filesystem::path& SequentialWriteFile::file_path() const {
    return _path;
}

}  // namespace turbo
