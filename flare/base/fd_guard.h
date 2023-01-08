

/****************************************************************
 * Copyright (c) 2022, liyinbin
 * All rights reserved.
 * Author by liyinbin (jeff.li) lijippy@163.com
 *****************************************************************/

#ifndef FLARE_BASE_FD_GUARD_H_
#define FLARE_BASE_FD_GUARD_H_

#include <unistd.h>                                  // close()

namespace flare::base {

    // RAII file descriptor.
    //
    // Example:
    //    fd_guard fd1(open(...));
    //    if (fd1 < 0) {
    //        printf("Fail to open\n");
    //        return -1;
    //    }
    //    if (another-error-happened) {
    //        printf("Fail to do sth\n");
    //        return -1;   // *** closing fd1 automatically ***
    //    }
    class fd_guard {
    public:
        fd_guard() : _fd(-1) {}

        explicit fd_guard(int fd) : _fd(fd) {}

        ~fd_guard() {
            if (_fd >= 0) {
                ::close(_fd);
                _fd = -1;
            }
        }

        // Close current fd and replace with another fd
        void reset(int fd) {
            if (_fd >= 0) {
                ::close(_fd);
                _fd = -1;
            }
            _fd = fd;
        }

        // Set internal fd to -1 and return the value before set.
        int release() {
            const int prev_fd = _fd;
            _fd = -1;
            return prev_fd;
        }

        operator int() const { return _fd; }

    private:
        // Copying this makes no sense.
        fd_guard(const fd_guard &);

        void operator=(const fd_guard &);

        int _fd;
    };

}  // namespace flare::base

#endif  // FLARE_BASE_FD_GUARD_H_
