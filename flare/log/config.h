
/****************************************************************
 * Copyright (c) 2022, liyinbin
 * All rights reserved.
 * Author by liyinbin (jeff.li) lijippy@163.com
 *****************************************************************/


#ifndef FLARE_LOG_CONFIG_H_
#define FLARE_LOG_CONFIG_H_

#include <cstdlib>               // for getenv
#include <cstring>               // for memchr
#include <string>


#include <gflags/gflags.h>

// Define FLARE_LOG_DEFINE_* using DEFINE_* . By using these macros, we
// have FLARE_LOG_* environ variables even if we have gflags installed.
//
// If both an environment variable and a flag are specified, the value
// specified by a flag wins. E.g., if FLARE_LOG_v=0 and --v=1, the
// verbosity will be 1, not 0.

#define FLARE_LOG_DEFINE_bool(name, value, meaning) \
  DEFINE_bool(name, FLARE_ENV_TO_BOOL("FLARE_LOG_" #name, value), meaning)

#define FLARE_LOG_DEFINE_int32(name, value, meaning) \
  DEFINE_int32(name, FLARE_ENV_TO_INT("FLARE_LOG_" #name, value), meaning)

#define FLARE_LOG_DEFINE_string(name, value, meaning) \
  DEFINE_string(name, FLARE_ENV_TO_STRING("FLARE_LOG_" #name, value), meaning)

// These macros (could be functions, but I don't want to bother with a .cc
// file), make it easier to initialize flags from the environment.

#define FLARE_ENV_TO_STRING(envname, dflt)   \
  (!getenv(envname) ? (dflt) : getenv(envname))

#define FLARE_ENV_TO_BOOL(envname, dflt)   \
  (!getenv(envname) ? (dflt) : memchr("tTyY1\0", getenv(envname)[0], 6) != nullptr)

#define FLARE_ENV_TO_INT(envname, dflt)  \
  (!getenv(envname) ? (dflt) : strtol(getenv(envname), nullptr, 10))



// Set whether appending a timestamp to the log file name
DECLARE_bool(flare_timestamp_in_logfile_name);

// Set whether log messages go to stderr instead of logfiles
DECLARE_bool(flare_logtostderr);

// Set whether log messages go to stderr in addition to logfiles.
DECLARE_bool(flare_also_logtostderr);

// Set color messages logged to stderr (if supported by terminal).
DECLARE_bool(flare_colorlogtostderr);

// Log messages at a level >= this flag are automatically sent to
// stderr in addition to log files.
DECLARE_int32(flare_stderrthreshold);

// Set whether the log prefix should be prepended to each line of output.
DECLARE_bool(flare_log_prefix);

// Log messages at a level <= this flag are buffered.
// Log messages at a higher level are flushed immediately.
DECLARE_int32(flare_logbuflevel);

// Sets the maximum number of seconds which logs may be buffered for.
DECLARE_int32(flare_logbufsecs);

// Log suppression level: messages logged at a lower level than this
// are suppressed.
DECLARE_int32(flare_minloglevel);

// If specified, logfiles are written into this directory instead of the
// default logging directory.
DECLARE_string(flare_log_dir);

// Set the log file mode.
DECLARE_int32(flare_logfile_mode);

// days to save log
DECLARE_int32(flare_log_save_days);

DECLARE_int32(flare_log_email_level);

// Sets the path of the directory into which to put additional links
// to the log files.
DECLARE_string(flare_log_link);

DECLARE_string(flare_also_log_to_email);

DECLARE_string(flare_log_mailer);

DECLARE_string(flare_log_backtrace_at);

DECLARE_int32(flare_v);  // in vlog_is_on.cc

DECLARE_string(flare_vmodule); // also in vlog_is_on.cc
// Sets the maximum log file size (in MB).
DECLARE_int32(flare_max_log_size);

// Sets whether to avoid logging to the disk if the disk is full.
DECLARE_bool(flare_stop_logging_if_full_disk);

// Use UTC time for logging
DECLARE_bool(flare_log_utc_time);

DECLARE_bool(flare_crash_on_fatal_log);

DECLARE_bool(flare_log_as_json);

#define HAVE_STACKTRACE
#define HAVE_SIGACTION
#define HAVE_SYMBOLIZE

#endif  // FLARE_LOG_CONFIG_H_
