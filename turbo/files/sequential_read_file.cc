
/****************************************************************
 * Copyright (c) 2022, liyinbin
 * All rights reserved.
 * Author by liyinbin (jeff.li) lijippy@163.com
 *****************************************************************/


#include "turbo/files/sequential_read_file.h"
#include "turbo/log/logging.h"
#include "turbo/base/errno.h"
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

    result_status sequential_read_file::open(const turbo::file_path &path) noexcept {
        TURBO_CHECK(_fd == -1) << "do not reopen";
        result_status rs;
        _path = path;
        _fd = ::open(path.c_str(), O_RDONLY | O_CLOEXEC, 0644);
        if (_fd < 0) {
            TURBO_LOG(ERROR) << "open file: " << path << "error: " << errno << " " << strerror(errno);
            rs.set_error(errno, "{}", strerror(errno));
        }
        return rs;
    }

    result_status sequential_read_file::read(std::string *content, size_t n) {
        turbo::IOPortal portal;
        auto frs = read(&portal, n);
        size_t size = 0;
        if (frs.is_ok()) {
            size = portal.size();
            portal.cutn(content, size);
        }
        return frs;
    }

    std::pair<result_status, size_t> sequential_read_file::read(void *buf, size_t n) {
        turbo::IOPortal portal;
        auto frs = read(&portal, n);
        size_t size = 0;
        if (frs.is_ok()) {
            size = portal.size();
            portal.cutn(buf, size);
        }
        return {frs, size};
    }

    result_status sequential_read_file::read(turbo::cord_buf *buf, size_t n) {
        size_t left = n;
        turbo::IOPortal portal;
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
                TURBO_LOG(WARNING) << "read failed, err: " << turbo_error()
                                   << " fd: " << _fd << " size: " << n;
                frs.set_error(errno, "{}", turbo_error());
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
        auto size = turbo::file_size(_path, ec);
        if (ec) {
            if (frs) {
                frs->set_error(errno, "{}", turbo_error());
            }
            return false;
        }
        return _has_read == size;
    }

}  // namespace turbo