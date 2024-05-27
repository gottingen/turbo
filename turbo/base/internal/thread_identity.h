// Copyright (C) 2024 EA group inc.
// Author: Jeff.li lijippy@163.com
// All rights reserved.
// This program is free software: you can redistribute it and/or modify
// it under the terms of the GNU Affero General Public License as published
// by the Free Software Foundation, either version 3 of the License, or
// (at your option) any later version.
//
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU Affero General Public License for more details.
//
// You should have received a copy of the GNU Affero General Public License
// along with this program.  If not, see <https://www.gnu.org/licenses/>.
//
//
// Each active thread has an ThreadIdentity that may represent the thread in
// various level interfaces.  ThreadIdentity objects are never deallocated.
// When a thread terminates, its ThreadIdentity object may be reused for a
// thread created later.

#ifndef TURBO_BASE_INTERNAL_THREAD_IDENTITY_H_
#define TURBO_BASE_INTERNAL_THREAD_IDENTITY_H_

#ifndef _WIN32
#include <pthread.h>
// Defines __GOOGLE_GRTE_VERSION__ (via glibc-specific features.h) when
// supported.
#include <unistd.h>
#endif

#include <atomic>
#include <cstdint>

#include <turbo/base/config.h>
#include <turbo/base/internal/per_thread_tls.h>
#include <turbo/base/optimization.h>

namespace turbo {
TURBO_NAMESPACE_BEGIN

struct SynchLocksHeld;
struct SynchWaitParams;

namespace base_internal {

class SpinLock;
struct ThreadIdentity;

// Used by the implementation of turbo::Mutex and turbo::CondVar.
struct PerThreadSynch {
  // The internal representation of turbo::Mutex and turbo::CondVar rely
  // on the alignment of PerThreadSynch. Both store the address of the
  // PerThreadSynch in the high-order bits of their internal state,
  // which means the low kLowZeroBits of the address of PerThreadSynch
  // must be zero.
  static constexpr int kLowZeroBits = 8;
  static constexpr int kAlignment = 1 << kLowZeroBits;

  // Returns the associated ThreadIdentity.
  // This can be implemented as a cast because we guarantee
  // PerThreadSynch is the first element of ThreadIdentity.
  ThreadIdentity* thread_identity() {
    return reinterpret_cast<ThreadIdentity*>(this);
  }

  PerThreadSynch* next;  // Circular waiter queue; initialized to 0.
  PerThreadSynch* skip;  // If non-zero, all entries in Mutex queue
                         // up to and including "skip" have same
                         // condition as this, and will be woken later
  bool may_skip;         // if false while on mutex queue, a mutex unlocker
                         // is using this PerThreadSynch as a terminator.  Its
                         // skip field must not be filled in because the loop
                         // might then skip over the terminator.
  bool wake;             // This thread is to be woken from a Mutex.
  // If "x" is on a waiter list for a mutex, "x->cond_waiter" is true iff the
  // waiter is waiting on the mutex as part of a CV Wait or Mutex Await.
  //
  // The value of "x->cond_waiter" is meaningless if "x" is not on a
  // Mutex waiter list.
  bool cond_waiter;
  bool maybe_unlocking;  // Valid at head of Mutex waiter queue;
                         // true if UnlockSlow could be searching
                         // for a waiter to wake.  Used for an optimization
                         // in Enqueue().  true is always a valid value.
                         // Can be reset to false when the unlocker or any
                         // writer releases the lock, or a reader fully
                         // releases the lock.  It may not be set to false
                         // by a reader that decrements the count to
                         // non-zero. protected by mutex spinlock
  bool suppress_fatal_errors;  // If true, try to proceed even in the face
                               // of broken invariants.  This is used within
                               // fatal signal handlers to improve the
                               // chances of debug logging information being
                               // output successfully.
  int priority;                // Priority of thread (updated every so often).

