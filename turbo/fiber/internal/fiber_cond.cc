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

#include <atomic>
#include "turbo/fiber/internal/fiber_cond.h"
#include "turbo/fiber/internal/waitable_event.h"
#include "turbo/fiber/internal/types.h"                       // fiber_cond_t
#include "turbo/base/internal/raw_logging.h"

namespace turbo::fiber_internal {
    struct CondInternal {
        std::atomic<fiber_mutex_t *> m;
        std::atomic<int> *seq;
    };

    static_assert(sizeof(CondInternal) == sizeof(fiber_cond_t),
                  "sizeof_innercond_must_equal_cond");
    static_assert(offsetof(CondInternal, m) == offsetof(fiber_cond_t, m),
                  "offsetof_cond_mutex_must_equal");
    static_assert(offsetof(CondInternal, seq) ==
                  offsetof(fiber_cond_t, seq),
                  "offsetof_cond_seq_must_equal");
}

namespace turbo::fiber_internal {

    turbo::Status fiber_cond_init(fiber_cond_t *TURBO_RESTRICT c,
                        const fiber_condattr_t *) {
        c->m = nullptr;
        c->seq = turbo::fiber_internal::waitable_event_create_checked<int>();
        TURBO_RAW_CHECK(c->seq != nullptr, "fiber_cond_init: waitable_event_create_checked failed");
        *c->seq = 0;
        return ok_status();
    }

    void fiber_cond_destroy(fiber_cond_t *c) {
        turbo::fiber_internal::waitable_event_destroy(c->seq);
        c->seq = nullptr;
    }

    void fiber_cond_signal(fiber_cond_t *c) {
        turbo::fiber_internal::CondInternal *ic = reinterpret_cast<turbo::fiber_internal::CondInternal *>(c);
        // ic is probably dereferenced after fetch_add, save required fields before
        // this point
        std::atomic<int> *const saved_seq = ic->seq;
        saved_seq->fetch_add(1, std::memory_order_release);
        // don't touch ic any more
        turbo::fiber_internal::waitable_event_wake(saved_seq);
    }

    void fiber_cond_broadcast(fiber_cond_t *c) {
        turbo::fiber_internal::CondInternal *ic = reinterpret_cast<turbo::fiber_internal::CondInternal *>(c);
        fiber_mutex_t *m = ic->m.load(std::memory_order_relaxed);
        std::atomic<int> *const saved_seq = ic->seq;
        if (!m) {
            return;
        }
        void *const saved_event = m->event;
        // Wakeup one thread and requeue the rest on the mutex.
        ic->seq->fetch_add(1, std::memory_order_release);
        turbo::fiber_internal::waitable_event_requeue(saved_seq, saved_event);
    }

    turbo::Status fiber_cond_wait(fiber_cond_t *TURBO_RESTRICT c,
                        fiber_mutex_t *TURBO_RESTRICT m) {
        turbo::fiber_internal::CondInternal *ic = reinterpret_cast<turbo::fiber_internal::CondInternal *>(c);
        const int expected_seq = ic->seq->load(std::memory_order_relaxed);
        if (ic->m.load(std::memory_order_relaxed) != m) {
            // bind m to c
            fiber_mutex_t *expected_m = nullptr;
            if (!ic->m.compare_exchange_strong(
                    expected_m, m, std::memory_order_relaxed)) {
                return turbo::make_status(kEINVAL);
            }
        }
        fiber_mutex_unlock(m);
        turbo::Status rc;
        auto rs = turbo::fiber_internal::waitable_event_wait(ic->seq, expected_seq);
        if (!rs.ok() && rs.code() != EWOULDBLOCK && rs.code() != kEINTR) {
            // EINTR should not be returned by cond_*wait according to docs on
            // pthread, however spurious wake-up is OK, just as we do here
            // so that users can check flags in the loop often companioning
            // with the cond_wait ASAP. For example:
            //   mutex.lock();
            //   while (!stop && other-predicates) {
            //     cond_wait(&mutex);
            //   }
            //   mutex.unlock();
            // After interruption, above code should wake up from the cond_wait
            // soon and check the `stop' flag and other predicates.
            rc = rs;
        }
        const auto rc2 = fiber_mutex_lock_contended(m);
        return (!rc2.ok() ? rc2 : rc);
    }

    turbo::Status fiber_cond_timedwait(fiber_cond_t *TURBO_RESTRICT c,
                                       fiber_mutex_t *TURBO_RESTRICT m,turbo::Time abstime) {
        turbo::fiber_internal::CondInternal *ic = reinterpret_cast<turbo::fiber_internal::CondInternal *>(c);
        const int expected_seq = ic->seq->load(std::memory_order_relaxed);
        if (ic->m.load(std::memory_order_relaxed) != m) {
            // bind m to c
            fiber_mutex_t *expected_m = nullptr;
            if (!ic->m.compare_exchange_strong(
                    expected_m, m, std::memory_order_relaxed)) {
                return turbo::make_status(kEINVAL);
            }
        }
        fiber_mutex_unlock(m);
        turbo::Status rc;
        auto rs = turbo::fiber_internal::waitable_event_wait(ic->seq, expected_seq, abstime);
        if (!rs.ok() && rs.code() != EWOULDBLOCK && rs.code() != kEINTR) {
            // note: see comments in fiber_cond_wait on EINTR.
            rc = rs;
        }
        const auto rc2 = fiber_mutex_lock_contended(m);
        return (!rc2.ok() ? rc2 : rc);
    }

}  // namespace turbo::fiber_internal
