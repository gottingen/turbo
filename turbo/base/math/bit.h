
/****************************************************************
 * Copyright (c) 2022, liyinbin
 * All rights reserved.
 * Author by liyinbin (jeff.li) lijippy@163.com
 *****************************************************************/

#ifndef TURBO_BASE_MATH_BIT_H_
#define TURBO_BASE_MATH_BIT_H_
#include "turbo/base/profile.h"

namespace turbo::base {

    template<typename T>
    constexpr bool has_single_bit(const T number) {
        return ((number != 0) && !(number & (number - 1)));
    }

    template<typename T>
    TURBO_FORCE_INLINE T bit_width(T number) {
        if (number == 0) return 0;

        T result = 1;
        T shifted_number = number;
        while (shifted_number >>= 1) {
            ++result;
        }
        return result;
    }

}  // namespace turbo::base

#endif  // TURBO_BASE_MATH_BIT_H_
