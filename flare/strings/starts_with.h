
/****************************************************************
 * Copyright (c) 2022, liyinbin
 * All rights reserved.
 * Author by liyinbin (jeff.li) lijippy@163.com
 *****************************************************************/

#ifndef FLARE_BASE_STRING_START_WITH_H_
#define FLARE_BASE_STRING_START_WITH_H_

#include <string_view>
#include <string>
#include <cstring>
#include "flare/base/profile.h"

namespace flare {

/*!
 * Checks if the given match string is located at the start of this string.
 */
FLARE_FORCE_INLINE bool starts_with(std::string_view text, std::string_view prefix) {
    return prefix.empty() ||
           (text.size() >= prefix.size() &&
            memcmp(text.data(), prefix.data(), prefix.size()) == 0);
}

/*!
 * Checks if the given match string is located at the start of this
 * string. Compares the characters case-insensitively.
 */
bool starts_with_case(std::string_view text, std::string_view prefix);

}  // namespace flare

#endif  // FLARE_BASE_STRING_START_WITH_H_
