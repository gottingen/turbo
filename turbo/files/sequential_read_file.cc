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

#include "turbo/files/sequential_read_file.h"
#include "turbo/base/casts.h"
#include "turbo/log/logging.h"
#include <cerrno>
#include <cstdio>
#include <cstring>
#include <fcntl.h>
#include <unistd.h>

namespace turbo {

SequentialReadFile::~SequentialReadFile() {
  if (_fd > 0) {
    ::close(_fd);
  }
}

turbo::Status
SequentialReadFile::open(const turbo::filesystem::path &path) noexcept {
  TURBO_CHECK(_fd == -1) << "do not reopen";
  turbo::Status rs;
  _path = path;
  _fd = ::open(path.c_str(), O_RDONLY | O_CLOEXEC, 0644);
  if (_fd < 0) {
    TURBO_LOG(ERROR) << "open file: " << path << "error: " << errno << " "
                     << strerror(errno);
    rs = ErrnoToStatus(errno, "{}");
  }
  return rs;
}

turbo::Status SequentialReadFile::read(std::string *content, size_t n) {
  turbo::Cord portal;
  auto frs = read(&portal, n);
  if (frs.ok()) {
    CopyCordToString(portal, content);
  }
  return frs;
}

turbo::Status SequentialReadFile::read(turbo::Cord *buf, size_t n) {
  size_t left = n;
  turbo::Status frs;
  std::vector<char> tmpBuf;
  static const size_t kMaxTempSize = 4096;
  tmpBuf.resize(kMaxTempSize);
  char *ptr = tmpBuf.data();
  while (left > 0) {
    ssize_t read_len = ::read(_fd, ptr, std::min(left, kMaxTempSize));
    if (read_len > 0) {
      left -= static_cast<size_t>(read_len);
    } else if (read_len == 0) {
      break;
    } else if (errno == EINTR) {
      continue;
    } else {
      TURBO_LOG(WARNING) << "read failed, err: " << errno << " fd: " << _fd
                         << " size: " << n;
      return ErrnoToStatus(errno, "");
    }
    buf->Append(std::string_view(ptr, static_cast<size_t>(read_len)));
    _has_read += static_cast<size_t>(read_len);
  }
  return turbo::OkStatus();
}

void SequentialReadFile::close() {
  if (_fd > 0) {
    ::close(_fd);
    _fd = -1;
  }
}

void SequentialReadFile::reset() {
  if (_fd > 0) {
    ::lseek(_fd, 0, SEEK_SET);
  }
}

turbo::Status SequentialReadFile::skip(off_t n) {
  if (_fd > 0) {
    ::lseek(_fd, implicit_cast<off_t>(n), SEEK_CUR);
  }
  return turbo::OkStatus();
}

bool SequentialReadFile::is_eof(turbo::Status *frs) {
  std::error_code ec;
  auto size = turbo::filesystem::file_size(_path, ec);
  if (ec) {
    if (frs) {
      *frs = ErrnoToStatus(errno, "");
    }
    return false;
  }
  return _has_read == size;
}

} // namespace turbo