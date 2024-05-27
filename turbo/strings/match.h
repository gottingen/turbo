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
//   assert(turbo::StrContains(s, sv));
//
// Note: The order of parameters in these functions is designed to mimic the
// order an equivalent member function would exhibit;
// e.g. `s.Contains(x)` ==> `turbo::StrContains(s, x).
#ifndef TURBO_STRINGS_MATCH_H_
#define TURBO_STRINGS_MATCH_H_

#include <cstring>

#include <turbo/strings/string_view.h>

namespace turbo {
TURBO_NAMESPACE_BEGIN

// StrContains()
//
// Returns whether a given string `haystack` contains the substring `needle`.
inline bool StrContains(turbo::string_view haystack,
                        turbo::string_view needle) noexcept {
  return haystack.find(needle, 0) != haystack.npos;
}

inline bool StrContains(turbo::string_view haystack, char needle) noexcept {
  return haystack.find(needle) != haystack.npos;
}

// StartsWith()
//
// Returns whether a given string `text` begins with `prefix`.
inline bool StartsWith(turbo::string_view text,
                       turbo::string_view prefix) noexcept {
  return prefix.empty() ||
         (text.size() >= prefix.size() &&
          memcmp(text.data(), prefix.data(), prefix.size()) == 0);
}

// EndsWith()
//
// Returns whether a given string `text` ends with `suffix`.
inline bool EndsWith(turbo::string_view text,
                     turbo::string_view suffix) noexcept {
  return suffix.empty() ||
         (text.size() >= suffix.size() &&
          memcmp(text.data() + (text.size() - suffix.size()), suffix.data(),
                 suffix.size()) == 0);
}
// StrContainsIgnoreCase()
//
// Returns whether a given ASCII string `haystack` contains the ASCII substring
// `needle`, ignoring case in the comparison.
bool StrContainsIgnoreCase(turbo::string_view haystack,
                           turbo::string_view needle) noexcept;

bool StrContainsIgnoreCase(turbo::string_view haystack,
                           char needle) noexcept;

// EqualsIgnoreCase()
//
// Returns whether given ASCII strings `piece1` and `piece2` are equal, ignoring
// case in the comparison.
bool EqualsIgnoreCase(turbo::string_view piece1,
                      turbo::string_view piece2) noexcept;

// StartsWithIgnoreCase()
//
// Returns whether a given ASCII string `text` starts with `prefix`,
// ignoring case in the comparison.
bool StartsWithIgnoreCase(turbo::string_view text,
                          turbo::string_view prefix) noexcept;

// EndsWithIgnoreCase()
//
// Returns whether a given ASCII string `text` ends with `suffix`, ignoring
// case in the comparison.
bool EndsWithIgnoreCase(turbo::string_view text,
                        turbo::string_view suffix) noexcept;

// Yields the longest prefix in common between both input strings.
// Pointer-wise, the returned result is a subset of input "a".
turbo::string_view FindLongestCommonPrefix(turbo::string_view a,
                                          turbo::string_view b);

// Yields the longest suffix in common between both input strings.
// Pointer-wise, the returned result is a subset of input "a".
turbo::string_view FindLongestCommonSuffix(turbo::string_view a,
                                          turbo::string_view b);

TURBO_NAMESPACE_END
}  // namespace turbo

#endif  // TURBO_STRINGS_MATCH_H_
