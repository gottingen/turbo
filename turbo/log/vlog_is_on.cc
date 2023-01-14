
/****************************************************************
 * Copyright (c) 2022, liyinbin
 * All rights reserved.
 * Author by liyinbin (jeff.li) lijippy@163.com
 *****************************************************************/


#include <cstring>
#include <cstdlib>
#include <cerrno>
#include <cstdio>
#include <string>
#include <mutex>
#include "turbo/log/config.h"
#include "turbo/log/raw_logging.h"
#include "turbo/log/init.h"

// doesn't have annotation
#define ANNOTATE_BENIGN_RACE(address, description)

using std::string;

TURBO_LOG_DEFINE_int32(turbo_v,
                       0, "Show all TURBO_VLOG(m) messages for m <= this."
                          " Overridable by --vmodule.");

TURBO_LOG_DEFINE_string(turbo_vmodule,
                        "", "per-module verbose level."
                            " Argument is a comma-separated list of <module name>=<log level>."
                            " <module name> is a glob pattern, matched against the filename base"
                            " (that is, name ignoring .cc/.h./-inl.h)."
                            " <log level> overrides any value given by --v.");

namespace turbo::log {

    namespace log_internal {

        // Used by logging_unittests.cc so can't make it static here.
        TURBO_EXPORT bool SafeFNMatch_(const char *pattern,
                                       size_t patt_len,
                                       const char *str,
                                       size_t str_len);

        // Implementation of fnmatch that does not need 0-termination
        // of arguments and does not allocate any memory,
        // but we only support "*" and "?" wildcards, not the "[...]" patterns.
        // It's not a static function for the unittest.
        TURBO_EXPORT bool SafeFNMatch_(const char *pattern,
                                       size_t patt_len,
                                       const char *str,
                                       size_t str_len) {
            size_t p = 0;
            size_t s = 0;
            while (1) {
                if (p == patt_len && s == str_len) return true;
                if (p == patt_len) return false;
                if (s == str_len) return p + 1 == patt_len && pattern[p] == '*';
                if (pattern[p] == str[s] || pattern[p] == '?') {
                    p += 1;
                    s += 1;
                    continue;
                }
                if (pattern[p] == '*') {
                    if (p + 1 == patt_len) return true;
                    do {
                        if (SafeFNMatch_(pattern + (p + 1), patt_len - (p + 1), str + s, str_len - s)) {
                            return true;
                        }
                        s += 1;
                    } while (s != str_len);
                    return false;
                }
                return false;
            }
        }

    }  // namespace log_internal

    using log_internal::SafeFNMatch_;

    // List of per-module log levels from FLAGS_turbo_vmodule.
    // Once created each element is never deleted/modified
    // except for the vlog_level: other threads will read VModuleInfo blobs
    // w/o locks and we'll store pointers to vlog_level at TURBO_VLOG locations
    // that will never go away.
    // We can't use an STL struct here as we wouldn't know
    // when it's safe to delete/update it: other threads need to use it w/o locks.
    struct VModuleInfo {
        string module_pattern;
        mutable int32_t vlog_level;  // Conceptually this is an AtomicWord, but it's
        // too much work to use AtomicWord type here
        // w/o much actual benefit.
        const VModuleInfo *next;
    };

    // This protects the following global variables.
    static std::mutex vmodule_lock;
    // Pointer to head of the VModuleInfo list.
    // It's a map from module pattern to logging level for those module(s).
    static VModuleInfo *vmodule_list = 0;
    // Boolean initialization flag.
    static bool inited_vmodule = false;

