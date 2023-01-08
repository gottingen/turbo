
/****************************************************************
 * Copyright (c) 2022, liyinbin
 * All rights reserved.
 * Author by liyinbin (jeff.li) lijippy@163.com
 *****************************************************************/

#ifndef FLARE_BASE_MATH_IS_EVEN_H_
#define FLARE_BASE_MATH_IS_EVEN_H_

#include "flare/base/math/is_odd.h"

namespace flare::base {


constexpr bool is_even(const long long x) noexcept {
    return !is_odd(x);
}

}  // namespace flare::base

#endif  // FLARE_BASE_MATH_IS_EVEN_H_
