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

#ifndef TURBO_NETWORK_NETWORK_BUFFER_SOCK_H_
#define TURBO_NETWORK_NETWORK_BUFFER_SOCK_H_


#if !defined(_WIN32)
#include <sys/uio.h>
#include <limits.h>
#endif
#include <cassert>
#include <memory>
#include <string>
#include <vector>
#include <type_traits>
#include <functional>
#include "turbo/network/util/util.h"
#include "turbo/network/util/list.h"
#include "turbo/network/util/resource_pool.h"
#include "turbo/network/network/sock_util.h"
#include "turbo/network/network/buffer.h"

namespace turbo {

#if !defined(IOV_MAX)
#define IOV_MAX 1024
#endif

class BufferSock : public Buffer {
public:
    using Ptr = std::shared_ptr<BufferSock>;
    BufferSock(Buffer::Ptr ptr, struct sockaddr *addr = nullptr, int addr_len = 0);
    ~BufferSock() override = default;

    char *data() const override;
    size_t size() const override;
    const struct sockaddr *sockaddr() const;
    socklen_t  socklen() const;

private:
    int _addr_len = 0;
    struct sockaddr_storage _addr;
    Buffer::Ptr _buffer;
};

class BufferList : public noncopyable {
public:
    using Ptr = std::shared_ptr<BufferList>;
    using SendResult = std::function<void(const Buffer::Ptr &buffer, bool send_success)>;

    BufferList() = default;
    virtual ~BufferList() = default;

    virtual bool empty() = 0;
    virtual size_t count() = 0;
    virtual ssize_t send(int fd, int flags) = 0;

    static Ptr create(List<std::pair<Buffer::Ptr, bool> > list, SendResult cb, bool is_udp);

private:
    //对象个数统计
    ObjectStatistic<BufferList> _statistic;
};

}
#endif //TURBO_NETWORK_NETWORK_BUFFER_SOCK_H_

