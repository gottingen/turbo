
#include "flare/base/static_atomic.h"     // std::atomic<int>
#include "flare/fiber/internal/waitable_event.h"
#include "flare/fiber/fiber_latch.h"

namespace flare {

    fiber_latch::fiber_latch(int initial_count) {
        if (initial_count < 0) {
            FLARE_LOG(FATAL) << "Invalid initial_count=" << initial_count;
            abort();
        }
        _event = flare::fiber_internal::waitable_event_create_checked<int>();
        *_event = initial_count;
        _wait_was_invoked = false;
    }

    fiber_latch::~fiber_latch() {
        flare::fiber_internal::waitable_event_destroy(_event);
    }

    void fiber_latch::signal(int sig) {
        // Have to save _event, *this is probably defreferenced by the wait thread
        // which sees fetch_sub
        void *const saved_event = _event;
        const int prev = ((std::atomic<int> *) _event)
                ->fetch_sub(sig, std::memory_order_release);
        // DON'T touch *this ever after
        if (prev > sig) {
            return;
        }
        FLARE_LOG_IF(ERROR, prev < sig) << "Counter is over decreased";
        flare::fiber_internal::waitable_event_wake_all(saved_event);
    }

    int fiber_latch::wait() {
        _wait_was_invoked = true;
        for (;;) {
            const int seen_counter =
                    ((std::atomic<int> *) _event)->load(std::memory_order_acquire);
            if (seen_counter <= 0) {
                return 0;
            }
            if (flare::fiber_internal::waitable_event_wait(_event, seen_counter, nullptr) < 0 &&
                errno != EWOULDBLOCK && errno != EINTR) {
                return errno;
            }
        }
    }

    void fiber_latch::add_count(int v) {
        if (v <= 0) {
            FLARE_LOG_IF(ERROR, v < 0) << "Invalid count=" << v;
            return;
        }
        FLARE_LOG_IF(ERROR, _wait_was_invoked)
                        << "Invoking add_count() after wait() was invoked";
        ((std::atomic<int> *) _event)->fetch_add(v, std::memory_order_release);
    }

    void fiber_latch::reset(int v) {
        if (v < 0) {
            FLARE_LOG(ERROR) << "Invalid count=" << v;
            return;
        }
        const int prev_counter =
                ((std::atomic<int> *) _event)
                        ->exchange(v, std::memory_order_release);
        FLARE_LOG_IF(ERROR, _wait_was_invoked && prev_counter)
                        << "Invoking reset() while count=" << prev_counter;
        _wait_was_invoked = false;
    }

    int fiber_latch::timed_wait(const timespec *duetime) {
        _wait_was_invoked = true;
        for (;;) {
            const int seen_counter =
                    ((std::atomic<int> *) _event)->load(std::memory_order_acquire);
            if (seen_counter <= 0) {
                return 0;
            }
            if (flare::fiber_internal::waitable_event_wait(_event, seen_counter, duetime) < 0 &&
                errno != EWOULDBLOCK && errno != EINTR) {
                return errno;
            }
        }
    }

}  // namespace flare
