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

//  Most users requiring mutual exclusion should use Mutex.
//  SpinLock is provided for use in two situations:
//   - for use by Turbo internal code that Mutex itself depends on
//   - for async signal safety (see below)

// SpinLock with a base_internal::SchedulingMode::SCHEDULE_KERNEL_ONLY is async
// signal safe. If a spinlock is used within a signal handler, all code that
// acquires the lock must ensure that the signal cannot arrive while they are
// holding the lock. Typically, this is done by blocking the signal.
//
// Threads waiting on a SpinLock may be woken in an arbitrary order.

#ifndef TURBO_BASE_INTERNAL_SPINLOCK_H_
#define TURBO_BASE_INTERNAL_SPINLOCK_H_

#include <atomic>
#include <cstdint>

#include <turbo/base/attributes.h>
#include <turbo/base/const_init.h>
#include <turbo/base/dynamic_annotations.h>
#include <turbo/base/internal/low_level_scheduling.h>
#include <turbo/base/internal/raw_logging.h>
#include <turbo/base/internal/scheduling_mode.h>
#include <turbo/base/internal/tsan_mutex_interface.h>
#include <turbo/base/thread_annotations.h>

namespace tcmalloc {
namespace tcmalloc_internal {

class AllocationGuardSpinLockHolder;

}  // namespace tcmalloc_internal
}  // namespace tcmalloc

namespace turbo {
TURBO_NAMESPACE_BEGIN
namespace base_internal {

class TURBO_LOCKABLE TURBO_ATTRIBUTE_WARN_UNUSED SpinLock {
 public:
  SpinLock() : lockword_(kSpinLockCooperative) {
    TURBO_TSAN_MUTEX_CREATE(this, __tsan_mutex_not_static);
  }

  // Constructors that allow non-cooperative spinlocks to be created for use
  // inside thread schedulers.  Normal clients should not use these.
  explicit SpinLock(base_internal::SchedulingMode mode);

  // Constructor for global SpinLock instances.  See turbo/base/const_init.h.
  constexpr SpinLock(turbo::ConstInitType, base_internal::SchedulingMode mode)
      : lockword_(IsCooperative(mode) ? kSpinLockCooperative : 0) {}

  // For global SpinLock instances prefer trivial destructor when possible.
  // Default but non-trivial destructor in some build configurations causes an
  // extra static initializer.
#ifdef TURBO_INTERNAL_HAVE_TSAN_INTERFACE
  ~SpinLock() { TURBO_TSAN_MUTEX_DESTROY(this, __tsan_mutex_not_static); }
#else
  ~SpinLock() = default;
#endif

  // Acquire this SpinLock.
  inline void Lock() TURBO_EXCLUSIVE_LOCK_FUNCTION() {
    TURBO_TSAN_MUTEX_PRE_LOCK(this, 0);
    if (!TryLockImpl()) {
      SlowLock();
    }
    TURBO_TSAN_MUTEX_POST_LOCK(this, 0, 0);
  }

  // Try to acquire this SpinLock without blocking and return true if the
  // acquisition was successful.  If the lock was not acquired, false is
  // returned.  If this SpinLock is free at the time of the call, TryLock
  // will return true with high probability.
  TURBO_MUST_USE_RESULT inline bool TryLock()
      TURBO_EXCLUSIVE_TRYLOCK_FUNCTION(true) {
    TURBO_TSAN_MUTEX_PRE_LOCK(this, __tsan_mutex_try_lock);
    bool res = TryLockImpl();
    TURBO_TSAN_MUTEX_POST_LOCK(
        this, __tsan_mutex_try_lock | (res ? 0 : __tsan_mutex_try_lock_failed),
        0);
    return res;
  }

  // Release this SpinLock, which must be held by the calling thread.
  inline void Unlock() TURBO_UNLOCK_FUNCTION() {
    TURBO_TSAN_MUTEX_PRE_UNLOCK(this, 0);
    uint32_t lock_value = lockword_.load(std::memory_order_relaxed);
    lock_value = lockword_.exchange(lock_value & kSpinLockCooperative,
                                    std::memory_order_release);

    if ((lock_value & kSpinLockDisabledScheduling) != 0) {
      base_internal::SchedulingGuard::EnableRescheduling(true);
    }
    if ((lock_value & kWaitTimeMask) != 0) {
      // Collect contentionz profile info, and speed the wakeup of any waiter.
      // The wait_cycles value indicates how long this thread spent waiting
      // for the lock.
      SlowUnlock(lock_value);
    }
    TURBO_TSAN_MUTEX_POST_UNLOCK(this, 0);
  }

  // Determine if the lock is held.  When the lock is held by the invoking
  // thread, true will always be returned. Intended to be used as
  // CHECK(lock.IsHeld()).
  TURBO_MUST_USE_RESULT inline bool IsHeld() const {
    return (lockword_.load(std::memory_order_relaxed) & kSpinLockHeld) != 0;
  }

  // Return immediately if this thread holds the SpinLock exclusively.
  // Otherwise, report an error by crashing with a diagnostic.
  inline void AssertHeld() const TURBO_ASSERT_EXCLUSIVE_LOCK() {
    if (!IsHeld()) {
      TURBO_RAW_LOG(FATAL, "thread should hold the lock on SpinLock");
    }
  }

 protected:
  // These should not be exported except for testing.

  // Store number of cycles between wait_start_time and wait_end_time in a
  // lock value.
  static uint32_t EncodeWaitCycles(int64_t wait_start_time,
                                   int64_t wait_end_time);

  // Extract number of wait cycles in a lock value.
  static int64_t DecodeWaitCycles(uint32_t lock_value);

