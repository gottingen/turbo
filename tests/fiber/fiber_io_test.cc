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
// Created by jeff on 24-1-3.
//

#define DOCTEST_CONFIG_IMPLEMENT_WITH_MAIN
#include "turbo/testing/test.h"

#include <sys/types.h>
#include <sys/socket.h>
#include <sys/utsname.h>                           // uname
#include <fcntl.h>
#include <pthread.h>
#include "turbo/times/clock.h"
#include "turbo/times/stop_watcher.h"
#include "turbo/system/io.h"
#include "turbo/system/threading.h"
#include "turbo/log/logging.h"
#include "turbo/fiber/internal/schedule_group.h"
#include "turbo/fiber/internal/fiber_worker.h"
#include "turbo/fiber/internal/fiber.h"
#include "turbo/fiber/io.h"
#include "turbo/fiber/fiber.h"


#if defined(TURBO_PLATFORM_OSX)

#include <sys/types.h>                           // struct kevent
#include <sys/event.h>                           // kevent(), kqueue()

#endif

#ifndef NDEBUG
namespace turbo::fiber_internal {
    extern std::atomic<int> break_nums;
    extern ScheduleGroup *global_task_control;

    int stop_and_join_epoll_threads();
}
#endif

namespace turbo::fiber_internal {
    TEST_CASE("FDTest, read_kernel_version") {
        utsname name;
        uname(&name);
        std::cout << "sysname=" << name.sysname << std::endl
                  << "nodename=" << name.nodename << std::endl
                  << "release=" << name.release << std::endl
                  << "version=" << name.version << std::endl
                  << "machine=" << name.machine << std::endl;
    }

#define RUN_CLIENT_IN_FIBER 1
//#define USE_BLOCKING_EPOLL 1
//#define RUN_EPOLL_IN_FIBER 1
//#define CREATE_THREAD_TO_PROCESS 1

    volatile bool stop = false;

    struct SocketMeta {
        int fd;
        int epfd;
    };

    struct TURBO_CACHE_LINE_ALIGNED ClientMeta {
        int fd;
        size_t count;
        size_t times;
    };

    struct EpollMeta {
        int epfd;
    };

    const size_t NCLIENT = 30;

    void *process_thread(void *arg) {
        SocketMeta *m = (SocketMeta *) arg;
        size_t count;
        //printf("begin to process fd=%d\n", m->fd);
        ssize_t n = read(m->fd, &count, sizeof(count));
        if (n != sizeof(count)) {
            TLOG_CRITICAL("Should not happen in this test");
            return nullptr;
        }
        count += NCLIENT;
        //printf("write result=%lu to fd=%d\n", count, m->fd);
        if (write(m->fd, &count, sizeof(count)) != sizeof(count)) {
            TLOG_CRITICAL("Should not happen in this test");
            return nullptr;
        }
#ifdef CREATE_THREAD_TO_PROCESS
# if defined(TURBO_PLATFORM_LINUX)
        epoll_event evt = { EPOLLIN | EPOLLONESHOT, { m } };
        if (epoll_ctl(m->epfd, EPOLL_CTL_MOD, m->fd, &evt) < 0) {
            epoll_ctl(m->epfd, EPOLL_CTL_ADD, m->fd, &evt);
        }
# elif defined(TURBO_PLATFORM_OSX)
        struct kevent kqueue_event;
        EV_SET(&kqueue_event, m->fd, EVFILT_READ, EV_ADD | EV_ENABLE | EV_ONESHOT,
                0, 0, m);
        kevent(m->epfd, &kqueue_event, 1, nullptr, 0, nullptr);
# endif
#endif
        return nullptr;
    }

