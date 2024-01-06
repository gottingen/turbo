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


#ifndef TURBO_FIBER_INTERNAL_FIBER_H_
#define TURBO_FIBER_INTERNAL_FIBER_H_

#include <pthread.h>
#include <sys/socket.h>
#include <iostream>
#include "turbo/fiber/internal/mutex.h"
#include "turbo/fiber/internal/token.h"
#include "turbo/fiber/internal/types.h"
#include "turbo/base/status.h"
#include "turbo/platform/port.h"

namespace turbo::fiber_internal {

    // Create fiber `fn(args)' with attributes `attr' and put the identifier into
    // `tid'. Switch to the new thread and schedule old thread to run. Use this
    // function when the new thread is more urgent.
    // Returns 0 on success, errno otherwise.
    [[maybe_unused]] turbo::Status fiber_start_urgent(fiber_id_t *TURBO_RESTRICT tid,
                                            const FiberAttribute *TURBO_RESTRICT attr,
                                            fiber_fn_t &&fn,
                                            void *TURBO_RESTRICT args);

    // Create fiber `fn(args)' with attributes `attr' and put the identifier into
    // `tid'. This function behaves closer to pthread_create: after scheduling the
    // new thread to run, it returns. In another word, the new thread may take
    // longer time than fiber_start_urgent() to run.
    // Return 0 on success, errno otherwise.
    [[maybe_unused]] turbo::Status fiber_start_background(fiber_id_t *TURBO_RESTRICT tid,
                                                const FiberAttribute *TURBO_RESTRICT attr,
                                                fiber_fn_t &&fn,
                                                void *TURBO_RESTRICT args);

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
    turbo::Status fiber_interrupt(fiber_id_t tid);

    // Make fiber_stopped() on the fiber return true and interrupt the fiber.
    // Note that current fiber_stop() solely sets the built-in "stop flag" and
    // calls fiber_interrupt(), which is different from earlier versions of
    // fiber, and replaceable by user-defined stop flags plus calls to
    // fiber_interrupt().
    // Returns 0 on success, errno otherwise.
    turbo::Status fiber_stop(fiber_id_t tid);

    // Returns 1 iff fiber_stop(tid) was called or the thread does not exist,
    // 0 otherwise.
    bool fiber_stopped(fiber_id_t tid);

    // Returns identifier of caller if caller is a fiber, 0 otherwise(Id of a
    // fiber is never zero)
    fiber_id_t fiber_self(void);

    // Compare two fiber identifiers.
    // Returns a non-zero value if t1 and t2 are equal, zero otherwise.
    int fiber_equal(fiber_id_t t1, fiber_id_t t2);

    // Terminate calling fiber/pthread and make `retval' available to any
    // successful join with the terminating thread. This function does not return.
    void fiber_exit(void *retval) __attribute__((__noreturn__));

    // Make calling thread wait for termination of fiber `bt'. Return immediately
    // if `bt' is already terminated.
    // Notes:
    //  - All fibers are "detached" but still joinable.
    //  - *fiber_return is always set to null. If you need to return value
    //    from a fiber, pass the value via the `args' created the fiber.
    //  - fiber_join() is not affected by fiber_interrupt.
    // Returns 0 on success, errno otherwise.
    turbo::Status fiber_join(fiber_id_t bt, void **fiber_return);

    // Track and join many fibers.
    // Notice that all fiber_list* functions are NOT thread-safe.
    int fiber_list_init(fiber_list_t *list,
                               unsigned size, unsigned conflict_size);

    void fiber_list_destroy(fiber_list_t *list);

    int fiber_list_add(fiber_list_t *list, fiber_id_t tid);

    int fiber_list_stop(fiber_list_t *list);

    int fiber_list_join(fiber_list_t *list);

    // ------------------------------------------
    // Functions for handling attributes.
    // ------------------------------------------

    // Initialize thread attribute `attr' with default attributes.
    int fiber_attr_init(FiberAttribute *attr);

    // Destroy thread attribute `attr'.
    int fiber_attr_destroy(FiberAttribute *attr);

    // Initialize fiber attribute `attr' with attributes corresponding to the
    // already running fiber `bt'.  It shall be called on unitialized `attr'
    // and destroyed with fiber_attr_destroy when no longer needed.
    int fiber_getattr(fiber_id_t bt, FiberAttribute *attr);

    // ---------------------------------------------
    // Functions for scheduling control.
    // ---------------------------------------------

    // Get number of worker pthreads
    int fiber_get_concurrency(void);

    // Set number of worker pthreads to `num'. After a successful call,
    // fiber_get_concurrency() shall return new set number, but workers may
    // take some time to quit or create.
    // NOTE: currently concurrency cannot be reduced after any fiber created.
    turbo::Status fiber_set_concurrency(int num);


    void fiber_flush();

    int fiber_about_to_quit();

    // Run `on_timer(arg)' at or after real-time `abstime'. Put identifier of the
    // timer into *id.
    // Return 0 on success, errno otherwise.
    int fiber_timer_add(fiber_timer_id *id, timespec abstime,
                        void (*on_timer)(void *), void *arg);

    // Unschedule the timer associated with `id'.
    // Returns: 0 - exist & not-run; 1 - still running; EINVAL - not exist.
    int fiber_timer_del(fiber_timer_id id);


    // Add a startup function that each pthread worker will run at the beginning
    // To run code at the end, use turbo::thread::atexit()
    // Returns 0 on success, error code otherwise.
    extern int fiber_set_worker_startfn(void (*start_fn)());

    // Stop all fiber and worker pthreads.
    // You should avoid calling this function which may cause fiber after main()
    // suspend indefinitely.
    void fiber_stop_world();

}  // namespace turbo::fiber_internal

#endif  // TURBO_FIBER_INTERNAL_FIBER_H_
