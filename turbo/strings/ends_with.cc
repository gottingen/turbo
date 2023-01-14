
/****************************************************************
 * Copyright (c) 2022, liyinbin
 * All rights reserved.
 * Author by liyinbin (jeff.li) lijippy@163.com
 *****************************************************************/

#include "turbo/strings/ends_with.h"
#include "turbo/strings/compare.h"
#include "turbo/strings/ascii.h"
#include <algorithm>
#include <cstring>

namespace turbo {


    bool ends_with_ignore_case(std::string_view text, std::string_view suffix) noexcept{
        if (text.size() >= suffix.size())  {
            return turbo::compare_case(text.substr(text.size() - suffix.size()), suffix) == 0;
        }
        return false;
    }

}  // namespace turbo
