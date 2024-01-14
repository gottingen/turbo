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
// Created by jeff on 23-12-16.
//

#include <new>                                   // std::nothrow
#include <sys/poll.h>                            // poll()
#include <atomic>
#include "turbo/platform/port.h"
#include "turbo/times/time.h"
#include "turbo/system/io.h"                     // make_non_blocking
#include "turbo/log/logging.h"
#include "turbo/hash/hash.h"   // fmix32
#include "turbo/fiber/internal/waitable_event.h"
#include "turbo/fiber/internal/fiber_worker.h"                  // FiberWorker
#include "turbo/fiber/internal/fiber.h"                             // fiber_start_urgent
#include "turbo/fiber/io.h"
#include "turbo/status/error.h"

// Implement fiber functions on file descriptors

namespace turbo::fiber_internal {

    extern TURBO_THREAD_LOCAL FiberWorker *tls_task_group;

    template<typename T, size_t NBLOCK, size_t BLOCK_SIZE>
    class LazyArray {
        struct Block {
            std::atomic<T> items[BLOCK_SIZE];
        };

    public:
        LazyArray() {
            memset(static_cast<void *>(_blocks), 0, sizeof(std::atomic<Block *>) * NBLOCK);
        }

        std::atomic<T> *get_or_new(size_t index) {
            const size_t block_index = index / BLOCK_SIZE;
            if (block_index >= NBLOCK) {
                return nullptr;
            }
            const size_t block_offset = index - block_index * BLOCK_SIZE;
            Block *b = _blocks[block_index].load(std::memory_order_consume);
            if (b != nullptr) {
                return b->items + block_offset;
            }
            b = new(std::nothrow) Block;
            if (nullptr == b) {
                b = _blocks[block_index].load(std::memory_order_consume);
                return (b ? b->items + block_offset : nullptr);
            }
            // Set items to default value of T.
            std::fill(b->items, b->items + BLOCK_SIZE, T());
            Block *expected = nullptr;
            if (_blocks[block_index].compare_exchange_strong(
                    expected, b, std::memory_order_release,
                    std::memory_order_consume)) {
                return b->items + block_offset;
            }
            delete b;
            return expected->items + block_offset;
        }

        std::atomic<T> *get(size_t index) const {
            const size_t block_index = index / BLOCK_SIZE;
            if (TURBO_LIKELY(block_index < NBLOCK)) {
                const size_t block_offset = index - block_index * BLOCK_SIZE;
                Block *const b = _blocks[block_index].load(std::memory_order_consume);
                if (TURBO_LIKELY(b != nullptr)) {
                    return b->items + block_offset;
                }
            }
            return nullptr;
        }

    private:
        std::atomic<Block *> _blocks[NBLOCK];
    };

    typedef std::atomic<int> EpollFutex;

    static EpollFutex *const CLOSING_GUARD = (EpollFutex *) (intptr_t) -1L;

#ifndef NDEBUG
    std::atomic<int> break_nums{0};
#endif

// Able to address 67108864 file descriptors, should be enough.
    LazyArray<EpollFutex *, 262144/*NBLOCK*/, 256/*BLOCK_SIZE*/> fd_futexes;

    static const int FIBER_DEFAULT_EPOLL_SIZE = 65536;

    class EpollThread {
    public:
        EpollThread()
                : _epfd(-1), _stop(false), _tid(0) {
        }

        int start(int epoll_size) {
            if (started()) {
                return -1;
            }
            _start_mutex.lock();
            // Double check
            if (started()) {
                _start_mutex.unlock();
                return -1;
            }
#if defined(TURBO_PLATFORM_LINUX)
            _epfd = epoll_create(epoll_size);
#elif defined(TURBO_PLATFORM_OSX)
            _epfd = kqueue();
#endif
            _start_mutex.unlock();
            if (_epfd < 0) {
                TLOG_CRITICAL("Fail to epoll_create/kqueue");
                return -1;
            }
            if (!fiber_start_background(
                    &_tid, nullptr, EpollThread::run_this, this).ok()) {
                close(_epfd);
                _epfd = -1;
                TLOG_CRITICAL("Fail to create epoll fiber");
                return -1;
            }
            return 0;
        }

