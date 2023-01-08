
/****************************************************************
 * Copyright (c) 2022, liyinbin
 * All rights reserved.
 * Author by liyinbin (jeff.li) lijippy@163.com
 *****************************************************************/

#ifndef FLARE_BASE_STRING_CONTAIN_H_
#define FLARE_BASE_STRING_CONTAIN_H_

#include <string_view>
#include "flare/base/profile.h"

namespace flare {

//! Tests of string contains pattern

FLARE_FORCE_INLINE bool string_contains(std::string_view str, std::string_view match) {
    return str.find(match, 0) != str.npos;
}

}  // namespace flare

#endif  // FLARE_BASE_STRING_CONTAIN_H_
