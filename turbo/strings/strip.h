
/****************************************************************
 * Copyright (c) 2022, liyinbin
 * All rights reserved.
 * Author by liyinbin (jeff.li) lijippy@163.com
 *****************************************************************/
#ifndef TURBO_STRINGS_STRIP_H_
#define TURBO_STRINGS_STRIP_H_

#include <cstddef>
#include <string>
#include "turbo/base/profile.h"
#include "turbo/strings/ascii.h"
#include "turbo/strings/ends_with.h"
#include "turbo/strings/starts_with.h"
#include <string_view>

namespace turbo {


    //  consume_prefix()
    //
    // Strips the `expected` prefix from the start of the given string, returning
    // `true` if the strip operation succeeded or false otherwise.
    //
    // Example:
    //
    //   std::string_view input("abc");
    //   EXPECT_TRUE(turbo:: consume_prefix(&input, "a"));
    //   EXPECT_EQ(input, "bc");
    TURBO_FORCE_INLINE bool consume_prefix(std::string_view *str, std::string_view expected) {
        if (!turbo::starts_with(*str, expected)) return false;
        str->remove_prefix(expected.size());
        return true;
    }
    //  consume_suffix()
    //
    // Strips the `expected` suffix from the end of the given string, returning
    // `true` if the strip operation succeeded or false otherwise.
    //
    // Example:
    //
    //   std::string_view input("abcdef");
    //   EXPECT_TRUE(turbo:: consume_suffix(&input, "def"));
    //   EXPECT_EQ(input, "abc");
    TURBO_FORCE_INLINE bool consume_suffix(std::string_view *str, std::string_view expected) {
        if (!turbo::ends_with(*str, expected)) return false;
        str->remove_suffix(expected.size());
        return true;
    }

    //  strip_prefix()
    //
    // Returns a view into the input string 'str' with the given 'prefix' removed,
    // but leaving the original string intact. If the prefix does not match at the
    // start of the string, returns the original string instead.
    TURBO_MUST_USE_RESULT TURBO_FORCE_INLINE std::string_view strip_prefix(
            std::string_view str, std::string_view prefix) {
        if (turbo::starts_with(str, prefix)) str.remove_prefix(prefix.size());
        return str;
    }

    //  strip_suffix()
    //
    // Returns a view into the input string 'str' with the given 'suffix' removed,
    // but leaving the original string intact. If the suffix does not match at the
    // end of the string, returns the original string instead.
    TURBO_MUST_USE_RESULT TURBO_FORCE_INLINE std::string_view strip_suffix(
            std::string_view str, std::string_view suffix) {
        if (turbo::ends_with(str, suffix)) str.remove_suffix(suffix.size());
        return str;
    }


}  // namespace turbo

#endif  // TURBO_STRINGS_STRIP_H_
