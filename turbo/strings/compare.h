
/****************************************************************
 * Copyright (c) 2022, liyinbin
 * All rights reserved.
 * Author by liyinbin (jeff.li) lijippy@163.com
 *****************************************************************/
#ifndef TURBO_STRINGS_COMPARE_H_
#define TURBO_STRINGS_COMPARE_H_

#include <string_view>
#include "turbo/base/profile.h"

namespace turbo {

    int compare_case(std::string_view a, std::string_view b);

    TURBO_FORCE_INLINE bool equal_case(std::string_view a, std::string_view b) {
        return compare_case(a, b) == 0;
    }

}  // namespace turbo

#endif  // TURBO_STRINGS_COMPARE_H_
