// Copyright 2023 The Turbo Authors.
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//      https://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.

#include "turbo/base/sysinfo.h"
#include "turbo/meta/type_traits.h"
#include <algorithm>
#include <chrono>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <ctime>
#include <string>
#include <thread>
#include <array>
#include <sys/stat.h>
#include <sys/types.h>


#ifdef _WIN32

#    include <io.h>      // _get_osfhandle and _isatty support
#    include <process.h> //  _get_pid support
#    include <turbo/log/details/windows_include.h>

#    ifdef __MINGW32__
#        include <share.h>
#    endif

#    if defined(TLOG_WCHAR_TO_UTF8_SUPPORT) || defined(TLOG_WCHAR_FILENAMES)
#        include <limits>
#    endif

#    include <direct.h> // for _mkdir/_wmkdir

#else // unix

#    include <fcntl.h>
#    include <unistd.h>

#    ifdef __linux__

#        include <sys/syscall.h> //Use gettid() syscall under linux to get thread id

#    elif defined(_AIX)
#        include <pthread.h> // for pthread_getthrds_np

#    elif defined(__DragonFly__) || defined(__FreeBSD__)
#        include <pthread_np.h> // for pthread_getthreadid_np

#    elif defined(__NetBSD__)
#        include <lwp.h> // for _lwp_self

#    elif defined(__sun)
#        include <thread.h> // for thr_self
#    endif

#endif // unix

#if defined(_WIN32)
#include <windows.h>

#elif defined(__unix__) || defined(__unix) || defined(unix) || \
    (defined(__APPLE__) && defined(__MACH__))
#include <sys/param.h>
#include <sys/types.h>
#include <unistd.h>

#if defined(BSD) && !defined(__gnu_hurd__)
#include <sys/sysctl.h>
#endif

#else
#define NOMEMORYSIZE
#endif

namespace turbo {
    static size_t _thread_id() noexcept;
    int pid() noexcept {

#ifdef _WIN32
        return conditional_static_cast<int>(::GetCurrentProcessId());
#else
        return conditional_static_cast<int>(::getpid());
#endif
    }

    // Determine if the terminal attached
// Source: https://github.com/agauniyal/rang/
    bool in_terminal(FILE *file) noexcept {

#ifdef _WIN32
        return ::_isatty(_fileno(file)) != 0;
#else
        return ::isatty(fileno(file)) != 0;
#endif
    }


    // Determine if the terminal supports colors
    // Based on: https://github.com/agauniyal/rang/
    bool is_color_terminal() noexcept {
#ifdef _WIN32
        return true;
#else

        static const bool result = []() {
            const char *env_colorterm_p = std::getenv("COLORTERM");
            if (env_colorterm_p != nullptr) {
                return true;
            }

            static constexpr std::array<const char *, 16> terms = {
                    {"ansi", "color", "console", "cygwin", "gnome", "konsole", "kterm", "linux",
                     "msys", "putty", "rxvt", "screen", "vt100", "xterm", "alacritty", "vt102"}};

            const char *env_term_p = std::getenv("TERM");
            if (env_term_p == nullptr) {
                return false;
            }

            return std::any_of(terms.begin(), terms.end(),
                               [&](const char *term) { return std::strstr(env_term_p, term) != nullptr; });
        }();

        return result;
#endif
    }


