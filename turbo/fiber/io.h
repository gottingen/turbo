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
//
// Created by jeff on 24-1-4.
//

#ifndef TURBO_FIBER_IO_H_
#define TURBO_FIBER_IO_H_

#include <pthread.h>
#include <sys/socket.h>
#include "turbo/platform/poll.h"
#include "turbo/fiber/internal/types.h"
#include "turbo/base/status.h"

namespace turbo {
    /**
     * @ingroup turbo_fiber
     * @brief fiber wait for fd events
     * @param fd
     * @param buf
     * @param count
     * @return
     */
    turbo::Status fiber_fd_wait(int fd, unsigned events);

    /**
     * @ingroup turbo_fiber
     * @brief fiber wait for fd events
     * @param fd
     * @param buf
     * @param count
     * @return
     */
    turbo::Status fiber_fd_timedwait(int fd, unsigned epoll_events,
                                  const timespec *abstime);
    /**
     * @ingroup turbo_fiber
     * @brief fiber close fd
     * @param fd
     * @param buf
     * @param count
     * @return
     */
    turbo::Status fiber_fd_close(int fd);

    /**
     * @ingroup turbo_fiber
     * @brief fiber connect to server
     * @param fd
     * @param buf
     * @param count
     * @return
     */
    turbo::Status fiber_connect(int sockfd, const sockaddr *serv_addr,
                             socklen_t addrlen);
}  // namespace turbo
#endif  // TURBO_FIBER_IO_H_
