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


#ifndef TURBO_RANDOM_UNIFORM_H_
#define TURBO_RANDOM_UNIFORM_H_

#include "turbo/random/fwd.h"

namespace turbo {


    // -----------------------------------------------------------------------------
    // turbo::uniform<T>(tag, bitgen, lo, hi)
    // -----------------------------------------------------------------------------
    //
    // `turbo::uniform()` produces random values of type `T` uniformly distributed in
    // a defined interval {lo, hi}. The interval `tag` defines the type of interval
    // which should be one of the following possible values:
    //
    //   * `turbo::IntervalOpenOpen`
    //   * `turbo::IntervalOpenClosed`
    //   * `turbo::IntervalClosedOpen`
    //   * `turbo::IntervalClosedClosed`
    //
    // where "open" refers to an exclusive value (excluded) from the output, while
    // "closed" refers to an inclusive value (included) from the output.
    //
    // In the absence of an explicit return type `T`, `turbo::uniform()` will deduce
    // the return type based on the provided endpoint arguments {A lo, B hi}.
    // Given these endpoints, one of {A, B} will be chosen as the return type, if
    // a type can be implicitly converted into the other in a lossless way. The
    // lack of any such implicit conversion between {A, B} will produce a
    // compile-time error
    //
    // See https://en.wikipedia.org/wiki/Uniform_distribution_(continuous)
    //
    // Example:
    //
    //   turbo::BitGen bitgen;
    //
    //   // Produce a random float value between 0.0 and 1.0, inclusive
    //   auto x = turbo::uniform(turbo::IntervalClosedClosed, bitgen, 0.0f, 1.0f);
    //
    //   // The most common interval of `turbo::IntervalClosedOpen` is available by
    //   // default:
    //
    //   auto x = turbo::uniform(bitgen, 0.0f, 1.0f);
    //
    //   // Return-types are typically inferred from the arguments, however callers
    //   // can optionally provide an explicit return-type to the template.
    //
    //   auto x = turbo::uniform<float>(bitgen, 0, 1);
    //
    template<typename R = void, typename TagType, typename URBG>
    typename std::enable_if_t<!std::is_same<R, void>::value, R>  //
    uniform(TagType tag,
            URBG &&urbg,  // NOLINT(runtime/references)
            R lo, R hi) {
        using gen_t = std::decay_t<URBG>;
        using distribution_t = random_internal::UniformDistributionWrapper<R>;

        auto a = random_internal::uniform_lower_bound(tag, lo, hi);
        auto b = random_internal::uniform_upper_bound(tag, lo, hi);
        if (!random_internal::is_uniform_range_valid(a, b)) return lo;

        return random_internal::DistributionCaller<gen_t>::template Call<
                distribution_t>(&urbg, tag, lo, hi);
    }

    template<typename R = void, typename TagType>
    typename std::enable_if_t<!std::is_same<R, void>::value && is_random_tag<TagType>::value, R>  //
    uniform(TagType tag,
            R lo, R hi) {
        using distribution_t = random_internal::UniformDistributionWrapper<R>;

        auto a = random_internal::uniform_lower_bound(tag, lo, hi);
        auto b = random_internal::uniform_upper_bound(tag, lo, hi);
        if (!random_internal::is_uniform_range_valid(a, b)) return lo;

        return random_internal::DistributionCaller<BitGen>::template Call<
                distribution_t>(&get_tls_bit_gen, tag, lo, hi);
    }

    // turbo::uniform<T>(bitgen, lo, hi)
    //
    // Overload of `uniform()` using the default closed-open interval of [lo, hi),
    // and returning values of type `T`
    template<typename R = void, typename URBG>
    typename std::enable_if_t<!std::is_same<R, void>::value && !is_random_tag<URBG>::value, R>  //
    uniform(URBG &&urbg,  // NOLINT(runtime/references)
            R lo, R hi) {
        using gen_t = std::decay_t<URBG>;
        using distribution_t = random_internal::UniformDistributionWrapper<R>;
        constexpr auto tag = turbo::IntervalClosedOpen;

        auto a = random_internal::uniform_lower_bound(tag, lo, hi);
        auto b = random_internal::uniform_upper_bound(tag, lo, hi);
        if (!random_internal::is_uniform_range_valid(a, b)) return lo;

        return random_internal::DistributionCaller<gen_t>::template Call<
                distribution_t>(&urbg, lo, hi);
    }

    template<typename R = void>
    typename std::enable_if_t<!std::is_same<R, void>::value, R>  //
    uniform(R lo, R hi) {
        using distribution_t = random_internal::UniformDistributionWrapper<R>;
        constexpr auto tag = turbo::IntervalClosedOpen;

        auto a = random_internal::uniform_lower_bound(tag, lo, hi);
        auto b = random_internal::uniform_upper_bound(tag, lo, hi);
        if (!random_internal::is_uniform_range_valid(a, b)) return lo;

        return random_internal::DistributionCaller<BitGen>::template Call<
                distribution_t>(&get_tls_bit_gen, lo, hi);
    }

