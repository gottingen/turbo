
/****************************************************************
 * Copyright (c) 2022, liyinbin
 * All rights reserved.
 * Author by liyinbin (jeff.li) lijippy@163.com
 *****************************************************************/

#ifndef FLARE_BASE_FD_UTILITY_H_
#define FLARE_BASE_FD_UTILITY_H_

namespace flare::base {

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

}  // namespace flare::base

#endif  // FLARE_BASE_FD_UTILITY_H_
