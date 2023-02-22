
/****************************************************************
 * Copyright (c) 2022, liyinbin
 * All rights reserved.
 * Author by liyinbin (jeff.li) lijippy@163.com
 *****************************************************************/

#include "turbo/base/turbo_error.h"
#include <cstdlib> // EXIT_FAILURE
#include <cerrno> // errno
#include <cstdio>  // snprintf

#include "turbo/platform/port.h"
#include "turbo/synchronization/mutex.h"
#include "turbo/base/internal/strerror.h"

namespace turbo {

const int ERRNO_BEGIN = -32768;
const int ERRNO_END = 32768;
static const char* errno_desc[ERRNO_END - ERRNO_BEGIN] = {};
static turbo::Mutex modify_desc_mutex(turbo::kConstInit);

const size_t ERROR_BUFSIZE = 64;
__thread char tls_error_buf[ERROR_BUFSIZE];

int DescribeCustomizedErrno(
    int error_code, const char* error_name, const char* description) {
    MutexLock l(&modify_desc_mutex);
    if (error_code < ERRNO_BEGIN || error_code >= ERRNO_END) {
        // error() is a non-portable GNU extension that should not be used.
        fprintf(stderr, "Fail to define %s(%d) which is out of range, abort.",
              error_name, error_code);
        std::exit(1);
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

std::string SystemError() {
    return turbo::base_internal::StrError(errno);
}
