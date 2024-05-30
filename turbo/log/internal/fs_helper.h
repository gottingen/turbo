//
// Copyright (C) 2024 EA group inc.
// Author: Jeff.li lijippy@163.com
// All rights reserved.
// This program is free software: you can redistribute it and/or modify
// it under the terms of the GNU Affero General Public License as published
// by the Free Software Foundation, either version 3 of the License, or
// (at your option) any later version.
//
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU Affero General Public License for more details.
//
// You should have received a copy of the GNU Affero General Public License
// along with this program.  If not, see <https://www.gnu.org/licenses/>.
//
//
#pragma once


#include <ctime>  // std::time_t
#include <string>
#include <turbo/base/config.h>

#include <algorithm>
#include <array>
#include <chrono>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <ctime>
#include <string>
#include <sys/stat.h>
#include <sys/types.h>
#include <thread>

#ifdef _WIN32
#include <spdlog/details/windows_include.h>
#include <fileapi.h>  // for FlushFileBuffers
#include <io.h>       // for _get_osfhandle, _isatty, _fileno
#include <process.h>  // for _get_pid

#ifdef __MINGW32__
#include <share.h>
#endif

#include <cassert>
#include <limits>
#include <direct.h>  // for _mkdir/_wmkdir

#else  // unix

#include <fcntl.h>
#include <unistd.h>

#ifdef __linux__

#include <sys/syscall.h>  //Use gettid() syscall under linux to get thread id

#elif defined(_AIX)
#include <pthread.h>  // for pthread_getthrds_np

#elif defined(__DragonFly__) || defined(__FreeBSD__)
#include <pthread_np.h>  // for pthread_getthreadid_np

#elif defined(__NetBSD__)
#include <lwp.h>  // for _lwp_self

#elif defined(__sun)
#include <thread.h>  // for thr_self
#endif

#endif  // unix

namespace turbo::log_internal {

// folder separator
#if !defined(FOLDER_SEPS)
#ifdef _WIN32
#define FOLDER_SEPS "\\/"
#else
#define FOLDER_SEPS "/"
#endif
#endif

    constexpr static const char folder_seps[] = FOLDER_SEPS;
    constexpr static const char folder_seps_filename[] = FOLDER_SEPS;

    // Remove filename. return 0 on success
    inline int remove(const std::string &filename) noexcept {
        return std::remove(filename.c_str());
    }

    // Remove file if exists. return 0 on success
    // Note: Non atomic (might return failure to delete if concurrently deleted by other process/thread)
    int remove_if_exists(const std::string &filename) noexcept;

    inline int rename(const std::string &filename1, const std::string &filename2) noexcept {
        return std::rename(filename1.c_str(), filename2.c_str());
    }

    // Return if file exists.
    inline bool path_exists(const std::string &filename) noexcept {
        struct stat buffer;
        return (::stat(filename.c_str(), &buffer) == 0);
    }

    // Return file size according to open FILE* object
    inline ssize_t filesize(FILE *f) {
        if (f == nullptr) {
            return -1;
        }
#if defined(_WIN32) && !defined(__CYGWIN__)
        int fd = ::_fileno(f);
#if defined(_WIN64)  // 64 bits
    __int64 ret = ::_filelengthi64(fd);
    if (ret >= 0) {
        return static_cast<size_t>(ret);
    }

#else  // windows 32 bits
    long ret = ::_filelength(fd);
    if (ret >= 0) {
        return static_cast<size_t>(ret);
    }
#endif

#else  // unix
        // OpenBSD and AIX doesn't compile with :: before the fileno(..)
#if defined(__OpenBSD__) || defined(_AIX)
        int fd = fileno(f);
#else
        int fd = ::fileno(f);
#endif
        // 64 bits(but not in osx, linux/musl or cygwin, where fstat64 is deprecated)
#if ((defined(__linux__) && defined(__GLIBC__)) || defined(__sun) || defined(_AIX)) && \
        (defined(__LP64__) || defined(_LP64))
        struct stat64 st;
        if (::fstat64(fd, &st) == 0) {
            return static_cast<size_t>(st.st_size);
        }
#else  // other unix or linux 32 bits or cygwin
        struct stat st;
    if (::fstat(fd, &st) == 0) {
        return static_cast<size_t>(st.st_size);
    }
#endif
#endif
        return -1;  // will not be reached.
    }

