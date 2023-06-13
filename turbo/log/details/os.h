// Copyright 2023 The titan-search Authors.
// Copyright(c) 2015-present, Gabi Melman & spdlog contributors.
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
//

#pragma once

#include "turbo/log/common.h"
#include <ctime> // std::time_t

namespace turbo::tlog {
namespace details {
namespace os {

TURBO_DLL turbo::tlog::log_clock::time_point now() noexcept;

TURBO_DLL std::tm localtime(const std::time_t &time_tt) noexcept;

TURBO_DLL std::tm localtime() noexcept;

TURBO_DLL std::tm gmtime(const std::time_t &time_tt) noexcept;

TURBO_DLL std::tm gmtime() noexcept;

// eol definition
#if !defined(TLOG_EOL)
#    ifdef _WIN32
#        define TLOG_EOL "\r\n"
#    else
#        define TLOG_EOL "\n"
#    endif
#endif

constexpr static const char *default_eol = TLOG_EOL;

// folder separator
#if !defined(TLOG_FOLDER_SEPS)
#    ifdef _WIN32
#        define TLOG_FOLDER_SEPS "\\/"
#    else
#        define TLOG_FOLDER_SEPS "/"
#    endif
#endif

constexpr static const char folder_seps[] = TLOG_FOLDER_SEPS;
constexpr static const filename_t::value_type folder_seps_filename[] = TLOG_FILENAME_T(TLOG_FOLDER_SEPS);

// fopen_s on non windows for writing
TURBO_DLL bool fopen_s(FILE **fp, const filename_t &filename, const filename_t &mode);

// Remove filename. return 0 on success
TURBO_DLL int remove(const filename_t &filename) noexcept;

// Remove file if exists. return 0 on success
// Note: Non atomic (might return failure to delete if concurrently deleted by other process/thread)
TURBO_DLL int remove_if_exists(const filename_t &filename) noexcept;

TURBO_DLL int rename(const filename_t &filename1, const filename_t &filename2) noexcept;

// Return if file exists.
TURBO_DLL bool path_exists(const filename_t &filename) noexcept;

// Return file size according to open FILE* object
TURBO_DLL size_t filesize(FILE *f);

// Return utc offset in minutes or throw tlog_ex on failure
TURBO_DLL int utc_minutes_offset(const std::tm &tm = details::os::localtime());

// Return current thread id as size_t
// It exists because the std::this_thread::get_id() is much slower(especially
// under VS 2013)
TURBO_DLL size_t _thread_id() noexcept;

// Return current thread id as size_t (from thread local storage)
TURBO_DLL size_t thread_id() noexcept;

// This is avoid msvc issue in sleep_for that happens if the clock changes.
// See https://github.com/gabime/spdlog/issues/609
TURBO_DLL void sleep_for_millis(unsigned int milliseconds) noexcept;

TURBO_DLL std::string filename_to_str(const filename_t &filename);

TURBO_DLL int pid() noexcept;

// Determine if the terminal supports colors
// Source: https://github.com/agauniyal/rang/
TURBO_DLL bool is_color_terminal() noexcept;

// Determine if the terminal attached
// Source: https://github.com/agauniyal/rang/
TURBO_DLL bool in_terminal(FILE *file) noexcept;

#if (defined(TLOG_WCHAR_TO_UTF8_SUPPORT) || defined(TLOG_WCHAR_FILENAMES)) && defined(_WIN32)
TURBO_DLL void wstr_to_utf8buf(wstring_view_t wstr, memory_buf_t &target);

TURBO_DLL void utf8_to_wstrbuf(string_view_t str, wmemory_buf_t &target);
#endif

// Return directory name from given path or empty string
// "abc/file" => "abc"
// "abc/" => "abc"
// "abc" => ""
// "abc///" => "abc//"
TURBO_DLL filename_t dir_name(const filename_t &path);

// Create a dir from the given path.
// Return true if succeeded or if this dir already exists.
TURBO_DLL bool create_dir(const filename_t &path);

// non thread safe, cross platform getenv/getenv_s
// return empty string if field not found
TURBO_DLL std::string getenv(const char *field);

} // namespace os
} // namespace details
} // namespace turbo::tlog
