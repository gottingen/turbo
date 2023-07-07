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

#include <turbo/log/tweakme.h>
#include "turbo/log/details/null_mutex.h"

#include <atomic>
#include <chrono>
#include <initializer_list>
#include <memory>
#include <exception>
#include <string>
#include <type_traits>
#include <functional>
#include <cstdio>
#include <string_view>
#include <turbo/format/format.h>

// disable thread local on msvc 2013
#ifndef TLOG_NO_TLS
#    if (defined(_MSC_VER) && (_MSC_VER < 1900)) || defined(__cplusplus_winrt)
#        define TLOG_NO_TLS 1
#    endif
#endif

#ifndef TLOG_FUNCTION
#    define TLOG_FUNCTION static_cast<const char *>(__FUNCTION__)
#endif

#ifdef TLOG_NO_EXCEPTIONS
#    define TLOG_TRY
#    define TLOG_THROW(ex)                                                                                                               \
        do                                                                                                                                 \
        {                                                                                                                                  \
            printf("tlog fatal error: %s\n", ex.what());                                                                                 \
            std::abort();                                                                                                                  \
        } while (0)
#    define TLOG_CATCH_STD
#else
#    define TLOG_TRY try
#    define TLOG_THROW(ex) throw(ex)
#    define TLOG_CATCH_STD                                                                                                               \
        catch (const std::exception &) {}
#endif

namespace turbo::tlog {

    class formatter;

    namespace sinks {
        class sink;
    }

#if defined(_WIN32) && defined(TLOG_WCHAR_FILENAMES)
    using filename_t = std::wstring;
    // allow macro expansion to occur in TLOG_FILENAME_T
#    define TLOG_FILENAME_T_INNER(s) L##s
#    define TLOG_FILENAME_T(s) TLOG_FILENAME_T_INNER(s)
#else
    using filename_t = std::string;
#    define TLOG_FILENAME_T(s) s
#endif

    using log_clock = std::chrono::system_clock;
    using sink_ptr = std::shared_ptr<sinks::sink>;
    using sinks_init_list = std::initializer_list<sink_ptr>;
    using err_handler = std::function<void(const std::string &err_msg)>;

    namespace fmt_lib = fmt;

    using string_view_t = fmt::basic_string_view<char>;
    using memory_buf_t = fmt::basic_memory_buffer<char, 250>;

    template<typename... Args>
    using format_string_t = fmt::format_string<Args...>;

    template<class T>
    using remove_cvref_t = typename std::remove_cv<typename std::remove_reference<T>::type>::type;

// clang doesn't like SFINAE disabled constructor in std::is_convertible<> so have to repeat the condition from basic_format_string here,
// in addition, fmt::basic_runtime<Char> is only convertible to basic_format_string<Char> but not basic_string_view<Char>
    template<class T, class Char = char>
    struct is_convertible_to_basic_format_string
            : std::integral_constant<bool,
                    std::is_convertible<T, fmt::basic_string_view<Char>>::value> {
    };

#    if defined(TLOG_WCHAR_FILENAMES) || defined(TLOG_WCHAR_TO_UTF8_SUPPORT)
    using wstring_view_t = fmt::basic_string_view<wchar_t>;
    using wmemory_buf_t = fmt::basic_memory_buffer<wchar_t, 250>;

    template<typename... Args>
    using wformat_string_t = fmt::wformat_string<Args...>;
#    endif
#    define TLOG_BUF_TO_STRING(x) fmt::to_string(x)

#ifdef TLOG_WCHAR_TO_UTF8_SUPPORT
#    ifndef _WIN32
#        error TLOG_WCHAR_TO_UTF8_SUPPORT only supported on windows
#    endif // _WIN32
#endif     // TLOG_WCHAR_TO_UTF8_SUPPORT

