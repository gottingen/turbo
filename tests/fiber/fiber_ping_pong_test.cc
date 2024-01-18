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
// Created by jeff on 24-1-6.
//

#define DOCTEST_CONFIG_IMPLEMENT_WITH_MAIN

#include "turbo/testing/test.h"

#include <inttypes.h>
#include <cstdlib>
#include <unistd.h>
#include <stdio.h>
#include <signal.h>
#include "turbo/times/stop_watcher.h"
#include <turbo/fiber/internal/waitable_event.h>
#include "turbo/fiber/fiber.h"
#include "turbo/system/sysinfo.h"
#include "turbo/concurrent/spinlock_wait.h"

using namespace turbo;

namespace {
    int thread_num = 1;
    bool loop = false;
    bool use_sutex = false;
    bool use_futex = false;

    void TURBO_MAYBE_UNUSED (*ignore_sigpipe)(int) = signal(SIGPIPE, SIG_IGN);

    volatile bool stop = false;

    void quit_handler(int) {
        stop = true;
    }

    struct TURBO_CACHE_LINE_ALIGNED AlignedIntWrapper {
        int value;
    };

    struct TURBO_CACHE_LINE_ALIGNED PlayerArg {
        int read_fd;
        int write_fd;
        int *wait_addr;
        int *wake_addr;
        long counter;
        long wakeup;
    };

    void *pipe_player(void *void_arg) {
        PlayerArg *arg = static_cast<PlayerArg *>(void_arg);
        char dummy = '\0';
        while (1) {
            ssize_t nr = read(arg->read_fd, &dummy, 1);
            if (nr <= 0) {
                if (nr == 0) {
                    printf("[%" PRIu64 "] EOF\n", thread_numeric_id());
                    break;
                }
                if (errno != EINTR) {
                    printf("[%" PRIu64 "] bad read, %m\n", thread_numeric_id());
                    break;
                }
                continue;
            }
            if (1L != write(arg->write_fd, &dummy, 1)) {
                printf("[%" PRIu64 "] bad write, %m\n", thread_numeric_id());
                break;
            }
            ++arg->counter;
        }
        return nullptr;
    }

    static const int INITIAL_FUTEX_VALUE = 0;

    void *sutex_player(void *void_arg) {
        PlayerArg *arg = static_cast<PlayerArg *>(void_arg);
        int counter = INITIAL_FUTEX_VALUE;
        while (!stop) {
            int rc = turbo::concurrent_internal::futex_wait_private(arg->wait_addr, counter, nullptr);
            ++counter;
            ++*arg->wake_addr;
            turbo::concurrent_internal::futex_wake_private(arg->wake_addr, 1);
            ++arg->counter;
            arg->wakeup += (rc == 0);
        }
        return nullptr;
    }

    void *futex_player(void *void_arg) {
        PlayerArg *arg = static_cast<PlayerArg *>(void_arg);
        int counter = INITIAL_FUTEX_VALUE;
        while (!stop) {
            auto rc = turbo::fiber_internal::waitable_event_wait(arg->wait_addr, counter);
            ++counter;
            ++*arg->wake_addr;
            turbo::fiber_internal::waitable_event_wake(arg->wake_addr);
            ++arg->counter;
            arg->wakeup += (rc.ok());
        }
        return nullptr;
    }

