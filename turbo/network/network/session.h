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

#ifndef TURBO_NETWORK_NETWORK_SESSION_H_
#define TURBO_NETWORK_NETWORK_SESSION_H_

#include <memory>
#include "turbo/network/network/socket.h"
#include "turbo/network/util/util.h"
#include "turbo/network/util/ssl_box.h"

namespace turbo {

// 会话, 用于存储一对客户端与服务端间的关系
class Server;
class TcpSession;
class UdpSession;

class Session : public SocketHelper {
public:
    using Ptr = std::shared_ptr<Session>;

    Session(const Socket::Ptr &sock);
    ~Session() override = default;

    /**
     * 在创建 Session 后, Server 会把自身的配置参数通过该函数传递给 Session
     * @param server, 服务器对象
     */
    virtual void attachServer(const Server &server) {}

    /**
     * 作为该 Session 的唯一标识符
     * @return 唯一标识符
     */
    std::string getIdentifier() const override;

private:
    mutable std::string _id;
    std::unique_ptr<turbo::ObjectStatistic<turbo::TcpSession> > _statistic_tcp;
    std::unique_ptr<turbo::ObjectStatistic<turbo::UdpSession> > _statistic_udp;
};

// 通过该模板可以让TCP服务器快速支持TLS
template <typename SessionType>
class SessionWithSSL : public SessionType {
public:
    template <typename... ArgsType>
    SessionWithSSL(ArgsType &&...args)
        : SessionType(std::forward<ArgsType>(args)...) {
        _ssl_box.setOnEncData([&](const Buffer::Ptr &buf) { public_send(buf); });
        _ssl_box.setOnDecData([&](const Buffer::Ptr &buf) { public_onRecv(buf); });
    }

    ~SessionWithSSL() override { _ssl_box.flush(); }

    void onRecv(const Buffer::Ptr &buf) override { _ssl_box.onRecv(buf); }

    // 添加public_onRecv和public_send函数是解决较低版本gcc一个lambad中不能访问protected或private方法的bug
    inline void public_onRecv(const Buffer::Ptr &buf) { SessionType::onRecv(buf); }
    inline void public_send(const Buffer::Ptr &buf) { SessionType::send(buf); }

    bool overSsl() const override { return true; }

protected:
    ssize_t send(Buffer::Ptr buf) override {
        auto size = buf->size();
        _ssl_box.onSend(std::move(buf));
        return size;
    }

private:
    SSL_Box _ssl_box;
};

}// namespace turbo

#endif // TURBO_NETWORK_NETWORK_SESSION_H_