  // State values:
  //   kAvailable: This PerThreadSynch is available.
  //   kQueued: This PerThreadSynch is unavailable, it's currently queued on a
  //            Mutex or CondVar waistlist.
  //
  // Transitions from kQueued to kAvailable require a release
  // barrier. This is needed as a waiter may use "state" to
  // independently observe that it's no longer queued.
  //
  // Transitions from kAvailable to kQueued require no barrier, they
  // are externally ordered by the Mutex.
  enum State { kAvailable, kQueued };
  std::atomic<State> state;

  // The wait parameters of the current wait.  waitp is null if the
  // thread is not waiting. Transitions from null to non-null must
  // occur before the enqueue commit point (state = kQueued in
  // Enqueue() and CondVarEnqueue()). Transitions from non-null to
  // null must occur after the wait is finished (state = kAvailable in
  // Mutex::Block() and CondVar::WaitCommon()). This field may be
  // changed only by the thread that describes this PerThreadSynch.  A
  // special case is Fer(), which calls Enqueue() on another thread,
  // but with an identical SynchWaitParams pointer, thus leaving the
  // pointer unchanged.
  SynchWaitParams* waitp;

  intptr_t readers;  // Number of readers in mutex.

  // When priority will next be read (cycles).
  int64_t next_priority_read_cycles;

  // Locks held; used during deadlock detection.
  // Allocated in Synch_GetAllLocks() and freed in ReclaimThreadIdentity().
  SynchLocksHeld* all_locks;
};

// The instances of this class are allocated in NewThreadIdentity() with an
// alignment of PerThreadSynch::kAlignment.
//
// NOTE: The layout of fields in this structure is critical, please do not
//       add, remove, or modify the field placements without fully auditing the
//       layout.
struct ThreadIdentity {
  // Must be the first member.  The Mutex implementation requires that
  // the PerThreadSynch object associated with each thread is
  // PerThreadSynch::kAlignment aligned.  We provide this alignment on
  // ThreadIdentity itself.
  PerThreadSynch per_thread_synch;

  // Private: Reserved for turbo::synchronization_internal::Waiter.
  struct WaiterState {
    alignas(void*) char data[256];
  } waiter_state;

  // Used by PerThreadSem::{Get,Set}ThreadBlockedCounter().
  std::atomic<int>* blocked_count_ptr;

  // The following variables are mostly read/written just by the
  // thread itself.  The only exception is that these are read by
  // a ticker thread as a hint.
  std::atomic<int> ticker;      // Tick counter, incremented once per second.
  std::atomic<int> wait_start;  // Ticker value when thread started waiting.
  std::atomic<bool> is_idle;    // Has thread become idle yet?

