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

#ifndef TURBO_VAR_OPERATORS_H_
#define TURBO_VAR_OPERATORS_H_

#include "turbo/var/reducer.h"

namespace turbo::var_internal {

    template<typename Tp>
    struct MaxTo {
        void operator()(Tp &lhs,
                        typename add_cr_non_integral<Tp>::type rhs) const {
            // Use operator< as well.
            if (lhs < rhs) {
                lhs = rhs;
            }
        }
    };

    template<typename Tp>
    struct MinTo {
        void operator()(Tp &lhs,
                        typename add_cr_non_integral<Tp>::type rhs) const {
            if (rhs < lhs) {
                lhs = rhs;
            }
        }
    };

    template<typename Tp>
    struct AddTo {
        void operator()(Tp &lhs,
                        typename add_cr_non_integral<Tp>::type rhs) const { lhs += rhs; }
    };

    template<typename Tp>
    struct MinusFrom {
        void operator()(Tp &lhs,
                        typename add_cr_non_integral<Tp>::type rhs) const { lhs -= rhs; }
    };

}  // namespace turbo::var_internal

#endif // TURBO_VAR_OPERATORS_H_