    void *epoll_thread(void *arg) {
        turbo::fiber_sleep_for(turbo::Duration::microseconds(1));
        EpollMeta *m = (EpollMeta *) arg;
        const int epfd = m->epfd;
#if defined(TURBO_PLATFORM_LINUX)
        epoll_event e[32];
#elif defined(TURBO_PLATFORM_OSX)
        struct kevent e[32];
#endif

        while (!stop) {

#if defined(TURBO_PLATFORM_LINUX)
# ifndef USE_BLOCKING_EPOLL
            const int n = epoll_wait(epfd, e, TURBO_ARRAY_SIZE(e), 0);
            if (stop) {
                break;
            }
            if (n == 0) {
                turbo::fiber_fd_wait(epfd, EPOLLIN);
                continue;
            }
# else
            const int n = epoll_wait(epfd, e, TURBO_ARRAY_SIZE(e), -1);
            if (stop) {
                break;
            }
            if (n == 0) {
                continue;
            }
# endif
#elif defined(TURBO_PLATFORM_OSX)
            const int n = kevent(epfd, nullptr, 0, e, TURBO_ARRAY_SIZE(e), nullptr);
            if (stop) {
                break;
            }
            if (n == 0) {
                continue;
            }
#endif
            if (n < 0) {
                if (EINTR == errno) {
                    continue;
                }
#if defined(TURBO_PLATFORM_LINUX)
                TLOG_CRITICAL("Fail to epoll_wait");
#elif defined(TURBO_PLATFORM_OSX)
                TURBO_PLOG(FATAL) << "Fail to kevent";
#endif
                break;
            }

#ifdef CREATE_THREAD_TO_PROCESS
            fiber_fvec vec[n];
            for (int i = 0; i < n; ++i) {
                vec[i].fn = process_thread;
# if defined(TURBO_PLATFORM_LINUX)
                vec[i].arg = e[i].data.ptr;
# elif defined(TURBO_PLATFORM_OSX)
                vec[i].arg = e[i].udata;
# endif
            }
            fiber_id_t tid[n];
            fiber_startv(tid, vec, n, &FIBER_ATTR_SMALL);
#else
            for (int i = 0; i < n; ++i) {
# if defined(TURBO_PLATFORM_LINUX)
                process_thread(e[i].data.ptr);
# elif defined(TURBO_PLATFORM_OSX)
                process_thread(e[i].udata);
# endif
            }
#endif
        }
        return nullptr;
    }

    void *client_thread(void *arg) {
        ClientMeta *m = (ClientMeta *) arg;
        for (size_t i = 0; i < m->times; ++i) {
            if (write(m->fd, &m->count, sizeof(m->count)) != sizeof(m->count)) {
                TLOG_CRITICAL("Should not happen in this test");
                return nullptr;
            }
#ifdef RUN_CLIENT_IN_FIBER
            ssize_t rc;
            do {
# if defined(TURBO_PLATFORM_LINUX)
                const auto wait_rc = turbo::fiber_fd_wait(m->fd, EPOLLIN);
# elif defined(TURBO_PLATFORM_OSX)
                const int wait_rc = fiber_fd_wait(m->fd, EVFILT_READ);
# endif
                REQUIRE(wait_rc.ok());
                rc = read(m->fd, &m->count, sizeof(m->count));
            } while (rc < 0 && errno == EAGAIN);
#else
            ssize_t rc = read(m->fd, &m->count, sizeof(m->count));
#endif
            if (rc != sizeof(m->count)) {
                TLOG_CRITICAL("Should not happen in this test {}", rc);
                return nullptr;
            }
        }
        return nullptr;
    }

    inline uint32_t fmix32(uint32_t h) {
        h ^= h >> 16;
        h *= 0x85ebca6b;
        h ^= h >> 13;
        h *= 0xc2b2ae35;
        h ^= h >> 16;
        return h;
    }

