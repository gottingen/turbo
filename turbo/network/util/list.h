// Copyright 2023 The turbo Authors.
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

#ifndef TURBO_NETWORK_UTIL_LIST_H_
#define TURBO_NETWORK_UTIL_LIST_H_

#include <list>
#include <type_traits>

namespace turbo {

template<typename T>
class List : public std::list<T> {
public:
    template<typename ... ARGS>
    List(ARGS &&...args) : std::list<T>(std::forward<ARGS>(args)...) {};

    ~List() = default;

    void append(List<T> &other) {
        if (other.empty()) {
            return;
        }
        this->insert(this->end(), other.begin(), other.end());
        other.clear();
    }

    template<typename FUNC>
    void for_each(FUNC &&func) {
        for (auto &t : *this) {
            func(t);
        }
    }

    template<typename FUNC>
    void for_each(FUNC &&func) const {
        for (auto &t : *this) {
            func(t);
        }
    }
};

} // namespace turbo

#endif // TURBO_NETWORK_THREAD_LIST_H_