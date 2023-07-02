//
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
#ifndef TURBO_STRINGS_STR_TRIM_H_
#define TURBO_STRINGS_STR_TRIM_H_

#include "turbo/strings/ascii.h"
#include "turbo/platform/port.h"

namespace turbo {
    TURBO_NAMESPACE_BEGIN

    // Returns std::string_view with whitespace stripped from the beginning of the
    // given std::string_view.
    TURBO_MUST_USE_RESULT inline std::string_view TrimLeft(std::string_view str) {
        auto it = std::find_if_not(str.begin(), str.end(), turbo::ascii_isspace);
        return str.substr(static_cast<size_t>(it - str.begin()));
    }

    // Strips in place whitespace from the beginning of the given string.
    template<typename String>
    inline typename std::enable_if<turbo::is_string_type<String>::value>::type
    TrimLeft(String *str) {
        auto it = std::find_if_not(str->begin(), str->end(), turbo::ascii_isspace);
        str->erase(str->begin(), it);
    }

    // Returns std::string_view with whitespace stripped from the end of the given
    // std::string_view.
    TURBO_MUST_USE_RESULT inline std::string_view TrimRight(
            std::string_view str) {
        auto it = std::find_if_not(str.rbegin(), str.rend(), turbo::ascii_isspace);
        return str.substr(0, static_cast<size_t>(str.rend() - it));
    }

    // Strips in place whitespace from the end of the given string
    template<typename String>
    inline typename std::enable_if<turbo::is_string_type<String>::value>::type
    TrimRight(String *str) {
        auto it = std::find_if_not(str->rbegin(), str->rend(), turbo::ascii_isspace);
        str->erase(static_cast<size_t>(str->rend() - it));
    }

    // Returns std::string_view with whitespace stripped from both ends of the
    // given std::string_view.
    TURBO_MUST_USE_RESULT inline std::string_view Trim(
            std::string_view str) {
        return TrimRight(TrimLeft(str));
    }

    // Strips in place whitespace from both ends of the given string
    template<typename String>
    inline typename std::enable_if<turbo::is_string_type<String>::value>::type
    Trim(String *str) {
        TrimRight(str);
        TrimLeft(str);
    }

    // Removes leading, trailing, and consecutive internal whitespace.
    template<typename String>
    typename std::enable_if<turbo::is_string_type<String>::value>::type
    TrimAll(String *);

    TURBO_NAMESPACE_END
}
#endif  // TURBO_STRINGS_STR_TRIM_H_
