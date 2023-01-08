
/****************************************************************
 * Copyright (c) 2022, liyinbin
 * All rights reserved.
 * Author by liyinbin (jeff.li) lijippy@163.com
 *****************************************************************/

#ifndef FLARE_STRING_END_WITH_H_
#define FLARE_STRING_END_WITH_H_

#include <string_view>
#include <string>
#include <cstring>
#include "flare/base/profile.h"

namespace flare {

    /**
     * @brief Checks if the given suffix string is located at the end of this string.
     * @param text todo
     * @param suffix todo
     * @return todo
     */
    FLARE_FORCE_INLINE bool ends_with(std::string_view text, std::string_view suffix) {
        return suffix.empty() ||
               (text.size() >= suffix.size() &&
                memcmp(text.data() + (text.size() - suffix.size()), suffix.data(),
                       suffix.size()) == 0);
    }



    // ends_with_ignore_case()
    //
    // Returns whether a given ASCII string `text` ends with `suffix`, ignoring
    // case in the comparison.
    bool ends_with_ignore_case(std::string_view text,
                               std::string_view suffix) noexcept;
}  // namespace flare

#endif  // FLARE_STRING_END_WITH_H_
