
/****************************************************************
 * Copyright (c) 2022, liyinbin
 * All rights reserved.
 * Author by liyinbin (jeff.li) lijippy@163.com
 *****************************************************************/

#include <fcntl.h>                                  // open
#include <unistd.h>                                 // close
#include <stdio.h>                                  // snprintf, vdprintf
#include <cstdlib>                                 // mkstemp
#include <string.h>                                 // strlen 
#include <stdarg.h>                                 // va_list
#include <errno.h>                                  // errno
#include <new>                                      // placement new
#include "turbo/files/temp_file.h"                              // temp_file
#include "turbo/log/logging.h"

// Initializing array. Needs to be macro.
#define BASE_FILES_TEMP_FILE_PATTERN "temp_file_XXXXXX"

namespace turbo {

    temp_file::temp_file() : _ever_opened(0) {
        char temp_name[] = BASE_FILES_TEMP_FILE_PATTERN;
        _fd = mkstemp(temp_name);
        if (_fd >= 0) {
            _ever_opened = 1;
            snprintf(_fname, sizeof(_fname), "%s", temp_name);
        } else {
            *_fname = '\0';
        }

    }

    temp_file::temp_file(const char *ext) {
        if (nullptr == ext || '\0' == *ext) {
            new(this) temp_file();
            return;
        }

        *_fname = '\0';
        _fd = -1;
        _ever_opened = 0;

        // Make a temp file to occupy the filename without ext.
        char temp_name[] = BASE_FILES_TEMP_FILE_PATTERN;
        const int tmp_fd = mkstemp(temp_name);
        if (tmp_fd < 0) {
            return;
        }

        // Open the temp_file_XXXXXX.ext
        snprintf(_fname, sizeof(_fname), "%s.%s", temp_name, ext);

        _fd = open(_fname, O_CREAT | O_WRONLY | O_TRUNC | O_EXCL, 0600);
        if (_fd < 0) {
            *_fname = '\0';
        } else {
            _ever_opened = 1;
        }

        // Close and remove temp_file_XXXXXX anyway.
        close(tmp_fd);
        unlink(temp_name);
    }

    int temp_file::_reopen_if_necessary() {
        if (_fd < 0) {
            _fd = open(_fname, O_CREAT | O_WRONLY | O_TRUNC, 0600);
        }
        return _fd;
    }

    temp_file::~temp_file() {
        if (_fd >= 0) {
            close(_fd);
            _fd = -1;
        }
        if (_ever_opened) {
            // Only remove temp file when _fd >= 0, otherwise we may
            // remove another temporary file (occupied the same name)
            unlink(_fname);
        }
    }

    int temp_file::save(const char *content) {
        return save_bin(content, strlen(content));
    }

    int temp_file::save_format(const char *fmt, ...) {
        if (_reopen_if_necessary() < 0) {
            return -1;
        }
        va_list ap;
        va_start(ap, fmt);
        const int rc = vdprintf(_fd, fmt, ap);
        va_end(ap);

        close(_fd);
        _fd = -1;
        // TODO: is this right?
        return (rc < 0 ? -1 : 0);
    }

    // Write until all buffer was written or an error except EINTR.
    // Returns:
    //    -1   error happened, errno is set
    // count   all written
    static ssize_t temp_file_write_all(int fd, const void *buf, size_t count) {
        size_t off = 0;
        for (;;) {
            ssize_t nw = write(fd, (char *) buf + off, count - off);
            if (nw == (ssize_t) (count - off)) {  // including count==0
                return count;
            }
            if (nw >= 0) {
                off += nw;
            } else if (errno != EINTR) {
                return -1;
            }
        }
    }

    int temp_file::save_bin(const void *buf, size_t count) {
        if (_reopen_if_necessary() < 0) {
            return -1;
        }

        const ssize_t len = temp_file_write_all(_fd, buf, count);

        close(_fd);
        _fd = -1;
        if (len < 0) {
            return -1;
        } else if ((size_t) len != count) {
            errno = ENOSPC;
            return -1;
        }
        return 0;
    }


} // namespace turbo
