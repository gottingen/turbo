
/****************************************************************
 * Copyright (c) 2022, liyinbin
 * All rights reserved.
 * Author by liyinbin (jeff.li) lijippy@163.com
 *****************************************************************/
#ifndef TURBO_BASE_MATH_BIT_SWAP_H_
#define TURBO_BASE_MATH_BIT_SWAP_H_

#include <cstdint>
#include <cstdlib>
#include "turbo/base/profile.h"

namespace turbo::base {

    TURBO_FORCE_INLINE uint16_t bit_swap16_generic(const uint16_t &x) {
        return ((x >> 8) & 0x00FFUL) | ((x << 8) & 0xFF00UL);
    }

#if defined(TURBO_COMPILER_GNUC) || defined(TURBO_COMPILER_CLANG)

    TURBO_FORCE_INLINE uint16_t bit_swap16(const uint16_t &v) {
        return __builtin_bswap16(v);
    }

#else

    TURBO_FORCE_INLINE uint16_t bit_swap16 (const uint16_t &v) {
        return bit_swap16_generic(v);
    }

#endif

    TURBO_FORCE_INLINE uint32_t bit_swap32_generic(const uint32_t &x) {
        return ((x >> 24) & 0x000000FFUL) | ((x << 24) & 0xFF000000UL) |
               ((x >> 8) & 0x0000FF00UL) | ((x << 8) & 0x00FF0000UL);
    }

#if defined(TURBO_COMPILER_GNUC) || defined(TURBO_COMPILER_CLANG)

    TURBO_FORCE_INLINE uint32_t bit_swap32(const uint32_t &v) {
        return __builtin_bswap32(v);
    }

#else

    TURBO_FORCE_INLINE uint32_t bit_swap32 (const uint32_t &v) {
        return bit_swap32_generic(v);
    }

#endif

    constexpr uint64_t bit_swap64_generic(const uint64_t &x) {
        return ((x >> 56) & 0x00000000000000FFull) |
               ((x >> 40) & 0x000000000000FF00ull) |
               ((x >> 24) & 0x0000000000FF0000ull) |
               ((x >> 8) & 0x00000000FF000000ull) |
               ((x << 8) & 0x000000FF00000000ull) |
               ((x << 24) & 0x0000FF0000000000ull) |
               ((x << 40) & 0x00FF000000000000ull) |
               ((x << 56) & 0xFF00000000000000ull);
    }

#if defined(TURBO_COMPILER_GNUC) || defined(TURBO_COMPILER_CLANG)

    TURBO_FORCE_INLINE uint64_t bit_swap64(const uint64_t &v) {
        return __builtin_bswap64(v);
    }

#else

    TURBO_FORCE_INLINE uint64_t bit_swap64(const uint64_t& v) {
        return bit_swap64_generic(v);
    }
#endif

}  // namespace turbo::base

#endif  // TURBO_BASE_MATH_BIT_SWAP_H_
