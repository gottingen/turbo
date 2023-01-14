
/****************************************************************
 * Copyright (c) 2022, liyinbin
 * All rights reserved.
 * Author by liyinbin (jeff.li) lijippy@163.com
 *****************************************************************/

#ifndef TURBO_BASE_MATH_INFINITY_H_
#define TURBO_BASE_MATH_INFINITY_H_

#include <limits>
#include <cmath>
#include "turbo/base/profile.h"

namespace turbo::base {

    /// Can't use std::isinfinite() because it doesn't exist on windows.
    TURBO_FORCE_INLINE bool is_finite(double d) {
        if (std::isnan(d)) return false;
        return d != std::numeric_limits<double>::infinity() &&
               d != -std::numeric_limits<double>::infinity();
    }

}  // namespace turbo::base

#endif  // TURBO_BASE_MATH_INFINITY_H_
