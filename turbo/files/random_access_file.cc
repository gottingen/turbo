
/****************************************************************
 * Copyright (c) 2022, liyinbin
 * All rights reserved.
 * Author by liyinbin (jeff.li) lijippy@163.com
 *****************************************************************/


#include "turbo/files/random_access_file.h"
#include "turbo/log/logging.h"
#include "turbo/base/errno.h"
#include <cstdio>
#include <unistd.h>
#include <fcntl.h>
#include <errno.h>
#include <string.h>

namespace turbo {


    random_access_file::~random_access_file() {
        if (_fd > 0) {
            ::close(_fd);
            _fd = -1;
        }
    }

    result_status random_access_file::open(const turbo::file_path &path) noexcept {
        TURBO_CHECK(_fd == -1) << "do not reopen";
        result_status rs;
        _path = path;
        _fd = ::open(path.c_str(), O_RDONLY | O_CLOEXEC, 0644);
        if (_fd < 0) {
            rs.set_error(errno, "{}", strerror(errno));
        }
        return rs;
    }

    result_status random_access_file::read(size_t n, off_t offset, std::string *content) {
        turbo::IOPortal portal;
        auto frs = read(n, offset, &portal);
        if (frs.is_ok()) {
            auto size = portal.size();
            portal.cutn(content, size);
        }
        return frs;
    }

    result_status random_access_file::read(size_t n, off_t offset, turbo::cord_buf *buf) {
        ssize_t left = n;
        turbo::IOPortal portal;
        result_status frs;
        off_t cur_off = offset;
        while (left > 0) {
            ssize_t read_len = portal.pappend_from_file_descriptor(_fd, cur_off, static_cast<size_t>(left));
            if (read_len > 0) {
                left -= read_len;
                cur_off += read_len;
            } else if (read_len == 0) {
                break;
            } else if (errno == EINTR) {
                continue;
            } else {
                TURBO_LOG(WARNING) << "read failed, errno: " << errno << " " << turbo_error()
                                   << " fd: " << _fd << " size: " << n;
                frs.set_error(errno, "{}", turbo_error());
                return frs;
            }
        }
        portal.swap(*buf);
        return frs;
    }

    result_status random_access_file::read(size_t n, off_t offset, char *buf) {
        turbo::IOPortal portal;
        auto frs = read(n, offset, &portal);
        if (frs.is_ok()) {
            auto size = portal.size();
            portal.cutn((void*)buf, size);
        }
        return frs;
    }
    void random_access_file::close() {
        if(_fd > 0) {
            ::close(_fd);
            _fd = -1;
        }
    }
    bool random_access_file::is_eof(off_t off, size_t has_read, result_status *frs) {
        std::error_code ec;
        auto size = turbo::file_size(_path, ec);
        if (ec) {
            if (frs) {
                frs->set_error(errno, "{}", turbo_error());
            }
            return false;
        }
        return off + has_read >= size;
    }

}  // namespace turbo