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

#ifndef TURBO_NETWORK_THREAD_WORK_POOL_H_
#define TURBO_NETWORK_THREAD_WORK_POOL_H_

// 基于epoll io复用及管道通信的多线程工作池，可以负载均衡的分布任务到各个线程中


#include <memory>
#include "turbo/network/thread/task_executor.h"
#include "turbo/network/poller/event_poller.h"

namespace turbo {

class WorkThreadPool : public std::enable_shared_from_this<WorkThreadPool>, public TaskExecutorGetterImp {
public:
    using Ptr = std::shared_ptr<WorkThreadPool>;

    ~WorkThreadPool() override = default;

    /**
     * 获取单例
     */
    static WorkThreadPool &Instance();

    /**
     * 设置EventPoller个数，在WorkThreadPool单例创建前有效
     * 在不调用此方法的情况下，默认创建thread::hardware_concurrency()个EventPoller实例
     * @param size EventPoller个数，如果为0则为thread::hardware_concurrency()
     */
    static void setPoolSize(size_t size = 0);

    /**
     * 内部创建线程是否设置cpu亲和性，默认设置cpu亲和性
     */
    static void enableCpuAffinity(bool enable);

    /**
     * 获取第一个实例
     * @return
     */
    EventPoller::Ptr getFirstPoller();

    /**
     * 根据负载情况获取轻负载的实例
     * 如果优先返回当前线程，那么会返回当前线程
     * 返回当前线程的目的是为了提高线程安全性
     * @return
     */
    EventPoller::Ptr getPoller();

protected:
    WorkThreadPool();
};

} /* namespace turbo */
#endif /* TURBO_NETWORK_UTIL_WORK_POOL_H_ */