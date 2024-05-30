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
// File: strip.h
// -----------------------------------------------------------------------------
//
// This file contains various functions for stripping substrings from a string.

#pragma once

#include <cstddef>
#include <string>

#include <turbo/base/macros.h>
#include <turbo/base/nullability.h>
#include <turbo/strings/ascii.h>
#include <turbo/strings/match.h>
#include <turbo/strings/string_view.h>

namespace turbo {

    // consume_prefix()
    //
    // Strips the `expected` prefix, if found, from the start of `str`.
    // If the operation succeeded, `true` is returned.  If not, `false`
    // is returned and `str` is not modified.
    //
    // Example:
    //
    //   turbo::string_view input("abc");
    //   EXPECT_TRUE(turbo::consume_prefix(&input, "a"));
    //   EXPECT_EQ(input, "bc");
    inline bool consume_prefix(turbo::Nonnull<turbo::string_view *> str,
                              turbo::string_view expected) {
        if (!turbo::starts_with(*str, expected)) return false;
        str->remove_prefix(expected.size());
        return true;
    }

    // consume_suffix()
    //
    // Strips the `expected` suffix, if found, from the end of `str`.
    // If the operation succeeded, `true` is returned.  If not, `false`
    // is returned and `str` is not modified.
    //
    // Example:
    //
    //   turbo::string_view input("abcdef");
    //   EXPECT_TRUE(turbo::consume_suffix(&input, "def"));
    //   EXPECT_EQ(input, "abc");
    inline bool consume_suffix(turbo::Nonnull<turbo::string_view *> str,
                              turbo::string_view expected) {
        if (!turbo::ends_with(*str, expected)) return false;
        str->remove_suffix(expected.size());
        return true;
    }

    // strip_prefix()
    //
    // Returns a view into the input string `str` with the given `prefix` removed,
    // but leaving the original string intact. If the prefix does not match at the
    // start of the string, returns the original string instead.
    TURBO_MUST_USE_RESULT inline turbo::string_view strip_prefix(
            turbo::string_view str, turbo::string_view prefix) {
        if (turbo::starts_with(str, prefix)) str.remove_prefix(prefix.size());
        return str;
    }

    // strip_suffix()
    //
    // Returns a view into the input string `str` with the given `suffix` removed,
    // but leaving the original string intact. If the suffix does not match at the
    // end of the string, returns the original string instead.
    TURBO_MUST_USE_RESULT inline turbo::string_view strip_suffix(
            turbo::string_view str, turbo::string_view suffix) {
        if (turbo::ends_with(str, suffix)) str.remove_suffix(suffix.size());
        return str;
    }

}  // namespace turbo