    // turbo::uniform(tag, bitgen, lo, hi)
    //
    // Overload of `uniform()` using different (but compatible) lo, hi types. Note
    // that a compile-error will result if the return type cannot be deduced
    // correctly from the passed types.
    template<typename R = void, typename TagType, typename URBG, typename A,
            typename B>
    typename std::enable_if_t<
            std::is_same<R, void>::value && is_random_tag<TagType>::value && !is_random_tag<URBG>::value,
            random_internal::uniform_inferred_return_t<A, B>>
    uniform(TagType tag,
            URBG &&urbg,  // NOLINT(runtime/references)
            A lo, B hi) {
        using gen_t = std::decay_t<URBG>;
        using return_t = typename random_internal::uniform_inferred_return_t<A, B>;
        using distribution_t = random_internal::UniformDistributionWrapper<return_t>;

        auto a = random_internal::uniform_lower_bound<return_t>(tag, lo, hi);
        auto b = random_internal::uniform_upper_bound<return_t>(tag, lo, hi);
        if (!random_internal::is_uniform_range_valid(a, b)) return lo;

        return random_internal::DistributionCaller<gen_t>::template Call<
                distribution_t>(&urbg, tag, static_cast<return_t>(lo),
                                static_cast<return_t>(hi));
    }

    template<typename R = void, typename TagType, typename A,
            typename B>
    typename std::enable_if_t<std::is_same<R, void>::value && is_random_tag<TagType>::value,
            random_internal::uniform_inferred_return_t<A, B>>
    uniform(TagType tag,
            A lo, B hi) {
        using return_t = typename random_internal::uniform_inferred_return_t<A, B>;
        using distribution_t = random_internal::UniformDistributionWrapper<return_t>;

        auto a = random_internal::uniform_lower_bound<return_t>(tag, lo, hi);
        auto b = random_internal::uniform_upper_bound<return_t>(tag, lo, hi);
        if (!random_internal::is_uniform_range_valid(a, b)) return lo;

        return random_internal::DistributionCaller<BitGen>::template Call<
                distribution_t>(&get_tls_bit_gen, tag, static_cast<return_t>(lo),
                                static_cast<return_t>(hi));
    }

    // turbo::uniform(bitgen, lo, hi)
    //
    // Overload of `uniform()` using different (but compatible) lo, hi types and the
    // default closed-open interval of [lo, hi). Note that a compile-error will
    // result if the return type cannot be deduced correctly from the passed types.
    template<typename R = void, typename URBG, typename A, typename B>
    typename std::enable_if_t<std::is_same<R, void>::value && !is_random_tag<URBG>::value,
            random_internal::uniform_inferred_return_t<A, B>>
    uniform(URBG &&urbg,  // NOLINT(runtime/references)
            A lo, B hi) {
        using gen_t = std::decay_t<URBG>;
        using return_t = typename random_internal::uniform_inferred_return_t<A, B>;
        using distribution_t = random_internal::UniformDistributionWrapper<return_t>;

        constexpr auto tag = turbo::IntervalClosedOpen;
        auto a = random_internal::uniform_lower_bound<return_t>(tag, lo, hi);
        auto b = random_internal::uniform_upper_bound<return_t>(tag, lo, hi);
        if (!random_internal::is_uniform_range_valid(a, b)) return lo;

        return random_internal::DistributionCaller<gen_t>::template Call<
                distribution_t>(&urbg, static_cast<return_t>(lo),
                                static_cast<return_t>(hi));
    }

    template<typename R = void, typename A, typename B>
    typename std::enable_if_t<std::is_same<R, void>::value,
            random_internal::uniform_inferred_return_t<A, B>>
    uniform(A lo, B hi) {
        using return_t = typename random_internal::uniform_inferred_return_t<A, B>;
        using distribution_t = random_internal::UniformDistributionWrapper<return_t>;

        constexpr auto tag = turbo::IntervalClosedOpen;
        auto a = random_internal::uniform_lower_bound<return_t>(tag, lo, hi);
        auto b = random_internal::uniform_upper_bound<return_t>(tag, lo, hi);
        if (!random_internal::is_uniform_range_valid(a, b)) return lo;

        return random_internal::DistributionCaller<BitGen>::template Call<
                distribution_t>(&get_tls_bit_gen, static_cast<return_t>(lo),
                                static_cast<return_t>(hi));
    }

    // turbo::uniform<unsigned T>(bitgen)
    //
    // Overload of uniform() using the minimum and maximum values of a given type
    // `T` (which must be unsigned), returning a value of type `unsigned T`
    template<typename R, typename URBG>
    typename std::enable_if_t<!std::is_signed<R>::value && !is_random_tag<URBG>::value, R>  //
    uniform(URBG &&urbg) {  // NOLINT(runtime/references)
        using gen_t = std::decay_t<URBG>;
        using distribution_t = random_internal::UniformDistributionWrapper<R>;

        return random_internal::DistributionCaller<gen_t>::template Call<
                distribution_t>(&urbg);
    }

    template<typename R>
    typename std::enable_if_t<!std::is_signed<R>::value, R>  //
    uniform() {
        using distribution_t = random_internal::UniformDistributionWrapper<R>;

        return random_internal::DistributionCaller<BitGen>::template Call<
                distribution_t>(&get_tls_bit_gen);
    }

}  // namespace turbo

#endif  // TURBO_RANDOM_UNIFORM_H_
