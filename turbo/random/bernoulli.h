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


#ifndef TURBO_RANDOM_BERNOULLI_H_
#define TURBO_RANDOM_BERNOULLI_H_

#include "turbo/random/fwd.h"

namespace turbo {

    // -----------------------------------------------------------------------------
    // turbo::bernoulli(bitgen, p)
    // -----------------------------------------------------------------------------
    //
    // `turbo::bernoulli` produces a random boolean value, with probability `p`
    // (where 0.0 <= p <= 1.0) equaling `true`.
    //
    // Prefer `turbo::bernoulli` to produce boolean values over other alternatives
    // such as comparing an `turbo::uniform()` value to a specific output.
    //
    // See https://en.wikipedia.org/wiki/Bernoulli_distribution
    //
    // Example:
    //
    //   turbo::BitGen bitgen;
    //   ...
    //   if (turbo::bernoulli(bitgen, 1.0/3721.0)) {
    //     std::cout << "Asteroid field navigation successful.";
    //   }
    //
    template<typename URBG>
    bool bernoulli(URBG &&urbg,  // NOLINT(runtime/references)
                   double p) {
        using gen_t = std::decay_t<URBG>;
        using distribution_t = turbo::bernoulli_distribution;

        return random_internal::DistributionCaller<gen_t>::template Call<
                distribution_t>(&urbg, p);
    }

    bool bernoulli(double p) {
        using distribution_t = turbo::bernoulli_distribution;

        return random_internal::DistributionCaller<BitGen>::template Call<
                distribution_t>(&get_tls_bit_gen(), p);
    }

}  // namespace turbo

#endif  // TURBO_RANDOM_BERNOULLI_H_
