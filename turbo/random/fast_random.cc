// Copyright 2023 The Elastic-AI Authors.
// part of Elastic AI Search
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//      https://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.
//

#include "turbo/random/fast_random.h"
#include "turbo/times/clock.h"

namespace turbo {

    typedef uint64_t SplitMix64Seed;

    inline uint64_t splitmix64_next(SplitMix64Seed* seed) {
        uint64_t z = (*seed += UINT64_C(0x9E3779B97F4A7C15));
        z = (z ^ (z >> 30)) * UINT64_C(0xBF58476D1CE4E5B9);
        z = (z ^ (z >> 27)) * UINT64_C(0x94D049BB133111EB);
        return z ^ (z >> 31);
    }

    inline uint64_t xorshift128_next(FastRandSeed* seed) {
        uint64_t s1 = seed->s[0];
        const uint64_t s0 = seed->s[1];
        seed->s[0] = s0;
        s1 ^= s1 << 23; // a
        seed->s[1] = s1 ^ s0 ^ (s1 >> 18) ^ (s0 >> 5); // b, c
        return seed->s[1] + s0;
    }


    inline uint64_t fast_rand_impl(uint64_t range, FastRandSeed* seed) {
        // Separating uint64_t values into following intervals:
        //   [0,range-1][range,range*2-1] ... [uint64_max/range*range,uint64_max]
        // If the generated 64-bit random value falls into any interval except the
        // last one, the probability of taking any value inside [0, range-1] is
        // same. If the value falls into last interval, we retry the process until
        // the value falls into other intervals. If min/max are limited to 32-bits,
        // the retrying is rare. The amortized retrying count at maximum is 1 when
        // range equals 2^32. A corner case is that even if the range is power of
        // 2(e.g. min=0 max=65535) in which case the retrying can be avoided, we
        // still retry currently. The reason is just to keep the code simpler
        // and faster for most cases.
        const uint64_t div = std::numeric_limits<uint64_t>::max() / range;
        uint64_t result;
        do {
            result = xorshift128_next(seed) / div;
        } while (result >= range);
        return result;
    }

    FastRandom::FastRandom() {
        SplitMix64Seed seed4seed = turbo::ToUniversal(turbo::Now());
        _seed.s[0] = splitmix64_next(&seed4seed);
        _seed.s[1] = splitmix64_next(&seed4seed);
    }

    int64_t FastRandom::generate() {
        return xorshift128_next(&_seed);
    }

    int64_t FastRandom::generate(uint64_t range) {
        if (range == 0) {
            return 0;
        }
        return fast_rand_impl(range, &_seed);
    }

    int64_t FastRandom::generate_64(int64_t lo, int64_t hi) {
        if (lo >= hi) {
            if (lo == hi) {
                return lo;
            }
            const int64_t tmp = lo;
            lo = hi;
            hi = tmp;
        }
        int64_t range = hi - lo + 1;
        if (range == 0) {
            // max = INT64_MAX, min = INT64_MIN
            return (int64_t)xorshift128_next(&_seed);
        }
        return lo + (int64_t)fast_rand_impl(hi - lo + 1, &_seed);
    }

    uint64_t FastRandom::generate_u64(uint64_t lo, uint64_t hi) {
        if (lo >= hi) {
            if (lo == hi) {
                return lo;
            }
            const uint64_t tmp = lo;
            lo = hi;
            hi = tmp;
        }
        uint64_t range = hi - lo + 1;
        if (range == 0) {
            // max = UINT64_MAX, min = UINT64_MIN
            return xorshift128_next(&_seed);
        }
        return lo + fast_rand_impl(range, &_seed);
    }

    inline double FastRandom::generate_double() {
        // Copied from rand_util.cc
        static_assert(std::numeric_limits<double>::radix == 2, "otherwise use scalbn");
        static const int kBits = std::numeric_limits<double>::digits;
        uint64_t random_bits = xorshift128_next(&_seed) & ((UINT64_C(1) << kBits) - 1);
        double result = ldexp(static_cast<double>(random_bits), -1 * kBits);
        return result;
    }
}