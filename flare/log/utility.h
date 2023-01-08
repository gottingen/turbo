
/****************************************************************
 * Copyright (c) 2022, liyinbin
 * All rights reserved.
 * Author by liyinbin (jeff.li) lijippy@163.com
 *****************************************************************/

#ifndef FLARE_LOG_UTILITY_H_
#define FLARE_LOG_UTILITY_H_

#include <cstdint>
#include <string>

namespace flare::log {

    namespace log_internal {

        const char *program_invocation_short_name();

        bool is_logging_initialized();

        // Get the part of filepath after the last path separator.
        // (Doesn't modify filepath, contrary to basename() in libgen.h.)
        const char *const_basename(const char *filepath);

        void dump_stack_trace_to_string(std::string *stacktrace);

        struct crash_reason {
            crash_reason() : filename(0), line_number(0), message(0), depth(0) {}

            const char *filename;
            int line_number;
            const char *message;

            // We'll also store a bit of stack trace context at the time of crash as
            // it may not be available later on.
            void *stack[32];
            int depth;
        };

        void set_crash_reason(const crash_reason *r);

        void init_logging_utilities(const char *argv0);

        void shutdown_logging_utilities();

    }  // namespace log_internal
}  // namespace flare::log

using namespace flare::log::log_internal;
#endif // FLARE_LOG_UTILITY_H_
