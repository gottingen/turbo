
/****************************************************************
 * Copyright (c) 2022, liyinbin
 * All rights reserved.
 * Author by liyinbin (jeff.li) lijippy@163.com
 *****************************************************************/

#ifndef FLARE_BASE_MATH_FFS_H_
#define FLARE_BASE_MATH_FFS_H_

#include "flare/base/profile.h"

namespace flare::base {

    /// ffs (find first set bit) - generic implementation
    template<typename Integral>
    static FLARE_FORCE_INLINE unsigned ffs_template(Integral x) {
        if (x == 0)
            return 0u;
        unsigned r = 1;
        while ((x & 1) == 0)
            x >>= 1, ++r;
        return r;
    }

#if defined(FLARE_COMPILER_GNUC) || defined(FLARE_COMPILER_CLANG)

    /// find first set bit in integer, or zero if none are set.
    static FLARE_FORCE_INLINE
    unsigned ffs(int i) {
        return static_cast<unsigned>(__builtin_ffs(i));
    }

    /// find first set bit in integer, or zero if none are set.
    static FLARE_FORCE_INLINE
    unsigned ffs(unsigned i) {
        return ffs(static_cast<int>(i));
    }

    /// find first set bit in integer, or zero if none are set.
    static FLARE_FORCE_INLINE
    unsigned ffs(long i) {
        return static_cast<unsigned>(__builtin_ffsl(i));
    }

    /// find first set bit in integer, or zero if none are set.
    static FLARE_FORCE_INLINE
    unsigned ffs(unsigned long i) {
        return ffs(static_cast<long>(i));
    }

    /// find first set bit in integer, or zero if none are set.
    static FLARE_FORCE_INLINE
    unsigned ffs(long long i) {
        return static_cast<unsigned>(__builtin_ffsll(i));
    }

    /// find first set bit in integer, or zero if none are set.
    static FLARE_FORCE_INLINE
    unsigned ffs(unsigned long long i) {
        return ffs(static_cast<long long>(i));
    }

#else

    /// find first set bit in integer, or zero if none are set.
    static FLARE_FORCE_INLINE
    unsigned ffs (int i) {
        return ffs_template(i);
    }

    /// find first set bit in integer, or zero if none are set.
    static FLARE_FORCE_INLINE
    unsigned ffs (unsigned int i) {
        return ffs_template(i);
    }

    /// find first set bit in integer, or zero if none are set.
    static FLARE_FORCE_INLINE
    unsigned ffs (long i) {
        return ffs_template(i);
    }

    /// find first set bit in integer, or zero if none are set.
    static FLARE_FORCE_INLINE
    unsigned ffs (unsigned long i) {
        return ffs_template(i);
    }

    /// find first set bit in integer, or zero if none are set.
    static FLARE_FORCE_INLINE
    unsigned ffs (long long i) {
        return ffs_template(i);
    }

    /// find first set bit in integer, or zero if none are set.
    static FLARE_FORCE_INLINE
    unsigned ffs (unsigned long long i) {
        return ffs_template(i);
    }

#endif
}  // namespace flare::base

#endif  // FLARE_BASE_MATH_FFS_H_