    TEST_CASE("PingPongTest, ping_pong") {
        signal(SIGINT, quit_handler);
        stop = false;
        PlayerArg *args[thread_num];

        for (int i = 0; i < thread_num; ++i) {
            int pipe1[2];
            int pipe2[2];
            if (!use_sutex && !use_futex) {
                REQUIRE_EQ(0, pipe(pipe1));
                REQUIRE_EQ(0, pipe(pipe2));
            }

            PlayerArg *arg1 = new PlayerArg;
            if (!use_sutex && !use_futex) {
                arg1->read_fd = pipe1[0];
                arg1->write_fd = pipe2[1];
            } else if (use_sutex) {
                AlignedIntWrapper *w1 = new AlignedIntWrapper;
                w1->value = INITIAL_FUTEX_VALUE;
                AlignedIntWrapper *w2 = new AlignedIntWrapper;
                w2->value = INITIAL_FUTEX_VALUE;
                arg1->wait_addr = &w1->value;
                arg1->wake_addr = &w2->value;
            } else if (use_futex) {
                arg1->wait_addr = turbo::fiber_internal::waitable_event_create_checked<int>();
                *arg1->wait_addr = INITIAL_FUTEX_VALUE;
                arg1->wake_addr = turbo::fiber_internal::waitable_event_create_checked<int>();
                *arg1->wake_addr = INITIAL_FUTEX_VALUE;
            } else {
                REQUIRE(false);
            }
            arg1->counter = 0;
            arg1->wakeup = 0;
            args[i] = arg1;

            PlayerArg *arg2 = new PlayerArg;
            if (!use_sutex && !use_futex) {
                arg2->read_fd = pipe2[0];
                arg2->write_fd = pipe1[1];
            } else {
                arg2->wait_addr = arg1->wake_addr;
                arg2->wake_addr = arg1->wait_addr;
            }
            arg2->counter = 0;
            arg2->wakeup = 0;

            pthread_t th1, th2;
            turbo::fiber_id_t bth1, bth2;
            if (!use_sutex && !use_futex) {
                REQUIRE_EQ(0, pthread_create(&th1, nullptr, pipe_player, arg1));
                REQUIRE_EQ(0, pthread_create(&th2, nullptr, pipe_player, arg2));
            } else if (use_sutex) {
                REQUIRE_EQ(0, pthread_create(&th1, nullptr, sutex_player, arg1));
                REQUIRE_EQ(0, pthread_create(&th2, nullptr, sutex_player, arg2));
            } else if (use_futex) {
                REQUIRE_EQ(turbo::ok_status(),
                           turbo::fiber_start_background(&bth1, nullptr, futex_player, arg1));
                REQUIRE_EQ(turbo::ok_status(),
                           turbo::fiber_start_background(&bth2, nullptr, futex_player, arg2));
            } else {
                REQUIRE(false);
            }

            if (!use_sutex && !use_futex) {
                // send the seed data.
                unsigned char seed = 255;
                REQUIRE_EQ(1L, write(pipe1[1], &seed, 1));
            } else if (use_sutex) {
                ++*arg1->wait_addr;
                turbo::concurrent_internal::futex_wake_private(arg1->wait_addr, 1);
            } else if (use_futex) {
                ++*arg1->wait_addr;
                turbo::fiber_internal::waitable_event_wake(arg1->wait_addr);
            } else {
                REQUIRE(false);
            }
        }

        long last_counter = 0;
        long last_wakeup = 0;
        while (!stop) {
            turbo::StopWatcher tm;
            tm.reset();
            sleep(1);
            tm.stop();
            long cur_counter = 0;
            long cur_wakeup = 0;
            for (int i = 0; i < thread_num; ++i) {
                cur_counter += args[i]->counter;
                cur_wakeup += args[i]->wakeup;
            }
            if (use_sutex || use_futex) {
                printf("pingpong-ed %" PRId64 "/s, wakeup=%" PRId64 "/s\n",
                       (cur_counter - last_counter) * 1000L / tm.elapsed_mill(),
                       (cur_wakeup - last_wakeup) * 1000L / tm.elapsed_mill());
            } else {
                printf("pingpong-ed %" PRId64 "/s\n",
                       (cur_counter - last_counter) * 1000L / tm.elapsed_mill());
            }
            last_counter = cur_counter;
            last_wakeup = cur_wakeup;
            if (!loop) {
                break;
            }
        }
        stop = true;
// Program quits, Let resource leak.
    }
} // namespace
