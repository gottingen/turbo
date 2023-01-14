// Licensed to the Apache Software Foundation (ASF) under one
// or more contributor license agreements.  See the NOTICE file
// distributed with this work for additional information
// regarding copyright ownership.  The ASF licenses this file
// to you under the Apache License, Version 2.0 (the
// "License"); you may not use this file except in compliance
// with the License.  You may obtain a copy of the License at
//
//   http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing,
// software distributed under the License is distributed on an
// "AS IS" BASIS, WITHOUT WARRANTIES OR CONDITIONS OF ANY
// KIND, either express or implied.  See the License for the
// specific language governing permissions and limitations
// under the License.

// fiber - A M:N threading library to make applications more concurrent.

// Date: Tue Jul 10 17:40:58 CST 2012

#ifndef TURBO_INTHERNAL_FIBER_H_
#define TURBO_INTHERNAL_FIBER_H_

#include <pthread.h>
#include <sys/socket.h>
#include "turbo/fiber/internal/types.h"
#include "turbo/fiber/internal/errno.h"

#if defined(__cplusplus)

#  include <iostream>
#  include "turbo/fiber/internal/mutex.h"        // use fiber_mutex_t in the RAII way

#endif

#include "turbo/fiber/internal/token.h"

__BEGIN_DECLS

// Create fiber `fn(args)' with attributes `attr' and put the identifier into
// `tid'. Switch to the new thread and schedule old thread to run. Use this
// function when the new thread is more urgent.
// Returns 0 on success, errno otherwise.
extern int fiber_start_urgent(fiber_id_t *__restrict tid,
                              const fiber_attribute *__restrict attr,
                              std::function<void*(void*)> && fn,
                              void *__restrict args);

// Create fiber `fn(args)' with attributes `attr' and put the identifier into
// `tid'. This function behaves closer to pthread_create: after scheduling the
// new thread to run, it returns. In another word, the new thread may take
// longer time than fiber_start_urgent() to run.
// Return 0 on success, errno otherwise.
extern int fiber_start_background(fiber_id_t *__restrict tid,
                                  const fiber_attribute *__restrict attr,
                                  std::function<void*(void*)> && fn,
                                  void *__restrict args);

// Wake up operations blocking the thread. Different functions may behave
// differently:
//   turbo::fiber_sleep_for(): returns -1 and sets errno to ESTOP if fiber_stop()
//                     is called, or to EINTR otherwise.
//   waitable_event_wait(): returns -1 and sets errno to EINTR
//   fiber_mutex_*lock: unaffected (still blocking)
//   fiber_cond_*wait: wakes up and returns 0.
//   fiber_*join: unaffected.
// Common usage of interruption is to make a thread to quit ASAP.
//    [Thread1]                  [Thread2]
//   set stopping flag
//   fiber_interrupt(Thread2)
//                               wake up
//                               see the flag and quit
//                               may block again if the flag is unchanged
// fiber_interrupt() guarantees that Thread2 is woken up reliably no matter
// how the 2 threads are interleaved.
// Returns 0 on success, errno otherwise.
extern int fiber_interrupt(fiber_id_t tid);

// Make fiber_stopped() on the fiber return true and interrupt the fiber.
// Note that current fiber_stop() solely sets the built-in "stop flag" and
// calls fiber_interrupt(), which is different from earlier versions of
// fiber, and replaceable by user-defined stop flags plus calls to
// fiber_interrupt().
// Returns 0 on success, errno otherwise.
extern int fiber_stop(fiber_id_t tid);

// Returns 1 iff fiber_stop(tid) was called or the thread does not exist,
// 0 otherwise.
extern int fiber_stopped(fiber_id_t tid);

// Returns identifier of caller if caller is a fiber, 0 otherwise(Id of a
// fiber is never zero)
extern fiber_id_t fiber_self(void);

// Compare two fiber identifiers.
// Returns a non-zero value if t1 and t2 are equal, zero otherwise.
extern int fiber_equal(fiber_id_t t1, fiber_id_t t2);

// Terminate calling fiber/pthread and make `retval' available to any
// successful join with the terminating thread. This function does not return.
extern void fiber_exit(void *retval) __attribute__((__noreturn__));

// Make calling thread wait for termination of fiber `bt'. Return immediately
// if `bt' is already terminated.
// Notes:
//  - All fibers are "detached" but still joinable.
//  - *fiber_return is always set to null. If you need to return value
//    from a fiber, pass the value via the `args' created the fiber.
//  - fiber_join() is not affected by fiber_interrupt.
// Returns 0 on success, errno otherwise.
extern int fiber_join(fiber_id_t bt, void **fiber_return);

