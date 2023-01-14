// Licensed to the Apache Software Foundation (ASF) under one
// or more contributor license agreements.  See the NOTICE file
// distributed with this work for additional information
// regarding copyright ownership.  The ASF licenses this file
// to you under the Apache License, Version 2.0 (the
// "License"); you may not use this file except in compliance
// with the License.  You may obtain a copy of the License at
//
//   http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing,
// software distributed under the License is distributed on an
// "AS IS" BASIS, WITHOUT WARRANTIES OR CONDITIONS OF ANY
// KIND, either express or implied.  See the License for the
// specific language governing permissions and limitations
// under the License.

// fiber - A M:N threading library to make applications more concurrent.

// Date: Thu Aug  7 18:56:27 CST 2014

#include "turbo/base/compat.h"
#include <new>                                   // std::nothrow
#include <sys/poll.h>                            // poll()

#if defined(TURBO_PLATFORM_OSX)

#include <sys/types.h>                           // struct kevent
#include <sys/event.h>                           // kevent(), kqueue()

#endif

#include "turbo/base/static_atomic.h"
#include "turbo/times/time.h"
#include "turbo/base/fd_utility.h"                     // make_non_blocking
#include "turbo/log/logging.h"
#include "turbo/hash/murmurhash3.h"   // fmix32
#include "turbo/fiber/internal/waitable_event.h"                       // butex_*
#include "turbo/fiber/internal/fiber_worker.h"                  // fiber_worker
#include "turbo/fiber/internal/fiber.h"                             // fiber_start_urgent

// Implement fiber functions on file descriptors

namespace turbo::fiber_internal {

    extern TURBO_THREAD_LOCAL fiber_worker *tls_task_group;

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
            if (__builtin_expect(block_index < NBLOCK, 1)) {
                const size_t block_offset = index - block_index * BLOCK_SIZE;
                Block *const b = _blocks[block_index].load(std::memory_order_consume);
                if (__builtin_expect(b != nullptr, 1)) {
                    return b->items + block_offset;
                }
            }
            return nullptr;
        }

    private:
        std::atomic<Block *> _blocks[NBLOCK];
    };

    typedef std::atomic<int> EpollButex;

    static EpollButex *const CLOSING_GUARD = (EpollButex *) (intptr_t) -1L;

#ifndef NDEBUG
    turbo::static_atomic<int> break_nums = TURBO_STATIC_ATOMIC_INIT(0);
#endif

// Able to address 67108864 file descriptors, should be enough.
    LazyArray<EpollButex *, 262144/*NBLOCK*/, 256/*BLOCK_SIZE*/> fd_butexes;

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
                TURBO_PLOG(FATAL) << "Fail to epoll_create/kqueue";
                return -1;
            }
            if (fiber_start_background(
                    &_tid, nullptr, EpollThread::run_this, this) != 0) {
                close(_epfd);
                _epfd = -1;
                TURBO_LOG(FATAL) << "Fail to create epoll fiber";
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
                TURBO_PLOG(FATAL) << "Fail to create closing_epoll_pipe";
                return -1;
            }
