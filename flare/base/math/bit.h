
/****************************************************************
 * Copyright (c) 2022, liyinbin
 * All rights reserved.
 * Author by liyinbin (jeff.li) lijippy@163.com
 *****************************************************************/

#ifndef FLARE_BASE_MATH_BIT_H_
#define FLARE_BASE_MATH_BIT_H_
#include "flare/base/profile.h"

namespace flare::base {

    template<typename T>
    constexpr bool has_single_bit(const T number) {
        return ((number != 0) && !(number & (number - 1)));
    }

    template<typename T>
    FLARE_FORCE_INLINE T bit_width(T number) {
        if (number == 0) return 0;

        T result = 1;
        T shifted_number = number;
        while (shifted_number >>= 1) {
            ++result;
        }
        return result;
    }

}  // namespace flare::base

#endif  // FLARE_BASE_MATH_BIT_H_
