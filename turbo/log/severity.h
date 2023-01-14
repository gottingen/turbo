
/****************************************************************
 * Copyright (c) 2022, liyinbin
 * All rights reserved.
 * Author by liyinbin (jeff.li) lijippy@163.com
 *****************************************************************/


#ifndef TURBO_LOG_SERVERITY_H_
#define TURBO_LOG_SERVERITY_H_

#include "turbo/base/profile.h"
// Variables of type log_severity are widely taken to lie in the range
// [0, NUM_SEVERITIES-1].  Be careful to preserve this assumption if
// you ever need to change their values or add a new severity.
namespace turbo::log {
    typedef int log_severity;

    const int TURBO_TRACE = 0, TURBO_DEBUG = 1, TURBO_INFO = 2, TURBO_WARNING = 3, TURBO_ERROR = 4, TURBO_FATAL = 5,
            NUM_SEVERITIES = 6;
#ifndef TURBO_NO_ABBREVIATED_SEVERITIES
# ifdef ERROR
#  error ERROR macro is defined. Define TURBO_LOG_NO_ABBREVIATED_SEVERITIES before including logging.h. See the document for detail.
# endif
    const int TRACE = turbo::log::TURBO_TRACE, DEBUG = turbo::log::TURBO_DEBUG, INFO = TURBO_INFO, WARNING = turbo::log::TURBO_WARNING,
            ERROR = turbo::log::TURBO_ERROR, FATAL = turbo::log::TURBO_FATAL;
#endif

// DFATAL is FATAL in debug mode, ERROR in normal mode
#ifdef NDEBUG
#define DFATAL_LEVEL ERROR
#else
#define DFATAL_LEVEL FATAL
#endif

    extern TURBO_EXPORT const char *const log_severity_names[NUM_SEVERITIES];

// NDEBUG usage helpers related to (RAW_)TURBO_DCHECK:
//
// DEBUG_MODE is for small !NDEBUG uses like
//   if (DEBUG_MODE) foo.CheckThatFoo();
// instead of substantially more verbose
//   #ifndef NDEBUG
//     foo.CheckThatFoo();
//   #endif
//
// TURBO_IF_DEBUG_MODE is for small !NDEBUG uses like
//   TURBO_IF_DEBUG_MODE( string error; )
//   TURBO_DCHECK(Foo(&error)) << error;
// instead of substantially more verbose
//   #ifndef NDEBUG
//     string error;
//     TURBO_DCHECK(Foo(&error)) << error;
//   #endif
//
#ifdef NDEBUG
    enum {
        DEBUG_MODE = 0
    };
#define TURBO_IF_DEBUG_MODE(x)
#else
    enum { DEBUG_MODE = 1 };
#define TURBO_IF_DEBUG_MODE(x) x
#endif
}

#endif // TURBO_LOG_SERVERITY_H_