    // Disable temporarily due to epoll's bug. The bug is fixed by
    // a kernel patch that lots of machines currently don't have
    /*
    TEST_CASE("FDTest, ping_pong") {
#ifndef NDEBUG
        turbo::fiber_internal::break_nums = 0;
#endif

        const size_t REP = 30000;
        const size_t NEPOLL = 2;

        int epfd[NEPOLL];
#ifdef RUN_EPOLL_IN_FIBER
        fiber_id_t eth[NEPOLL];
#else
        pthread_t eth[NEPOLL];
#endif
        int fds[2 * NCLIENT];
#ifdef RUN_CLIENT_IN_FIBER
        fiber_id_t cth[NCLIENT];
#else
        pthread_t cth[NCLIENT];
#endif
        ClientMeta *cm[NCLIENT];

        for (size_t i = 0; i < NEPOLL; ++i) {
#if defined(TURBO_PLATFORM_LINUX)
            epfd[i] = epoll_create(1024);
#elif defined(TURBO_PLATFORM_OSX)
            epfd[i] = kqueue();
#endif
            REQUIRE_GT(epfd[i], 0);
        }

        for (size_t i = 0; i < NCLIENT; ++i) {
            REQUIRE_EQ(0, socketpair(AF_UNIX, SOCK_STREAM, 0, fds + 2 * i));
            //printf("Created fd=%d,%d i=%lu\n", fds[2*i], fds[2*i+1], i);
            SocketMeta *m = new SocketMeta;
            m->fd = fds[i * 2];
            m->epfd = epfd[fmix32(i) % NEPOLL];
            REQUIRE_EQ(0, fcntl(m->fd, F_SETFL, fcntl(m->fd, F_GETFL, 0) | O_NONBLOCK));

#ifdef CREATE_THREAD_TO_PROCESS
# if defined(TURBO_PLATFORM_LINUX)
            epoll_event evt = { EPOLLIN | EPOLLONESHOT, { m } };
# elif defined(TURBO_PLATFORM_OSX)
            struct kevent kqueue_event;
            EV_SET(&kqueue_event, m->fd, EVFILT_READ, EV_ADD | EV_ENABLE | EV_ONESHOT,
                    0, 0, m);
# endif
#else
# if defined(TURBO_PLATFORM_LINUX)
            epoll_event evt = { EPOLLIN, { m } };
# elif defined(TURBO_PLATFORM_OSX)
            struct kevent kqueue_event;
            EV_SET(&kqueue_event, m->fd, EVFILT_READ, EV_ADD | EV_ENABLE, 0, 0, m);
# endif
#endif

#if defined(TURBO_PLATFORM_LINUX)
            REQUIRE_EQ(0, epoll_ctl(m->epfd, EPOLL_CTL_ADD, m->fd, &evt));
#elif defined(TURBO_PLATFORM_OSX)
            REQUIRE_EQ(0, kevent(m->epfd, &kqueue_event, 1, nullptr, 0, nullptr));
#endif
            cm[i] = new ClientMeta;
            cm[i]->fd = fds[i * 2 + 1];
            cm[i]->count = i;
            cm[i]->times = REP;
#ifdef RUN_CLIENT_IN_FIBER
            turbo::make_non_blocking(cm[i]->fd);
            REQUIRE_EQ(0, fiber_start(&cth[i], nullptr, client_thread, cm[i]));
#else
            REQUIRE_EQ(0, pthread_create(&cth[i], nullptr, client_thread, cm[i]));
#endif
        }

        ProfilerStart("ping_pong.prof");
        turbo::stop_watcher tm;
        tm.start();

        for (size_t i = 0; i < NEPOLL; ++i) {
            EpollMeta *em = new EpollMeta;
            em->epfd = epfd[i];
#ifdef RUN_EPOLL_IN_FIBER
            REQUIRE_EQ(0, fiber_start(&eth[i], epoll_thread, em, nullptr);
#else
            REQUIRE_EQ(0, pthread_create(&eth[i], nullptr, epoll_thread, em));
#endif
        }

        for (size_t i = 0; i < NCLIENT; ++i) {
#ifdef RUN_CLIENT_IN_FIBER
            fiber_join(cth[i], nullptr);
#else
            pthread_join(cth[i], nullptr);
#endif
            REQUIRE_EQ(i + REP * NCLIENT, cm[i]->count);
        }
        tm.stop();
        ProfilerStop();
        TURBO_LOG(INFO) << "tid=" << REP * NCLIENT * 1000000L / tm.u_elapsed();
        stop = true;
        for (size_t i = 0; i < NEPOLL; ++i) {
#if defined(TURBO_PLATFORM_LINUX)
            epoll_event evt = { EPOLLOUT,  { nullptr } };
            REQUIRE_EQ(0, epoll_ctl(epfd[i], EPOLL_CTL_ADD, 0, &evt));
#elif defined(TURBO_PLATFORM_OSX)
            struct kevent kqueue_event;
            EV_SET(&kqueue_event, 0, EVFILT_WRITE, EV_ADD | EV_ENABLE, 0, 0, nullptr);
            REQUIRE_EQ(0, kevent(epfd[i], &kqueue_event, 1, nullptr, 0, nullptr));
#endif
#ifdef RUN_EPOLL_IN_FIBER
            fiber_join(eth[i], nullptr);
#else
            pthread_join(eth[i], nullptr);
#endif
        }
        //turbo::fiber_internal::stop_and_join_epoll_threads();
        turbo::fiber_sleep_for(100000);

#ifndef NDEBUG
        std::cout << "break_nums=" << turbo::fiber_internal::break_nums << std::endl;
#endif
    }*/

