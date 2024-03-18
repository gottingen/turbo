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

#include "turbo/network/network/tcp_server.h"
#include "turbo/network/util/uv_errno.h"
#include "turbo/network/util/once_token.h"


using namespace std;

namespace turbo {

INSTANCE_IMP(SessionMap)
StatisticImp(TcpServer)

TcpServer::TcpServer(const EventPoller::Ptr &poller) : Server(poller) {
    setOnCreateSocket(nullptr);
}

void TcpServer::setupEvent() {
    _socket = createSocket(_poller);
    weak_ptr<TcpServer> weak_self = std::static_pointer_cast<TcpServer>(shared_from_this());
    // 拦截服务监听到的accpet fd连接构造，主要是指定socket的生成，保障负载均衡
    _socket->setOnBeforeAccept([weak_self](const EventPoller::Ptr &poller) -> Socket::Ptr {
        if (auto strong_self = weak_self.lock()) {
            return strong_self->onBeforeAcceptConnection(poller);
        }
        return nullptr;
    });
    // 设置当前sever的socket接收accept时的回调
    // 对不同的子服务器，接收的连接socket连接到自己server来处理，具体是处理socket的poller是从poller池中选择的负载最小的poller
    _socket->setOnAccept([weak_self](Socket::Ptr &sock, shared_ptr<void> &complete) {
        if (auto strong_self = weak_self.lock()) {
            auto ptr = sock->getPoller().get();
            auto server = strong_self->getServer(ptr);
            ptr->async([server, sock, complete]() {
                //该tcp客户端派发给对应线程的TcpServer服务器
                server->onAcceptConnection(sock);
            });
        }
    });
}

TcpServer::~TcpServer() {
    if (_main_server && _socket && _socket->rawFD() != -1) {
        InfoL << "Close tcp server [" << _socket->get_local_ip() << "]: " << _socket->get_local_port();
    }
    _timer.reset();
    //先关闭socket监听，防止收到新的连接
    _socket.reset();
    _session_map.clear();
    _cloned_server.clear();
}

uint16_t TcpServer::getPort() {
    if (!_socket) {
        return 0;
    }
    return _socket->get_local_port();
}

void TcpServer::setOnCreateSocket(Socket::onCreateSocket cb) {
    if (cb) {
        _on_create_socket = std::move(cb);
    } else {
        _on_create_socket = [](const EventPoller::Ptr &poller) {
            return Socket::createSocket(poller, false);
        };
    }
    for (auto &pr : _cloned_server) {
        pr.second->setOnCreateSocket(cb);
    }
}

TcpServer::Ptr TcpServer::onCreatServer(const EventPoller::Ptr &poller) {
    return Ptr(new TcpServer(poller), [poller](TcpServer *ptr) { poller->async([ptr]() { delete ptr; }); });
}

Socket::Ptr TcpServer::onBeforeAcceptConnection(const EventPoller::Ptr &poller) {
    assert(_poller->isCurrentThread());
    // 此处改成自定义获取poller对象，防止负载不均衡：因为虽然把连接派发到子tcp服务，但是也会存在子tcp服务抢占次数比较多，负载不均衡的情况，这里还是拦截peer socket的构造
    // 从poller池中选取一个负载最小的poller来负载管理监听socket
    return createSocket(EventPollerPool::Instance().getPoller(false));
}

void TcpServer::cloneFrom(const TcpServer &that) {
    if (!that._socket) {
        throw std::invalid_argument("TcpServer::cloneFrom other with null socket");
    }
    setupEvent();
    _main_server = false;
    _on_create_socket = that._on_create_socket;
    _session_alloc = that._session_alloc;
    weak_ptr<TcpServer> weak_self = std::static_pointer_cast<TcpServer>(shared_from_this());
    _timer = std::make_shared<Timer>(2.0f, [weak_self]() -> bool {
        auto strong_self = weak_self.lock();
        if (!strong_self) {
            return false;
        }
        strong_self->onManagerSession();
        return true;
    }, _poller);
    this->mINI::operator=(that);
    _parent = static_pointer_cast<TcpServer>(const_cast<TcpServer &>(that).shared_from_this());
}

// 接收到客户端连接请求
Session::Ptr TcpServer::onAcceptConnection(const Socket::Ptr &sock) {
    assert(_poller->isCurrentThread());
    weak_ptr<TcpServer> weak_self = std::static_pointer_cast<TcpServer>(shared_from_this());
    //创建一个Session;这里实现创建不同的服务会话实例
    auto helper = _session_alloc(std::static_pointer_cast<TcpServer>(shared_from_this()), sock);
    auto session = helper->session();
    //把本服务器的配置传递给Session
    session->attachServer(*this);

    //_session_map::emplace肯定能成功
    auto success = _session_map.emplace(helper.get(), helper).second;
    assert(success == true);

    weak_ptr<Session> weak_session = session;
    //会话接收数据事件
    sock->setOnRead([weak_session](const Buffer::Ptr &buf, struct sockaddr *, int) {
        //获取会话强应用
        auto strong_session = weak_session.lock();
        if (!strong_session) {
            return;
        }
        try {
            strong_session->onRecv(buf);
        } catch (SockException &ex) {
            strong_session->shutdown(ex);
        } catch (exception &ex) {
            strong_session->shutdown(SockException(ErrCode::Err_shutdown, ex.what()));
        }
    });

    SessionHelper *ptr = helper.get();
    auto cls = ptr->className();
    //会话接收到错误事件
    sock->setOnErr([weak_self, weak_session, ptr, cls](const SockException &err) {
        //在本函数作用域结束时移除会话对象
        //目的是确保移除会话前执行其onError函数
        //同时避免其onError函数抛异常时没有移除会话对象
        onceToken token(nullptr, [&]() {
            //移除掉会话
            auto strong_self = weak_self.lock();
            if (!strong_self) {
                return;
            }

            assert(strong_self->_poller->isCurrentThread());
            if (!strong_self->_is_on_manager) {
                //该事件不是onManager时触发的，直接操作map
                strong_self->_session_map.erase(ptr);
            } else {
                //遍历map时不能直接删除元素
                strong_self->_poller->async([weak_self, ptr]() {
                    auto strong_self = weak_self.lock();
                    if (strong_self) {
                        strong_self->_session_map.erase(ptr);
                    }
                }, false);
            }
        });

        //获取会话强应用
        auto strong_session = weak_session.lock();
        if (strong_session) {
            //触发onError事件回调
            TraceP(strong_session) << cls << " on err: " << err;
            strong_session->onError(err);
        }
    });
    return session;
}

void TcpServer::start_l(uint16_t port, const std::string &host, uint32_t backlog) {
    setupEvent();

    //新建一个定时器定时管理这些tcp会话
    weak_ptr<TcpServer> weak_self = std::static_pointer_cast<TcpServer>(shared_from_this());
    _timer = std::make_shared<Timer>(2.0f, [weak_self]() -> bool {
        auto strong_self = weak_self.lock();
        if (!strong_self) {
            return false;
        }
        strong_self->onManagerSession();
        return true;
    }, _poller);
    // 对每个poller创建一个子server，创建对应套接字，相关事件回调
    EventPollerPool::Instance().for_each([&](const TaskExecutor::Ptr &executor) {
        EventPoller::Ptr poller = static_pointer_cast<EventPoller>(executor);
        if (poller == _poller) {
            return;
        }
        // 如果当前事件轮询器没有指定服务器，则创建一个
        auto &serverRef = _cloned_server[poller.get()];
        if (!serverRef) {
            serverRef = onCreatServer(poller);
        }
        if (serverRef) {
            serverRef->cloneFrom(*this);
        }
    });
    // 当前server的事件轮询器开始监听端口
    if (!_socket->listen(port, host.c_str(), backlog)) {
        // 创建tcp监听失败，可能是由于端口占用或权限问题
        string err = (StrPrinter << "Listen on " << host << " " << port << " failed: " << get_uv_errmsg(true));
        throw std::runtime_error(err);
    }
    for (auto &pr: _cloned_server) {
        // 启动子Server, 监听相同的listen fd， epoll操作的accept是抢占式的
        pr.second->_socket->cloneSocket(*_socket);
    }

    InfoL << "TCP server listening on [" << host << "]: " << port;
}

void TcpServer::onManagerSession() {
    assert(_poller->isCurrentThread());

    onceToken token([&]() {
        _is_on_manager = true;
    }, [&]() {
        _is_on_manager = false;
    });

    for (auto &pr : _session_map) {
        //遍历时，可能触发onErr事件(也会操作_session_map)
        try {
            pr.second->session()->onManager();
        } catch (exception &ex) {
            WarnL << ex.what();
        }
    }
}

Socket::Ptr TcpServer::createSocket(const EventPoller::Ptr &poller) {
    return _on_create_socket(poller);
}

TcpServer::Ptr TcpServer::getServer(const EventPoller *poller) const {
    auto parent = _parent.lock();
    auto &ref = parent ? parent->_cloned_server : _cloned_server;
    auto it = ref.find(poller);
    if (it != ref.end()) {
        //派发到cloned server
        return it->second;
    }
    //派发到parent server
    return static_pointer_cast<TcpServer>(parent ? parent : const_cast<TcpServer *>(this)->shared_from_this());
}

Session::Ptr TcpServer::createSession(const Socket::Ptr &sock) {
    return getServer(sock->getPoller().get())->onAcceptConnection(sock);
}

} /* namespace turbo */