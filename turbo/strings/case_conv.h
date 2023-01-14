
/****************************************************************
 * Copyright (c) 2022, liyinbin
 * All rights reserved.
 * Author by liyinbin (jeff.li) lijippy@163.com
 *****************************************************************/
#ifndef TURBO_STRINGS_CASE_CONV_H_
#define TURBO_STRINGS_CASE_CONV_H_

#include <string>
#include <string_view>
#include "turbo/base/profile.h"

namespace turbo {

    std::string &string_to_lower(std::string *str);

    TURBO_MUST_USE_RESULT TURBO_FORCE_INLINE
    std::string string_to_lower(std::string_view str) {
        std::string result(str);
        turbo::string_to_lower(&result);
        return result;
    }

    std::string &string_to_upper(std::string *str);

    TURBO_MUST_USE_RESULT TURBO_FORCE_INLINE
    std::string string_to_upper(std::string_view str) {
        std::string result(str);
        turbo::string_to_upper(&result);
        return result;
    }

}
#endif  // TURBO_STRINGS_CASE_CONV_H_
