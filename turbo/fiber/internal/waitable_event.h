
#ifndef TURBO_FIBER_INTERNAL_WAITABLE_EVENT_H_
#define TURBO_FIBER_INTERNAL_WAITABLE_EVENT_H_

#include <errno.h>                               // users need to check errno
#include <time.h>                                // timespec
#include "turbo/fiber/internal/types.h"                       // fiber_id_t

namespace turbo::fiber_internal {

    // Create a waitable_event which is a futex-like 32-bit primitive for synchronizing
    // fiber/pthreads.
    // Returns a pointer to 32-bit data, nullptr on failure.
    // NOTE: all waitable_events are private(not inter-process).
    void *waitable_event_create();

    // Check width of user type before casting.
    template<typename T>
    T *waitable_event_create_checked() {
        static_assert(sizeof(T) == sizeof(int), "sizeof_T_must_equal_int");
        return static_cast<T *>(waitable_event_create());
    }

    // Destroy the event.
    void waitable_event_destroy(void *event);

    // Wake up at most 1 thread waiting on |event|.
    // Returns # of threads woken up.
    int waitable_event_wake(void *event);

    // Wake up all threads waiting on |event|.
    // Returns # of threads woken up.
    int waitable_event_wake_all(void *event);

    // Wake up all threads waiting on |event| except a fiber whose identifier
    // is |excluded_fiber|. This function does not yield.
    // Returns # of threads woken up.
    int waitable_event_wake_except(void *event, fiber_id_t excluded_fiber);

    // Wake up at most 1 thread waiting on |butex1|, let all other threads wait
    // on |butex2| instead.
    // Returns # of threads woken up.
    int waitable_event_requeue(void *event1, void *event2);

    // Atomically wait on |event| if *event equals |expected_value|, until the
    // event is woken up by waitable_event_wake*, or CLOCK_REALTIME reached |abstime| if
    // abstime is not nullptr.
    // About |abstime|:
    //   Different from FUTEX_WAIT, waitable_event_wait uses absolute time.
    // Returns 0 on success, -1 otherwise and errno is set.
    int waitable_event_wait(void *event, int expected_value, const timespec *abstime);

}  // namespace turbo::fiber_internal

#endif  // TURBO_FIBER_INTERNAL_WAITABLE_EVENT_H_
