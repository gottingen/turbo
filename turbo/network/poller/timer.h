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

#ifndef TURBO_NETWORK_POLLER_TIMER_H_
#define TURBO_NETWORK_POLLER_TIMER_H_

#include <functional>
#include "turbo/network/poller/event_poller.h"

namespace turbo {

class Timer {
public:
    using Ptr = std::shared_ptr<Timer>;

    /**
     * 构造定时器
     * @param second 定时器重复秒数
     * @param cb 定时器任务，返回true表示重复下次任务，否则不重复，如果任务中抛异常，则默认重复下次任务
     * @param poller EventPoller对象，可以为nullptr
     */
    Timer(float second, const std::function<bool()> &cb, const EventPoller::Ptr &poller);
    ~Timer();

private:
    std::weak_ptr<EventPoller::DelayTask> _tag;
    //定时器保持EventPoller的强引用
    EventPoller::Ptr _poller;
};

}  // namespace turbo
#endif /* TURBO_NETWORK_POLLER_TIMER_H_ */