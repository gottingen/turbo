
/****************************************************************
 * Copyright (c) 2022, liyinbin
 * All rights reserved.
 * Author by liyinbin (jeff.li) lijippy@163.com
 *****************************************************************/

#ifndef TURBO_BASE_STRING_CONTAIN_H_
#define TURBO_BASE_STRING_CONTAIN_H_

#include <string_view>
#include "turbo/base/profile.h"

namespace turbo {

//! Tests of string contains pattern

TURBO_FORCE_INLINE bool string_contains(std::string_view str, std::string_view match) {
    return str.find(match, 0) != str.npos;
}

}  // namespace turbo

#endif  // TURBO_BASE_STRING_CONTAIN_H_
