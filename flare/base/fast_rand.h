
/****************************************************************
 * Copyright (c) 2022, liyinbin
 * All rights reserved.
 * Author by liyinbin (jeff.li) lijippy@163.com
 *****************************************************************/

#ifndef FLARE_BASE_FAST_RAND_H_
#define FLARE_BASE_FAST_RAND_H_

#include <cstddef>
#include <stdint.h>
#include <string>

namespace flare::base {

    // Generate random values fast without global contentions.
    // All functions in this header are thread-safe.

    struct FastRandSeed {
        uint64_t s[2];
    };

    // Initialize the seed.
    void init_fast_rand_seed(FastRandSeed *seed);

    // Generate an unsigned 64-bit random number from thread-local or given seed.
    // Cost: ~5ns
    uint64_t fast_rand();

    uint64_t fast_rand(FastRandSeed *);

    // Generate an unsigned 64-bit random number inside [0, range) from
    // thread-local seed.
    // Returns 0 when range is 0.
    // Cost: ~30ns
    // Note that this can be used as an adapter for std::random_shuffle():
    //   std::random_shuffle(myvector.begin(), myvector.end(), flare::base::fast_rand_less_than);
    uint64_t fast_rand_less_than(uint64_t range);

    // Generate a 64-bit random number inside [min, max] (inclusive!)
    // from thread-local seed.
    // NOTE: this function needs to be a template to be overloadable properly.
    // Cost: ~30ns
    template<typename T>
    T fast_rand_in(T min, T max) {
        extern int64_t fast_rand_in_64(int64_t min, int64_t max);
        extern uint64_t fast_rand_in_u64(uint64_t min, uint64_t max);
        if ((T) -1 < 0) {
            return fast_rand_in_64((int64_t) min, (int64_t) max);
        } else {
            return fast_rand_in_u64((uint64_t) min, (uint64_t) max);
        }
    }

    // Generate a random double in [0, 1) from thread-local seed.
    // Cost: ~15ns
    double fast_rand_double();

    // Fills |output_length| bytes of |output| with random data.
    void fast_rand_bytes(void *output, size_t output_length);

    // Generate a random printable string of |length| bytes
    std::string fast_rand_printable(size_t length);

}  // namespace flare::base

#endif  // FLARE_BASE_FAST_RAND_H_
