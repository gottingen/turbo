
/****************************************************************
 * Copyright (c) 2022, liyinbin
 * All rights reserved.
 * Author by liyinbin (jeff.li) lijippy@163.com
 *****************************************************************/

#ifndef FLARE_BASE_MATH_ABS_DIFF_H_
#define FLARE_BASE_MATH_ABS_DIFF_H_


#include <vector>
#include <cstddef>
#include "flare/base/profile.h"

namespace flare::base {

    /**
     * @brief todo
     * @tparam T todo
     * @param a todo
     * @param b todo
     * @return todo
     */
    template<typename T>
    FLARE_FORCE_INLINE T abs_diff(const T &a, const T &b) {
        return a > b ? a - b : b - a;
    }

    template<typename T>
    FLARE_FORCE_INLINE T
    sum_abs_diff(const std::vector<T> &X, const std::vector<T> &Y) {
        T val_out = T(0);
        size_t n_elem = X.size();

        for (size_t i = 0; i < n_elem; ++i) {
            val_out += abs(X[i] - Y[i]);
        }

        return val_out;
    }

}  // namespace flare::base

#endif  // FLARE_BASE_MATH_ABS_DIFF_H_
