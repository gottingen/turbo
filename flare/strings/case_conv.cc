
/****************************************************************
 * Copyright (c) 2022, liyinbin
 * All rights reserved.
 * Author by liyinbin (jeff.li) lijippy@163.com
 *****************************************************************/
#include "flare/strings/case_conv.h"

#include <algorithm>
#include "flare/strings/ascii.h"

namespace flare {

std::string &string_to_lower(std::string *str) {
    std::transform(str->begin(), str->end(), str->begin(),
                   [](char c) { return ascii::to_lower(c); });
    return *str;
}

std::string &string_to_upper(std::string *str) {
    std::transform(str->begin(), str->end(), str->begin(),
                   [](char c) { return ascii::to_upper(c); });
    return *str;
}

}
