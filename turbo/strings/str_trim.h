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
    struct ByAnyOf {
        explicit ByAnyOf(std::string_view str) : trimmer(str) {

        }

        bool operator()(char c) {
            return trimmer.find(c) != std::string_view::npos;
        }

        bool operator()(unsigned char c) {
            return trimmer.find(c) != std::string_view::npos;
        }

    private:
        std::string_view trimmer;
    };

    struct ByWhitespace {
        ByWhitespace() = default;

        bool operator()(unsigned char c) {
            return turbo::ascii_isspace(c);
        }

    private:
        std::string_view trimmer;
    };

    template<typename Pred = ByWhitespace>
    TURBO_MUST_USE_RESULT inline std::string_view TrimLeft(std::string_view str, Pred pred = Pred()) {
        auto it = std::find_if_not(str.begin(), str.end(), pred);
        return str.substr(static_cast<size_t>(it - str.begin()));
    }

    // Strips in place whitespace from the beginning of the given string.
    template<typename String, typename Pred = ByWhitespace>
    inline typename std::enable_if<turbo::is_string_type<String>::value>::type
    TrimLeft(String *str, Pred pred = Pred()) {
        auto it = std::find_if_not(str->begin(), str->end(), pred);
        str->erase(str->begin(), it);
    }

    // Returns std::string_view with whitespace stripped from the end of the given
    // std::string_view.
    template<typename Pred = ByWhitespace>
    TURBO_MUST_USE_RESULT inline std::string_view TrimRight(std::string_view str, Pred pred = Pred()) {
        auto it = std::find_if_not(str.rbegin(), str.rend(), pred);
        return str.substr(0, static_cast<size_t>(str.rend() - it));
    }

    // Strips in place whitespace from the end of the given string
    template<typename String, typename Pred = ByWhitespace>
    inline typename std::enable_if<turbo::is_string_type<String>::value>::type
    TrimRight(String *str, Pred pred = Pred()) {
        auto it = std::find_if_not(str->rbegin(), str->rend(), pred);
        str->erase(static_cast<size_t>(str->rend() - it));
    }

    // Returns std::string_view with whitespace stripped from both ends of the
    // given std::string_view.
    template<typename Pred = ByWhitespace>
    TURBO_MUST_USE_RESULT inline std::string_view Trim(std::string_view str, Pred pred = Pred()) {
        return TrimRight(TrimLeft(str, pred), pred);
    }

    // Strips in place whitespace from both ends of the given string
    template<typename String, typename Pred = ByWhitespace>
    inline typename std::enable_if<turbo::is_string_type<String>::value>::type
    Trim(String *str, Pred pred = Pred()) {
        TrimRight(str, pred);
        TrimLeft(str, pred);
    }

    // Removes leading, trailing, and consecutive internal whitespace.
    template<typename String>
    typename std::enable_if<turbo::is_string_type<String>::value>::type
    TrimAll(String *);

    TURBO_NAMESPACE_END
}
#endif  // TURBO_STRINGS_STR_TRIM_H_
