// Copyright 2023 The Elastic-AI Authors.
// part of Elastic AI Search
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

#include "turbo/event/internal/epoll_poller.h"
#include "turbo/platform/port.h"
#if defined(TURBO_PLATFORM_LINUX)
#include <sys/epoll.h>
#include "turbo/log/logging.h"
#include <unistd.h>
#include <sys/eventfd.h>
#include <sys/timerfd.h>
#include <sys/signalfd.h>
#include <sys/socket.h>
#include <sys/epoll.h>

namespace turbo {

    EpollPoller::~EpollPoller() {
        destroy();
    }
    turbo::Status EpollPoller::initialize()  {
        _epfd = epoll_create(1024 * 1024);
        if (_epfd < 0) {
            auto rs = make_status();
            TLOG_CRITICAL("Fail to create epoll, {}", rs.to_string());
            return rs;
        }
        return turbo::ok_status();
    }

    turbo::Status EpollPoller::destroy() {
        if (_epfd >= 0) {
            ::close(_epfd);
            _epfd = -1;
        }
        return turbo::ok_status();
    }

    bool EpollPoller::valid() const {
        return _epfd >= 0;
    }

    turbo::Status EpollPoller::add_poll_in(EventChannelId socket_id, int fd) {
        if (_epfd < 0) {
            errno = EINVAL;
            return make_status();
        }
        epoll_event evt;
        evt.events = EPOLLIN | EPOLLET;
        evt.data.u64 = socket_id;
        auto rc = epoll_ctl(_epfd, EPOLL_CTL_ADD, fd, &evt);
        if (rc < 0) {
            return make_status();
        }
        return turbo::ok_status();
    }

    turbo::Status EpollPoller::add_poll_out(EventChannelId socket_id, int fd, bool pollin) {
        if (_epfd < 0) {
            errno = EINVAL;
            return make_status();
        }

        epoll_event evt;
        evt.data.u64 = socket_id;
        evt.events = EPOLLOUT | EPOLLET;
#ifdef BRPC_SOCKET_HAS_EOF
        evt.events |= has_epollrdhup;
#endif
        if (pollin) {
            evt.events |= EPOLLIN;
            if (epoll_ctl(_epfd, EPOLL_CTL_MOD, fd, &evt) < 0) {
                // This fd has been removed from epoll via `RemoveConsumer',
                // in which case errno will be ENOENT
                return make_status();
            }
        } else {
            if (epoll_ctl(_epfd, EPOLL_CTL_ADD, fd, &evt) < 0) {
                return make_status();
            }
        }
        return turbo::ok_status();
    }

    turbo::Status EpollPoller::remove_poll_out(EventChannelId socket_id, int fd, bool keep_pollin) {
        int rc;
        if (keep_pollin) {
            epoll_event evt;
            evt.data.u64 = socket_id;
            evt.events = EPOLLIN | EPOLLET;
            rc = epoll_ctl(_epfd, EPOLL_CTL_MOD, fd, &evt);
        } else {
            rc = epoll_ctl(_epfd, EPOLL_CTL_DEL, fd, NULL);
        }
        return rc < 0 ? make_status() : turbo::ok_status();
    }

    turbo::Status EpollPoller::remove_poll_in(int fd) {
        if (fd < 0) {
            return turbo::make_status(kEINVAL, "invalid fd");
        }
        // Removing the consumer from dispatcher before closing the fd because
        // if process was forked and the fd is not marked as close-on-exec,
        // closing does not set reference count of the fd to 0, thus does not
        // remove the fd from epoll. More badly, the fd will not be removable
        // from epoll again! If the fd was level-triggered and there's data left,
        // epoll_wait will keep returning events of the fd continuously, making
        // program abnormal.
        if (epoll_ctl(_epfd, EPOLL_CTL_DEL, fd, NULL) < 0) {
            TLOG_WARN("Fail to remove fd={} from epfd={}", fd, _epfd);
            return make_status();
        }
        return turbo::ok_status();
    }

    turbo::Status EpollPoller::poll(std::vector<PollResult> &poll_results, int timeout) {
        poll_results.clear();
        epoll_event e[32];
        const int n = epoll_wait(_epfd, e, TURBO_ARRAY_SIZE(e), timeout);
        if (n < 0) {
            if (EINTR == errno) {
                // We've checked _stop, no wake-up will be missed.
                return turbo::ok_status();
            }
            TLOG_CRITICAL("Fail to epoll_wait epfd={}", _epfd);
            return make_status();
        }
        for (int i = 0; i < n; ++i) {
            if (e[i].events & (EPOLLIN | EPOLLERR | EPOLLHUP)) {
                // We don't care about the return value.
                poll_results.emplace_back(PollResult{e[i].data.fd, static_cast<int>(e[i].events), e[i].data.u64});
            }
        }
        for (int i = 0; i < n; ++i) {
            if (e[i].events & (EPOLLOUT | EPOLLERR | EPOLLHUP)) {
                // We don't care about the return value.
                poll_results.emplace_back(PollResult{e[i].data.fd, static_cast<int>(e[i].events), e[i].data.u64});
            }
        }
        return turbo::ok_status();
    }
}  // namespace turbo
#endif // TURBO_PLATFORM_LINUX