// Track and join many fibers.
// Notice that all fiber_list* functions are NOT thread-safe.
extern int fiber_list_init(fiber_list_t *list,
                           unsigned size, unsigned conflict_size);
extern void fiber_list_destroy(fiber_list_t *list);
extern int fiber_list_add(fiber_list_t *list, fiber_id_t tid);
extern int fiber_list_stop(fiber_list_t *list);
extern int fiber_list_join(fiber_list_t *list);

// ------------------------------------------
// Functions for handling attributes.
// ------------------------------------------

// Initialize thread attribute `attr' with default attributes.
extern int fiber_attr_init(fiber_attribute *attr);

// Destroy thread attribute `attr'.
extern int fiber_attr_destroy(fiber_attribute *attr);

// Initialize fiber attribute `attr' with attributes corresponding to the
// already running fiber `bt'.  It shall be called on unitialized `attr'
// and destroyed with fiber_attr_destroy when no longer needed.
extern int fiber_getattr(fiber_id_t bt, fiber_attribute *attr);

// ---------------------------------------------
// Functions for scheduling control.
// ---------------------------------------------

// Get number of worker pthreads
extern int fiber_getconcurrency(void);

// Set number of worker pthreads to `num'. After a successful call,
// fiber_getconcurrency() shall return new set number, but workers may
// take some time to quit or create.
// NOTE: currently concurrency cannot be reduced after any fiber created.
extern int fiber_setconcurrency(int num);


// ---------------------------------------------
// Functions for mutex handling.
// ---------------------------------------------

// Initialize `mutex' using attributes in `mutex_attr', or use the
// default values if later is nullptr.
// NOTE: mutexattr is not used in current mutex implementation. User shall
//       always pass a nullptr attribute.
extern int fiber_mutex_init(fiber_mutex_t *__restrict mutex,
                            const fiber_mutexattr_t *__restrict mutex_attr);

// Destroy `mutex'.
extern int fiber_mutex_destroy(fiber_mutex_t *mutex);

// Try to lock `mutex'.
extern int fiber_mutex_trylock(fiber_mutex_t *mutex);

// Wait until lock for `mutex' becomes available and lock it.
extern int fiber_mutex_lock(fiber_mutex_t *mutex);

// Wait until lock becomes available and lock it or time exceeds `abstime'
extern int fiber_mutex_timedlock(fiber_mutex_t *__restrict mutex,
                                 const struct timespec *__restrict abstime);

// Unlock `mutex'.
extern int fiber_mutex_unlock(fiber_mutex_t *mutex);

// -----------------------------------------------
// Functions for handling conditional variables.
// -----------------------------------------------

// Initialize condition variable `cond' using attributes `cond_attr', or use
// the default values if later is nullptr.
// NOTE: cond_attr is not used in current condition implementation. User shall
//       always pass a nullptr attribute.
extern int fiber_cond_init(fiber_cond_t *__restrict cond,
                           const fiber_condattr_t *__restrict cond_attr);

// Destroy condition variable `cond'.
extern int fiber_cond_destroy(fiber_cond_t *cond);

// Wake up one thread waiting for condition variable `cond'.
extern int fiber_cond_signal(fiber_cond_t *cond);

// Wake up all threads waiting for condition variables `cond'.
extern int fiber_cond_broadcast(fiber_cond_t *cond);

// Wait for condition variable `cond' to be signaled or broadcast.
// `mutex' is assumed to be locked before.
extern int fiber_cond_wait(fiber_cond_t *__restrict cond,
                           fiber_mutex_t *__restrict mutex);

// Wait for condition variable `cond' to be signaled or broadcast until
// `abstime'. `mutex' is assumed to be locked before.  `abstime' is an
// absolute time specification; zero is the beginning of the epoch
// (00:00:00 GMT, January 1, 1970).
extern int fiber_cond_timedwait(
        fiber_cond_t *__restrict cond,
        fiber_mutex_t *__restrict mutex,
        const struct timespec *__restrict abstime);

// -------------------------------------------
// Functions for handling read-write locks.
// -------------------------------------------

// Initialize read-write lock `rwlock' using attributes `attr', or use
// the default values if later is nullptr.
extern int fiber_rwlock_init(fiber_rwlock_t *__restrict rwlock,
                             const fiber_rwlockattr_t *__restrict attr);