    TEST_CASE("FDTest, mod_closed_fd") {
#if defined(TURBO_PLATFORM_LINUX)
        // Conclusion:
        //   If fd is never added into epoll, MOD returns ENOENT
        //   If fd is inside epoll and valid, MOD returns 0
        //   If fd is closed and not-reused, MOD returns EBADF
        //   If fd is closed and reused, MOD returns ENOENT again

        const int epfd = epoll_create(1024);
        int new_fd[2];
        int fd[2];
        REQUIRE_EQ(0, pipe(fd));
        epoll_event e = { EPOLLIN, { nullptr } };
        errno = 0;
        REQUIRE_EQ(-1, epoll_ctl(epfd, EPOLL_CTL_MOD, fd[0], &e));
        REQUIRE_EQ(ENOENT, errno);
        REQUIRE_EQ(0, epoll_ctl(epfd, EPOLL_CTL_ADD, fd[0], &e));
        // mod after add
        REQUIRE_EQ(0, epoll_ctl(epfd, EPOLL_CTL_MOD, fd[0], &e));
        // mod after mod
        REQUIRE_EQ(0, epoll_ctl(epfd, EPOLL_CTL_MOD, fd[0], &e));
        REQUIRE_EQ(0, close(fd[0]));
        REQUIRE_EQ(0, close(fd[1]));

        errno = 0;
        REQUIRE_EQ(-1, epoll_ctl(epfd, EPOLL_CTL_MOD, fd[0], &e));
        REQUIRE_EQ(EBADF, errno) ;

        REQUIRE_EQ(0, pipe(new_fd));
        REQUIRE_EQ(fd[0], new_fd[0]);
        REQUIRE_EQ(fd[1], new_fd[1]);

        errno = 0;
        REQUIRE_EQ(-1, epoll_ctl(epfd, EPOLL_CTL_MOD, fd[0], &e));
        REQUIRE_EQ(ENOENT, errno);

        REQUIRE_EQ(0, close(epfd));
#endif
    }

    TEST_CASE("FDTest, add_existing_fd") {
#if defined(TURBO_PLATFORM_LINUX)
        const int epfd = epoll_create(1024);
        epoll_event e = { EPOLLIN, { nullptr } };
        REQUIRE_EQ(0, epoll_ctl(epfd, EPOLL_CTL_ADD, 0, &e));
        errno = 0;
        REQUIRE_EQ(-1, epoll_ctl(epfd, EPOLL_CTL_ADD, 0, &e));
        REQUIRE_EQ(EEXIST, errno);
        REQUIRE_EQ(0, close(epfd));
#endif
    }

