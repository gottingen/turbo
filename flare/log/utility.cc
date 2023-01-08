
/****************************************************************
 * Copyright (c) 2022, liyinbin
 * All rights reserved.
 * Author by liyinbin (jeff.li) lijippy@163.com
 *****************************************************************/

#include <atomic>
#include "flare/log/utility.h"
#include <syslog.h>
#include "flare/log/logging.h"
#include "flare/base/profile.h"
#include <sys/syscall.h>
#ifdef FLARE_PLATFORM_LINUX
#include <signal.h>
#endif
#ifdef HAVE_STACKTRACE

#include "flare/debugging/stacktrace.h"
#include "flare/debugging/symbolize.h"
#include "flare/log/config.h"

FLARE_LOG_DEFINE_bool(symbolize_stacktrace, true,
                      "Symbolize the stack trace in the tombstone");

namespace flare::log {

    typedef void DebugWriter(const char *, void *);

    // The %p field width for printf() functions is two characters per byte.
    // For some environments, add two extra bytes for the leading "0x".
    static const int kPrintfPointerFieldWidth = 2 + 2 * sizeof(void *);

    static void DebugWriteToStderr(const char *data, void *) {
        // This one is signal-safe.
        if (write(STDERR_FILENO, data, strlen(data)) < 0) {
            // Ignore errors.
        }
    }

    static void DebugWriteToString(const char *data, void *arg) {
        reinterpret_cast<std::string *>(arg)->append(data);
    }


#ifdef HAVE_SYMBOLIZE

    // Print a program counter and its symbol name.
    static void DumpPCAndSymbol(DebugWriter *writerfn, void *arg, void *pc,
                                const char *const prefix) {
        char tmp[1024];
        const char *symbol = "(unknown)";
        // Symbolizes the previous address of pc because pc may be in the
        // next function.  The overrun happens when the function ends with
        // a call to a function annotated noreturn (e.g. FLARE_CHECK).
        if (flare::debugging::symbolize(reinterpret_cast<char *>(pc) - 1, tmp, sizeof(tmp))) {
            symbol = tmp;
        }
        char buf[1024];
        snprintf(buf, sizeof(buf), "%s@ %*p  %s\n",
                 prefix, kPrintfPointerFieldWidth, pc, symbol);
        writerfn(buf, arg);
    }

#endif

    static void DumpPC(DebugWriter *writerfn, void *arg, void *pc,
                       const char *const prefix) {
        char buf[100];
        snprintf(buf, sizeof(buf), "%s@ %*p\n",
                 prefix, kPrintfPointerFieldWidth, pc);
        writerfn(buf, arg);
    }

// Dump current stack trace as directed by writerfn
    static void DumpStackTrace(int skip_count, DebugWriter *writerfn, void *arg) {
        // Print stack trace
        void *stack[32];
        int depth = flare::debugging::get_stack_trace(stack, FLARE_ARRAY_SIZE(stack), skip_count + 1);
        for (int i = 0; i < depth; i++) {
#if defined(HAVE_SYMBOLIZE)
            if (FLAGS_symbolize_stacktrace) {
                DumpPCAndSymbol(writerfn, arg, stack[i], "    ");
            } else {
                DumpPC(writerfn, arg, stack[i], "    ");
            }
#else
            DumpPC(writerfn, arg, stack[i], "    ");
#endif
        }
    }

    static void DumpStackTraceAndExit() {
        DumpStackTrace(1, DebugWriteToStderr, nullptr);

        // TODO(hamaji): Use signal instead of sigaction?
        if (IsFailureSignalHandlerInstalled()) {
            // Set the default signal handler for SIGABRT, to avoid invoking our
            // own signal handler installed by InstallFailureSignalHandler().
#ifdef FLARE_PLATFORM_LINUX
            struct sigaction sig_action;
        memset(&sig_action, 0, sizeof(sig_action));
        sigemptyset(&sig_action.sa_mask);
        sig_action.sa_handler = SIG_DFL;
        sigaction(SIGABRT, &sig_action, nullptr);
#endif  // HAVE_SIGACTION
        }

        abort();
    }

}  // namespace flare::log

#endif  // HAVE_STACKTRACE

namespace flare::log {

    namespace log_internal {
#ifdef HAVE_STACKTRACE

        void dump_stack_trace_to_string(std::string *stacktrace) {
            DumpStackTrace(1, DebugWriteToString, stacktrace);
        }

#endif
        static const char *g_program_invocation_short_name = nullptr;

        const char *program_invocation_short_name() {
            if (g_program_invocation_short_name != nullptr) {
                return g_program_invocation_short_name;
            } else {
                // TODO(hamaji): Use /proc/self/cmdline and so?
                return "UNKNOWN";
            }
        }

        bool is_logging_initialized() {
            return g_program_invocation_short_name != nullptr;
        }

        void init_logging_utilities(const char *argv0) {
            FLARE_CHECK(!is_logging_initialized())
                            << "You called init_logging() twice!";
            const char *slash = strrchr(argv0, '/');
            g_program_invocation_short_name = slash ? slash + 1 : argv0;

#ifdef HAVE_STACKTRACE
            InstallFailureFunction(&DumpStackTraceAndExit);
#endif
        }

        void shutdown_logging_utilities() {
            FLARE_CHECK(is_logging_initialized())
                            << "You called shutdown_logging() without calling init_logging() first!";
            g_program_invocation_short_name = nullptr;
            closelog();
        }

        static std::atomic<const crash_reason*> g_reason{nullptr};

        void set_crash_reason(const crash_reason *r) {
            const crash_reason* old = nullptr;
            g_reason.compare_exchange_weak(old,r);
        }


        const char *const_basename(const char *filepath) {
            const char *base = strrchr(filepath, '/');
            return base ? (base + 1) : filepath;
        }


    }  // namespace log_internal
}  // namespace flare::log
