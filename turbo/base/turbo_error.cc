
/****************************************************************
 * Copyright (c) 2022, liyinbin
 * All rights reserved.
 * Author by liyinbin (jeff.li) lijippy@163.com
 *****************************************************************/

#include "turbo/base/turbo_error.h"
#include "turbo/platform/config.h"
#include <errno.h>                                     // errno
#include <string.h>                                    // strerror_r
#include <cstdlib>                                    // EXIT_FAILURE
#include <stdio.h>                                     // snprintf
#include <unistd.h>                                    // _exit
#include <mutex>

namespace turbo {

const int ERRNO_BEGIN = -32768;
const int ERRNO_END = 32768;
static const char* errno_desc[ERRNO_END - ERRNO_BEGIN] = {};
static std::mutex modify_desc_mutex;

const size_t ERROR_BUFSIZE = 64;
__thread char tls_error_buf[ERROR_BUFSIZE];

int DescribeCustomizedErrno(
    int error_code, const char* error_name, const char* description) {
    std::unique_lock l (modify_desc_mutex);
    if (error_code < ERRNO_BEGIN || error_code >= ERRNO_END) {
        // error() is a non-portable GNU extension that should not be used.
        fprintf(stderr, "Fail to define %s(%d) which is out of range, abort.",
              error_name, error_code);
        _exit(1);
    }
    const char* desc = errno_desc[error_code - ERRNO_BEGIN];
    if (desc) {
        if (strcmp(desc, description) == 0) {
            fprintf(stderr, "WARNING: Detected shared library loading\n");
            return -1;
        }
    }
    errno_desc[error_code - ERRNO_BEGIN] = description;
    return 0;  // must
}

}  // namespace turbo

const char* TurboError(int error_code) {
    if (error_code == -1) {
        return "";
    }
    if (error_code >= turbo::ERRNO_BEGIN && error_code < turbo::ERRNO_END) {
        const char* s = turbo::errno_desc[error_code - turbo::ERRNO_BEGIN];
        if (s) {
            return s;
        }
    }
    return "";
}

const char* TurboError() {
#if defined(TURBO_PLATFORM_OSX)
    const int rc = strerror_r(errno, turbo::tls_error_buf, turbo::ERROR_BUFSIZE);
    if (rc == 0 || rc == ERANGE/*bufsize is not long enough*/) {
        return turbo::tls_error_buf;
    }
#else
    const char* s = strerror_r(errno, turbo::tls_error_buf, turbo::ERROR_BUFSIZE);
        if (s) {
            return s;
        }
#endif
}
