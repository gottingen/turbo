
/****************************************************************
 * Copyright (c) 2022, liyinbin
 * All rights reserved.
 * Author by liyinbin (jeff.li) lijippy@163.com
 *****************************************************************/

#ifndef TURBO_BASE_MATH_SGN_H_
#define TURBO_BASE_MATH_SGN_H_

namespace turbo::base {

    template<typename T>
    constexpr int sgn(const T x) noexcept {
        return (x > T(0) ? 1 : x < T(0) ? -1 : 0);
    }
}  // namespace turbo::base

#endif  // TURBO_BASE_MATH_SGN_H_
