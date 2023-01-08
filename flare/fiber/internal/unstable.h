
#ifndef FLARE_FIBER_INTERNAL_UNSTABLE_H_
#define FLARE_FIBER_INTERNAL_UNSTABLE_H_

#include <pthread.h>
#include <sys/socket.h>
#include "flare/fiber/internal/types.h"
#include "flare/fiber/internal/errno.h"

// NOTICE:
//   As the filename implies, this file lists UNSTABLE fiber functions
//   which are likely to be modified or even removed in future release. We
//   don't guarantee any kind of backward compatibility. Don't use these
//   functions if you're not ready to change your code according to newer
//   versions of fiber.

__BEGIN_DECLS

// Schedule tasks created by FIBER_NOSIGNAL
extern void fiber_flush();

// Mark the calling fiber as "about to quit". When the fiber is scheduled,
// worker pthreads are not notified.
extern int fiber_about_to_quit();

// Run `on_timer(arg)' at or after real-time `abstime'. Put identifier of the
// timer into *id.
// Return 0 on success, errno otherwise.
extern int fiber_timer_add(fiber_timer_id *id, timespec abstime,
                           void (*on_timer)(void *), void *arg);

// Unschedule the timer associated with `id'.
// Returns: 0 - exist & not-run; 1 - still running; EINVAL - not exist.
extern int fiber_timer_del(fiber_timer_id id);

// Suspend caller thread until the file descriptor `fd' has `epoll_events'.
// Returns 0 on success, -1 otherwise and errno is set.
// NOTE: Due to an epoll bug(https://patchwork.kernel.org/patch/1970231),
// current implementation relies on EPOLL_CTL_ADD and EPOLL_CTL_DEL which
// are not scalable, don't use fiber_fd_*wait functions in performance
// critical scenario.
extern int fiber_fd_wait(int fd, unsigned events);

// Suspend caller thread until the file descriptor `fd' has `epoll_events'
// or CLOCK_REALTIME reached `abstime' if abstime is not nullptr.
// Returns 0 on success, -1 otherwise and errno is set.
extern int fiber_fd_timedwait(int fd, unsigned epoll_events,
                                const timespec *abstime);

// Close file descriptor `fd' and wake up all threads waiting on it.
// User should call this function instead of close(2) if fiber_fd_wait,
// fiber_fd_timedwait, fiber_connect were called on the file descriptor,
// otherwise waiters will suspend indefinitely and fiber's internal epoll
// may work abnormally after fork() is called.
// NOTE: This function does not wake up pthread waiters.(tested on linux 2.6.32)
extern int fiber_fd_close(int fd);

// Replacement of connect(2) in fibers.
extern int fiber_connect(int sockfd, const sockaddr *serv_addr,
                           socklen_t addrlen);

// Add a startup function that each pthread worker will run at the beginning
// To run code at the end, use flare::thread::atexit()
// Returns 0 on success, error code otherwise.
extern int fiber_set_worker_startfn(void (*start_fn)());

// Stop all fiber and worker pthreads.
// You should avoid calling this function which may cause fiber after main()
// suspend indefinitely.
extern void fiber_stop_world();

// Create a fiber_local_key with an additional arg to destructor.
// Generally the dtor_arg is for passing the creator of data so that we can
// return the data back to the creator in destructor. Without this arg, we
// have to do an extra heap allocation to contain data and its creator.
extern int fiber_key_create2(fiber_local_key *key,
                               void (*destructor)(void *data, const void *dtor_arg),
                               const void *dtor_arg);

// CAUTION: functions marked with [RPC INTERNAL] are NOT supposed to be called
// by RPC users.

// [RPC INTERNAL]
// Create a pool to cache KeyTables so that frequently created/destroyed
// fibers reuse these tables, namely when a fiber needs a KeyTable,
// it fetches one from the pool instead of creating on heap. When a fiber
// exits, it puts the table back to pool instead of deleting it.
// Returns 0 on success, error code otherwise.
extern int fiber_keytable_pool_init(fiber_keytable_pool_t *);

// [RPC INTERNAL]
// Destroy the pool. All KeyTables inside are destroyed.
// Returns 0 on success, error code otherwise.
extern int fiber_keytable_pool_destroy(fiber_keytable_pool_t *);

// [RPC INTERNAL]
// Put statistics of `pool' into `stat'.
extern int fiber_keytable_pool_getstat(fiber_keytable_pool_t *pool,
                                         fiber_keytable_pool_stat_t *stat);

// [RPC INTERNAL]
// Reserve at most `nfree' keytables with `key' pointing to data created by
// ctor(args).
extern void fiber_keytable_pool_reserve(
        fiber_keytable_pool_t *pool, size_t nfree,
        fiber_local_key key, void *ctor(const void *args), const void *args);

__END_DECLS

#endif  // FLARE_FIBER_INTERNAL_UNSTABLE_H_
