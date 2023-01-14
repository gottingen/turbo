
/****************************************************************
 * Copyright (c) 2022, liyinbin
 * All rights reserved.
 * Author by liyinbin (jeff.li) lijippy@163.com
 *****************************************************************/


#include "turbo/log/config.h"
#include "turbo/log/severity.h"
#include <gflags/gflags.h>

static bool BoolFromEnv(const char *varname, bool defval) {
    const char *const valstr = getenv(varname);
    if (!valstr) {
        return defval;
    }
    return memchr("tTyY1\0", valstr[0], 6) != nullptr;
}

TURBO_LOG_DEFINE_bool(turbo_timestamp_in_logfile_name,
                      BoolFromEnv("GOOGLE_TIMESTAMP_IN_LOGFILE_NAME", true),
                      "put a timestamp at the end of the log file name");
TURBO_LOG_DEFINE_bool(turbo_logtostderr, BoolFromEnv("GOOGLE_LOGTOSTDERR", false),
                      "log messages go to stderr instead of logfiles");
TURBO_LOG_DEFINE_bool(turbo_also_logtostderr, BoolFromEnv("GOOGLE_ALSOLOGTOSTDERR", false),
                      "log messages go to stderr in addition to logfiles");
TURBO_LOG_DEFINE_bool(turbo_colorlogtostderr, false,
                      "color messages logged to stderr (if supported by terminal)");
#ifdef TURBO_PLATFORM_LINUX
TURBO_LOG_DEFINE_bool(drop_log_memory, true, "Drop in-memory buffers of log contents. "
                 "Logs can grow very quickly and they are rarely read before they "
                 "need to be evicted from memory. Instead, drop them from memory "
                 "as soon as they are flushed to disk.");
#endif

// By default, errors (including fatal errors) get logged to stderr as
// well as the file.
//
// The default is ERROR instead of FATAL so that users can see problems
// when they run a program without having to look in another file.
DEFINE_int32(turbo_stderrthreshold,
             turbo::log::TURBO_ERROR,
             "log messages at or above this level are copied to stderr in "
             "addition to logfiles.  This flag obsoletes --turbo_also_logtostderr.");

TURBO_LOG_DEFINE_string(turbo_also_log_to_email, "",
                        "log messages go to these email addresses "
                        "in addition to logfiles");
TURBO_LOG_DEFINE_bool(turbo_log_prefix, true,
                      "Prepend the log prefix to the start of each log line");
TURBO_LOG_DEFINE_int32(turbo_minloglevel, 2, "Messages logged at a lower level than this don't "
                                       "actually get logged anywhere");
TURBO_LOG_DEFINE_int32(turbo_logbuflevel, 2,
                       "Buffer log messages logged at this level or lower"
                       " (-1 means don't buffer; 0 means buffer INFO only;"
                       " ...)");
TURBO_LOG_DEFINE_int32(turbo_logbufsecs, 30,
                       "Buffer log messages for at most this many seconds");
TURBO_LOG_DEFINE_int32(turbo_log_email_level, 999,
                       "Email log messages logged at this level or higher"
                       " (0 means email all; 3 means email FATAL only;"
                       " ...)");
TURBO_LOG_DEFINE_string(turbo_log_mailer, "",
                        "Mailer used to send logging email");


// Compute the default value for --turbo_log_dir
static const char *DefaultLogDir() {
    const char *env;
    env = getenv("GOOGLE_LOG_DIR");
    if (env != nullptr && env[0] != '\0') {
        return env;
    }
    env = getenv("TEST_TMPDIR");
    if (env != nullptr && env[0] != '\0') {
        return env;
    }
    return "";
}

DEFINE_int32(turbo_log_save_days, 7, "log keep days, default is 7 days");

TURBO_LOG_DEFINE_int32(turbo_logfile_mode, 0664, "Log file mode/permissions.");

TURBO_LOG_DEFINE_string(turbo_log_dir, DefaultLogDir(),
                        "If specified, logfiles are written into this directory instead "
                        "of the default logging directory.");
TURBO_LOG_DEFINE_string(turbo_log_link, "", "Put additional links to the log "
                                      "files in this directory");

TURBO_LOG_DEFINE_int32(turbo_max_log_size, 1800,
                       "approx. maximum log file size (in MB). A value of 0 will "
                       "be silently overridden to 1.");

TURBO_LOG_DEFINE_bool(turbo_stop_logging_if_full_disk, false,
                      "Stop attempting to log to disk if the disk is full.");

TURBO_LOG_DEFINE_string(turbo_log_backtrace_at, "",
                        "Emit a backtrace when logging at file:linenum.");

TURBO_LOG_DEFINE_bool(turbo_log_utc_time, false,
                      "Use UTC time for logging.");

DEFINE_bool(turbo_log_as_json, false, "Print log as a valid JSON");
DEFINE_bool(turbo_crash_on_fatal_log, false, "crash on fatal log");
