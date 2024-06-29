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

#include <turbo/flags/cli/encoding.h>
#include <turbo/base/macros.h>
#include <array>
#include <clocale>
#include <cstdlib>
#include <cstring>
#include <cwchar>
#include <locale>
#include <stdexcept>
#include <string>
#include <type_traits>
#include <utility>

namespace turbo::cli {

    namespace detail {


        TURBO_DIAGNOSTIC_PUSH
        TURBO_DIAGNOSTIC_IGNORE_DEPRECATED

        std::string narrow_impl(const wchar_t *str, std::size_t str_size) {
#ifdef _WIN32
            return std::wstring_convert<std::codecvt_utf8_utf16<wchar_t>>().to_bytes(str, str + str_size);

#else
            return std::wstring_convert<std::codecvt_utf8<wchar_t>>().to_bytes(str, str + str_size);

#endif  // _WIN32
        }

        std::wstring widen_impl(const char *str, std::size_t str_size) {
#ifdef _WIN32
            return std::wstring_convert<std::codecvt_utf8_utf16<wchar_t>>().from_bytes(str, str + str_size);

#else
            return std::wstring_convert<std::codecvt_utf8<wchar_t>>().from_bytes(str, str + str_size);

#endif  // _WIN32
        }

        TURBO_DIAGNOSTIC_POP

    }  // namespace detail

    std::string narrow(const wchar_t *str, std::size_t str_size) { return detail::narrow_impl(str, str_size); }

    std::string narrow(const std::wstring &str) { return detail::narrow_impl(str.data(), str.size()); }

    // Flawfinder: ignore
    std::string narrow(const wchar_t *str) { return detail::narrow_impl(str, std::wcslen(str)); }

    std::wstring widen(const char *str, std::size_t str_size) { return detail::widen_impl(str, str_size); }

    std::wstring widen(const std::string &str) { return detail::widen_impl(str.data(), str.size()); }

    // Flawfinder: ignore
    std::wstring widen(const char *str) { return detail::widen_impl(str, std::strlen(str)); }

    std::string narrow(std::wstring_view str) { return detail::narrow_impl(str.data(), str.size()); }

    std::wstring widen(std::string_view str) { return detail::widen_impl(str.data(), str.size()); }

}  // namespace turbo::cli