#if defined(TURBO_PLATFORM_LINUX)
            epoll_event evt = { EPOLLOUT, { nullptr } };
            if (epoll_ctl(saved_epfd, EPOLL_CTL_ADD,
                          closing_epoll_pipe[1], &evt) < 0) {
#elif defined(TURBO_PLATFORM_OSX)
            struct kevent kqueue_event;
            EV_SET(&kqueue_event, closing_epoll_pipe[1], EVFILT_WRITE, EV_ADD | EV_ENABLE,
                   0, 0, nullptr);
            if (kevent(saved_epfd, &kqueue_event, 1, nullptr, 0, nullptr) < 0) {
#endif
                TURBO_PLOG(FATAL) << "Fail to add closing_epoll_pipe into epfd="
                            << saved_epfd;
                return -1;
            }

            const int rc = fiber_join(_tid, nullptr);
            if (rc) {
                TURBO_LOG(FATAL) << "Fail to join EpollThread, " << turbo_error(rc);
                return -1;
            }
            close(closing_epoll_pipe[0]);
            close(closing_epoll_pipe[1]);
            close(saved_epfd);
            return 0;
        }

        int fd_wait(int fd, unsigned events, const timespec *abstime) {
            std::atomic<EpollButex *> *p = fd_butexes.get_or_new(fd);
            if (nullptr == p) {
                errno = ENOMEM;
                return -1;
            }

            EpollButex *butex = p->load(std::memory_order_consume);
            if (nullptr == butex) {
                // It is rare to wait on one file descriptor from multiple threads
                // simultaneously. Creating singleton by optimistic locking here
                // saves mutexes for each butex.
                butex = waitable_event_create_checked<EpollButex>();
                butex->store(0, std::memory_order_relaxed);
                EpollButex *expected = nullptr;
                if (!p->compare_exchange_strong(expected, butex,
                                                std::memory_order_release,
                                                std::memory_order_consume)) {
                    waitable_event_destroy(butex);
                    butex = expected;
                }
            }

            while (butex == CLOSING_GUARD) {  // fiber_fd_close() is running.
                if (sched_yield() < 0) {
                    return -1;
                }
                butex = p->load(std::memory_order_consume);
            }
            // Save value of butex before adding to epoll because the butex may
            // be changed before waitable_event_wait. No memory fence because EPOLL_CTL_MOD
            // and EPOLL_CTL_ADD shall have release fence.
            const int expected_val = butex->load(std::memory_order_relaxed);

#if defined(TURBO_PLATFORM_LINUX)
            epoll_event evt;
            evt.events = events;
            evt.data.fd = fd;
            if (epoll_ctl(_epfd, EPOLL_CTL_ADD, fd, &evt) < 0 &&
                errno != EEXIST) {
                TURBO_PLOG(FATAL) << "Fail to add fd=" << fd << " into epfd=" << _epfd;
                return -1;
            }
#elif defined(TURBO_PLATFORM_OSX)
            struct kevent kqueue_event;
            EV_SET(&kqueue_event, fd, events, EV_ADD | EV_ENABLE | EV_ONESHOT,
                   0, 0, butex);
            if (kevent(_epfd, &kqueue_event, 1, nullptr, 0, nullptr) < 0) {
                TURBO_PLOG(FATAL) << "Fail to add fd=" << fd << " into kqueuefd=" << _epfd;
                return -1;
            }
#endif
            if (waitable_event_wait(butex, expected_val, abstime) < 0 &&
                errno != EWOULDBLOCK && errno != EINTR) {
                return -1;
            }
            return 0;
        }

        int fd_close(int fd) {
            if (fd < 0) {
                // what close(-1) returns
                errno = EBADF;
                return -1;
            }
            std::atomic<EpollButex *> *pbutex = turbo::fiber_internal::fd_butexes.get(fd);
            if (nullptr == pbutex) {
                // Did not call fiber_fd functions, close directly.
                return close(fd);
            }
            EpollButex *butex = pbutex->exchange(
                    CLOSING_GUARD, std::memory_order_relaxed);
            if (butex == CLOSING_GUARD) {
                // concurrent double close detected.
                errno = EBADF;
                return -1;
            }
            if (butex != nullptr) {
                butex->fetch_add(1, std::memory_order_relaxed);
                waitable_event_wake_all(butex);
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
            pbutex->exchange(butex, std::memory_order_relaxed);
            return rc;
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
            epoll_event* e = new (std::nothrow) epoll_event[MAX_EVENTS];
#elif defined(TURBO_PLATFORM_OSX)
            typedef struct kevent KEVENT;
            struct kevent *e = new(std::nothrow) KEVENT[MAX_EVENTS];
#endif
            if (nullptr == e) {
                TURBO_LOG(FATAL) << "Fail to new epoll_event";
                return nullptr;
            }

#if defined(TURBO_PLATFORM_LINUX)
            TURBO_DLOG(INFO) << "Use DEL+ADD instead of EPOLLONESHOT+MOD due to kernel bug. Performance will be much lower.";
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
                        int* p = &errno;
                        const char* b = turbo_error();
                        const char* b2 = turbo_error(errno);
                        TURBO_DLOG(FATAL) << "Fail to epoll epfd=" << epfd << ", "
                                    << errno << " " << p << " " <<  b << " " <<  b2;
#endif
                        continue;
                    }

                    TURBO_PLOG(INFO) << "Fail to epoll epfd=" << epfd;
                    break;
                }

#if defined(TURBO_PLATFORM_LINUX)
                for (int i = 0; i < n; ++i) {
                    epoll_ctl(epfd, EPOLL_CTL_DEL, e[i].data.fd, nullptr);
                }