        // Note: This function does not wake up suspended fd_wait. This is fine
        // since stop_and_join is only called on program's termination
        // (g_task_control.stop()), suspended fibers do not block quit of
        // worker pthreads and completion of g_task_control.stop().
        int stop_and_join() {
            if (!started()) {
                return 0;
            }
            // No matter what this function returns, _epfd will be set to -1
            // (making started() false) to avoid latter stop_and_join() to
            // enter again.
            const int saved_epfd = _epfd;
            _epfd = -1;

            // epoll_wait cannot be woken up by closing _epfd. We wake up
            // epoll_wait by inserting a fd continuously triggering EPOLLOUT.
            // Visibility of _stop: constant EPOLLOUT forces epoll_wait to see
            // _stop (to be true) finally.
            _stop = true;
            int closing_epoll_pipe[2];
            if (pipe(closing_epoll_pipe)) {
                TLOG_CRITICAL("Fail to create closing_epoll_pipe");
                return -1;
            }
#if defined(TURBO_PLATFORM_LINUX)
            epoll_event evt = {EPOLLOUT, {nullptr}};
            if (epoll_ctl(saved_epfd, EPOLL_CTL_ADD,
                          closing_epoll_pipe[1], &evt) < 0) {
#elif defined(TURBO_PLATFORM_OSX)
                struct kevent kqueue_event;
                EV_SET(&kqueue_event, closing_epoll_pipe[1], EVFILT_WRITE, EV_ADD | EV_ENABLE,
                       0, 0, nullptr);
                if (kevent(saved_epfd, &kqueue_event, 1, nullptr, 0, nullptr) < 0) {
#endif
                TLOG_CRITICAL("Fail to add closing_epoll_pipe into epfd={}", saved_epfd);
                return -1;
            }

            const auto rc = fiber_join(_tid, nullptr);
            if (rc.ok()) {
                TLOG_CRITICAL("Fail to join EpollThread");
                return -1;
            }
            close(closing_epoll_pipe[0]);
            close(closing_epoll_pipe[1]);
            close(saved_epfd);
            return 0;
        }

        turbo::Status fd_wait(int fd, unsigned events, const timespec *abstime) {
            std::atomic<EpollFutex *> *p = fd_futexes.get_or_new(fd);
            if (nullptr == p) {
                errno = ENOMEM;
                return make_status();
            }

            EpollFutex *futex = p->load(std::memory_order_consume);
            if (nullptr == futex) {
                // It is rare to wait on one file descriptor from multiple threads
                // simultaneously. Creating singleton by optimistic locking here
                // saves mutexes for each futex.
                futex = waitable_event_create_checked<EpollFutex>();
                futex->store(0, std::memory_order_relaxed);
                EpollFutex *expected = nullptr;
                if (!p->compare_exchange_strong(expected, futex,
                                                std::memory_order_release,
                                                std::memory_order_consume)) {
                    waitable_event_destroy(futex);
                    futex = expected;
                }
            }

            while (futex == CLOSING_GUARD) {  // fiber_fd_close() is running.
                if (sched_yield() < 0) {
                    return make_status();
                }
                futex = p->load(std::memory_order_consume);
            }
            // Save value of futex before adding to epoll because the futex may
            // be changed before waitable_event_wait. No memory fence because EPOLL_CTL_MOD
            // and EPOLL_CTL_ADD shall have release fence.
            const int expected_val = futex->load(std::memory_order_relaxed);

#if defined(TURBO_PLATFORM_LINUX)
            epoll_event evt;
            evt.events = events;
            evt.data.fd = fd;
            if (epoll_ctl(_epfd, EPOLL_CTL_ADD, fd, &evt) < 0 &&
                errno != EEXIST) {
                TLOG_CRITICAL("Fail to add fd={} into epfd={} errno={}", fd, _epfd, errno);
                return make_status();
            }
#elif defined(TURBO_PLATFORM_OSX)
            struct kevent kqueue_event;
            EV_SET(&kqueue_event, fd, events, EV_ADD | EV_ENABLE | EV_ONESHOT,
                   0, 0, futex);
            if (kevent(_epfd, &kqueue_event, 1, nullptr, 0, nullptr) < 0) {
                TLOG_CRITICAL("Fail to add fd={} into kqueuefd={}", fd, _epfd);
                return make_status();
            }
#endif
            auto rs = waitable_event_wait(futex, expected_val, abstime);
            if (!rs.ok() && !is_unavailable(rs)) {
                return rs;
            }
            return turbo::ok_status();
        }

        turbo::Status fd_close(int fd) {
            if (fd < 0) {
                // what close(-1) returns
                errno = EBADF;
                return make_status();
            }
            std::atomic<EpollFutex *> *pfutex = turbo::fiber_internal::fd_futexes.get(fd);
            if (nullptr == pfutex) {
                // Did not call fiber_fd functions, close directly.
                return close(fd) ? make_status() : turbo::ok_status();
            }
            EpollFutex *futex = pfutex->exchange(
                    CLOSING_GUARD, std::memory_order_relaxed);
            if (futex == CLOSING_GUARD) {
                // concurrent double close detected.
                errno = EBADF;
                return make_status();
            }
            if (futex != nullptr) {
                futex->fetch_add(1, std::memory_order_relaxed);
                waitable_event_wake_all(futex);
            }
#if defined(TURBO_PLATFORM_LINUX)
            epoll_ctl(_epfd, EPOLL_CTL_DEL, fd, nullptr);
#elif defined(TURBO_PLATFORM_OSX)
            struct kevent evt;
            EV_SET(&evt, fd, EVFILT_WRITE, EV_DELETE, 0, 0, nullptr);
            kevent(_epfd, &evt, 1, nullptr, 0, nullptr);
            EV_SET(&evt, fd, EVFILT_READ, EV_DELETE, 0, 0, nullptr);
            kevent(_epfd, &evt, 1, nullptr, 0, nullptr);
#endif
            const int rc = close(fd);
            pfutex->exchange(futex, std::memory_order_relaxed);
            return rc == 0 ? turbo::ok_status() : make_status();
        }

        bool started() const {
            return _epfd >= 0;
        }

    private:
        static void *run_this(void *arg) {
            return static_cast<EpollThread *>(arg)->run();
        }

        void *run() {
            const int initial_epfd = _epfd;
            const size_t MAX_EVENTS = 32;
#if defined(TURBO_PLATFORM_LINUX)
            epoll_event *e = new(std::nothrow) epoll_event[MAX_EVENTS];
#elif defined(TURBO_PLATFORM_OSX)
            typedef struct kevent KEVENT;
            struct kevent *e = new(std::nothrow) KEVENT[MAX_EVENTS];
#endif
            if (nullptr == e) {
                TLOG_CRITICAL("Fail to new epoll_event");
                return nullptr;
            }

#if defined(TURBO_PLATFORM_LINUX)
            TDLOG_INFO("Use DEL+ADD instead of EPOLLONESHOT+MOD due to kernel bug. Performance will be much lower.");
#endif
            while (!_stop) {
                const int epfd = _epfd;
#if defined(TURBO_PLATFORM_LINUX)
                const int n = epoll_wait(epfd, e, MAX_EVENTS, -1);
#elif defined(TURBO_PLATFORM_OSX)
                const int n = kevent(epfd, nullptr, 0, e, MAX_EVENTS, nullptr);
#endif
                if (_stop) {
                    break;
                }

                if (n < 0) {
                    if (errno == EINTR) {
#ifndef NDEBUG
                        break_nums.fetch_add(1, std::memory_order_relaxed);
                        TLOG_CRITICAL("Fail to epoll epfd={} errno: {}", epfd, errno);
#endif
                        continue;
                    }

                    TDLOG_INFO("Fail to epoll epfd={}", epfd);
                    break;
                }

#if defined(TURBO_PLATFORM_LINUX)
                for (int i = 0; i < n; ++i) {
                    epoll_ctl(epfd, EPOLL_CTL_DEL, e[i].data.fd, nullptr);
                }
#endif
                for (int i = 0; i < n; ++i) {
#if defined(TURBO_PLATFORM_LINUX)
                    std::atomic<EpollFutex *> *pfutex = fd_futexes.get(e[i].data.fd);
                    EpollFutex *futex = pfutex ?
                                        pfutex->load(std::memory_order_consume) : nullptr;
#elif defined(TURBO_PLATFORM_OSX)
                    EpollFutex *futex = static_cast<EpollFutex *>(e[i].udata);
#endif
                    if (futex != nullptr && futex != CLOSING_GUARD) {
                        futex->fetch_add(1, std::memory_order_relaxed);
                        waitable_event_wake_all(futex);
                    }
                }
            }

            delete[] e;
            TDLOG_INFO("EpollThread={} epfd= {} is about to stop", initial_epfd, _tid);
            return nullptr;
        }

        int _epfd;
        bool _stop;
        fiber_id_t _tid;
        std::mutex _start_mutex;
    };

