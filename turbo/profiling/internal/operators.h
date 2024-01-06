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


#ifndef TURBO_PROFILING_OPERATORS_H_
#define TURBO_PROFILING_OPERATORS_H_

#include <atomic>
#include "turbo/profiling/internal/combiner.h"

namespace turbo::profiling_internal {

    template<typename Tp>
    struct AddTo {
        void operator()(Tp &lhs,
                        typename add_cr_non_integral<Tp>::type rhs) const {
            lhs += rhs;
        }
    };

    template<typename Tp>
    struct AtomicAddTo {
        void operator()(std::atomic<Tp>  &lhs,
                        typename add_cr_non_integral<Tp>::type rhs) const {
            lhs.fetch_add(rhs, std::memory_order_relaxed);
        }
    };

    template<typename Tp>
    struct SetTo {
        void operator()(Tp &lhs,
                        typename add_cr_non_integral<Tp>::type rhs) const {
            lhs = rhs;
        }
    };
    template<typename Tp>
    struct AtomicSetTo {
        void operator()(std::atomic<Tp> &lhs,
                        typename add_cr_non_integral<Tp>::type rhs) const {
            lhs.store(rhs, std::memory_order_relaxed);
        }
    };


    template<typename Tp>
    struct AvgTo {
        void operator()(Tp &lhs,
                        typename add_cr_non_integral<Tp>::type rhs) const {
            lhs += rhs;
            ++count;
        }
        mutable size_t count;
    };

    template<typename Tp>
    struct MaxerTo {
        void operator()(Tp &lhs,
                        typename add_cr_non_integral<Tp>::type rhs) const {
            if(lhs < rhs) {
                lhs = rhs;
            }
        }
    };

    template<typename Tp>
    struct MinerTo {
        void operator()(Tp &lhs,
                        typename add_cr_non_integral<Tp>::type rhs) const {
            if(lhs > rhs) {
                lhs = rhs;
            }
        }
    };

}  // namespace turbo::profiling_internal
#endif  // TURBO_PROFILING_OPERATORS_H_
