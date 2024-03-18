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

#include "turbo/network/thread/work_pool.h"

namespace turbo {

static size_t s_pool_size = 0;
static bool s_enable_cpu_affinity = true;

INSTANCE_IMP(WorkThreadPool)

EventPoller::Ptr WorkThreadPool::getFirstPoller() {
    return std::static_pointer_cast<EventPoller>(_threads.front());
}

EventPoller::Ptr WorkThreadPool::getPoller() {
    return std::static_pointer_cast<EventPoller>(getExecutor());
}

WorkThreadPool::WorkThreadPool() {
    //最低优先级
    addPoller("work poller", s_pool_size, static_cast<int>(ThreadPool::Priority::PRIORITY_LOWEST), false, s_enable_cpu_affinity);
}

void WorkThreadPool::setPoolSize(size_t size) {
    s_pool_size = size;
}

void WorkThreadPool::enableCpuAffinity(bool enable) {
    s_enable_cpu_affinity = enable;
}

} /* namespace turbo */