    EpollThread epoll_thread[turbo::FiberConfig::FIBER_EPOLL_THREAD_NUM];

    static inline EpollThread &get_epoll_thread(int fd) {
        if (turbo::FiberConfig::FIBER_EPOLL_THREAD_NUM == 1UL) {
            EpollThread &et = epoll_thread[0];
            et.start(FIBER_DEFAULT_EPOLL_SIZE);
            return et;
        }

        EpollThread &et = epoll_thread[turbo::hash_mixer4(fd) % turbo::FiberConfig::FIBER_EPOLL_THREAD_NUM];
        et.start(FIBER_DEFAULT_EPOLL_SIZE);
        return et;
    }

    int stop_and_join_epoll_threads() {
        // Returns -1 if any epoll thread failed to stop.
        int rc = 0;
        for (size_t i = 0; i < turbo::FiberConfig::FIBER_EPOLL_THREAD_NUM; ++i) {
            if (epoll_thread[i].stop_and_join() < 0) {
                rc = -1;
            }
        }
        return rc;
    }

#if defined(TURBO_PLATFORM_LINUX)

    short epoll_to_poll_events(uint32_t epoll_events) {
        // Most POLL* and EPOLL* are same values.
        short poll_events = (epoll_events &
                             (EPOLLIN | EPOLLPRI | EPOLLOUT |
                              EPOLLRDNORM | EPOLLRDBAND |
                              EPOLLWRNORM | EPOLLWRBAND |
                              EPOLLMSG | EPOLLERR | EPOLLHUP));
        // nocheck always not equal
        //  TURBO_CHECK_EQ((uint32_t)poll_events, epoll_events);
        return poll_events;
    }

#elif defined(TURBO_PLATFORM_OSX)