  ThreadIdentity* next;
};

// Returns the ThreadIdentity object representing the calling thread; guaranteed
// to be unique for its lifetime.  The returned object will remain valid for the
// program's lifetime; although it may be re-assigned to a subsequent thread.
// If one does not exist, return nullptr instead.
//
// Does not malloc(*), and is async-signal safe.
// [*] Technically pthread_setspecific() does malloc on first use; however this
// is handled internally within tcmalloc's initialization already. Note that
// darwin does *not* use tcmalloc, so this can catch you if using MallocHooks
// on Apple platforms. Whatever function is calling your MallocHooks will need
// to watch for recursion on Apple platforms.
//
// New ThreadIdentity objects can be constructed and associated with a thread
// by calling GetOrCreateCurrentThreadIdentity() in per-thread-sem.h.
ThreadIdentity* CurrentThreadIdentityIfPresent();

using ThreadIdentityReclaimerFunction = void (*)(void*);

// Sets the current thread identity to the given value.  'reclaimer' is a
// pointer to the global function for cleaning up instances on thread
// destruction.
void SetCurrentThreadIdentity(ThreadIdentity* identity,
                              ThreadIdentityReclaimerFunction reclaimer);

// Removes the currently associated ThreadIdentity from the running thread.
// This must be called from inside the ThreadIdentityReclaimerFunction, and only
// from that function.
void ClearCurrentThreadIdentity();

// May be chosen at compile time via: -DTURBO_FORCE_THREAD_IDENTITY_MODE=<mode
// index>
#ifdef TURBO_THREAD_IDENTITY_MODE_USE_POSIX_SETSPECIFIC
#error TURBO_THREAD_IDENTITY_MODE_USE_POSIX_SETSPECIFIC cannot be directly set
#else
#define TURBO_THREAD_IDENTITY_MODE_USE_POSIX_SETSPECIFIC 0
#endif

#ifdef TURBO_THREAD_IDENTITY_MODE_USE_TLS
#error TURBO_THREAD_IDENTITY_MODE_USE_TLS cannot be directly set
#else
#define TURBO_THREAD_IDENTITY_MODE_USE_TLS 1
#endif

#ifdef TURBO_THREAD_IDENTITY_MODE_USE_CPP11
#error TURBO_THREAD_IDENTITY_MODE_USE_CPP11 cannot be directly set
#else
#define TURBO_THREAD_IDENTITY_MODE_USE_CPP11 2
#endif

#ifdef TURBO_THREAD_IDENTITY_MODE
#error TURBO_THREAD_IDENTITY_MODE cannot be directly set
#elif defined(TURBO_FORCE_THREAD_IDENTITY_MODE)
#define TURBO_THREAD_IDENTITY_MODE TURBO_FORCE_THREAD_IDENTITY_MODE
#elif defined(_WIN32) && !defined(__MINGW32__)
#define TURBO_THREAD_IDENTITY_MODE TURBO_THREAD_IDENTITY_MODE_USE_CPP11
#elif defined(__APPLE__) && defined(TURBO_HAVE_THREAD_LOCAL)
#define TURBO_THREAD_IDENTITY_MODE TURBO_THREAD_IDENTITY_MODE_USE_CPP11
#elif TURBO_PER_THREAD_TLS && defined(__GOOGLE_GRTE_VERSION__) && \
    (__GOOGLE_GRTE_VERSION__ >= 20140228L)
// Support for async-safe TLS was specifically added in GRTEv4.  It's not
// present in the upstream eglibc.
// Note:  Current default for production systems.
#define TURBO_THREAD_IDENTITY_MODE TURBO_THREAD_IDENTITY_MODE_USE_TLS
#else
#define TURBO_THREAD_IDENTITY_MODE \
  TURBO_THREAD_IDENTITY_MODE_USE_POSIX_SETSPECIFIC
#endif

#if TURBO_THREAD_IDENTITY_MODE == TURBO_THREAD_IDENTITY_MODE_USE_TLS || \
    TURBO_THREAD_IDENTITY_MODE == TURBO_THREAD_IDENTITY_MODE_USE_CPP11

#if TURBO_PER_THREAD_TLS
TURBO_CONST_INIT extern TURBO_PER_THREAD_TLS_KEYWORD ThreadIdentity*
    thread_identity_ptr;
#elif defined(TURBO_HAVE_THREAD_LOCAL)
TURBO_CONST_INIT extern thread_local ThreadIdentity* thread_identity_ptr;
#else
#error Thread-local storage not detected on this platform
#endif

// thread_local variables cannot be in headers exposed by DLLs or in certain
// build configurations on Apple platforms. However, it is important for
// performance reasons in general that `CurrentThreadIdentityIfPresent` be
// inlined. In the other cases we opt to have the function not be inlined. Note
// that `CurrentThreadIdentityIfPresent` is declared above so we can exclude
// this entire inline definition.
#if !defined(__APPLE__) && !defined(TURBO_BUILD_DLL) && \
    !defined(TURBO_CONSUME_DLL)
#define TURBO_INTERNAL_INLINE_CURRENT_THREAD_IDENTITY_IF_PRESENT 1
#endif

#ifdef TURBO_INTERNAL_INLINE_CURRENT_THREAD_IDENTITY_IF_PRESENT
inline ThreadIdentity* CurrentThreadIdentityIfPresent() {
  return thread_identity_ptr;
}
#endif

#elif TURBO_THREAD_IDENTITY_MODE != \
    TURBO_THREAD_IDENTITY_MODE_USE_POSIX_SETSPECIFIC
#error Unknown TURBO_THREAD_IDENTITY_MODE
#endif

}  // namespace base_internal
TURBO_NAMESPACE_END
}  // namespace turbo

#endif  // TURBO_BASE_INTERNAL_THREAD_IDENTITY_H_
