
/****************************************************************
 * Copyright (c) 2022, liyinbin
 * All rights reserved.
 * Author by liyinbin (jeff.li) lijippy@163.com
 *****************************************************************/


#include "turbo/files/sequential_read_file.h"
#include "turbo/log/turbo_log.h"
#include "turbo/log/check.h"
#include <cstdio>
#include <unistd.h>
#include <fcntl.h>
#include <errno.h>
#include <string.h>

namespace turbo {

    sequential_read_file::~sequential_read_file() {
        if (_fd > 0) {
            ::close(_fd);
        }
    }

    turbo::Status sequential_read_file::open(const turbo::filesystem::path &path) noexcept {
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

    turbo::Status sequential_read_file::read(std::string *content, size_t n) {
        turbo::Cord portal;
        auto frs = read(&portal, n);
        size_t size = 0;
        if (frs.ok()) {
            size = portal.size();
            CopyCordToString(portal,content);
        }
        return frs;
    }

    turbo::Status sequential_read_file::read(turbo::Cord *buf, size_t n) {
        size_t left = n;
        turbo::Status frs;
        std::vector<char> tmpBuf;
        static const size_t kMaxTempSize = 4096;
        tmpBuf.resize(kMaxTempSize);
        char *ptr = tmpBuf.data();
        while (left > 0) {
            ssize_t read_len = ::read(_fd, ptr, std::min(left, kMaxTempSize));
            if (read_len > 0) {
                left -= read_len;
            } else if (read_len == 0) {
                break;
            } else if (errno == EINTR) {
                continue;
            } else {
                TURBO_LOG(WARNING) << "read failed, err: " << errno
                                   << " fd: " << _fd << " size: " << n;
                return ErrnoToStatus(errno,"");
            }
            buf->Append(std::string_view(ptr, read_len));
            _has_read += read_len;
        }
        return turbo::OkStatus();
    }

    void sequential_read_file::close() {
        if (_fd > 0) {
            ::close(_fd);
            _fd = -1;
        }
    }

    void sequential_read_file::reset() {
        if (_fd > 0) {
            ::lseek(_fd, 0, SEEK_SET);
        }
    }

    turbo::Status sequential_read_file::skip(size_t n) {
        if (_fd > 0) {
            ::lseek(_fd, n, SEEK_CUR);
        }
        return turbo::OkStatus();
    }

    bool sequential_read_file::is_eof(turbo::Status *frs) {
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