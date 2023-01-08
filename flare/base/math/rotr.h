
/****************************************************************
 * Copyright (c) 2022, liyinbin
 * All rights reserved.
 * Author by liyinbin (jeff.li) lijippy@163.com
 *****************************************************************/

#ifndef FLARE_BASE_MATH_ROTR_H_
#define FLARE_BASE_MATH_ROTR_H_

#include <cstdint>
#include <cstdlib>
#include "flare/base/profile.h"

namespace flare::base {


    /// rotr() - rotate bits right in 32-bit integers
    /// rotr - generic implementation
    static FLARE_FORCE_INLINE uint32_t rotr_generic(const uint32_t &x, int i) {
        return (x >> static_cast<uint32_t>(i & 31)) |
               (x << static_cast<uint32_t>((32 - (i & 31)) & 31));
    }

#if (defined(__GNUC__) || defined(__clang__)) && (defined(__i386__) || defined(__x86_64__))

    /// rotr - gcc/clang assembler
    static FLARE_FORCE_INLINE uint32_t rotr(const uint32_t &x, int i) {
        uint32_t x1 = x;
        asm ("rorl %%cl,%0" : "=r" (x1) : "0" (x1), "c" (i));
        return x1;
    }

#elif defined(_MSC_VER)

    /// rotr - MSVC intrinsic
    static FLARE_FORCE_INLINE uint32_t rotr(const uint32_t& x, int i) {
        return _rotr(x, i);
    }

#else

    /// rotr - generic
    static FLARE_FORCE_INLINE uint32_t rotr (const uint32_t &x, int i) {
        return rotr_generic(x, i);
    }

#endif


    /// rotr() - rotate bits right in 64-bit integers
    /// rotr - generic implementation
    static FLARE_FORCE_INLINE uint64_t rotr_generic(const uint64_t &x, int i) {
        return (x >> static_cast<uint64_t>(i & 63)) |
               (x << static_cast<uint64_t>((64 - (i & 63)) & 63));
    }

#if (defined(__GNUC__) || defined(__clang__)) && defined(__x86_64__)

    /// rotr - gcc/clang assembler
    static FLARE_FORCE_INLINE uint64_t rotr(const uint64_t &x, int i) {
        uint64_t x1 = x;
        asm ("rorq %%cl,%0" : "=r" (x1) : "0" (x1), "c" (i));
        return x1;
    }

#elif defined(_MSC_VER)

    /// rotr - MSVC intrinsic
    static FLARE_FORCE_INLINE uint64_t rotr(const uint64_t& x, int i) {
        return _rotr64(x, i);
    }

#else

    /// rotr - generic
    static FLARE_FORCE_INLINE uint64_t rotr (const uint64_t &x, int i) {
        return rotr_generic(x, i);
    }

#endif


}  // namespace flare::base

#endif  // FLARE_BASE_MATH_ROTR_H_
