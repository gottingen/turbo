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


#ifndef TURBO_RANDOM_BETA_H_
#define TURBO_RANDOM_BETA_H_

#include "turbo/random/fwd.h"

namespace turbo {

    // -----------------------------------------------------------------------------
    // turbo::beta<T>(bitgen, alpha, beta)
    // -----------------------------------------------------------------------------
    //
    // `turbo::beta` produces a floating point number distributed in the closed
    // interval [0,1] and parameterized by two values `alpha` and `beta` as per a
    // Beta distribution. `T` must be a floating point type, but may be inferred
    // from the types of `alpha` and `beta`.
    //
    // See https://en.wikipedia.org/wiki/Beta_distribution.
    //
    // Example:
    //
    //   turbo::BitGen bitgen;
    //   ...
    //   double sample = turbo::beta(bitgen, 3.0, 2.0);
    //
    template<typename RealType, typename URBG>
    RealType beta(URBG &&urbg,  // NOLINT(runtime/references)
                  RealType alpha, RealType beta) {
        static_assert(
                std::is_floating_point<RealType>::value,
                "Template-argument 'RealType' must be a floating-point type, in "
                "turbo::beta<RealType, URBG>(...)");

        using gen_t = std::decay_t<URBG>;
        using distribution_t = typename turbo::beta_distribution<RealType>;

        return random_internal::DistributionCaller<gen_t>::template Call<
                distribution_t>(&urbg, alpha, beta);
    }

    template<typename RealType>
    RealType beta(RealType alpha, RealType beta) {
        static_assert(
                std::is_floating_point<RealType>::value,
                "Template-argument 'RealType' must be a floating-point type, in "
                "turbo::beta<RealType, URBG>(...)");

        using distribution_t = typename turbo::beta_distribution<RealType>;

        return random_internal::DistributionCaller<BitGen>::template Call<
                distribution_t>(&get_tls_bit_gen, alpha, beta);
    }

}  // namespace turbo

#endif  // TURBO_RANDOM_BETA_H_
