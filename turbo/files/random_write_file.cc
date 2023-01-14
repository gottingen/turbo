
/****************************************************************
 * Copyright (c) 2022, liyinbin
 * All rights reserved.
 * Author by liyinbin (jeff.li) lijippy@163.com
 *****************************************************************/


#include "turbo/files/random_write_file.h"
#include "turbo/log/logging.h"
#include "turbo/base/errno.h"
#include <cstdio>
#include <unistd.h>
#include <fcntl.h>
#include <errno.h>
#include <string.h>

namespace turbo {

    random_write_file::~random_write_file() {
        if (_fd > 0) {
            ::close(_fd);
            _fd = -1;
        }
    }

    result_status random_write_file::open(const turbo::file_path &path, bool truncate) noexcept {
        TURBO_CHECK(_fd == -1) << "do not reopen";
        result_status rs;
        _path = path;
        if (truncate) {
            _fd = ::open(path.c_str(), O_RDWR | O_CREAT | O_TRUNC | O_CLOEXEC, 0644);
        } else {
            _fd = ::open(path.c_str(), O_RDWR | O_CREAT | O_CLOEXEC, 0644);
        }
        if (_fd < 0) {
            TURBO_LOG(ERROR) << "open file to append : " << path << "error: " << errno << " " << strerror(errno);
            rs.set_error(errno, "{}", strerror(errno));
        }
        return rs;
    }

    result_status random_write_file::write(off_t offset, std::string_view content) {
        size_t size = content.size();
        off_t orig_offset = offset;
        result_status frs;
        ssize_t left = size;
        const char *data = content.data();
        while (left > 0) {
            ssize_t written = ::pwrite(_fd, data, left, offset);
            if (written >= 0) {
                offset += written;
                left -= written;
                data += written;
            } else if (errno == EINTR) {
                continue;
            } else {
                TURBO_LOG(WARNING) << "write falied, err: " << turbo_error()
                             << " fd: " << _fd << " offset: " << orig_offset << " size: " << size;
                frs.set_error(errno, "{}", turbo_error());
                return frs;
            }
        }

        return frs;
    }

    result_status random_write_file::write(off_t offset, const void *content, size_t size) {
        off_t orig_offset = offset;
        result_status frs;
        ssize_t left = size;
        const char *data = (char*)content;
        while (left > 0) {
            ssize_t written = ::pwrite(_fd, data, left, offset);
            if (written >= 0) {
                offset += written;
                left -= written;
                data += written;
            } else if (errno == EINTR) {
                continue;
            } else {
                TURBO_LOG(WARNING) << "write falied, err: " << turbo_error()
                                   << " fd: " << _fd << " offset: " << orig_offset << " size: " << size;
                frs.set_error(errno, "{}", turbo_error());
                return frs;
            }
        }

        return frs;
    }
    result_status random_write_file::write(off_t offset, const turbo::cord_buf &data) {
        size_t size = data.size();
        turbo::cord_buf piece_data(data);
        off_t orig_offset = offset;
        result_status frs;
        ssize_t left = size;
        while (left > 0) {
            ssize_t written = piece_data.pcut_into_file_descriptor(_fd, offset, left);
            if (written >= 0) {
                offset += written;
                left -= written;
            } else if (errno == EINTR) {
                continue;
            } else {
                TURBO_LOG(WARNING) << "write falied, err: " << turbo_error()
                             << " fd: " << _fd << " offset: " << orig_offset << " size: " << size;
                frs.set_error(errno, "{}", turbo_error());
                return frs;
            }
        }

        return frs;
    }

    void random_write_file::close() {
        if(_fd > 0) {
            ::close(_fd);
            _fd = -1;
        }
    }

    void random_write_file::flush() {
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

}  // namespace turbo
