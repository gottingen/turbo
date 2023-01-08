
/****************************************************************
 * Copyright (c) 2022, liyinbin
 * All rights reserved.
 * Author by liyinbin (jeff.li) lijippy@163.com
 *****************************************************************/


#ifndef FLARE_LOG_SERVERITY_H_
#define FLARE_LOG_SERVERITY_H_

#include "flare/base/profile.h"
// Variables of type log_severity are widely taken to lie in the range
// [0, NUM_SEVERITIES-1].  Be careful to preserve this assumption if
// you ever need to change their values or add a new severity.
namespace flare::log {
    typedef int log_severity;

    const int FLARE_TRACE = 0, FLARE_DEBUG = 1, FLARE_INFO = 2, FLARE_WARNING = 3, FLARE_ERROR = 4, FLARE_FATAL = 5,
            NUM_SEVERITIES = 6;
#ifndef FLARE_NO_ABBREVIATED_SEVERITIES
# ifdef ERROR
#  error ERROR macro is defined. Define FLARE_LOG_NO_ABBREVIATED_SEVERITIES before including logging.h. See the document for detail.
# endif
    const int TRACE = flare::log::FLARE_TRACE, DEBUG = flare::log::FLARE_DEBUG, INFO = FLARE_INFO, WARNING = flare::log::FLARE_WARNING,
            ERROR = flare::log::FLARE_ERROR, FATAL = flare::log::FLARE_FATAL;
#endif

// DFATAL is FATAL in debug mode, ERROR in normal mode
#ifdef NDEBUG
#define DFATAL_LEVEL ERROR
#else
#define DFATAL_LEVEL FATAL
#endif

    extern FLARE_EXPORT const char *const log_severity_names[NUM_SEVERITIES];

// NDEBUG usage helpers related to (RAW_)FLARE_DCHECK:
//
// DEBUG_MODE is for small !NDEBUG uses like
//   if (DEBUG_MODE) foo.CheckThatFoo();
// instead of substantially more verbose
//   #ifndef NDEBUG
//     foo.CheckThatFoo();
//   #endif
//
// FLARE_IF_DEBUG_MODE is for small !NDEBUG uses like
//   FLARE_IF_DEBUG_MODE( string error; )
//   FLARE_DCHECK(Foo(&error)) << error;
// instead of substantially more verbose
//   #ifndef NDEBUG
//     string error;
//     FLARE_DCHECK(Foo(&error)) << error;
//   #endif
//
#ifdef NDEBUG
    enum {
        DEBUG_MODE = 0
    };
#define FLARE_IF_DEBUG_MODE(x)
#else
    enum { DEBUG_MODE = 1 };
#define FLARE_IF_DEBUG_MODE(x) x
#endif
}

#endif // FLARE_LOG_SERVERITY_H_