    // L >= vmodule_lock.
    static void VLOG2Initializer() {
        // Can now parse --vmodule flag and initialize mapping of module-specific
        // logging levels.
        inited_vmodule = false;
        const char *vmodule = FLAGS_turbo_vmodule.c_str();
        const char *sep;
        VModuleInfo *head = nullptr;
        VModuleInfo *tail = nullptr;
        while ((sep = strchr(vmodule, '=')) != nullptr) {
            string pattern(vmodule, sep - vmodule);
            int module_level;
            if (sscanf(sep, "=%d", &module_level) == 1) {
                VModuleInfo *info = new VModuleInfo;
                info->module_pattern = pattern;
                info->vlog_level = module_level;
                if (head) tail->next = info;
                else head = info;
                tail = info;
            }
            // Skip past this entry
            vmodule = strchr(sep, ',');
            if (vmodule == nullptr) break;
            vmodule++;  // Skip past ","
        }
        if (head) {  // Put them into the list at the head:
            tail->next = vmodule_list;
            vmodule_list = head;
        }
        inited_vmodule = true;
    }

    // This can be called very early, so we use SpinLock and TURBO_RAW_VLOG here.
    int SetVLOGLevel(const char *module_pattern, int log_level) {
        int result = FLAGS_turbo_v;
        int const pattern_len = strlen(module_pattern);
        bool found = false;
        {
            std::unique_lock<std::mutex> l(vmodule_lock);  // protect whole read-modify-write
            for (const VModuleInfo *info = vmodule_list;
                 info != nullptr; info = info->next) {
                if (info->module_pattern == module_pattern) {
                    if (!found) {
                        result = info->vlog_level;
                        found = true;
                    }
                    info->vlog_level = log_level;
                } else if (!found &&
                           SafeFNMatch_(info->module_pattern.c_str(),
                                        info->module_pattern.size(),
                                        module_pattern, pattern_len)) {
                    result = info->vlog_level;
                    found = true;
                }
            }
            if (!found) {
                VModuleInfo *info = new VModuleInfo;
                info->module_pattern = module_pattern;
                info->vlog_level = log_level;
                info->next = vmodule_list;
                vmodule_list = info;
            }
        }
        TURBO_RAW_VLOG(1, "Set TURBO_VLOG level for \"%s\" to %d", module_pattern, log_level);
        return result;
    }

    // NOTE: Individual TURBO_VLOG statements cache the integer log level pointers.
    // NOTE: This function must not allocate memory or require any locks.
    bool init_vlog(int32_t **site_flag, int32_t *site_default,
                   const char *fname, int32_t verbose_level) {
        std::unique_lock<std::mutex> l(vmodule_lock);
        bool read_vmodule_flag = inited_vmodule;
        if (!read_vmodule_flag) {
            VLOG2Initializer();
        }

        // protect the errno global in case someone writes:
        // TURBO_VLOG(..) << "The last error was " << strerror(errno)
        int old_errno = errno;

        // site_default normally points to FLAGS_turbo_v
        int32_t *site_flag_value = site_default;

        // Get basename for file
        const char *base = strrchr(fname, '/');
        base = base ? (base + 1) : fname;
        const char *base_end = strchr(base, '.');
        size_t base_length = base_end ? size_t(base_end - base) : strlen(base);

        // Trim out trailing "-inl" if any
        if (base_length >= 4 && (memcmp(base + base_length - 4, "-inl", 4) == 0)) {
            base_length -= 4;
        }

        // TODO: Trim out _unittest suffix?  Perhaps it is better to have
        // the extra control and just leave it there.

        // find target in vector of modules, replace site_flag_value with
        // a module-specific verbose level, if any.
        for (const VModuleInfo *info = vmodule_list;
             info != nullptr; info = info->next) {
            if (SafeFNMatch_(info->module_pattern.c_str(), info->module_pattern.size(),
                             base, base_length)) {
                site_flag_value = &info->vlog_level;
                // value at info->vlog_level is now what controls
                // the TURBO_VLOG at the caller site forever
                break;
            }
        }

        // Cache the vlog value pointer if --vmodule flag has been parsed.
        ANNOTATE_BENIGN_RACE(site_flag,
                             "*site_flag may be written by several threads,"
                             " but the value will be the same");
        if (read_vmodule_flag) *site_flag = site_flag_value;

        // restore the errno in case something recoverable went wrong during
        // the initialization of the TURBO_VLOG mechanism (see above note "protect the..")
        errno = old_errno;
        return *site_flag_value >= verbose_level;
    }

}  // namespace turbo::log
