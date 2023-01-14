
/****************************************************************
 * Copyright (c) 2022, liyinbin
 * All rights reserved.
 * Author by liyinbin (jeff.li) lijippy@163.com
 *****************************************************************/


#include "turbo/files/sequential_write_file.h"
#include "turbo/log/logging.h"
#include "turbo/base/errno.h"
#include <cstdio>
#include <unistd.h>
#include <fcntl.h>
#include <errno.h>
#include <string.h>

namespace turbo {

    sequential_write_file::~sequential_write_file() {
        if (_fd > 0) {
            ::close(_fd);
        }
    }

    result_status sequential_write_file::open(const turbo::file_path &path, bool truncate) noexcept {
        TURBO_CHECK(_fd == -1)<<"do not reopen";
        result_status rs;
        _path = path;
        if(truncate) {
            _fd = ::open(path.c_str(), O_RDWR | O_CREAT | O_TRUNC | O_CLOEXEC, 0644);
        } else {
            _fd = ::open(path.c_str(), O_RDWR | O_CREAT | O_APPEND | O_CLOEXEC, 0644);
        }
        if(_fd < 0) {
            TURBO_LOG(ERROR)<<"open file to append : "<<path<<"error: "<<errno<<" "<<strerror(errno);
            rs.set_error(errno, "{}", strerror(errno));
        }
        return rs;
    }

    result_status sequential_write_file::write(std::string_view content) {
        result_status frs;
        size_t size = content.size();
        ssize_t has_write = 0;
        while (static_cast<size_t>(has_write) != content.size()) {
            ssize_t written = ::write(_fd,  content.data() + has_write, content.size() - has_write);
            if (written >= 0) {
                has_write += written;
            } else if (errno == EINTR) {
                continue;
            } else {
                TURBO_LOG(WARNING) << "write falied, err: " << turbo_error()
                             << " fd: " << _fd <<" size: " << size;
                frs.set_error(errno, "{}", turbo_error());
                return frs;
            }
        }

        return frs;
    }

    result_status sequential_write_file::write(const turbo::cord_buf &data) {
        result_status frs;
        size_t size = data.size();
        turbo::cord_buf piece_data(data);
        ssize_t left = size;
        while (left > 0) {
            ssize_t written = piece_data.cut_into_file_descriptor(_fd,  left);
            if (written >= 0) {
                left -= written;
            } else if (errno == EINTR) {
                continue;
            } else {
                TURBO_LOG(WARNING) << "write falied, err: " << turbo_error()
                             << " fd: " << _fd <<" size: " << size;
                frs.set_error(errno, "{}", turbo_error());
                return frs;
            }
        }

        return frs;
    }
    void sequential_write_file::reset(size_t n) {
        if(_fd > 0) {
            ::lseek(_fd, n , SEEK_SET);
        }
    }
    void sequential_write_file::close() {
        if(_fd > 0) {
            ::close(_fd);
            _fd = -1;
        }
    }
    void sequential_write_file::flush() {
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
