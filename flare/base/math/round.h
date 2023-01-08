
/****************************************************************
 * Copyright (c) 2022, liyinbin
 * All rights reserved.
 * Author by liyinbin (jeff.li) lijippy@163.com
 *****************************************************************/

#ifndef FLARE_BASE_MATH_ROUND_H_
#define FLARE_BASE_MATH_ROUND_H_

#include <cmath>
#include "flare/base/profile.h"

namespace flare::base {

    /// Can't use std::round() because it is only available in C++11.
    /// Note that we ignore the possibility of floating-point over/underflow.
    template<typename Double>
    FLARE_FORCE_INLINE double round(Double d) {
        return d < 0 ? std::ceil(d - 0.5) : std::floor(d + 0.5);
    }
}  // namespace flare::base

#endif  // FLARE_BASE_MATH_ROUND_H_
