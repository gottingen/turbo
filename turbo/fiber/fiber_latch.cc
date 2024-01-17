

#include "turbo/fiber/internal/waitable_event.h"
#include "turbo/fiber/fiber_latch.h"
#include "turbo/log/logging.h"

namespace turbo {

    FiberLatch::FiberLatch(int initial_count) {
        if (initial_count < 0) {
            TLOG_CRITICAL("Invalid initial_count={}", initial_count);
            abort();
        }
        _event = turbo::fiber_internal::waitable_event_create_checked<int>();
        *_event = initial_count;
        _wait_was_invoked = false;
    }

    FiberLatch::~FiberLatch() {
        turbo::fiber_internal::waitable_event_destroy(_event);
    }

    void FiberLatch::signal(int sig) {
        // Have to save _event, *this is probably defreferenced by the wait thread
        // which sees fetch_sub
        void *const saved_event = _event;
        const int prev = ((std::atomic<int> *) _event)
                ->fetch_sub(sig, std::memory_order_release);
        // DON'T touch *this ever after
        if (prev > sig) {
            return;
        }
        TLOG_ERROR_IF(prev < sig, "Counter is over decreased");
        turbo::fiber_internal::waitable_event_wake_all(saved_event);
    }

    int FiberLatch::wait() {
        _wait_was_invoked = true;
        for (;;) {
            const int seen_counter =
                    ((std::atomic<int> *) _event)->load(std::memory_order_acquire);
            if (seen_counter <= 0) {
                return 0;
            }
            auto rs = turbo::fiber_internal::waitable_event_wait(_event, seen_counter);
            if (!rs.ok() && !turbo::is_unavailable(rs)) {
                return errno;
            }
        }
    }

    void FiberLatch::add_count(int v) {
        if (v <= 0) {
            TLOG_ERROR_IF(v < 0, "Invalid count={}", v);
            return;
        }
        TLOG_ERROR_IF(_wait_was_invoked, "Invoking add_count() after wait() was invoked");
        ((std::atomic<int> *) _event)->fetch_add(v, std::memory_order_release);
    }

    void FiberLatch::reset(int v) {
        if (v < 0) {
            TLOG_ERROR("Invalid count={}", v);
            return;
        }
        const int prev_counter =
                ((std::atomic<int> *) _event)
                        ->exchange(v, std::memory_order_release);
        TLOG_ERROR_IF(_wait_was_invoked && prev_counter,
                      "Invoking reset() while count={}", prev_counter);
        _wait_was_invoked = false;
    }

    int FiberLatch::timed_wait(turbo::Time duetime) {
        _wait_was_invoked = true;
        for (;;) {
            const int seen_counter =
                    ((std::atomic<int> *) _event)->load(std::memory_order_acquire);
            if (seen_counter <= 0) {
                return 0;
            }
            auto rs = turbo::fiber_internal::waitable_event_wait(_event, seen_counter, duetime);
            if (!rs.ok() && !turbo::is_unavailable(rs)) {
                return errno;
            }
        }
    }

}  // namespace turbo
