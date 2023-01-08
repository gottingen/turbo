
/****************************************************************
 * Copyright (c) 2022, liyinbin
 * All rights reserved.
 * Author by liyinbin (jeff.li) lijippy@163.com
 *****************************************************************/

#ifndef FLARE_BASE_MATH_POW_TRAITS_H_
#define FLARE_BASE_MATH_POW_TRAITS_H_

#include "flare/base/profile.h"


namespace flare::base {

    template<typename Integral>
    constexpr bool is_power_of_two_template(Integral i) {
        return (i < 0 ? false : i == 0 ?
                                true : !(i & (i - 1)) ?
                                       true : false);
    }

    /// is_power_of_two()
    /// does what it says: true if i is a power of two
    constexpr bool is_power_of_two(int i) {
        return is_power_of_two_template(i);
    }

    /// does what it says: true if i is a power of two
    constexpr bool is_power_of_two(unsigned int i) {
        return is_power_of_two_template(i);
    }

    /// does what it says: true if i is a power of two
    constexpr bool is_power_of_two(long i) {
        return is_power_of_two_template(i);
    }

    /// does what it says: true if i is a power of two
    constexpr bool is_power_of_two(unsigned long i) {
        return is_power_of_two_template(i);
    }

    /// does what it says: true if i is a power of two
    constexpr bool is_power_of_two(long long i) {
        return is_power_of_two_template(i);
    }

    /// does what it says: true if i is a power of two
    constexpr bool is_power_of_two(unsigned long long i) {
        return is_power_of_two_template(i);
    }
}
#endif  // FLARE_BASE_MATH_POW_TRAITS_H_