// Destroy read-write lock `rwlock'.
extern int fiber_rwlock_destroy(fiber_rwlock_t *rwlock);

// Acquire read lock for `rwlock'.
extern int fiber_rwlock_rdlock(fiber_rwlock_t *rwlock);

// Try to acquire read lock for `rwlock'.
extern int fiber_rwlock_tryrdlock(fiber_rwlock_t *rwlock);

// Try to acquire read lock for `rwlock' or return after specfied time.
extern int fiber_rwlock_timedrdlock(
        fiber_rwlock_t *__restrict rwlock,
        const struct timespec *__restrict abstime);

// Acquire write lock for `rwlock'.
extern int fiber_rwlock_wrlock(fiber_rwlock_t *rwlock);

// Try to acquire write lock for `rwlock'.
extern int fiber_rwlock_trywrlock(fiber_rwlock_t *rwlock);

// Try to acquire write lock for `rwlock' or return after specfied time.
extern int fiber_rwlock_timedwrlock(
        fiber_rwlock_t *__restrict rwlock,
        const struct timespec *__restrict abstime);

// Unlock `rwlock'.
extern int fiber_rwlock_unlock(fiber_rwlock_t *rwlock);

// ---------------------------------------------------
// Functions for handling read-write lock attributes.
// ---------------------------------------------------

// Initialize attribute object `attr' with default values.
extern int fiber_rwlockattr_init(fiber_rwlockattr_t *attr);

// Destroy attribute object `attr'.
extern int fiber_rwlockattr_destroy(fiber_rwlockattr_t *attr);

// Return current setting of reader/writer preference.
extern int fiber_rwlockattr_getkind_np(const fiber_rwlockattr_t *attr,
                                       int *pref);

// Set reader/write preference.
extern int fiber_rwlockattr_setkind_np(fiber_rwlockattr_t *attr,
                                       int pref);


// ----------------------------------------------------------------------
// Functions for handling barrier which is a new feature in 1003.1j-2000.
// ----------------------------------------------------------------------

extern int fiber_barrier_init(fiber_barrier_t *__restrict barrier,
                              const fiber_barrierattr_t *__restrict attr,
                              unsigned count);

extern int fiber_barrier_destroy(fiber_barrier_t *barrier);

extern int fiber_barrier_wait(fiber_barrier_t *barrier);

// ---------------------------------------------------------------------
// Functions for handling thread-specific data. 
// Notice that they can be used in pthread: get pthread-specific data in 
// pthreads and get fiber-specific data in fibers.
// ---------------------------------------------------------------------

// Create a key value identifying a slot in a thread-specific data area.
// Each thread maintains a distinct thread-specific data area.
// `destructor', if non-nullptr, is called with the value associated to that key
// when the key is destroyed. `destructor' is not called if the value
// associated is nullptr when the key is destroyed.
// Returns 0 on success, error code otherwise.
extern int fiber_key_create(fiber_local_key *key,
                            void (*destructor)(void *data));

// Delete a key previously returned by fiber_key_create().
// It is the responsibility of the application to free the data related to
// the deleted key in any running thread. No destructor is invoked by
// this function. Any destructor that may have been associated with key
// will no longer be called upon thread exit.
// Returns 0 on success, error code otherwise.
extern int fiber_key_delete(fiber_local_key key);

// Store `data' in the thread-specific slot identified by `key'.
// fiber_setspecific() is callable from within destructor. If the application
// does so, destructors will be repeatedly called for at most
// PTHREAD_DESTRUCTOR_ITERATIONS times to clear the slots.
// NOTE: If the thread is not created by turbo server and lifetime is
// very short(doing a little thing and exit), avoid using fiber-local. The
// reason is that fiber-local always allocate keytable on first call to 
// fiber_setspecific, the overhead is negligible in long-lived threads,
// but noticeable in shortly-lived threads. Threads in turbo server
// are special since they reuse keytables from a fiber_keytable_pool_t
// in the server.
// Returns 0 on success, error code otherwise.
// If the key is invalid or deleted, return EINVAL.
extern int fiber_setspecific(fiber_local_key key, void *data);

// Return current value of the thread-specific slot identified by `key'.
// If fiber_setspecific() had not been called in the thread, return nullptr.
// If the key is invalid or deleted, return nullptr.
extern void *fiber_getspecific(fiber_local_key key);

__END_DECLS

#endif  // TURBO_INTHERNAL_FIBER_H_
