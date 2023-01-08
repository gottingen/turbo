
/****************************************************************
 * Copyright (c) 2022, liyinbin
 * All rights reserved.
 * Author by liyinbin (jeff.li) lijippy@163.com
 *****************************************************************/


#ifndef FLARE_LOG_VLOG_IS_ON_H_
#define FLARE_LOG_VLOG_IS_ON_H_

#include "flare/log/severity.h"


#if defined(__GNUC__)
// We emit an anonymous static int* variable at every FLARE_VLOG_IS_ON(n) site.
// (Normally) the first time every FLARE_VLOG_IS_ON(n) site is hit,
// we determine what variable will dynamically control logging at this site:
// it's either FLAGS_flare_v or an appropriate internal variable
// matching the current source file that represents results of
// parsing of --vmodule flag and/or SetVLOGLevel calls.
#define FLARE_VLOG_IS_ON(verboselevel)                                \
  __extension__  \
  ({ static int32_t* vlocal__ = nullptr;           \
     int32_t verbose_level__ = (verboselevel);                    \
     (vlocal__ == nullptr ? flare::log::init_vlog(&vlocal__, &FLAGS_flare_v, \
                        __FILE__, verbose_level__) : *vlocal__ >= verbose_level__); \
  })
#else
// GNU extensions not available, so we do not support --vmodule.
// Dynamic value of FLAGS_flare_v always controls the logging level.
#define FLARE_VLOG_IS_ON(verboselevel) (FLAGS_flare_v >= (verboselevel))
#endif
namespace flare::log {
    // Set FLARE_VLOG(_IS_ON) level for module_pattern to log_level.
    // This lets us dynamically control what is normally set by the --vmodule flag.
    // Returns the level that previously applied to module_pattern.
    // NOTE: To change the log level for FLARE_VLOG(_IS_ON) sites
    //	 that have already executed after/during init_logging,
    //	 one needs to supply the exact --vmodule pattern that applied to them.
    //       (If no --vmodule pattern applied to them
    //       the value of FLAGS_flare_v will continue to control them.)
    extern FLARE_EXPORT int set_vlog_level(const char *module_pattern,
                                                   int log_level);

    // Various declarations needed for FLARE_VLOG_IS_ON above: =========================

    // Helper routine which determines the logging info for a particalur FLARE_VLOG site.
    //   site_flag     is the address of the site-local pointer to the controlling
    //                 verbosity level
    //   site_default  is the default to use for *site_flag
    //   fname         is the current source file name
    //   verbose_level is the argument to FLARE_VLOG_IS_ON
    // We will return the return value for FLARE_VLOG_IS_ON
    // and if possible set *site_flag appropriately.
    extern FLARE_EXPORT bool
    init_vlog(int32_t **site_flag, int32_t *site_default, const char *fname, int32_t verbose_level);
}

#endif  // FLARE_LOG_VLOG_IS_ON_H_
