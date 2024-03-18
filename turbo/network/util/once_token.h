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

#ifndef TURBO_NETWORK_UTIL_ONCE_TOKEN_H_
#define TURBO_NETWORK_UTIL_ONCE_TOKEN_H_

#include <functional>
#include <type_traits>

namespace turbo {
// 设计思想，用栈变量管理堆变量，同理也可以使用智能指针+删除器实现
class onceToken {
public:
    using task = std::function<void(void)>;

    template<typename FUNC>
    onceToken(const FUNC &onConstructed, task onDestructed = nullptr) {
        onConstructed();
        _onDestructed = std::move(onDestructed);
    }

    onceToken(std::nullptr_t, task onDestructed = nullptr) {
        _onDestructed = std::move(onDestructed);
    }

    ~onceToken() {
        if (_onDestructed) {
            _onDestructed();
        }
    }

private:
    onceToken() = delete;
    onceToken(const onceToken &) = delete;
    onceToken(onceToken &&) = delete;
    onceToken &operator=(const onceToken &) = delete;
    onceToken &operator=(onceToken &&) = delete;

private:
    task _onDestructed;
};

} /* namespace turbo */
#endif /* TURBO_NETWORK__ONCE_TOKEN_H_ */