    template<class T>
    struct is_convertible_to_any_format_string : std::integral_constant<bool,
            is_convertible_to_basic_format_string<T, char>::value ||
            is_convertible_to_basic_format_string<T, wchar_t>::value> {
    };

#if defined(TLOG_NO_ATOMIC_LEVELS)
    using level_t = details::null_atomic_int;
#else
    using level_t = std::atomic<int>;
#endif

#define TLOG_LEVEL_TRACE 0
#define TLOG_LEVEL_DEBUG 1
#define TLOG_LEVEL_INFO 2
#define TLOG_LEVEL_WARN 3
#define TLOG_LEVEL_ERROR 4
#define TLOG_LEVEL_CRITICAL 5
#define TLOG_LEVEL_OFF 6

#if !defined(TLOG_ACTIVE_LEVEL)
#    define TLOG_ACTIVE_LEVEL TLOG_LEVEL_TRACE
#endif

// Log level enum
    namespace level {
        enum level_enum : int {
            trace = TLOG_LEVEL_TRACE,
            debug = TLOG_LEVEL_DEBUG,
            info = TLOG_LEVEL_INFO,
            warn = TLOG_LEVEL_WARN,
            err = TLOG_LEVEL_ERROR,
            critical = TLOG_LEVEL_CRITICAL,
            off = TLOG_LEVEL_OFF,
            n_levels
        };

#define TLOG_LEVEL_NAME_TRACE turbo::tlog::string_view_t("trace", 5)
#define TLOG_LEVEL_NAME_DEBUG turbo::tlog::string_view_t("debug", 5)
#define TLOG_LEVEL_NAME_INFO turbo::tlog::string_view_t("info", 4)
#define TLOG_LEVEL_NAME_WARNING turbo::tlog::string_view_t("warning", 7)
#define TLOG_LEVEL_NAME_ERROR turbo::tlog::string_view_t("error", 5)
#define TLOG_LEVEL_NAME_CRITICAL turbo::tlog::string_view_t("critical", 8)
#define TLOG_LEVEL_NAME_OFF turbo::tlog::string_view_t("off", 3)

#if !defined(TLOG_LEVEL_NAMES)
#    define TLOG_LEVEL_NAMES                                                                                                             \
        {                                                                                                                                  \
            TLOG_LEVEL_NAME_TRACE, TLOG_LEVEL_NAME_DEBUG, TLOG_LEVEL_NAME_INFO, TLOG_LEVEL_NAME_WARNING, TLOG_LEVEL_NAME_ERROR,  \
                TLOG_LEVEL_NAME_CRITICAL, TLOG_LEVEL_NAME_OFF                                                                          \
        }
#endif

#if !defined(TLOG_SHORT_LEVEL_NAMES)

#    define TLOG_SHORT_LEVEL_NAMES                                                                                                       \
        {                                                                                                                                  \
            "T", "D", "I", "W", "E", "C", "O"                                                                                              \
        }
#endif

        TURBO_DLL const string_view_t &to_string_view(turbo::tlog::level::level_enum l) noexcept;

        TURBO_DLL const char *to_short_c_str(turbo::tlog::level::level_enum l) noexcept;

        TURBO_DLL turbo::tlog::level::level_enum from_str(const std::string &name) noexcept;

    } // namespace level

    //
    // Color mode used by sinks with color support.
    //
    enum class color_mode {
        always,
        automatic,
        never
    };

    //
    // Pattern time - specific time getting to use for pattern_formatter.
    // local time by default
    //
    enum class pattern_time_type {
        local, // log localtime
        utc    // log utc
    };

    //
    // Log exception
    //
    class TURBO_DLL tlog_ex : public std::exception {
    public:
        explicit tlog_ex(std::string msg);

        tlog_ex(const std::string &msg, int last_errno);

        const char *what() const noexcept override;

    private:
        std::string msg_;
    };

    [[noreturn]] TURBO_DLL void throw_tlog_ex(const std::string &msg, int last_errno);

    [[noreturn]] TURBO_DLL void throw_tlog_ex(std::string msg);

    struct source_loc {
        constexpr source_loc() = default;

        constexpr source_loc(const char *filename_in, int line_in, const char *funcname_in)
                : filename{filename_in}, line{line_in}, funcname{funcname_in} {}

        constexpr bool empty() const noexcept {
            return line == 0;
        }

        const char *filename{nullptr};
        int line{0};
        const char *funcname{nullptr};
    };

    struct file_event_handlers {
        file_event_handlers()
                : before_open(nullptr), after_open(nullptr), before_close(nullptr), after_close(nullptr) {}

        std::function<void(const filename_t &filename)> before_open;
        std::function<void(const filename_t &filename, std::FILE *file_stream)> after_open;
        std::function<void(const filename_t &filename, std::FILE *file_stream)> before_close;
        std::function<void(const filename_t &filename)> after_close;
    };

    namespace details {

        using std::enable_if_t;
        using std::make_unique;

        // to avoid useless casts (see https://github.com/nlohmann/json/issues/2893#issuecomment-889152324)
        template<typename T, typename U, enable_if_t<!std::is_same<T, U>::value, int> = 0>
        constexpr T conditional_static_cast(U value) {
            return static_cast<T>(value);
        }

        template<typename T, typename U, enable_if_t<std::is_same<T, U>::value, int> = 0>
        constexpr T conditional_static_cast(U value) {
            return value;
        }

    } // namespace details
} // namespace turbo::tlog

