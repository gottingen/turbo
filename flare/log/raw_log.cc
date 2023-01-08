
/****************************************************************
 * Copyright (c) 2022, liyinbin
 * All rights reserved.
 * Author by liyinbin (jeff.li) lijippy@163.com
 *****************************************************************/

#include <stdarg.h>
#include <cstdio>
#include <atomic>
#include <cerrno>
#include <fcntl.h>                 // for open()
#include <ctime>
#include <unistd.h>
#include "flare/log/config.h"
#include "flare/log/logging.h"          // To pick up flag settings etc.
#include "flare/log/raw_logging.h"
#include "flare/system/sysinfo.h"

#ifdef HAVE_STACKTRACE

# include "flare/debugging/stacktrace.h"

#endif


namespace flare::log {

    // CAVEAT: vsnprintf called from *DoRawLog below has some (exotic) code paths
    // that invoke malloc() and getenv() that might acquire some locks.
    // If this becomes a problem we should reimplement a subset of vsnprintf
    // that does not need locks and malloc.

    // Helper for RawLog__ below.
    // *DoRawLog writes to *buf of *size and move them past the written portion.
    // It returns true iff there was no overflow or error.
    static bool DoRawLog(char **buf, int *size, const char *format, ...) {
        va_list ap;
        va_start(ap, format);
        int n = vsnprintf(*buf, *size, format, ap);
        va_end(ap);
        if (n < 0 || n > *size) return false;
        *size -= n;
        *buf += n;
        return true;
    }

// Helper for RawLog__ below.
    inline static bool VADoRawLog(char **buf, int *size,
                                  const char *format, va_list ap) {
        int n = vsnprintf(*buf, *size, format, ap);
        if (n < 0 || n > *size) return false;
        *size -= n;
        *buf += n;
        return true;
    }

    static const int kLogBufSize = 3000;
    static std::atomic<bool> crashed{false};
    static crash_reason crash_reason;
    static char crash_buf[kLogBufSize + 1] = {0};  // Will end in '\0'

    void RawLog__(log_severity severity, const char *file, int line,
                  const char *format, ...) {
        if (!(FLAGS_flare_logtostderr || severity >= FLAGS_flare_stderrthreshold ||
              FLAGS_flare_also_logtostderr || !is_logging_initialized())) {
            return;  // this stderr log message is suppressed
        }
        // can't call localtime_r here: it can allocate
        char buffer[kLogBufSize];
        char *buf = buffer;
        int size = sizeof(buffer);

        // NOTE: this format should match the specification in base/logging.h
        DoRawLog(&buf, &size, "%c00000000 00:00:00.000000 %5u %s:%d] RAW: ",
                 log_severity_names[severity][0],
                 static_cast<unsigned int>(flare::sysinfo::get_tid()),
                 const_basename(const_cast<char *>(file)), line);

        // Record the position and size of the buffer after the prefix
        const char *msg_start = buf;
        const int msg_size = size;

        va_list ap;
        va_start(ap, format);
        bool no_chop = VADoRawLog(&buf, &size, format, ap);
        va_end(ap);
        if (no_chop) {
            DoRawLog(&buf, &size, "\n");
        } else {
            DoRawLog(&buf, &size, "FLARE_RAW_LOG ERROR: The Message was too long!\n");
        }
        // We make a raw syscall to write directly to the stderr file descriptor,
        // avoiding FILE buffering (to avoid invoking malloc()), and bypassing
        // libc (to side-step any libc interception).
        // We write just once to avoid races with other invocations of RawLog__.
        ::write(STDERR_FILENO, buffer, strlen(buffer));
        if (severity == FLARE_FATAL) {
            bool old = false;
            if (crashed.compare_exchange_weak(old, true, std::memory_order_acquire) && !old) {
                crash_reason.filename = file;
                crash_reason.line_number = line;
                memcpy(crash_buf, msg_start, msg_size);  // Don't include prefix
                crash_reason.message = crash_buf;
#ifdef HAVE_STACKTRACE
                crash_reason.depth =
                        flare::debugging::get_stack_trace(crash_reason.stack, FLARE_ARRAY_SIZE(crash_reason.stack), 1);
#else
                crash_reason.depth = 0;
#endif
                set_crash_reason(&crash_reason);
            }
            log_message::fail();  // abort()
        }
    }

}  // namespace flare::log
