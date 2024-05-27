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
// Core interfaces and definitions used by by low-level interfaces such as
// SpinLock.

#ifndef TURBO_BASE_INTERNAL_LOW_LEVEL_SCHEDULING_H_
#define TURBO_BASE_INTERNAL_LOW_LEVEL_SCHEDULING_H_

#include <turbo/base/internal/raw_logging.h>
#include <turbo/base/internal/scheduling_mode.h>
#include <turbo/base/macros.h>

// The following two declarations exist so SchedulingGuard may friend them with
// the appropriate language linkage.  These callbacks allow libc internals, such
// as function level statics, to schedule cooperatively when locking.
extern "C" bool __google_disable_rescheduling(void);
extern "C" void __google_enable_rescheduling(bool disable_result);

namespace turbo {
TURBO_NAMESPACE_BEGIN
class CondVar;
class Mutex;

namespace synchronization_internal {
int MutexDelay(int32_t c, int mode);
}  // namespace synchronization_internal

namespace base_internal {

class SchedulingHelper;  // To allow use of SchedulingGuard.
class SpinLock;          // To allow use of SchedulingGuard.

// SchedulingGuard
// Provides guard semantics that may be used to disable cooperative rescheduling
// of the calling thread within specific program blocks.  This is used to
// protect resources (e.g. low-level SpinLocks or Domain code) that cooperative
// scheduling depends on.
//
// Domain implementations capable of rescheduling in reaction to involuntary
// kernel thread actions (e.g blocking due to a pagefault or syscall) must
// guarantee that an annotated thread is not allowed to (cooperatively)
// reschedule until the annotated region is complete.
//
// It is an error to attempt to use a cooperatively scheduled resource (e.g.
// Mutex) within a rescheduling-disabled region.
//
// All methods are async-signal safe.
class SchedulingGuard {
 public:
  // Returns true iff the calling thread may be cooperatively rescheduled.
  static bool ReschedulingIsAllowed();
  SchedulingGuard(const SchedulingGuard&) = delete;
  SchedulingGuard& operator=(const SchedulingGuard&) = delete;

 private:
  // Disable cooperative rescheduling of the calling thread.  It may still
  // initiate scheduling operations (e.g. wake-ups), however, it may not itself
  // reschedule.  Nestable.  The returned result is opaque, clients should not
  // attempt to interpret it.
  // REQUIRES: Result must be passed to a pairing EnableScheduling().
  static bool DisableRescheduling();

  // Marks the end of a rescheduling disabled region, previously started by
  // DisableRescheduling().
  // REQUIRES: Pairs with innermost call (and result) of DisableRescheduling().
  static void EnableRescheduling(bool disable_result);

  // A scoped helper for {Disable, Enable}Rescheduling().
  // REQUIRES: destructor must run in same thread as constructor.
  struct ScopedDisable {
    ScopedDisable() { disabled = SchedulingGuard::DisableRescheduling(); }
    ~ScopedDisable() { SchedulingGuard::EnableRescheduling(disabled); }

    bool disabled;
  };

  // A scoped helper to enable rescheduling temporarily.
  // REQUIRES: destructor must run in same thread as constructor.
  class ScopedEnable {
   public:
    ScopedEnable();
    ~ScopedEnable();

   private:
    int scheduling_disabled_depth_;
  };

  // Access to SchedulingGuard is explicitly permitted.
  friend class turbo::CondVar;
  friend class turbo::Mutex;
  friend class SchedulingHelper;
  friend class SpinLock;
  friend int turbo::synchronization_internal::MutexDelay(int32_t c, int mode);
};

//------------------------------------------------------------------------------
// End of public interfaces.
//------------------------------------------------------------------------------

inline bool SchedulingGuard::ReschedulingIsAllowed() {
  return false;
}

inline bool SchedulingGuard::DisableRescheduling() {
  return false;
}

inline void SchedulingGuard::EnableRescheduling(bool /* disable_result */) {
  return;
}

inline SchedulingGuard::ScopedEnable::ScopedEnable()
    : scheduling_disabled_depth_(0) {}
inline SchedulingGuard::ScopedEnable::~ScopedEnable() {
  TURBO_RAW_CHECK(scheduling_disabled_depth_ == 0, "disable unused warning");
}

}  // namespace base_internal
TURBO_NAMESPACE_END
}  // namespace turbo

#endif  // TURBO_BASE_INTERNAL_LOW_LEVEL_SCHEDULING_H_