    // Return directory name from given path or empty string
    // "abc/file" => "abc"
    // "abc/" => "abc"
    // "abc" => ""
    // "abc///" => "abc//"
    inline std::string dir_name(const std::string &path) {
        auto pos = path.find_last_of(folder_seps_filename);
        return pos != std::string::npos ? path.substr(0, pos) : std::string{};
    }

    // Create a dir from the given path.
    // Return true if succeeded or if this dir already exists.
    inline bool create_dir(const std::string &path) {
        if (path_exists(path)) {
            return true;
        }

        if (path.empty()) {
            return false;
        }

        size_t search_offset = 0;
        do {
            auto token_pos = path.find_first_of(folder_seps_filename, search_offset);
            // treat the entire path as a folder if no folder separator not found
            if (token_pos == std::string::npos) {
                token_pos = path.size();
            }

            auto subdir = path.substr(0, token_pos);
#ifdef _WIN32
            // if subdir is just a drive letter, add a slash e.g. "c:"=>"c:\",
        // otherwise path_exists(subdir) returns false (issue #3079)
        const bool is_drive = subdir.length() == 2 && subdir[1] == ':';
        if (is_drive) {
            subdir += '\\';
            token_pos++;
        }
#endif

            if (!subdir.empty() && !path_exists(subdir) && ::mkdir(path.c_str(), mode_t(0755)) != 0) {
                return false;  // return error if failed creating dir
            }
            search_offset = token_pos + 1;
        } while (search_offset < path.size());

        return true;
    }

    // non thread safe, cross platform getenv/getenv_s
    // return empty string if field not found
    std::string getenv(const char *field);

    // Do fsync by FILE objectpointer.
    // Return true on success.
    bool fsync(FILE *fp);

    inline int remove_if_exists(const std::string &filename) noexcept {
        return path_exists(filename) ? remove(filename) : 0;
    }

    inline bool in_terminal(FILE *file) noexcept {
#ifdef _WIN32
        return ::_isatty(_fileno(file)) != 0;
#else
        return ::isatty(fileno(file)) != 0;
#endif
    }

    inline bool is_color_terminal() noexcept {
#ifdef _WIN32
        return true;
#else

        static const bool result = []() {
            const char *env_colorterm_p = std::getenv("COLORTERM");
            if (env_colorterm_p != nullptr) {
                return true;
            }

            static constexpr std::array<const char *, 16> terms = {
                    {"ansi", "color", "console", "cygwin", "gnome", "konsole", "kterm", "linux", "msys",
                     "putty", "rxvt", "screen", "vt100", "xterm", "alacritty", "vt102"}};

            const char *env_term_p = std::getenv("TERM");
            if (env_term_p == nullptr) {
                return false;
            }

            return std::any_of(terms.begin(), terms.end(), [&](const char *term) {
                return std::strstr(env_term_p, term) != nullptr;
            });
        }();

        return result;
#endif
    }

    inline std::tuple<std::string, std::string> split_by_extension(
            const std::string &fname) {
        auto ext_index = fname.rfind('.');

        // no valid extension found - return whole path and empty string as
        // extension
        if (ext_index == std::string::npos || ext_index == 0 || ext_index == fname.size() - 1) {
            return std::make_tuple(fname, std::string());
        }

        // treat cases like "/etc/rc.d/somelogfile or "/abc/.hiddenfile"
        auto folder_index = fname.find_last_of('/');
        if (folder_index != std::string::npos && folder_index >= ext_index - 1) {
            return std::make_tuple(fname, std::string());
        }

        // finally - return a valid base and extension tuple
        return std::make_tuple(fname.substr(0, ext_index), fname.substr(ext_index));
    }
}  // namespace turbo::log_internal
