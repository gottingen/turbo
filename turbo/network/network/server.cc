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

#include "turbo/network/network/server.h"


using namespace std;

namespace turbo {

Server::Server(EventPoller::Ptr poller) {
    _poller = poller ? std::move(poller) : EventPollerPool::Instance().getPoller();
}

////////////////////////////////////////////////////////////////////////////////////

SessionHelper::SessionHelper(const std::weak_ptr<Server> &server, Session::Ptr session, std::string cls) {
    _server = server;
    _session = std::move(session);
    _cls = std::move(cls);
    //记录session至全局的map，方便后面管理
    _session_map = SessionMap::Instance().shared_from_this();
    _identifier = _session->getIdentifier();
    _session_map->add(_identifier, _session);
}

SessionHelper::~SessionHelper() {
    if (!_server.lock()) {
        //务必通知Session已从TcpServer脱离
        _session->onError(SockException(ErrCode::Err_other, "Server shutdown"));
    }
    //从全局map移除相关记录
    _session_map->del(_identifier);
}

const Session::Ptr &SessionHelper::session() const {
    return _session;
}

const std::string &SessionHelper::className() const {
    return _cls;
}

////////////////////////////////////////////////////////////////////////////////////

bool SessionMap::add(const string &tag, const Session::Ptr &session) {
    lock_guard<mutex> lck(_mtx_session);
    return _map_session.emplace(tag, session).second;
}

bool SessionMap::del(const string &tag) {
    lock_guard<mutex> lck(_mtx_session);
    return _map_session.erase(tag);
}

Session::Ptr SessionMap::get(const string &tag) {
    lock_guard<mutex> lck(_mtx_session);
    auto it = _map_session.find(tag);
    if (it == _map_session.end()) {
        return nullptr;
    }
    return it->second.lock();
}

void SessionMap::for_each_session(const function<void(const string &id, const Session::Ptr &session)> &cb) {
    lock_guard<mutex> lck(_mtx_session);
    for (auto it = _map_session.begin(); it != _map_session.end();) {
        auto session = it->second.lock();
        if (!session) {
            it = _map_session.erase(it);
            continue;
        }
        cb(it->first, session);
        ++it;
    }
}

} // namespace turbo