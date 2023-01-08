
/****************************************************************
 * Copyright (c) 2022, liyinbin
 * All rights reserved.
 * Author by liyinbin (jeff.li) lijippy@163.com
 *****************************************************************/
#include "flare/strings/starts_with.h"
#include "flare/strings/compare.h"
#include <algorithm>

namespace flare {

bool starts_with_case(std::string_view text, std::string_view suffix) {
    if (text.size() >= suffix.size()) {
        return flare::compare_case(text.substr(0, suffix.size()), suffix) == 0;
    }
    return false;
}

}  // namespace flare
