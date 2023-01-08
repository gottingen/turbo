
/****************************************************************
 * Copyright (c) 2022, liyinbin
 * All rights reserved.
 * Author by liyinbin (jeff.li) lijippy@163.com
 *****************************************************************/
#ifndef FLARE_STRINGS_CASE_CONV_H_
#define FLARE_STRINGS_CASE_CONV_H_

#include <string>
#include <string_view>
#include "flare/base/profile.h"

namespace flare {

    std::string &string_to_lower(std::string *str);

    FLARE_MUST_USE_RESULT FLARE_FORCE_INLINE
    std::string string_to_lower(std::string_view str) {
        std::string result(str);
        flare::string_to_lower(&result);
        return result;
    }

    std::string &string_to_upper(std::string *str);

    FLARE_MUST_USE_RESULT FLARE_FORCE_INLINE
    std::string string_to_upper(std::string_view str) {
        std::string result(str);
        flare::string_to_upper(&result);
        return result;
    }

}
#endif  // FLARE_STRINGS_CASE_CONV_H_
