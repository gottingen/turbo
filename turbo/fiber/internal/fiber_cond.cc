
#include "turbo/base/static_atomic.h"
#include "turbo/fiber/internal/waitable_event.h"
#include "turbo/fiber/internal/types.h"                       // fiber_cond_t

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

extern "C" {

extern int fiber_mutex_unlock(fiber_mutex_t *);
extern int fiber_mutex_lock_contended(fiber_mutex_t *);

int fiber_cond_init(fiber_cond_t *__restrict c,
                    const fiber_condattr_t *) {
    c->m = nullptr;
    c->seq = turbo::fiber_internal::waitable_event_create_checked<int>();
    *c->seq = 0;
    return 0;
}

int fiber_cond_destroy(fiber_cond_t *c) {
    turbo::fiber_internal::waitable_event_destroy(c->seq);
    c->seq = nullptr;
    return 0;
}

int fiber_cond_signal(fiber_cond_t *c) {
    turbo::fiber_internal::CondInternal *ic = reinterpret_cast<turbo::fiber_internal::CondInternal *>(c);
    // ic is probably dereferenced after fetch_add, save required fields before
    // this point
    std::atomic<int> *const saved_seq = ic->seq;
    saved_seq->fetch_add(1, std::memory_order_release);
    // don't touch ic any more
    turbo::fiber_internal::waitable_event_wake(saved_seq);
    return 0;
}

int fiber_cond_broadcast(fiber_cond_t *c) {
    turbo::fiber_internal::CondInternal *ic = reinterpret_cast<turbo::fiber_internal::CondInternal *>(c);
    fiber_mutex_t *m = ic->m.load(std::memory_order_relaxed);
    std::atomic<int> *const saved_seq = ic->seq;
    if (!m) {
        return 0;
    }
    void *const saved_event = m->event;
    // Wakeup one thread and requeue the rest on the mutex.
    ic->seq->fetch_add(1, std::memory_order_release);
    turbo::fiber_internal::waitable_event_requeue(saved_seq, saved_event);
    return 0;
}

int fiber_cond_wait(fiber_cond_t *__restrict c,
                    fiber_mutex_t *__restrict m) {
    turbo::fiber_internal::CondInternal *ic = reinterpret_cast<turbo::fiber_internal::CondInternal *>(c);
    const int expected_seq = ic->seq->load(std::memory_order_relaxed);
    if (ic->m.load(std::memory_order_relaxed) != m) {
        // bind m to c
        fiber_mutex_t *expected_m = nullptr;
        if (!ic->m.compare_exchange_strong(
                expected_m, m, std::memory_order_relaxed)) {
            return EINVAL;
        }
    }
    fiber_mutex_unlock(m);
    int rc1 = 0;
    if (turbo::fiber_internal::waitable_event_wait(ic->seq, expected_seq, nullptr) < 0 &&
        errno != EWOULDBLOCK && errno != EINTR/*note*/) {
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
        rc1 = errno;
    }
    const int rc2 = fiber_mutex_lock_contended(m);
    return (rc2 ? rc2 : rc1);
}

int fiber_cond_timedwait(fiber_cond_t *__restrict c,
                         fiber_mutex_t *__restrict m,
                         const struct timespec *__restrict abstime) {
    turbo::fiber_internal::CondInternal *ic = reinterpret_cast<turbo::fiber_internal::CondInternal *>(c);
    const int expected_seq = ic->seq->load(std::memory_order_relaxed);
    if (ic->m.load(std::memory_order_relaxed) != m) {
        // bind m to c
        fiber_mutex_t *expected_m = nullptr;
        if (!ic->m.compare_exchange_strong(
                expected_m, m, std::memory_order_relaxed)) {
            return EINVAL;
        }
    }
    fiber_mutex_unlock(m);
    int rc1 = 0;
    if (turbo::fiber_internal::waitable_event_wait(ic->seq, expected_seq, abstime) < 0 &&
        errno != EWOULDBLOCK && errno != EINTR/*note*/) {
        // note: see comments in fiber_cond_wait on EINTR.
        rc1 = errno;
    }
    const int rc2 = fiber_mutex_lock_contended(m);
    return (rc2 ? rc2 : rc1);
}

}  // extern "C"
