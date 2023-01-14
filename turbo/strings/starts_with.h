
/****************************************************************
 * Copyright (c) 2022, liyinbin
 * All rights reserved.
 * Author by liyinbin (jeff.li) lijippy@163.com
 *****************************************************************/

#ifndef TURBO_BASE_STRING_START_WITH_H_
#define TURBO_BASE_STRING_START_WITH_H_

#include <string_view>
#include <string>
#include <cstring>
#include "turbo/base/profile.h"

namespace turbo {

/*!
 * Checks if the given match string is located at the start of this string.
 */
TURBO_FORCE_INLINE bool starts_with(std::string_view text, std::string_view prefix) {
    return prefix.empty() ||
           (text.size() >= prefix.size() &&
            memcmp(text.data(), prefix.data(), prefix.size()) == 0);
}

/*!
 * Checks if the given match string is located at the start of this
 * string. Compares the characters case-insensitively.
 */
bool starts_with_case(std::string_view text, std::string_view prefix);

}  // namespace turbo

#endif  // TURBO_BASE_STRING_START_WITH_H_