  // Provide access to protected method above.  Use for testing only.
  friend struct SpinLockTest;
  friend class tcmalloc::tcmalloc_internal::AllocationGuardSpinLockHolder;

 private:
  // lockword_ is used to store the following:
  //
  // bit[0] encodes whether a lock is being held.
  // bit[1] encodes whether a lock uses cooperative scheduling.
  // bit[2] encodes whether the current lock holder disabled scheduling when
  //        acquiring the lock. Only set when kSpinLockHeld is also set.
  // bit[3:31] encodes time a lock spent on waiting as a 29-bit unsigned int.
  //        This is set by the lock holder to indicate how long it waited on
  //        the lock before eventually acquiring it. The number of cycles is
  //        encoded as a 29-bit unsigned int, or in the case that the current
  //        holder did not wait but another waiter is queued, the LSB
  //        (kSpinLockSleeper) is set. The implementation does not explicitly
  //        track the number of queued waiters beyond this. It must always be
  //        assumed that waiters may exist if the current holder was required to
  //        queue.
  //
  // Invariant: if the lock is not held, the value is either 0 or
  // kSpinLockCooperative.
  static constexpr uint32_t kSpinLockHeld = 1;
  static constexpr uint32_t kSpinLockCooperative = 2;
  static constexpr uint32_t kSpinLockDisabledScheduling = 4;
  static constexpr uint32_t kSpinLockSleeper = 8;
  // Includes kSpinLockSleeper.
  static constexpr uint32_t kWaitTimeMask =
      ~(kSpinLockHeld | kSpinLockCooperative | kSpinLockDisabledScheduling);

  // Returns true if the provided scheduling mode is cooperative.
  static constexpr bool IsCooperative(
      base_internal::SchedulingMode scheduling_mode) {
    return scheduling_mode == base_internal::SCHEDULE_COOPERATIVE_AND_KERNEL;
  }

  bool IsCooperative() const {
    return lockword_.load(std::memory_order_relaxed) & kSpinLockCooperative;
  }

  uint32_t TryLockInternal(uint32_t lock_value, uint32_t wait_cycles);
  void SlowLock() TURBO_ATTRIBUTE_COLD;
  void SlowUnlock(uint32_t lock_value) TURBO_ATTRIBUTE_COLD;
  uint32_t SpinLoop();

  inline bool TryLockImpl() {
    uint32_t lock_value = lockword_.load(std::memory_order_relaxed);
    return (TryLockInternal(lock_value, 0) & kSpinLockHeld) == 0;
  }

  std::atomic<uint32_t> lockword_;

  SpinLock(const SpinLock&) = delete;
  SpinLock& operator=(const SpinLock&) = delete;
};

// Corresponding locker object that arranges to acquire a spinlock for
// the duration of a C++ scope.
//
// TODO(b/176172494): Use only [[nodiscard]] when baseline is raised.
// TODO(b/6695610): Remove forward declaration when #ifdef is no longer needed.
#if TURBO_HAVE_CPP_ATTRIBUTE(nodiscard)
class [[nodiscard]] SpinLockHolder;
#else
class TURBO_MUST_USE_RESULT TURBO_ATTRIBUTE_TRIVIAL_ABI SpinLockHolder;
#endif

class TURBO_SCOPED_LOCKABLE SpinLockHolder {
 public:
  inline explicit SpinLockHolder(SpinLock* l) TURBO_EXCLUSIVE_LOCK_FUNCTION(l)
      : lock_(l) {
    l->Lock();
  }
  inline ~SpinLockHolder() TURBO_UNLOCK_FUNCTION() { lock_->Unlock(); }

  SpinLockHolder(const SpinLockHolder&) = delete;
  SpinLockHolder& operator=(const SpinLockHolder&) = delete;

 private:
  SpinLock* lock_;
};

// Register a hook for profiling support.
//
// The function pointer registered here will be called whenever a spinlock is
// contended.  The callback is given an opaque handle to the contended spinlock
// and the number of wait cycles.  This is thread-safe, but only a single
// profiler can be registered.  It is an error to call this function multiple
// times with different arguments.
void RegisterSpinLockProfiler(void (*fn)(const void* lock,
                                         int64_t wait_cycles));

//------------------------------------------------------------------------------
// Public interface ends here.
//------------------------------------------------------------------------------

// If (result & kSpinLockHeld) == 0, then *this was successfully locked.
// Otherwise, returns last observed value for lockword_.
inline uint32_t SpinLock::TryLockInternal(uint32_t lock_value,
                                          uint32_t wait_cycles) {
  if ((lock_value & kSpinLockHeld) != 0) {
    return lock_value;
  }

  uint32_t sched_disabled_bit = 0;
  if ((lock_value & kSpinLockCooperative) == 0) {
    // For non-cooperative locks we must make sure we mark ourselves as
    // non-reschedulable before we attempt to CompareAndSwap.
    if (base_internal::SchedulingGuard::DisableRescheduling()) {
      sched_disabled_bit = kSpinLockDisabledScheduling;
    }
  }

  if (!lockword_.compare_exchange_strong(
          lock_value,
          kSpinLockHeld | lock_value | wait_cycles | sched_disabled_bit,
          std::memory_order_acquire, std::memory_order_relaxed)) {
    base_internal::SchedulingGuard::EnableRescheduling(sched_disabled_bit != 0);
  }

  return lock_value;
}

}  // namespace base_internal
TURBO_NAMESPACE_END
}  // namespace turbo

#endif  // TURBO_BASE_INTERNAL_SPINLOCK_H_
