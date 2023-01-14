
/****************************************************************
 * Copyright (c) 2022, liyinbin
 * All rights reserved.
 * Author by liyinbin (jeff.li) lijippy@163.com
 *****************************************************************/

#ifndef TURBO_LOG_RAW_LOGGING_H_
#define TURBO_LOG_RAW_LOGGING_H_


#include <ctime>


#include "turbo/log/severity.h"
#include "turbo/log//vlog_is_on.h"

namespace turbo::log {

// This is similar to TURBO_LOG(severity) << format... and TURBO_VLOG(level) << format..,
// but
// * it is to be used ONLY by low-level modules that can't use normal TURBO_LOG()
// * it is desiged to be a low-level logger that does not allocate any
//   memory and does not need any locks, hence:
// * it logs straight and ONLY to STDERR w/o buffering
// * it uses an explicit format and arguments list
// * it will silently chop off really long message strings
// Usage example:
//   TURBO_RAW_LOG(ERROR, "Failed foo with %i: %s", status, error);
//   TURBO_RAW_VLOG(3, "status is %i", status);
// These will print an almost standard log lines like this to stderr only:
//   E20200821 211317 file.cc:123] RAW: Failed foo with 22: bad_file
//   I20200821 211317 file.cc:142] RAW: status is 20
#define TURBO_RAW_LOG(severity, ...) \
  do { \
    switch (turbo::log::TURBO_ ## severity) {  \
      case 0: \
        TURBO_RAW_LOG_INFO(__VA_ARGS__); \
        break; \
      case 1: \
        TURBO_RAW_LOG_WARNING(__VA_ARGS__); \
        break; \
      case 2: \
        TURBO_RAW_LOG_ERROR(__VA_ARGS__); \
        break; \
      case 3: \
        TURBO_RAW_LOG_FATAL(__VA_ARGS__); \
        break; \
      default: \
        break; \
    } \
  } while (0)

// The following STRIP_LOG testing is performed in the header file so that it's
// possible to completely compile out the logging code and the log messages.
#if STRIP_LOG == 0
#define TURBO_RAW_VLOG(verboselevel, ...) \
  do { \
    if (TURBO_VLOG_IS_ON(verboselevel)) { \
      TURBO_RAW_LOG_TRACE(__VA_ARGS__); \
    } \
  } while (0)
#else
#define TURBO_RAW_VLOG(verboselevel, ...) raw_log_stub(0, __VA_ARGS__)
#endif // STRIP_LOG == 0

#if STRIP_LOG == 0
#define TURBO_RAW_LOG_TRACE(...) turbo::log::RawLog__(turbo::log::TURBO_TRACE, \
                                   __FILE__, __LINE__, __VA_ARGS__)
#else
#define TURBO_RAW_LOG_TRACE(...) turbo::log::raw_log_stub(0, __VA_ARGS__)
#endif // STRIP_LOG == 0

#if STRIP_LOG <= 1
#define TURBO_RAW_LOG_DEBUG(...) turbo::log::RawLog__(turbo::log::TURBO_DEBUG, \
                                   __FILE__, __LINE__, __VA_ARGS__)
#else
#define TURBO_RAW_LOG_DEBUG(...) turbo::log::raw_log_stub(0, __VA_ARGS__)
#endif // STRIP_LOG <= 1

#if STRIP_LOG <= 2
#define TURBO_RAW_LOG_INFO(...) turbo::log::RawLog__(turbo::log::TURBO_INFO, \
                                   __FILE__, __LINE__, __VA_ARGS__)
#else
#define TURBO_RAW_LOG_INFO(...) turbo::log::raw_log_stub(0, __VA_ARGS__)
#endif // STRIP_LOG == 2

#if STRIP_LOG <= 3
#define TURBO_RAW_LOG_WARNING(...) turbo::log::RawLog__(turbo::log::TURBO_WARNING,   \
                                      __FILE__, __LINE__, __VA_ARGS__)
#else
#define TURBO_RAW_LOG_WARNING(...) turbo::log::raw_log_stub(0, __VA_ARGS__)
#endif // STRIP_LOG <= 3

#if STRIP_LOG <= 4
#define TURBO_RAW_LOG_ERROR(...) turbo::log::RawLog__(turbo::log::TURBO_ERROR,       \
                                    __FILE__, __LINE__, __VA_ARGS__)
#else
#define TURBO_RAW_LOG_ERROR(...) turbo::log::raw_log_stub(0, __VA_ARGS__)
#endif // STRIP_LOG <= 4

#if STRIP_LOG <= 5
#define TURBO_RAW_LOG_FATAL(...) turbo::log::RawLog__(turbo::log::TURBO_FATAL,       \
                                    __FILE__, __LINE__, __VA_ARGS__)
#else
#define TURBO_RAW_LOG_FATAL(...) \
  do { \
    turbo::log::raw_log_stub(0, __VA_ARGS__);        \
    exit(1); \
  } while (0)
#endif // STRIP_LOG <= 5

// Similar to TURBO_CHECK(condition) << message,
// but for low-level modules: we use only TURBO_RAW_LOG that does not allocate memory.
// We do not want to provide args list here to encourage this usage:
//   if (!cond)  TURBO_RAW_LOG(FATAL, "foo ...", hard_to_compute_args);
// so that the args are not computed when not needed.
#define TURBO_RAW_CHECK(condition, message)                                   \
  do {                                                                  \
    if (!(condition)) {                                                 \
      TURBO_RAW_LOG(FATAL, "Check %s failed: %s", #condition, message);       \
    }                                                                   \
  } while (0)

// Debug versions of TURBO_RAW_LOG and TURBO_RAW_CHECK
#ifndef NDEBUG

#define TURBO_RAW_DLOG(severity, ...) TURBO_RAW_LOG(severity, __VA_ARGS__)
#define TURBO_RAW_DCHECK(condition, message) TURBO_RAW_CHECK(condition, message)

#else  // NDEBUG

#define TURBO_RAW_DLOG(severity, ...)                                 \
  while (false)                                                 \
    TURBO_RAW_LOG(severity, __VA_ARGS__)
#define TURBO_RAW_DCHECK(condition, message) \
  while (false) \
    TURBO_RAW_CHECK(condition, message)

#endif  // NDEBUG

    // Stub log function used to work around for unused variable warnings when
    // building with STRIP_LOG > 0.
    static inline void raw_log_stub(int /* ignored */, ...) {
    }

    // Helper function to implement TURBO_RAW_LOG and TURBO_RAW_VLOG
    // Logs format... at "severity" level, reporting it
    // as called from file:line.
    // This does not allocate memory or acquire locks.
    TURBO_EXPORT void RawLog__(turbo::log::log_severity severity,
                               const char *file,
                               int line,
                               const char *format, ...);

}  // namespace turbo::log

#endif // TURBO_LOG_RAW_LOGGING_H_

