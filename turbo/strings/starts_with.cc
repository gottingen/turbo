
/****************************************************************
 * Copyright (c) 2022, liyinbin
 * All rights reserved.
 * Author by liyinbin (jeff.li) lijippy@163.com
 *****************************************************************/
#include "turbo/strings/starts_with.h"
#include "turbo/strings/compare.h"
#include <algorithm>

namespace turbo {

bool starts_with_case(std::string_view text, std::string_view suffix) {
    if (text.size() >= suffix.size()) {
        return turbo::compare_case(text.substr(0, suffix.size()), suffix) == 0;
    }
    return false;
}

}  // namespace turbo
