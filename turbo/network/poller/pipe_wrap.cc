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

#include <stdexcept>
#include "turbo/network/poller/pipe_wrap.h"
#include "turbo/network/util/util.h"
#include "turbo/network/util/uv_errno.h"
#include "turbo/network/network/sock_util.h"


using namespace std;

#define checkFD(fd) \
    if (fd == -1) { \
        clearFD(); \
        throw runtime_error(StrPrinter << "Create windows pipe failed: " << get_uv_errmsg());\
    }

#define closeFD(fd) \
    if (fd != -1) { \
        close(fd);\
        fd = -1;\
    }

namespace turbo {

PipeWrap::PipeWrap() {
    reOpen();
}

void PipeWrap::reOpen() {
    clearFD();
#if defined(_WIN32)
    const char *localip = SockUtil::support_ipv6() ? "::1" : "127.0.0.1";
    auto listener_fd = SockUtil::listen(0, localip);
    checkFD(listener_fd)
    SockUtil::setNoBlocked(listener_fd,false);
    auto localPort = SockUtil::get_local_port(listener_fd);
    _pipe_fd[1] = SockUtil::connect(localip, localPort,false);
    checkFD(_pipe_fd[1])
    _pipe_fd[0] = (int)accept(listener_fd, nullptr, nullptr);
    checkFD(_pipe_fd[0])
    SockUtil::setNoDelay(_pipe_fd[0]);
    SockUtil::setNoDelay(_pipe_fd[1]);
    close(listener_fd);
#else
    if (pipe(_pipe_fd) == -1) {
        throw runtime_error(StrPrinter << "Create posix pipe failed: " << get_uv_errmsg());
    }
#endif // defined(_WIN32)
    SockUtil::setNoBlocked(_pipe_fd[0], true);
    SockUtil::setNoBlocked(_pipe_fd[1], false);
    SockUtil::setCloExec(_pipe_fd[0]);
    SockUtil::setCloExec(_pipe_fd[1]);
}

void PipeWrap::clearFD() {
    closeFD(_pipe_fd[0]);
    closeFD(_pipe_fd[1]);
}

PipeWrap::~PipeWrap() {
    clearFD();
}

int PipeWrap::write(const void *buf, int n) {
    int ret;
    do {
#if defined(_WIN32)
        ret = send(_pipe_fd[1], (char *)buf, n, 0);
#else
        ret = ::write(_pipe_fd[1], buf, n);
#endif // defined(_WIN32)
    } while (-1 == ret && UV_EINTR == get_uv_error(true));
    return ret;
}

int PipeWrap::read(void *buf, int n) {
    int ret;
    do {
#if defined(_WIN32)
        ret = recv(_pipe_fd[0], (char *)buf, n, 0);
#else
        ret = ::read(_pipe_fd[0], buf, n);
#endif // defined(_WIN32)
    } while (-1 == ret && UV_EINTR == get_uv_error(true));
    return ret;
}

} /* namespace turbo*/