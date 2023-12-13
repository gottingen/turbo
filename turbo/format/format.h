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
//

#ifndef TURBO_FORMAT_FORMAT_H_
#define TURBO_FORMAT_FORMAT_H_

#include <string>
#include "turbo/format/fmt/format.h"
#include "turbo/format/fmt/core.h"
#include "turbo/format/fmt/ranges.h"
#include "turbo/format/fmt/printf.h"
#include "turbo/format/fmt/std.h"
#include "turbo/platform/port.h"

namespace turbo {

    template<typename T>
    auto Ptr(T p) -> const void * {
        return fmt::ptr(p);
    }

    template<typename T, typename Deleter>
    auto Ptr(const std::unique_ptr<T, Deleter> &p) -> const void * {
        return fmt::ptr(p);
    }

    template<typename T>
    auto Ptr(const std::shared_ptr<T> &p) -> const void * {
        return fmt::ptr(p);
    }

    template<typename String = std::string, typename ...Args>
    TURBO_MUST_USE_RESULT inline String format(std::string_view fmt, Args &&... args) {
        String result;
        fmt::memory_buffer buf;
        fmt::format_to(std::back_inserter(buf), fmt, std::forward<Args>(args)...);
        return String(buf.data(), buf.size());
    }

    template<typename String = std::string, typename T>
    TURBO_MUST_USE_RESULT inline String format(const T &t) {
        String result;
        fmt::memory_buffer buf;
        fmt::format_to(std::back_inserter(buf), "{}", t);
        return String(buf.data(), buf.size());
    }

    template<typename String = std::string, typename ...Args>
    void format_append(String *dst, std::string_view fmt, Args &&... args) {
        fmt::memory_buffer buf;
        fmt::format_to(std::back_inserter(buf), fmt, std::forward<Args>(args)...);
        dst->append(buf.data(), buf.size());
    }

    template<typename String = std::string, typename T>
    void format_append(String *dst, const T &t) {
        fmt::memory_buffer buf;
        fmt::format_to(std::back_inserter(buf), "{}", t);
        dst->append(buf.data(), buf.size());
    }


    template<typename String = std::string, typename ...Args>
    String format_range(std::string_view fmt, const std::tuple<Args...> &tuple, std::string_view sep) {
        fmt::memory_buffer view_buf;
        fmt::format_to(std::back_inserter(view_buf), fmt, fmt::join(tuple, sep));
        return String(view_buf.data(), view_buf.size());
    }

    template<typename String = std::string, typename T>

    String format_range(std::string_view fmt, std::initializer_list<T> list, std::string_view sep) {
        fmt::memory_buffer view_buf;
        fmt::format_to(std::back_inserter(view_buf), fmt, fmt::join(list, sep));
        return String(view_buf.data(), view_buf.size());
    }

    template<typename It, typename Sentinel, typename String = std::string>
    String format_range(std::string_view fmt, It begin, Sentinel end, std::string_view sep) {
        fmt::memory_buffer view_buf;
        fmt::format_to(std::back_inserter(view_buf), fmt,
                       fmt::join(std::forward<It>(begin), std::forward<Sentinel>(end), sep));
        return String(view_buf.data(), view_buf.size());
    }

    template<typename String = std::string, typename Range>
    String format_range(std::string_view fmt, Range &&range, std::string_view sep) {
        fmt::memory_buffer view_buf;
        fmt::format_to(std::back_inserter(view_buf), fmt, fmt::join(std::forward<Range>(range), sep));
        return String(view_buf.data(), view_buf.size());
    }

    /// format_range_append
    template<typename String = std::string, typename ...Args>
    void format_range_append(String *dst, std::string_view fmt, const std::tuple<Args...> &tuple, std::string_view sep) {
        fmt::memory_buffer view_buf;
        fmt::format_to(std::back_inserter(view_buf), fmt, fmt::join(tuple, sep));
        dst->append(view_buf.data(), view_buf.size());
    }

    template<typename String = std::string, typename T>

    void format_range_append(String *dst, std::string_view fmt, std::initializer_list<T> list, std::string_view sep) {
        fmt::memory_buffer view_buf;
        fmt::format_to(std::back_inserter(view_buf), fmt, fmt::join(list, sep));
        dst->append(view_buf.data(), view_buf.size());
    }

    template<typename String = std::string, typename It, typename Sentinel>
    void format_range_append(String *dst, std::string_view fmt, It begin, Sentinel end, std::string_view sep) {
        fmt::memory_buffer view_buf;
        fmt::format_to(std::back_inserter(view_buf), fmt, fmt::join(begin, end, sep));
        dst->append(view_buf.data(), view_buf.size());
    }

    template<typename String = std::string, typename Range>
    void format_range_append(String *dst, std::string_view fmt, Range &&range, std::string_view sep) {
        fmt::memory_buffer view_buf;
        fmt::format_to(std::back_inserter(view_buf), fmt, fmt::join(std::forward<Range>(range), sep));
        dst->append(view_buf.data(), view_buf.size());
    }
}  // namespace turbo

#endif  // TURBO_FORMAT_FORMAT_H_
