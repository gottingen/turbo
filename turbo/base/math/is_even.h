
/****************************************************************
 * Copyright (c) 2022, liyinbin
 * All rights reserved.
 * Author by liyinbin (jeff.li) lijippy@163.com
 *****************************************************************/

#ifndef TURBO_BASE_MATH_IS_EVEN_H_
#define TURBO_BASE_MATH_IS_EVEN_H_

#include "turbo/base/math/is_odd.h"

namespace turbo::base {


constexpr bool is_even(const long long x) noexcept {
    return !is_odd(x);
}

}  // namespace turbo::base

#endif  // TURBO_BASE_MATH_IS_EVEN_H_
