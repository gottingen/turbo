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
//
// Created by jeff on 23-12-19.
//

#ifndef TURBO_PROFILING_INTERNAL_BATCH_REDUCER_H_
#define TURBO_PROFILING_INTERNAL_BATCH_REDUCER_H_

#include <atomic>
#include "turbo/profiling/internal/batch_combiner.h"
#include "turbo/profiling/variable.h"

#include "turbo/meta/reflect.h"
#include "turbo/log/logging.h"

namespace turbo::profiling_internal {

    template<typename T, size_t N, typename CombineOp, typename SetterOp>
    class BatchReducer {
    public:
        typedef BatchAgentCombiner<T, T, N, CombineOp, SetterOp> combiner_type;
        typedef typename combiner_type::Agent agent_type;
    public:
        BatchReducer(typename add_cr_non_integral<T>::type identity = static_cast<add_cr_non_integral<int>::type>(T()),
                const CombineOp &cop = CombineOp(),
                const SetterOp &setter_op = SetterOp())
                : _combiner(identity, identity, cop, setter_op) {
        }

        ~BatchReducer() {

        }

        // Add a value.
        // Returns self reference for chaining.
        BatchReducer &set_value(typename add_cr_non_integral<T>::type value, size_t i);

        // Get reduced value.
        // Notice that this function walks through threads that ever add values
        // into this reducer. You should avoid calling it frequently.
        turbo::Batch<T,N> get_value() const {
            return _combiner.combine_agents();
        }

        T get_value(size_t i) const {
            return _combiner.combine_agents(i);
        }

        turbo::Batch<T,N> reset() { return _combiner.reset_all_agents(); }

        bool valid() const { return _combiner.valid(); }

        // Get instance of Op.
        const CombineOp &combine_op() const { return _combiner.combine_op(); }

        CombineOp &combine_op() { return _combiner.combine_op(); }

        const SetterOp &setter_op() const { return _combiner.setter_op(); }

    private:
        combiner_type _combiner;
    };

    template<typename T, size_t N,typename Op, typename InvOp>
    inline BatchReducer<T,N,  Op, InvOp> &BatchReducer<T, N, Op, InvOp>::set_value(
            typename add_cr_non_integral<T>::type value, size_t i) {
        // It's wait-free for most time
        agent_type *agent = _combiner.get_or_create_tls_agent();
        if (TURBO_UNLIKELY(!agent)) {
            TLOG_CRITICAL("Fail to create agent");
            return *this;
        }
        agent->element.modify(_combiner.setter_op(), value, i);
        return *this;
    }

}  // namespace turbo
#endif  // TURBO_PROFILING_INTERNAL_BATCH_REDUCER_H_
