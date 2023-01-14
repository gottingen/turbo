
/****************************************************************
 * Copyright (c) 2022, liyinbin
 * All rights reserved.
 * Author by liyinbin (jeff.li) lijippy@163.com
 *****************************************************************/

#ifndef TURBO_BASE_FD_UTILITY_H_
#define TURBO_BASE_FD_UTILITY_H_

namespace turbo::base {

    // Make file descriptor |fd| non-blocking
    // Returns 0 on success, -1 otherwise and errno is set (by fcntl)
    int make_non_blocking(int fd);

    // Make file descriptor |fd| blocking
    // Returns 0 on success, -1 otherwise and errno is set (by fcntl)
    int make_blocking(int fd);

    // Make file descriptor |fd| automatically closed during exec()
    // Returns 0 on success, -1 when error and errno is set (by fcntl)
    int make_close_on_exec(int fd);

    // Disable nagling on file descriptor |socket|.
    // Returns 0 on success, -1 when error and errno is set (by setsockopt)
    int make_no_delay(int socket);

}  // namespace turbo::base

#endif  // TURBO_BASE_FD_UTILITY_H_
