
/****************************************************************
 * Copyright (c) 2022, liyinbin
 * All rights reserved.
 * Author by liyinbin (jeff.li) lijippy@163.com
 *****************************************************************/

#ifndef TURBO_BASE_MATH_IS_ODD_H_
#define TURBO_BASE_MATH_IS_ODD_H_

namespace turbo::base {

    constexpr bool is_odd(const long long x) noexcept {
        return (x & 1U) != 0;
    }

}  // namespace turbo::base

#endif  // TURBO_BASE_MATH_IS_ODD_H_
