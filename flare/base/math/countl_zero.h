
/****************************************************************
 * Copyright (c) 2022, liyinbin
 * All rights reserved.
 * Author by liyinbin (jeff.li) lijippy@163.com
 *****************************************************************/

#ifndef FLARE_BASE_MATH_COUNTL_ZERO_H_
#define FLARE_BASE_MATH_COUNTL_ZERO_H_

#include "flare/base/profile.h"

namespace flare::base {

    template<typename Integral>
    FLARE_FORCE_INLINE unsigned countl_zero_template(Integral x) {
        if (x == 0)
            return 8 * sizeof(x);
        unsigned r = 0;
        while ((x & (static_cast<Integral>(1) << (8 * sizeof(x) - 1))) == 0)
            x <<= 1, ++r;
        return r;
    }

    template<typename Integral>
    FLARE_FORCE_INLINE unsigned clz_non_template(Integral x) {
        auto lz = countl_zero_template(x);
        return sizeof(x) * 8 - 1 - lz;
    }


    template<typename Integral>
    FLARE_FORCE_INLINE unsigned countl_zero(Integral x);

    template<typename Integral>
    FLARE_FORCE_INLINE unsigned leading_set_bit(Integral x);

#if defined(FLARE_COMPILER_GNUC) || defined(FLARE_COMPILER_CLANG)

    template<>
    FLARE_FORCE_INLINE unsigned countl_zero<unsigned>(unsigned x) {
        if (x == 0)
            return 8 * sizeof(x);
        return static_cast<unsigned>(__builtin_clz(x));
    }

    template<>
    FLARE_FORCE_INLINE unsigned leading_set_bit<unsigned>(unsigned x) {
        return sizeof(unsigned) * 8 - countl_zero(x);
    }

    /// countl_zero (count leading zeros)
    template<>
    FLARE_FORCE_INLINE unsigned countl_zero<int>(int i) {
        return countl_zero(static_cast<unsigned>(i));
    }

    template<>
    FLARE_FORCE_INLINE unsigned leading_set_bit<int>(int x) {
        return sizeof(int) * 8 - countl_zero(x);
    }

    /// countl_zero (count leading zeros)
    template<>
    FLARE_FORCE_INLINE unsigned countl_zero<unsigned long>(unsigned long i) {
        if (i == 0)
            return 8 * sizeof(i);
        return static_cast<unsigned>(__builtin_clzl(i));
    }

    template<>
    FLARE_FORCE_INLINE unsigned leading_set_bit<unsigned long>(unsigned long x) {
        return sizeof(unsigned long) * 8 - countl_zero(x);
    }

    /// countl_zero (count leading zeros)
    template<>
    FLARE_FORCE_INLINE unsigned countl_zero<long>(long i) {
        return countl_zero(static_cast<unsigned long>(i));
    }

    template<>
    FLARE_FORCE_INLINE unsigned leading_set_bit<long>(long x) {
        return sizeof(long) * 8 - countl_zero(x);
    }

    /// countl_zero (count leading zeros)
    template<>
    FLARE_FORCE_INLINE unsigned countl_zero<unsigned long long>(unsigned long long i) {
        if (i == 0)
            return 8 * sizeof(i);
        return static_cast<unsigned>(__builtin_clzll(i));
    }

    template<>
    FLARE_FORCE_INLINE unsigned leading_set_bit<unsigned long long>(unsigned long long x) {
        return sizeof(unsigned long long) * 8 - countl_zero(x);
    }

    /// countl_zero (count leading zeros)
    template<>
    FLARE_FORCE_INLINE unsigned countl_zero<long long>(long long i) {
        return countl_zero(static_cast<unsigned long long>(i));
    }

    template<>
    FLARE_FORCE_INLINE unsigned leading_set_bit<long long>(long long x) {
        return sizeof(long long) * 8 - countl_zero(x);
    }

#else

    /// countl_zero (count leading zeros)
    template<>
    FLARE_FORCE_INLINE unsigned countl_zero<int> (int i) {
        return countl_zero_template(i);
    }

    template <>
    FLARE_FORCE_INLINE unsigned leading_set_bit<int>(int x) {
        return sizeof(int) * 8 - countl_zero(x);
    }

    /// countl_zero (count leading zeros)
    template<>
    FLARE_FORCE_INLINE unsigned countl_zero<unsigned> (unsigned i) {
        return countl_zero_template(i);
    }

    template <>
    FLARE_FORCE_INLINE unsigned leading_set_bit<unsigned>(unsigned x) {
        return sizeof(unsigned) * 8 - countl_zero(x);
    }

    /// countl_zero (count leading zeros)
    template<>
    FLARE_FORCE_INLINE unsigned countl_zero<long> (long i) {
        return countl_zero_template(i);
    }

    template <>
    FLARE_FORCE_INLINE unsigned leading_set_bit<long>(long x) {
        return sizeof(long) * 8 - countl_zero(x);
    }

    /// countl_zero (count leading zeros)
    template<>
    FLARE_FORCE_INLINE unsigned countl_zero<unsigned long> (unsigned long i) {
        return countl_zero_template(i);
    }

    template <>
    FLARE_FORCE_INLINE unsigned leading_set_bit<unsigned long>(unsigned long x) {
        return sizeof(unsigned long) * 8 - countl_zero(x);
    }

    /// countl_zero (count leading zeros)
    template<>
    FLARE_FORCE_INLINE unsigned countl_zero<long long> (long long i) {
        return countl_zero_template(i);
    }

    template <>
    FLARE_FORCE_INLINE unsigned leading_set_bit<long long>(long long x) {
        return sizeof(long long) * 8 - countl_zero(x);
    }

    /// countl_zero (count leading zeros)
    template<>
    FLARE_FORCE_INLINE unsigned countl_zero<unsigned long long> (unsigned long long i) {
        return countl_zero_template(i);
    }

    template <>
    FLARE_FORCE_INLINE unsigned leading_set_bit<unsigned long long>(unsigned long long x) {
        return sizeof(unsigned long long) * 8 - countl_zero(x);
    }

#endif

    template<typename Integral>
    FLARE_FORCE_INLINE unsigned count_leading_non_zeros(Integral x) {
        auto lz = countl_zero(x);
        return sizeof(x) * 8 - 1 - lz;
    }


}  // namespace flare::base

#endif  // FLARE_BASE_MATH_COUNTL_ZERO_H_
