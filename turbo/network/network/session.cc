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

#include <atomic>
#include "turbo/network/network/session.h"


using namespace std;

namespace turbo {

class TcpSession : public Session {};
class UdpSession : public Session {};

StatisticImp(UdpSession)
StatisticImp(TcpSession)

Session::Session(const Socket::Ptr &sock) : SocketHelper(sock) {
    if (sock->sockType() == SockNum::Sock_TCP) {
        _statistic_tcp.reset(new ObjectStatistic<TcpSession>);
    } else {
        _statistic_udp.reset(new ObjectStatistic<UdpSession>);
    }
}

string Session::getIdentifier() const {
    if (_id.empty()) {
        static atomic<uint64_t> s_session_index{0};
        _id = to_string(++s_session_index) + '-' + to_string(getSock()->rawFD());
    }
    return _id;
}

}// namespace turbo