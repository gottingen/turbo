
/****************************************************************
 * Copyright (c) 2022, liyinbin
 * All rights reserved.
 * Author by liyinbin (jeff.li) lijippy@163.com
 *****************************************************************/


#include "turbo/files/sequential_read_file.h"
#include <cstdio>
#include <unistd.h>
#include <fcntl.h>
#include <cerrno>
#include <cstring>
#include "turbo/log/turbo_log.h"
#include "turbo/log/check.h"
#include "turbo/base/casts.h"

namespace turbo {

    SequentialReadFile::~SequentialReadFile() {
        if (_fd > 0) {
            ::close(_fd);
        }
    }

    turbo::Status SequentialReadFile::open(const turbo::filesystem::path &path) noexcept {
        CHECK(_fd == -1) << "do not reopen";
        turbo::Status rs;
        _path = path;
        _fd = ::open(path.c_str(), O_RDONLY | O_CLOEXEC, 0644);
        if (_fd < 0) {
            TURBO_LOG(ERROR) << "open file: " << path << "error: " << errno << " " << strerror(errno);
            rs = ErrnoToStatus(errno, "{}");
        }
        return rs;
    }

    turbo::Status SequentialReadFile::read(std::string *content, size_t n) {
        turbo::Cord portal;
        auto frs = read(&portal, n);
        if (frs.ok()) {
            CopyCordToString(portal,content);
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
                TURBO_LOG(WARNING) << "read failed, err: " << errno
                                   << " fd: " << _fd << " size: " << n;
                return ErrnoToStatus(errno,"");
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

    turbo::Status SequentialReadFile::skip(size_t n) {
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

}  // namespace turbo