    static short kqueue_to_poll_events(int kqueue_events) {
        //TODO: add more values?
        short poll_events = 0;
        if (kqueue_events == EVFILT_READ) {
            poll_events |= POLLIN;
        }
        if (kqueue_events == EVFILT_WRITE) {
            poll_events |= POLLOUT;
        }
        return poll_events;
    }

#endif

    // For pthreads.
    turbo::Status pthread_fd_wait(int fd, unsigned events,
                        const timespec *abstime) {
        int diff_ms = -1;
        if (abstime) {
            int64_t now_us = turbo::get_current_time_micros();
            int64_t abstime_us = turbo::Time::from_timespec(*abstime).to_microseconds();
            if (abstime_us <= now_us) {
                errno = ETIMEDOUT;
                return make_status();
            }
            diff_ms = (abstime_us - now_us + 999L) / 1000L;
        }
#if defined(TURBO_PLATFORM_LINUX)
        const short poll_events = turbo::fiber_internal::epoll_to_poll_events(events);
#elif defined(TURBO_PLATFORM_OSX)
        const short poll_events = turbo::fiber_internal::kqueue_to_poll_events(events);
#endif
        if (poll_events == 0) {
            errno = EINVAL;
            return make_status();
        }
        pollfd ufds = {fd, poll_events, 0};
        const int rc = poll(&ufds, 1, diff_ms);
        if (rc < 0) {
            return make_status();
        }
        if (rc == 0) {
            errno = ETIMEDOUT;
            return make_status();
        }
        if (ufds.revents & POLLNVAL) {
            errno = EBADF;
            return make_status();
        }
        return turbo::ok_status();
    }

}  // namespace turbo::fiber_internal

namespace turbo {

