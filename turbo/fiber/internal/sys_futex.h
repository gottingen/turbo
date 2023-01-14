

#ifndef TURBO_FIBER_INTERNAL_SYS_FUTEX_H_
#define TURBO_FIBER_INTERNAL_SYS_FUTEX_H_

#include "turbo/base/profile.h"         // TURBO_PLATFORM_OSX
#include <unistd.h>                     // syscall
#include <time.h>                       // timespec

#if defined(TURBO_PLATFORM_LINUX)
#include <syscall.h>                    // SYS_futex
#include <linux/futex.h>                // FUTEX_WAIT, FUTEX_WAKE

namespace turbo::fiber_internal {

#ifndef FUTEX_PRIVATE_FLAG
#define FUTEX_PRIVATE_FLAG 128
#endif

inline int futex_wait_private(
    void* addr1, int expected, const timespec* timeout) {
    return syscall(SYS_futex, addr1, (FUTEX_WAIT | FUTEX_PRIVATE_FLAG),
                   expected, timeout, nullptr, 0);
}

inline int futex_wake_private(void* addr1, int nwake) {
    return syscall(SYS_futex, addr1, (FUTEX_WAKE | FUTEX_PRIVATE_FLAG),
                   nwake, nullptr, nullptr, 0);
}

inline int futex_requeue_private(void* addr1, int nwake, void* addr2) {
    return syscall(SYS_futex, addr1, (FUTEX_REQUEUE | FUTEX_PRIVATE_FLAG),
                   nwake, nullptr, addr2, 0);
}

}  // namespace turbo::fiber_internal

#elif defined(TURBO_PLATFORM_OSX)

namespace turbo::fiber_internal {

    int futex_wait_private(void *addr1, int expected, const timespec *timeout);

    int futex_wake_private(void *addr1, int nwake);

    int futex_requeue_private(void *addr1, int nwake, void *addr2);

}  // namespace turbo::fiber_internal

#else
#error "Unsupported OS"
#endif

#endif  // TURBO_FIBER_INTERNAL_SYS_FUTEX_H_
