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

#ifndef TURBO_VAR_MAXER_GAUGE_H_
#define TURBO_VAR_MAXER_GAUGE_H_

#include "turbo/var/operators.h"

namespace turbo {

    namespace var_internal {
        class LatencyRecorderBase;
    }
    template<typename T>
    class MaxerGauge : public Reducer<T, var_internal::MaxTo<T> > {
    public:
        typedef Reducer<T, var_internal::MaxTo<T> > Base;
        typedef T value_type;
        typedef typename Base::sampler_type sampler_type;
    public:
        MaxerGauge() : Base(std::numeric_limits<T>::min()) {}

        explicit MaxerGauge(const std::string &name)
                : Base(std::numeric_limits<T>::min()) {
            this->expose(name);
        }

        MaxerGauge(const std::string &prefix, const std::string &name)
                : Base(std::numeric_limits<T>::min()) {
            this->expose_as(prefix, name);
        }

        ~MaxerGauge() { Variable::hide(); }

    private:
        friend class var_internal::LatencyRecorderBase;

        // The following private funcition a now used in LatencyRecorder,
        // it's dangerous so we don't make them public
        explicit MaxerGauge(T default_value) : Base(default_value) {
        }

        MaxerGauge(T default_value, const std::string &prefix,
              const std::string &name)
                : Base(default_value) {
            this->expose_as(prefix, name);
        }

        MaxerGauge(T default_value, const std::string &name) : Base(default_value) {
            this->expose(name);
        }
    };
}  // namespace turbo
#endif  // TURBO_VAR_MAXER_GAUGE_H_
