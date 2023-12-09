// Copyright 2020 The Turbo Authors.
//
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
// -----------------------------------------------------------------------------
// File: random.h
// -----------------------------------------------------------------------------
//
// This header defines the recommended Uniform Random Bit Generator (URBG)
// types for use within the Turbo Random library. These types are not
// suitable for security-related use-cases, but should suffice for most other
// uses of generating random values.
//
// The Turbo random library provides the following URBG types:
//
//   * BitGen, a good general-purpose bit generator, optimized for generating
//     random (but not cryptographically secure) values
//   * InsecureBitGen, a slightly faster, though less random, bit generator, for
//     cases where the existing BitGen is a drag on performance.

#ifndef TURBO_RANDOM_RANDOM_H_
#define TURBO_RANDOM_RANDOM_H_

#include <random>
#include <numeric>
#include "turbo/random/engine.h"
#include "turbo/random/fast_random.h"
#include "turbo/random/uniform.h"
#include "turbo/random/bernoulli.h"
#include "turbo/random/beta.h"
#include "turbo/random/exponential.h"
#include "turbo/random/gaussian.h"
#include "turbo/random/log_uniform.h"
#include "turbo/random/poisson.h"
#include "turbo/random/zipf.h"
#include "turbo/meta/type_traits.h"
#include <vector>

namespace turbo {

    inline uint64_t fast_random() {
        return FastRandom::get_thread_instance()->generate();
    }

    inline uint64_t fast_random(uint64_t range) {
        return FastRandom::get_thread_instance()->generate(range);
    }

    inline uint64_t fast_random(uint64_t lo, uint64_t hi) {
        return FastRandom::get_thread_instance()->generate_64(lo, hi);
    }

    inline uint64_t fast_random_u64(uint64_t lo, uint64_t hi) {
        return FastRandom::get_thread_instance()->generate_u64(lo, hi);
    }

    template<class T = int64_t, std::enable_if_t<std::is_integral_v<T> ||std::is_floating_point_v<T>, int>>
    inline T fast_random(int64_t lo, int64_t hi) {
        return FastRandom::get_thread_instance()->generate<T>(lo, hi);
    }

    inline double fast_random_double() {
        return FastRandom::get_thread_instance()->generate_double();
    }

    void fast_random_bytes(void *output, size_t output_length);

    std::string fast_random_printable(size_t length);


    template<typename T>
    class UniformRandom {
    public:
        template<check_requires<std::is_integral<T>>>
        UniformRandom(): _gen(), _hi(std::numeric_limits<T>::max()), _lo(std::numeric_limits<T>::min()) {}

        UniformRandom(T hi, T lo) : _gen(), _hi(hi), _lo(lo) {}

        UniformRandom(BitGen gen, T hi, T lo) : _gen(gen), _hi(hi), _lo(lo) {}

        T generate() {
            return uniform(_gen, _lo, _hi);
        }

    private:
        BitGen _gen;
        T _hi;
        T _lo;
    };

    template<typename T, check_requires<std::is_integral<T>>>
    class UniformRandomRanges {
    public:

        UniformRandomRanges(const std::vector<std::pair<T, T>> &ranges) : _gen(), _ranges(ranges) {}

        T generate() {
            auto index = uniform(IntervalClosedOpen, _gen, 0, _ranges.size());
            return uniform(_gen, _ranges[index].first, _ranges[index].second);
        }

    private:
        BitGen _gen;
        std::vector<std::pair<T, T>> _ranges;
    };
}  // namespace turbo

#endif  // TURBO_RANDOM_RANDOM_H_
