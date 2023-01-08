
/****************************************************************
 * Copyright (c) 2022, liyinbin
 * All rights reserved.
 * Author by liyinbin (jeff.li) lijippy@163.com
 *****************************************************************/

#include <sstream>
#include <stdexcept>
#include <cmath>
#include <climits>
#include "flare/strings/str_format.h"
#include "flare/log/logging.h"

namespace flare {
    namespace {
        inline int string_printf_impl(std::string &output, const char *format,
                                      va_list args) {
            // Tru to the space at the end of output for our output buffer.
            // Find out write point then inflate its size temporarily to its
            // capacity; we will later shrink it to the size needed to represent
            // the formatted string.  If this buffer isn't large enough, we do a
            // resize and try again.

            const int write_point = output.size();
            int remaining = output.capacity() - write_point;
            output.resize(output.capacity());

            va_list copied_args;
            va_copy(copied_args, args);
            int bytes_used = vsnprintf(&output[write_point], remaining, format,
                                       copied_args);
            va_end(copied_args);
            if (bytes_used < 0) {
                return -1;
            } else if (bytes_used < remaining) {
                // There was enough room, just shrink and return.
                output.resize(write_point + bytes_used);
            } else {
                output.resize(write_point + bytes_used + 1);
                remaining = bytes_used + 1;
                bytes_used = vsnprintf(&output[write_point], remaining, format, args);
                if (bytes_used + 1 != remaining) {
                    return -1;
                }
                output.resize(write_point + bytes_used);
            }
            return 0;
        }
    }  // end anonymous namespace

    std::string string_printf(const char *format, ...) {
        // snprintf will tell us how large the output buffer should be, but
        // we then have to call it a second time, which is costly.  By
        // guestimating the final size, we avoid the double snprintf in many
        // cases, resulting in a performance win.  We use this constructor
        // of std::string to avoid a double allocation, though it does pad
        // the resulting string with nul bytes.  Our guestimation is twice
        // the format string size, or 32 bytes, whichever is larger.  This
        // is a hueristic that doesn't affect correctness but attempts to be
        // reasonably fast for the most common cases.
        std::string ret;
        ret.reserve(std::max(32UL, strlen(format) * 2));

        va_list ap;
        va_start(ap, format);
        if (string_printf_impl(ret, format, ap) != 0) {
            ret.clear();
        }
        va_end(ap);
        return ret;
    }

    // Basic declarations; allow for parameters of strings and string
    // pieces to be specified.
    int string_appendf(std::string *output, const char *format, ...) {
        va_list ap;
        va_start(ap, format);
        const size_t old_size = output->size();
        const int rc = string_printf_impl(*output, format, ap);
        if (rc != 0) {
            output->resize(old_size);
        }
        va_end(ap);
        return rc;
    }

    int string_vappendf(std::string *output, const char *format, va_list args) {
        const size_t old_size = output->size();
        const int rc = string_printf_impl(*output, format, args);
        if (rc != 0) {
            output->resize(old_size);
        }
        return rc;
    }

    int string_printf(std::string *output, const char *format, ...) {
        va_list ap;
        va_start(ap, format);
        output->clear();
        const int rc = string_printf_impl(*output, format, ap);
        if (rc != 0) {
            output->clear();
        }
        va_end(ap);
        return rc;
    }

    int string_vprintf(std::string *output, const char *format, va_list args) {
        output->clear();
        const int rc = string_printf_impl(*output, format, args);
        if (rc != 0) {
            output->clear();
        }
        return rc;
    }

}