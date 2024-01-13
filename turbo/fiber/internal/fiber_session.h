
#ifndef TURBO_FIBER_INTERNAL_SESSION_H_
#define TURBO_FIBER_INTERNAL_SESSION_H_

#include "turbo/platform/port.h"              // TURBO_STRINGIFY
#include "turbo/fiber/internal/types.h"
#include "turbo/fiber/fiber_mutex.h"

namespace turbo::fiber_internal {

    // ----------------------------------------------------------------------
    // Functions to create 64-bit identifiers that can be attached with data
    // and locked without ABA issues. All functions can be called from
    // multiple threads simultaneously. Notice that FiberSessionImpl is designed
    // for managing a series of non-heavily-contended actions on an object.
    // It's slower than mutex and not proper for general synchronizations.
    // ----------------------------------------------------------------------

    // Create a FiberSessionImpl and put it into *id. Crash when `id' is nullptr.
    // id->value will never be zero.
    // `on_error' will be called after fiber_session_error() is called.
    // -------------------------------------------------------------------------
    // ! User must call fiber_session_unlock() or fiber_session_unlock_and_destroy()
    // ! inside on_error.
    // -------------------------------------------------------------------------
    // Returns 0 on success, error code otherwise.
    int fiber_session_create(
            FiberSessionImpl *id, void *data,
            const session_on_error &on_error);

    // When this function is called successfully, *id, *id+1 ... *id + range - 1
    // are mapped to same internal entity. Operations on any of the id work as
    // if they're manipulating a same id. `on_error' is called with the id issued
    // by corresponding fiber_session_error(). This is designed to let users encode
    // versions into identifiers.
    // `range' is limited inside [1, 1024].
    int fiber_session_create_ranged(
            FiberSessionImpl *id, void *data,
            const session_on_error &on_error,
            int range);

    // Wait until `id' being destroyed.
    // Waiting on a destroyed FiberSessionImpl returns immediately.
    // Returns 0 on success, error code otherwise.
    int fiber_session_join(FiberSessionImpl id);

    // Destroy a created but never-used FiberSessionImpl.
    // Returns 0 on success, EINVAL otherwise.
    int fiber_session_cancel(FiberSessionImpl id);

    // Issue an error to `id'.
    // If `id' is not locked, lock the id and run `on_error' immediately. Otherwise
    // `on_error' will be called with the error just before `id' being unlocked.
    // If `id' is destroyed, un-called on_error are dropped.
    // Returns 0 on success, error code otherwise.

    int fiber_session_error_verbose(FiberSessionImpl id, int error_code,
                                    const char *location);

    // Make other fiber_session_lock/fiber_session_trylock on the id fail, the id must
    // already be locked. If the id is unlocked later rather than being destroyed,
    // effect of this function is cancelled. This function avoids useless
    // waiting on a fiber_token which will be destroyed soon but still needs to
    // be joinable.
    // Returns 0 on success, error code otherwise.
    int fiber_session_about_to_destroy(FiberSessionImpl id);

    // Try to lock `id' (for using the data exclusively)
    // On success return 0 and set `pdata' with the `data' parameter to
    // fiber_session_create[_ranged], EBUSY on already locked, error code otherwise.
    int fiber_session_trylock(FiberSessionImpl id, void **pdata);

    // Lock `id' (for using the data exclusively). If `id' is locked
    // by others, wait until `id' is unlocked or destroyed.
    // On success return 0 and set `pdata' with the `data' parameter to
    // fiber_session_create[_ranged], error code otherwise.


    int fiber_session_lock_verbose(FiberSessionImpl id, void **pdata,
                                   const char *location);

    // Lock `id' (for using the data exclusively) and reset the range. If `id' is
    // locked by others, wait until `id' is unlocked or destroyed. if `range' is
    // smaller than the original range of this id, nothing happens about the range
    int fiber_session_lock_and_reset_range_verbose(
            FiberSessionImpl id, void **pdata,
            int range, const char *location);

    // Unlock `id'. Must be called after a successful call to fiber_session_trylock()
    // or fiber_session_lock().
    // Returns 0 on success, error code otherwise.
    int fiber_session_unlock(FiberSessionImpl id);

    // Unlock and destroy `id'. Waiters blocking on fiber_session_join() or
    // fiber_session_lock() will wake up. Must be called after a successful call to
    // fiber_session_trylock() or fiber_session_lock().
    // Returns 0 on success, error code otherwise.
    int fiber_session_unlock_and_destroy(FiberSessionImpl id);

    // **************************************************************************
    // fiber_token_list_xxx functions are NOT thread-safe unless explicitly stated

    // Initialize a list for storing FiberSessionImpl. When an id is destroyed, it will
    // be removed from the list automatically.
    // The commented parameters are not used anymore and just kept to not break
    // compatibility.
    int fiber_session_list_init(FiberSessionList *list,
                                unsigned /*size*/,
                                unsigned /*conflict_size*/);

    // Destroy the list.
    void fiber_session_list_destroy(FiberSessionList *list);

    // Add a FiberSessionImpl into the list.
    int fiber_session_list_add(FiberSessionList *list, FiberSessionImpl id);

    // Swap internal fields of two lists.
    void fiber_session_list_swap(FiberSessionList *dest,
                                 FiberSessionList *src);

    // Issue error_code to all FiberSessionImpl inside `list' and clear `list'.
    // Notice that this function iterates all id inside the list and may call
    // on_error() of each valid id in-place, in another word, this thread-unsafe
    // function is not suitable to be enclosed within a lock directly.
    // To make the critical section small, swap the list inside the lock and
    // reset the swapped list outside the lock as follows:
    //   FiberSessionList tmplist;
    //   fiber_session_list_init(&tmplist, 0, 0);
    //   LOCK;
    //   fiber_session_list_swap(&tmplist, &the_list_to_reset);
    //   UNLOCK;
    //   fiber_session_list_reset(&tmplist, error_code);
    //   fiber_session_list_destroy(&tmplist);
    int fiber_session_list_reset(FiberSessionList *list, int error_code);

    // Following 2 functions wrap above process.
    int fiber_session_list_reset_pthreadsafe(
            FiberSessionList *list, int error_code, std::mutex *mutex);

    int fiber_session_list_reset_fibersafe(
            FiberSessionList *list, int error_code, FiberMutex *mutex);


    // cpp specific API, with an extra `error_text' so that error information
    // is more comprehensive
    int fiber_session_create2(
            FiberSessionImpl *id, void *data,
            const session_on_error_msg &on_error);

    int fiber_session_create2_ranged(
            FiberSessionImpl *id, void *data,
            const session_on_error_msg &on_error,
            int range);

    int fiber_session_error2_verbose(FiberSessionImpl id, int error_code,
                                     const std::string &error_text,
                                     const char *location);

    int fiber_session_reset2(FiberSessionList *list, int error_code,
                             const std::string &error_text);

    int fiber_session_list_reset2_pthreadsafe(FiberSessionList *list, int error_code,
                                              const std::string &error_text,
                                              std::mutex *mutex);

    int fiber_session_list_reset2_fibersafe(FiberSessionList *list, int error_code,
                                            const std::string &error_text,
                                            FiberMutex *mutex);

}  // namespace turbo::fiber_internal

#endif  // TURBO_FIBER_INTERNAL_SESSION_H_
