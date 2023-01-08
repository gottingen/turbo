
/****************************************************************
 * Copyright (c) 2022, liyinbin
 * All rights reserved.
 * Author by liyinbin (jeff.li) lijippy@163.com
 *****************************************************************/


#include "flare/files/sequential_read_file.h"
#include "flare/log/logging.h"
#include "flare/base/errno.h"
#include <cstdio>
#include <unistd.h>
#include <fcntl.h>
#include <errno.h>
#include <string.h>

namespace flare {

    sequential_read_file::~sequential_read_file() {
        if (_fd > 0) {
            ::close(_fd);
        }
    }

    result_status sequential_read_file::open(const flare::file_path &path) noexcept {
        FLARE_CHECK(_fd == -1) << "do not reopen";
        result_status rs;
        _path = path;
        _fd = ::open(path.c_str(), O_RDONLY | O_CLOEXEC, 0644);
        if (_fd < 0) {
            FLARE_LOG(ERROR) << "open file: " << path << "error: " << errno << " " << strerror(errno);
            rs.set_error(errno, "{}", strerror(errno));
        }
        return rs;
    }

    result_status sequential_read_file::read(std::string *content, size_t n) {
        flare::IOPortal portal;
        auto frs = read(&portal, n);
        size_t size = 0;
        if (frs.is_ok()) {
            size = portal.size();
            portal.cutn(content, size);
        }
        return frs;
    }

    std::pair<result_status, size_t> sequential_read_file::read(void *buf, size_t n) {
        flare::IOPortal portal;
        auto frs = read(&portal, n);
        size_t size = 0;
        if (frs.is_ok()) {
            size = portal.size();
            portal.cutn(buf, size);
        }
        return {frs, size};
    }

    result_status sequential_read_file::read(flare::cord_buf *buf, size_t n) {
        size_t left = n;
        flare::IOPortal portal;
        result_status frs;
        while (left > 0) {
            ssize_t read_len = portal.append_from_file_descriptor(_fd, static_cast<size_t>(left));
            if (read_len > 0) {
                left -= read_len;
            } else if (read_len == 0) {
                break;
            } else if (errno == EINTR) {
                continue;
            } else {
                FLARE_LOG(WARNING) << "read failed, err: " << flare_error()
                                   << " fd: " << _fd << " size: " << n;
                frs.set_error(errno, "{}", flare_error());
                return frs;
            }
        }
        _has_read += portal.size();
        portal.swap(*buf);
        return frs;
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

    result_status sequential_read_file::skip(size_t n) {
        if (_fd > 0) {
            ::lseek(_fd, n, SEEK_CUR);
        }
        return result_status::success();
    }

    bool sequential_read_file::is_eof(result_status *frs) {
        std::error_code ec;
        auto size = flare::file_size(_path, ec);
        if (ec) {
            if (frs) {
                frs->set_error(errno, "{}", flare_error());
            }
            return false;
        }
        return _has_read == size;
    }

}  // namespace flare