#endif
                for (int i = 0; i < n; ++i) {
#if defined(TURBO_PLATFORM_LINUX)
                    std::atomic<EpollButex*>* pbutex = fd_butexes.get(e[i].data.fd);
                    EpollButex* butex = pbutex ?
                        pbutex->load(std::memory_order_consume) : nullptr;
#elif defined(TURBO_PLATFORM_OSX)
                    EpollButex *butex = static_cast<EpollButex *>(e[i].udata);
#endif
                    if (butex != nullptr && butex != CLOSING_GUARD) {
                        butex->fetch_add(1, std::memory_order_relaxed);
                        waitable_event_wake_all(butex);
                    }
                }
            }

            delete[] e;
            TURBO_DLOG(INFO) << "EpollThread=" << _tid << "(epfd="
                       << initial_epfd << ") is about to stop";
            return nullptr;
        }

        int _epfd;
        bool _stop;
        fiber_id_t _tid;
        std::mutex _start_mutex;
    };

    EpollThread epoll_thread[FIBER_EPOLL_THREAD_NUM];

    static inline EpollThread &get_epoll_thread(int fd) {
        if (FIBER_EPOLL_THREAD_NUM == 1UL) {
            EpollThread &et = epoll_thread[0];
            et.start(FIBER_DEFAULT_EPOLL_SIZE);
            return et;
        }

        EpollThread &et = epoll_thread[turbo::hash::fmix32(fd) % FIBER_EPOLL_THREAD_NUM];
        et.start(FIBER_DEFAULT_EPOLL_SIZE);
        return et;
    }

//TODO(zhujiashun): change name
    int stop_and_join_epoll_threads() {
        // Returns -1 if any epoll thread failed to stop.
        int rc = 0;
        for (size_t i = 0; i < FIBER_EPOLL_THREAD_NUM; ++i) {
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
    int pthread_fd_wait(int fd, unsigned events,
                        const timespec *abstime) {
        int diff_ms = -1;
        if (abstime) {
            int64_t now_us = turbo::time_now().to_unix_micros();
            int64_t abstime_us = turbo::time_point::from_timespec(*abstime).to_unix_micros();
            if (abstime_us <= now_us) {
                errno = ETIMEDOUT;
                return -1;
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
            return -1;
        }
        pollfd ufds = {fd, poll_events, 0};
        const int rc = poll(&ufds, 1, diff_ms);
        if (rc < 0) {
            return -1;
        }
        if (rc == 0) {
            errno = ETIMEDOUT;
            return -1;
        }
        if (ufds.revents & POLLNVAL) {
            errno = EBADF;
            return -1;
        }
        return 0;
    }

}  // namespace turbo::fiber_internal

extern "C" {

int fiber_fd_wait(int fd, unsigned events) {
    if (fd < 0) {
        errno = EINVAL;
        return -1;
    }
    turbo::fiber_internal::fiber_worker *g = turbo::fiber_internal::tls_task_group;
    if (nullptr != g && !g->is_current_pthread_task()) {
        return turbo::fiber_internal::get_epoll_thread(fd).fd_wait(
                fd, events, nullptr);
    }
    return turbo::fiber_internal::pthread_fd_wait(fd, events, nullptr);
}

int fiber_fd_timedwait(int fd, unsigned events,
                       const timespec *abstime) {
    if (nullptr == abstime) {
        return fiber_fd_wait(fd, events);
    }
    if (fd < 0) {
        errno = EINVAL;
        return -1;
    }
    turbo::fiber_internal::fiber_worker *g = turbo::fiber_internal::tls_task_group;
    if (nullptr != g && !g->is_current_pthread_task()) {
        return turbo::fiber_internal::get_epoll_thread(fd).fd_wait(
                fd, events, abstime);
    }
    return turbo::fiber_internal::pthread_fd_wait(fd, events, abstime);
}

int fiber_connect(int sockfd, const sockaddr *serv_addr,
                  socklen_t addrlen) {
    turbo::fiber_internal::fiber_worker *g = turbo::fiber_internal::tls_task_group;
    if (nullptr == g || g->is_current_pthread_task()) {
        return ::connect(sockfd, serv_addr, addrlen);
    }
    // FIXME: Scoped non-blocking?
    turbo::base::make_non_blocking(sockfd);
    const int rc = connect(sockfd, serv_addr, addrlen);
    if (rc == 0 || errno != EINPROGRESS) {
        return rc;
    }
#if defined(TURBO_PLATFORM_LINUX)
    if (fiber_fd_wait(sockfd, EPOLLOUT) < 0) {
#elif defined(TURBO_PLATFORM_OSX)
    if (fiber_fd_wait(sockfd, EVFILT_WRITE) < 0) {
#endif
        return -1;
    }
    int err;
    socklen_t errlen = sizeof(err);
    if (getsockopt(sockfd, SOL_SOCKET, SO_ERROR, &err, &errlen) < 0) {
        TURBO_PLOG(FATAL) << "Fail to getsockopt";
        return -1;
    }
    if (err != 0) {
        TURBO_CHECK(err != EINPROGRESS);
        errno = err;
        return -1;
    }
    return 0;
}

// This does not wake pthreads calling fiber_fd_*wait.
int fiber_fd_close(int fd) {
    return turbo::fiber_internal::get_epoll_thread(fd).fd_close(fd);
}

}  // extern "C"
