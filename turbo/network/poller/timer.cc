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

#include "turbo/network/poller/timer.h"

namespace turbo {

Timer::Timer(float second, const std::function<bool()> &cb, const EventPoller::Ptr &poller) {
    _poller = poller;
    if (!_poller) {
        _poller = EventPollerPool::Instance().getPoller();
    }
    _tag = _poller->doDelayTask((uint64_t) (second * 1000), [cb, second]() {
        try {
            if (cb()) {
                //重复的任务,基于epoll系统调用的定时器
                return (uint64_t) (1000 * second);
            }
            //该任务不再重复
            return (uint64_t) 0;
        } catch (std::exception &ex) {
            ErrorL << "Exception occurred when do timer task: " << ex.what();
            return (uint64_t) (1000 * second);
        }
    });
}

Timer::~Timer() {
    auto tag = _tag.lock();
    if (tag) {
        tag->cancel();
    }
}

}  // namespace turbo