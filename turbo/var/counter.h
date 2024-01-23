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

#ifndef TURBO_VAR_COUNTER_H_
#define TURBO_VAR_COUNTER_H_

#include "turbo/var/operators.h"

namespace turbo {

    template<typename T>
    class Counter : public Reducer<T, var_internal::AddTo<T>, var_internal::MinusFrom<T> > {
    public:
        typedef Reducer<T, var_internal::AddTo<T>, var_internal::MinusFrom<T> > Base;
        typedef T value_type;
        typedef typename Base::sampler_type sampler_type;
    public:
        Counter() : Base() {}

        explicit Counter(const std::string &name) : Base() {
            this->expose(name);
        }

        Counter(const std::string &prefix,
              const std::string &name) : Base() {
            this->expose_as(prefix, name);
        }

        ~Counter() { Variable::hide(); }
    };

}  // namespace turbo

#endif // TURBO_VAR_COUNTER_H_
