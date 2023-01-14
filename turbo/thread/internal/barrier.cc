
/****************************************************************
 * Copyright (c) 2022, liyinbin
 * All rights reserved.
 * Author by liyinbin (jeff.li) lijippy@163.com
 *****************************************************************/


#include "turbo/thread/internal/barrier.h"

#include <sys/mman.h>
#include <mutex>

#include "turbo/log/logging.h"
#include "turbo/memory/resident.h"

namespace turbo::thread_internal {

    namespace {

        void *create_one_byte_dummy_page() {
            auto ptr = mmap(nullptr, 1, PROT_READ, MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);
            TURBO_CHECK(ptr) << "Cannot create dummy page for asymmetric memory barrier.";
            (void) mlock(ptr, 1);
            return ptr;
        }

        // `membarrier()` is not usable until Linux 4.3, which is not available until
        //
        // Here Folly provides a workaround by mutating our page tables. Mutating page
        // tables, for the moment, implicitly cause the system to execute a barrier on
        // every core running our threads. I shamelessly copied their solution here.
        //
        // @sa:
        // https://github.com/facebook/folly/blob/master/folly/synchronization/AsymmetricMemoryBarrier.cc
        void homemade_membarrier() {
            static void *dummy_page = create_one_byte_dummy_page();  // Never freed.

            // Previous memory accesses may not be reordered after syscalls below.
            // (Frankly this is implied by acquired lock below.)
            memory_barrier();

            static resident<std::mutex> lock;
            std::scoped_lock _(*lock);

            // Upgrading protection does not always result in fence in each core (as it
            // can be delayed until #PF).
            TURBO_CHECK(mprotect(dummy_page, 1, PROT_READ | PROT_WRITE) == 0);
            *static_cast<char *>(dummy_page) = 0;  // Make sure the page is present.
            // This time a barrier should be issued to every cores.
            TURBO_CHECK(mprotect(dummy_page, 1, PROT_READ) == 0);

            // Subsequent memory accesses may not be reordered before syscalls above. (Not
            // sure if it's already implied by `mprotect`?)
            memory_barrier();
        }

    }  // namespace

    void asymmetric_barrier_heavy() {
        homemade_membarrier();
    }
}  // namespace turbo::thread_internal