    void *epoll_waiter(void *arg) {
#if defined(TURBO_PLATFORM_LINUX)
        epoll_event e;
        if (1 == epoll_wait((int)(intptr_t)arg, &e, 1, -1)) {
            std::cout << e.events << std::endl;
        }
#elif defined(TURBO_PLATFORM_OSX)
        struct kevent e;
        if (1 == kevent((int) (intptr_t) arg, nullptr, 0, &e, 1, nullptr)) {
            std::cout << e.flags << std::endl;
        }
#endif
        std::cout << pthread_self() << " quits" << std::endl;
        return nullptr;
    }

    TEST_CASE("FDTest, interrupt_pthread") {
#if defined(TURBO_PLATFORM_LINUX)
        const int epfd = epoll_create(1024);
#elif defined(TURBO_PLATFORM_OSX)
        const int epfd = kqueue();
#endif
        pthread_t th, th2;
        REQUIRE_EQ(0, pthread_create(&th, nullptr, epoll_waiter, (void *) (intptr_t) epfd));
        REQUIRE_EQ(0, pthread_create(&th2, nullptr, epoll_waiter, (void *) (intptr_t) epfd));
        turbo::fiber_sleep_for(turbo::Duration::microseconds(100000L));
        std::cout << "wake up " << th << std::endl;
        turbo::PlatformThread::kill_thread(th);
        turbo::fiber_sleep_for(turbo::Duration::microseconds(100000L));
        std::cout << "wake up " << th2 << std::endl;
        turbo::PlatformThread::kill_thread(th2);
        pthread_join(th, nullptr);
        pthread_join(th2, nullptr);
    }

    void *close_the_fd(void *arg) {
        turbo::fiber_sleep_for(turbo::Duration::milliseconds(10));
        REQUIRE(turbo::fiber_fd_close(*(int *) arg).ok());
        return nullptr;
    }

    TEST_CASE("FDTest, invalid_epoll_events") {
        errno = 0;
#if defined(TURBO_PLATFORM_LINUX)
        REQUIRE_EQ(turbo::fiber_fd_wait(-1, EPOLLIN).code(), turbo::kEINVAL);
#elif defined(TURBO_PLATFORM_OSX)
        REQUIRE_EQ(turbo::fiber_fd_wait(-1, EVFILT_READ).code(), turbo::kEINVAL);
#endif
        REQUIRE_EQ(EINVAL, errno);
        errno = 0;
#if defined(TURBO_PLATFORM_LINUX)
        REQUIRE_EQ(turbo::fiber_fd_timedwait(-1, EPOLLIN, turbo::Time::infinite_future()).code(), turbo::kEINVAL);
#elif defined(TURBO_PLATFORM_OSX)
        REQUIRE_EQ(turbo::fiber_fd_timedwait(-1, EVFILT_READ, nullptr).code(), turbo::kEINVAL);
#endif
        REQUIRE_EQ(EINVAL, errno);

        int fds[2];
        REQUIRE_EQ(0, pipe(fds));
#if defined(TURBO_PLATFORM_LINUX)
        REQUIRE_EQ(turbo::fiber_fd_wait(fds[0], EPOLLET).code(), turbo::kEINVAL);
        REQUIRE_EQ(EINVAL, errno);
#endif
        fiber_id_t th;
        REQUIRE_EQ(true, fiber_start(&th, nullptr, close_the_fd, &fds[1]).ok());
        turbo::StopWatcher tm;
        tm.reset();
#if defined(TURBO_PLATFORM_LINUX)
        REQUIRE(turbo::fiber_fd_wait(fds[0], EPOLLIN | EPOLLET).ok());
#elif defined(TURBO_PLATFORM_OSX)
        REQUIRE_EQ(0, fiber_fd_wait(fds[0], EVFILT_READ));
#endif
        tm.stop();
        REQUIRE_LT(tm.elapsed_mill(), 20);
        REQUIRE(fiber_join(th, nullptr).ok());
        REQUIRE(turbo::fiber_fd_close(fds[0]).ok());
    }