    // Return current thread id as size_t
    // It exists because the std::this_thread::get_id() is much slower(especially
    // under VS 2013)
    size_t _thread_id() noexcept {
#ifdef _WIN32
        return static_cast<size_t>(::GetCurrentThreadId());
#elif defined(__linux__)
#    if defined(__ANDROID__) && defined(__ANDROID_API__) && (__ANDROID_API__ < 21)
#        define SYS_gettid __NR_gettid
#    endif
        return static_cast<size_t>(::syscall(SYS_gettid));
#elif defined(_AIX)
        struct __pthrdsinfo buf;
                int reg_size = 0;
                pthread_t pt = pthread_self();
                int retval = pthread_getthrds_np(&pt, PTHRDSINFO_QUERY_TID, &buf, sizeof(buf), NULL, &reg_size);
                int tid = (!retval) ? buf.__pi_tid : 0;
                return static_cast<size_t>(tid);
#elif defined(__DragonFly__) || defined(__FreeBSD__)
                return static_cast<size_t>(::pthread_getthreadid_np());
#elif defined(__NetBSD__)
                return static_cast<size_t>(::_lwp_self());
#elif defined(__OpenBSD__)
                return static_cast<size_t>(::getthrid());
#elif defined(__sun)
                return static_cast<size_t>(::thr_self());
#elif __APPLE__
                uint64_t tid;
                pthread_threadid_np(nullptr, &tid);
                return static_cast<size_t>(tid);
#else // Default to standard C++11 (other Unix)
                return static_cast<size_t>(std::hash<std::thread::id>()(std::this_thread::get_id()));
#endif
    }

// Return current thread id as size_t (from thread local storage)
    size_t thread_id() noexcept {
        static thread_local const size_t tid = _thread_id();
        return tid;
    }


#ifdef NOMEMORYSIZE
    size_t get_host_memory_size() {
    return 0L;  // Can't detect
}

#else

/**
 * Returns the size of physical memory (RAM) in bytes.
 */
    size_t get_host_memory_size() {
#if defined(_WIN32) && (defined(__CYGWIN__) || defined(__CYGWIN32__))
        /* Cygwin under Windows. ------------------------------------ */
    /* New 64-bit MEMORYSTATUSEX isn't available.  Use old 32.bit */
    MEMORYSTATUS status;
    status.dwLength = sizeof(status);
    GlobalMemoryStatus(&status);
    return (size_t)status.dwTotalPhys;

#elif defined(_WIN32)
        /* Windows. ------------------------------------------------- */
    /* Use new 64-bit MEMORYSTATUSEX, not old 32-bit MEMORYSTATUS */
    MEMORYSTATUSEX status;
    status.dwLength = sizeof(status);
    GlobalMemoryStatusEx(&status);
    return (size_t)status.ullTotalPhys;

#elif defined(__unix__) || defined(__unix) || defined(unix) || \
    (defined(__APPLE__) && defined(__MACH__))
        /* UNIX variants. ------------------------------------------- */
        /* Prefer sysctl() over sysconf() except sysctl() HW_REALMEM and HW_PHYSMEM
         */

#if defined(CTL_HW) && (defined(HW_MEMSIZE) || defined(HW_PHYSMEM64))
        int mib[2];
    mib[0]       = CTL_HW;
#if defined(HW_MEMSIZE)
    mib[1]       = HW_MEMSIZE; /* OSX. --------------------- */
#elif defined(HW_PHYSMEM64)
    mib[1] = HW_PHYSMEM64; /* NetBSD, OpenBSD. --------- */
#endif
    int64_t size = 0;          /* 64-bit */
    size_t len   = sizeof(size);
    if (sysctl(mib, 2, &size, &len, NULL, 0) == 0) return (size_t)size;
    return 0L; /* Failed? */

#elif defined(_SC_AIX_REALMEM)
        /* AIX. ----------------------------------------------------- */
    return (size_t)sysconf(_SC_AIX_REALMEM) * (size_t)1024L;

#elif defined(_SC_PHYS_PAGES) && defined(_SC_PAGESIZE)
        /* FreeBSD, Linux, OpenBSD, and Solaris. -------------------- */
        return static_cast<size_t>(sysconf(_SC_PHYS_PAGES)) *
               static_cast<size_t>(sysconf(_SC_PAGESIZE));

#elif defined(_SC_PHYS_PAGES) && defined(_SC_PAGE_SIZE)
        /* Legacy. -------------------------------------------------- */
    return (size_t)sysconf(_SC_PHYS_PAGES) * (size_t)sysconf(_SC_PAGE_SIZE);

#elif defined(CTL_HW) && (defined(HW_PHYSMEM) || defined(HW_REALMEM))
    /* DragonFly BSD, FreeBSD, NetBSD, OpenBSD, and OSX. -------- */
    int mib[2];
    mib[0]            = CTL_HW;
#if defined(HW_REALMEM)
    mib[1]            = HW_REALMEM; /* FreeBSD. ----------------- */
#elif defined(HW_PYSMEM)
    mib[1] = HW_PHYSMEM; /* Others. ------------------ */
#endif
    unsigned int size = 0;          /* 32-bit */
    size_t len        = sizeof(size);
    if (sysctl(mib, 2, &size, &len, NULL, 0) == 0) return (size_t)size;
    return 0L; /* Failed? */
#endif /* sysctl and sysconf variants */

#else
        return 0L; /* Unknown OS. */
#endif
    }

#endif  // NOMEMORYSIZE
}  // namespace turbo
