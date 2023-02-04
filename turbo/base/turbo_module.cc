
/****************************************************************
 * Copyright (c) 2022, liyinbin
 * All rights reserved.
 * Author by liyinbin (jeff.li) lijippy@163.com
 *****************************************************************/

#include "turbo/base/turbo_module.h"
#include "turbo/platform/port.h"
#include <cstdlib> // EXIT_FAILURE
#include <errno.h> // errno
#include <mutex>
#include <stdio.h>  // snprintf
#include <string.h> // strerror_r
#include <unistd.h> // _exit

namespace turbo {

const int INDEX_BEGIN = 0;
const int INDEX_END = 4096;
static const char* module_desc[INDEX_END - INDEX_BEGIN] = {};
static std::mutex modify_desc_mutex;

const size_t MODULE_BUFSIZE = 64;
__thread char tls_module_buf[MODULE_BUFSIZE];

int DescribeCustomizedModule(
    int module_index, const char* module_name, const char* description) {
    std::unique_lock l (modify_desc_mutex);
    if (module_index < INDEX_BEGIN || module_index >= INDEX_END) {
        // error() is a non-portable GNU extension that should not be used.
        fprintf(stderr, "Fail to define module %s(%d) which is out of range, abort.",
                module_name, module_index);
        _exit(1);
    }
    const char* desc = module_desc[module_index - INDEX_BEGIN];
    if (desc) {
        if (strcmp(desc, description) == 0) {
            fprintf(stderr, "WARNING: Detected shared library loading\n");
            return -1;
        }
    }
    module_desc[module_index - INDEX_BEGIN] = description;
    return 0;  // must
}

}  // namespace turbo

const char* TurboModule(int module_index) {
    if (module_index >= turbo::INDEX_BEGIN && module_index < turbo::INDEX_END) {
        const char* s = turbo::module_desc[module_index - turbo::INDEX_BEGIN];
        if (s) {
            return s;
        }
    }
    snprintf(turbo::tls_module_buf, turbo::MODULE_BUFSIZE-1, "%d_UDM",module_index);
    return turbo::tls_module_buf;
}

TURBO_REGISTER_MODULE_INDEX(0,"TURBO");