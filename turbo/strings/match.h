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
// -----------------------------------------------------------------------------
// File: match.h
// -----------------------------------------------------------------------------
//
// This file contains simple utilities for performing string matching checks.
// All of these function parameters are specified as `turbo::string_view`,
// meaning that these functions can accept `std::string`, `turbo::string_view` or
// NUL-terminated C-style strings.
//
// Examples:
//   std::string s = "foo";
//   turbo::string_view sv = "f";
//   assert(turbo::str_contains(s, sv));
//
// Note: The order of parameters in these functions is designed to mimic the
// order an equivalent member function would exhibit;
// e.g. `s.Contains(x)` ==> `turbo::str_contains(s, x).

#pragma once

#include <cstring>

#include <turbo/strings/string_view.h>

namespace turbo {

    // str_contains()
    //
    // Returns whether a given string `haystack` contains the substring `needle`.
    inline bool str_contains(turbo::string_view haystack,
                            turbo::string_view needle) noexcept {
        return haystack.find(needle, 0) != haystack.npos;
    }

    inline bool str_contains(turbo::string_view haystack, char needle) noexcept {
        return haystack.find(needle) != haystack.npos;
    }

    // starts_with()
    //
    // Returns whether a given string `text` begins with `prefix`.
    inline bool starts_with(turbo::string_view text,
                           turbo::string_view prefix) noexcept {
        return prefix.empty() ||
               (text.size() >= prefix.size() &&
                memcmp(text.data(), prefix.data(), prefix.size()) == 0);
    }

    // ends_with()
    //
    // Returns whether a given string `text` ends with `suffix`.
    inline bool ends_with(turbo::string_view text,
                         turbo::string_view suffix) noexcept {
        return suffix.empty() ||
               (text.size() >= suffix.size() &&
                memcmp(text.data() + (text.size() - suffix.size()), suffix.data(),
                       suffix.size()) == 0);
    }

    // str_contains_ignore_case()
    //
    // Returns whether a given ASCII string `haystack` contains the ASCII substring
    // `needle`, ignoring case in the comparison.
    bool str_contains_ignore_case(turbo::string_view haystack,
                               turbo::string_view needle) noexcept;

    bool str_contains_ignore_case(turbo::string_view haystack,
                               char needle) noexcept;

    // equals_ignore_case()
    //
    // Returns whether given ASCII strings `piece1` and `piece2` are equal, ignoring
    // case in the comparison.
    bool equals_ignore_case(turbo::string_view piece1,
                          turbo::string_view piece2) noexcept;

    // starts_with_ignore_case()
    //
    // Returns whether a given ASCII string `text` starts with `prefix`,
    // ignoring case in the comparison.
    bool starts_with_ignore_case(turbo::string_view text,
                              turbo::string_view prefix) noexcept;

    // ends_with_ignore_case()
    //
    // Returns whether a given ASCII string `text` ends with `suffix`, ignoring
    // case in the comparison.
    bool ends_with_ignore_case(turbo::string_view text,
                            turbo::string_view suffix) noexcept;

    // Yields the longest prefix in common between both input strings.
    // Pointer-wise, the returned result is a subset of input "a".
    turbo::string_view find_longest_common_prefix(turbo::string_view a,
                                               turbo::string_view b);

    // Yields the longest suffix in common between both input strings.
    // Pointer-wise, the returned result is a subset of input "a".
    turbo::string_view find_longest_common_suffix(turbo::string_view a,
                                               turbo::string_view b);

}  // namespace turbo
