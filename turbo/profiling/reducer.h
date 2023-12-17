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
// Created by jeff on 23-12-16.
//

#ifndef TURBO_PROFILING_REDUCER_H_
#define TURBO_PROFILING_REDUCER_H_

#include "turbo/profiling/variable.h"
#include "turbo/profiling/internal/combiner.h"
#include "turbo/profiling/internal/sampler.h"

namespace turbo {


    template <typename T, typename Op, typename InvOp = profiling_internal::VoidOp>
    class Reducer : public Variable {
    public:
        typedef typename profiling_internal::AgentCombiner<T, T, Op> combiner_type;
        typedef typename combiner_type::Agent agent_type;
        typedef profiling_internal::ReducerSampler<Reducer, T, Op, InvOp> sampler_type;
        class SeriesSampler : public profiling_internal::Sampler {
        public:
            SeriesSampler(Reducer* owner, const Op& op)
                    : _owner(owner), _series(op) {}
            ~SeriesSampler() {}
            void take_sample() override { _series.append(_owner->get_value()); }
            void describe(std::ostream& os) { _series.describe(os, NULL); }
        private:
            Reducer* _owner;
            detail::Series<T, Op> _series;
        };

    public:
        // The `identify' must satisfy: identity Op a == a
        Reducer(typename profiling_internal::add_cr_non_integral<T>::type identity = T(),
                const Op& op = Op(),
                const InvOp& inv_op = InvOp())
                : _combiner(identity, identity, op)
                , _sampler(NULL)
                , _series_sampler(NULL)
                , _inv_op(inv_op) {
        }

        ~Reducer() {
            // Calling hide() manually is a MUST required by Variable.
            hide();
            if (_sampler) {
                _sampler->destroy();
                _sampler = NULL;
            }
            if (_series_sampler) {
                _series_sampler->destroy();
                _series_sampler = NULL;
            }
        }

        // Add a value.
        // Returns self reference for chaining.
        Reducer& operator<<(typename butil::add_cr_non_integral<T>::type value);

        // Get reduced value.
        // Notice that this function walks through threads that ever add values
        // into this reducer. You should avoid calling it frequently.
        T get_value() const {
            CHECK(!(std::is_same<InvOp, detail::VoidOp>::value) || _sampler == NULL)
                    << "You should not call Reducer<" << butil::class_name_str<T>()
                    << ", " << butil::class_name_str<Op>() << ">::get_value() when a"
                    << " Window<> is used because the operator does not have inverse.";
            return _combiner.combine_agents();
        }


        // Reset the reduced value to T().
        // Returns the reduced value before reset.
        T reset() { return _combiner.reset_all_agents(); }

        void describe(std::ostream& os, DescribeOption *option ) const override {
            if (std::is_same<T, std::string>::value) {
                os << '"' << get_value() << '"';
            } else {
                os << get_value();
            }
        }

        // True if this reducer is constructed successfully.
        bool valid() const { return _combiner.valid(); }

        // Get instance of Op.
        const Op& op() const { return _combiner.op(); }
        const InvOp& inv_op() const { return _inv_op; }

        sampler_type* get_sampler() {
            if (NULL == _sampler) {
                _sampler = new sampler_type(this);
                _sampler->schedule();
            }
            return _sampler;
        }

        int describe_series(std::ostream& os, const SeriesOptions& options) const override {
            if (_series_sampler == NULL) {
                return 1;
            }
            if (!options.test_only) {
                _series_sampler->describe(os);
            }
            return 0;
        }

    protected:
        int expose_impl(const butil::StringPiece& prefix,
                        const butil::StringPiece& name,
                        DisplayFilter display_filter) override {
            const int rc = Variable::expose_impl(prefix, name, display_filter);
            if (rc == 0 &&
                _series_sampler == NULL &&
                !butil::is_same<InvOp, detail::VoidOp>::value &&
                !butil::is_same<T, std::string>::value &&
                FLAGS_save_series) {
                _series_sampler = new SeriesSampler(this, _combiner.op());
                _series_sampler->schedule();
            }
            return rc;
        }

    private:
        combiner_type   _combiner;
        sampler_type* _sampler;
        SeriesSampler* _series_sampler;
        InvOp _inv_op;
    };

}  // namespace turbo
#endif  // TURBO_PROFILING_REDUCER_H_
