
/****************************************************************
 * Copyright (c) 2022, liyinbin
 * All rights reserved.
 * Author by liyinbin (jeff.li) lijippy@163.com
 *****************************************************************///
// https://code.google.com/p/cityhash/
//
// This file provides a few functions for hashing strings.  All of them are
// high-quality functions in the sense that they pass standard tests such
// as Austin Appleby's SMHasher.  They are also fast.
//
// For 64-bit x86 code, on short strings, we don't know of anything faster than
// city_hash64 that is of comparable quality.  We believe our nearest competitor
// is Murmur3.  For 64-bit x86 code, city_hash64 is an excellent choice for hash
// tables and most other hashing (excluding cryptography).
//
// For 32-bit x86 code, we don't know of anything faster than city_hash32 that
// is of comparable quality.  We believe our nearest competitor is Murmur3A.
// (On 64-bit CPUs, it is typically faster to use the other CityHash variants.)
//
// Functions in the CityHash family are not suitable for cryptography.
//
// Please see CityHash's README file for more details on our performance
// measurements and so on.
//
// WARNING: This code has been only lightly tested on big-endian platforms!
// It is known to work well on little-endian platforms that have a small penalty
// for unaligned reads, such as current Intel and AMD moderate-to-high-end CPUs.
// It should work on all 32-bit and 64-bit platforms that allow unaligned reads;
// bug reports are welcome.
//
// By the way, for some hash functions, given strings a and b, the hash
// of a+b is easily derived from the hashes of a and b.  This property
// doesn't hold for any hash functions in this file.

#ifndef FLARE_HASH_INTERNAL_CITY_H_
#define FLARE_HASH_INTERNAL_CITY_H_

#include <stdint.h>
#include <cstdlib>  // for size_t.
#include <utility>
#include "flare/base/profile.h"


namespace flare::hash_internal {

    typedef std::pair<uint64_t, uint64_t> uint128;

    FLARE_FORCE_INLINE uint64_t uint128_low64(const uint128 &x) { return x.first; }

    FLARE_FORCE_INLINE uint64_t uint128_high64(const uint128 &x) { return x.second; }

    // Hash function for a byte array.
    uint64_t city_hash64(const char *s, size_t len);

    // Hash function for a byte array.  For convenience, a 64-bit seed is also
    // hashed into the result.
    uint64_t city_hash64_with_seed(const char *s, size_t len, uint64_t seed);

    // Hash function for a byte array.  For convenience, two seeds are also
    // hashed into the result.
    uint64_t city_hash64_with_seeds(const char *s, size_t len, uint64_t seed0,
                                    uint64_t seed1);

    // Hash function for a byte array.  Most useful in 32-bit binaries.
    uint32_t city_hash32(const char *s, size_t len);

    // Hash 128 input bits down to 64 bits of output.
    // This is intended to be a reasonably good hash function.
    FLARE_FORCE_INLINE uint64_t hash128_to_64(const uint128 &x) {
        // Murmur-inspired hashing.
        const uint64_t kMul = 0x9ddfea08eb382d69ULL;
        uint64_t a = (uint128_low64(x) ^ uint128_high64(x)) * kMul;
        a ^= (a >> 47);
        uint64_t b = (uint128_high64(x) ^ a) * kMul;
        b ^= (b >> 47);
        b *= kMul;
        return b;
    }

}  // namespace flare::hash_internal


#endif  // FLARE_HASH_INTERNAL_CITY_H_