    turbo::Status fiber_fd_wait(int fd, unsigned events) {
        if (fd < 0) {
            errno = EINVAL;
            return make_status();
        }
        turbo::fiber_internal::FiberWorker *g = turbo::fiber_internal::tls_task_group;
        if (nullptr != g && !g->is_current_pthread_task()) {
            return turbo::fiber_internal::get_epoll_thread(fd).fd_wait(fd, events, nullptr);
        }
        return turbo::fiber_internal::pthread_fd_wait(fd, events, nullptr);
    }

    turbo::Status fiber_fd_timedwait(int fd, unsigned events,
                           const timespec *abstime) {
        if (nullptr == abstime) {
            return fiber_fd_wait(fd, events);
        }
        if (fd < 0) {
            errno = EINVAL;
            return make_status();
        }
        turbo::fiber_internal::FiberWorker *g = turbo::fiber_internal::tls_task_group;
        if (nullptr != g && !g->is_current_pthread_task()) {
            return turbo::fiber_internal::get_epoll_thread(fd).fd_wait(
                    fd, events, abstime);
        }
        return turbo::fiber_internal::pthread_fd_wait(fd, events, abstime);
    }

    turbo::Status fiber_connect(int sockfd, const sockaddr *serv_addr,
                      socklen_t addrlen) {
        turbo::fiber_internal::FiberWorker *g = turbo::fiber_internal::tls_task_group;
        if (nullptr == g || g->is_current_pthread_task()) {
            auto r = ::connect(sockfd, serv_addr, addrlen);
            return  r == 0 ?turbo::ok_status() : make_status();
        }
        // FIXME: Scoped non-blocking?
        turbo::make_non_blocking(sockfd);
        const int rc = connect(sockfd, serv_addr, addrlen);
        if (rc == 0 || errno != EINPROGRESS) {
            return make_status();
        }
#if defined(TURBO_PLATFORM_LINUX)
        if (!fiber_fd_wait(sockfd, EPOLLOUT).ok()) {
#elif defined(TURBO_PLATFORM_OSX)
            if (fiber_fd_wait(sockfd, EVFILT_WRITE) < 0) {
#endif
            return make_status();
        }
        int err;
        socklen_t errlen = sizeof(err);
        if (getsockopt(sockfd, SOL_SOCKET, SO_ERROR, &err, &errlen) < 0) {
            TLOG_CRITICAL("Fail to getsockopt");
            return make_status();
        }
        if (err != 0) {
            TDLOG_CHECK(err != EINPROGRESS);
            errno = err;
            return make_status();
        }
        return turbo::ok_status();
    }

    // This does not wake pthreads calling fiber_fd_*wait.
    turbo::Status fiber_fd_close(int fd) {
        return turbo::fiber_internal::get_epoll_thread(fd).fd_close(fd);
    }

}  // namespace turbo
