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

#include <sys/mman.h>                             // mmap, munmap, mprotect
#include <algorithm>                              // std::max
#include <atomic>                                 // std::atomic
#include "turbo/fiber/internal/types.h"
#include "turbo/fiber/internal/stack.h"
#include "turbo/log/logging.h"
#include "turbo/memory/memory_info.h"

namespace turbo::fiber_internal {

    static std::atomic<int64_t> s_stack_count{0};

    static int64_t get_stack_count(void *) {
        return s_stack_count.load(std::memory_order_relaxed);
    }

    int allocate_stack_storage(StackStorage *s, int stacksize_in, int guardsize_in) {
        const static int PAGESIZE = get_page_size();
        const int PAGESIZE_M1 = PAGESIZE - 1;
        const int MIN_STACKSIZE = PAGESIZE * 2;
        const int MIN_GUARDSIZE = PAGESIZE;

        // Align stacksize
        const int stacksize =
                (std::max(stacksize_in, MIN_STACKSIZE) + PAGESIZE_M1) &
                ~PAGESIZE_M1;

        if (guardsize_in <= 0) {
            void *mem = malloc(stacksize);
            if (nullptr == mem) {
                TLOG_ERROR_EVERY_SEC("Fail to malloc (size={})", stacksize);
                return -1;
            }
            s_stack_count.fetch_add(1, std::memory_order_relaxed);
            s->bottom = (char *) mem + stacksize;
            s->stacksize = stacksize;
            s->guardsize = 0;
            return 0;
        } else {
            // Align guardsize
            const int guardsize =
                    (std::max(guardsize_in, MIN_GUARDSIZE) + PAGESIZE_M1) &
                    ~PAGESIZE_M1;

            const int memsize = stacksize + guardsize;
            void *const mem = mmap(nullptr, memsize, (PROT_READ | PROT_WRITE),
                                   (MAP_PRIVATE | MAP_ANONYMOUS), -1, 0);

            if (MAP_FAILED == mem) {
                TLOG_ERROR_EVERY_SEC(
                        "Fail to mmap size={} stack_count={}, possibly limited by /proc/sys/vm/max_map_count", memsize,
                        s_stack_count.load(std::memory_order_relaxed));
                // may fail due to limit of max_map_count (65536 in default)
                return -1;
            }

            void *aligned_mem = (void *) (((intptr_t) mem + PAGESIZE_M1) & ~PAGESIZE_M1);
            if (aligned_mem != mem) {
                TLOG_ERROR("addr={} returned by mmap is not aligned by pagesize={}",
                           mem, PAGESIZE);
            }
            const int offset = (char *) aligned_mem - (char *) mem;
            if (guardsize <= offset ||
                mprotect(aligned_mem, guardsize - offset, PROT_NONE) != 0) {
                munmap(mem, memsize);
                TLOG_ERROR_EVERY_SEC("Fail to mprotect (addr={}, length={})", (void *) aligned_mem,
                                     guardsize - offset);
                return -1;
            }

            s_stack_count.fetch_add(1, std::memory_order_relaxed);
            s->bottom = (char *) mem + memsize;
            s->stacksize = stacksize;
            s->guardsize = guardsize;
            return 0;
        }
    }

    void deallocate_stack_storage(StackStorage *s) {
        const int memsize = s->stacksize + s->guardsize;
        if ((uintptr_t) s->bottom <= (uintptr_t) memsize) {
            return;
        }
        s_stack_count.fetch_sub(1, std::memory_order_relaxed);
        if (s->guardsize <= 0) {
            free((char *) s->bottom - memsize);
        } else {
            munmap((char *) s->bottom - memsize, memsize);
        }
    }

}  // namespace turbo::internal
