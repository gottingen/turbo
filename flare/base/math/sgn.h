
/****************************************************************
 * Copyright (c) 2022, liyinbin
 * All rights reserved.
 * Author by liyinbin (jeff.li) lijippy@163.com
 *****************************************************************/

#ifndef FLARE_BASE_MATH_SGN_H_
#define FLARE_BASE_MATH_SGN_H_

namespace flare::base {

    template<typename T>
    constexpr int sgn(const T x) noexcept {
        return (x > T(0) ? 1 : x < T(0) ? -1 : 0);
    }
}  // namespace flare::base

#endif  // FLARE_BASE_MATH_SGN_H_