    void *wait_for_the_fd(void *arg) {
        auto ts =turbo::microseconds_from_now(50);
#if defined(TURBO_PLATFORM_LINUX)
        turbo::fiber_fd_timedwait(*(int*)arg, EPOLLIN, ts);
#elif defined(TURBO_PLATFORM_OSX)
        fiber_fd_timedwait(*(int *) arg, EVFILT_READ, &ts);
#endif
        return nullptr;
    }

    TEST_CASE("FDTest, timeout") {
        int fds[2];
        REQUIRE_EQ(0, pipe(fds));
        pthread_t th;
        REQUIRE_EQ(0, pthread_create(&th, nullptr, wait_for_the_fd, &fds[0]));
        fiber_id_t bth;
        REQUIRE_EQ(true, fiber_start(&bth, nullptr, wait_for_the_fd, &fds[0]).ok());
        turbo::StopWatcher tm;
        tm.reset();
        REQUIRE_EQ(0, pthread_join(th, nullptr));
        REQUIRE(fiber_join(bth, nullptr).ok());
        tm.stop();
        REQUIRE_LT(tm.elapsed_mill(), 80);
        REQUIRE(turbo::fiber_fd_close(fds[0]).ok());
        REQUIRE(turbo::fiber_fd_close(fds[1]).ok());
    }

    TEST_CASE("FDTest, close_should_wakeup_waiter") {
        int fds[2];
        REQUIRE_EQ(0, pipe(fds));
        fiber_id_t bth;
        REQUIRE_EQ(true, fiber_start(&bth, nullptr, wait_for_the_fd, &fds[0]).ok());
        turbo::StopWatcher tm;
        tm.reset();
        REQUIRE(turbo::fiber_fd_close(fds[0]).ok());
        REQUIRE(fiber_join(bth, nullptr).ok());
        tm.stop();
        REQUIRE_LT(tm.elapsed_mill(), 5);

        // Launch again, should quit soon due to EBADF
#if defined(TURBO_PLATFORM_LINUX)
        REQUIRE(!turbo::fiber_fd_timedwait(fds[0], EPOLLIN, turbo::Time::infinite_future()).ok());
#elif defined(TURBO_PLATFORM_OSX)
        REQUIRE_EQ(-1, fiber_fd_timedwait(fds[0], EVFILT_READ, nullptr));
#endif
        REQUIRE_EQ(EBADF, errno);

        REQUIRE(turbo::fiber_fd_close(fds[1]).ok());
    }

    TEST_CASE("FDTest, close_definitely_invalid") {
        int ec = 0;
        REQUIRE_EQ(-1, close(-1));
        ec = errno;
        REQUIRE(!turbo::fiber_fd_close(-1).ok());
        REQUIRE_EQ(ec, errno);
    }

    TEST_CASE("FDTest, fiber_close_fd_which_did_not_call_fiber_functions") {
        int fds[2];
        REQUIRE_EQ(0, pipe(fds));
        REQUIRE(turbo::fiber_fd_close(fds[0]).ok());
        REQUIRE(turbo::fiber_fd_close(fds[1]).ok());
    }

    TEST_CASE("FDTest, double_close") {
        int fds[2];
        REQUIRE_EQ(0, pipe(fds));
        REQUIRE_EQ(0, close(fds[0]));
        int ec = 0;
        REQUIRE_EQ(-1, close(fds[0]));
        ec = errno;
        REQUIRE(turbo::fiber_fd_close(fds[1]).ok());
        REQUIRE(!turbo::fiber_fd_close(fds[1]).ok());
        REQUIRE_EQ(ec, errno);
    }
} // namespace turbo::fiber_internal
