// Copyright 2023 The Turbo Authors.
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//      https://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.

#include "turbo/base/turbo_error.h"
#include <cstdlib> // EXIT_FAILURE
#include <cerrno> // errno
#include <cstdio>  // snprintf
#include <mutex>
#include <cstring>

#include "turbo/platform/port.h"
#include "turbo/base/internal/strerror.h"

namespace turbo {

const int ERRNO_BEGIN = -32768;
const int ERRNO_END = 32768;
static const char* errno_desc[ERRNO_END - ERRNO_BEGIN] = {};
static std::mutex modify_desc_mutex;

const size_t ERROR_BUFSIZE = 64;
__thread char tls_error_buf[ERROR_BUFSIZE];

int DescribeCustomizedErrno(
    int error_code, const char* error_name, const char* description) {
    std::unique_lock<std::mutex> l(modify_desc_mutex);
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
