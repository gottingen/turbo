
/****************************************************************
 * Copyright (c) 2022, liyinbin
 * All rights reserved.
 * Author by liyinbin (jeff.li) lijippy@163.com
 *****************************************************************/

#ifndef FLARE_BASE_MATH_POPCOUNT_H_
#define FLARE_BASE_MATH_POPCOUNT_H_

#include <cstdint>
#include <cstdlib>
#include "flare/base/profile.h"

namespace flare::base {


    /// popcount() - count one bits

    /// popcount (count one bits) - generic SWAR implementation
    static FLARE_FORCE_INLINE unsigned popcount_generic8(uint8_t x) {
        x = x - ((x >> 1) & 0x55);
        x = (x & 0x33) + ((x >> 2) & 0x33);
        return static_cast<uint8_t>((x + (x >> 4)) & 0x0F);
    }

    /// popcount (count one bits) - generic SWAR implementation
    static FLARE_FORCE_INLINE unsigned popcount_generic16(uint16_t x) {
        x = x - ((x >> 1) & 0x5555);
        x = (x & 0x3333) + ((x >> 2) & 0x3333);
        return static_cast<uint16_t>(((x + (x >> 4)) & 0x0F0F) * 0x0101) >> 8;
    }

    /// popcount (count one bits) -
    /// generic SWAR implementation from https://stackoverflow.com/questions/109023
    static FLARE_FORCE_INLINE unsigned popcount_generic32(uint32_t x) {
        x = x - ((x >> 1) & 0x55555555);
        x = (x & 0x33333333) + ((x >> 2) & 0x33333333);
        return (((x + (x >> 4)) & 0x0F0F0F0F) * 0x01010101) >> 24;
    }

    /// popcount (count one bits) - generic SWAR implementation
    static FLARE_FORCE_INLINE unsigned popcount_generic64(uint64_t x) {
        x = x - ((x >> 1) & 0x5555555555555555);
        x = (x & 0x3333333333333333) + ((x >> 2) & 0x3333333333333333);
        return (((x + (x >> 4)) & 0x0F0F0F0F0F0F0F0F) * 0x0101010101010101) >> 56;
    }

#if defined(FLARE_COMPILER_CLANG) || defined(FLARE_COMPILER_GNUC)

    /// popcount (count one bits)
    static FLARE_FORCE_INLINE unsigned popcount(unsigned i) {
        return static_cast<unsigned>(__builtin_popcount(i));
    }

    /// popcount (count one bits)
    static FLARE_FORCE_INLINE unsigned popcount(int i) {
        return popcount(static_cast<unsigned>(i));
    }

    /// popcount (count one bits)
    static FLARE_FORCE_INLINE unsigned popcount(unsigned long i) {
        return static_cast<unsigned>(__builtin_popcountl(i));
    }

    /// popcount (count one bits)
    static FLARE_FORCE_INLINE unsigned popcount(long i) {
        return popcount(static_cast<unsigned long>(i));
    }

    /// popcount (count one bits)
    static FLARE_FORCE_INLINE unsigned popcount(unsigned long long i) {
        return static_cast<unsigned>(__builtin_popcountll(i));
    }

    /// popcount (count one bits)
    static FLARE_FORCE_INLINE unsigned popcount(long long i) {
        return popcount(static_cast<unsigned long long>(i));
    }

#else

    /// popcount (count one bits)
    template<typename Integral>
    FLARE_FORCE_INLINE unsigned popcount (Integral i) {
        if (sizeof(i) <= sizeof(uint8_t))
            return popcount_generic8(i);
        else if (sizeof(i) <= sizeof(uint16_t))
            return popcount_generic16(i);
        else if (sizeof(i) <= sizeof(uint32_t))
            return popcount_generic32(i);
        else if (sizeof(i) <= sizeof(uint64_t))
            return popcount_generic64(i);
        else
            abort();
    }

#endif

    /// popcount range
    static FLARE_FORCE_INLINE
    size_t popcount(const void *data, size_t size) {
        const uint8_t *begin = reinterpret_cast<const uint8_t *>(data);
        const uint8_t *end = begin + size;
        size_t total = 0;
        while (begin + 7 < end) {
            total += popcount(*reinterpret_cast<const uint64_t *>(begin));
            begin += 8;
        }
        if (begin + 3 < end) {
            total += popcount(*reinterpret_cast<const uint32_t *>(begin));
            begin += 4;
        }
        while (begin < end) {
            total += popcount(*begin++);
        }
        return total;
    }


}  // namespace flare::base

#endif  // FLARE_BASE_MATH_POPCOUNT_H_
