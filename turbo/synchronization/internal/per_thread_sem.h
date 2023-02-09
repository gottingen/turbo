// Copyright 2020 The Turbo Authors.
//
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

// PerThreadSem is a low-level synchronization primitive controlling the
// runnability of a single thread, used internally by Mutex and CondVar.
//
// This is NOT a general-purpose synchronization mechanism, and should not be
// used directly by applications.  Applications should use Mutex and CondVar.
//
// The semantics of PerThreadSem are the same as that of a counting semaphore.
// Each thread maintains an abstract "count" value associated with its identity.

#ifndef TURBO_SYNCHRONIZATION_INTERNAL_PER_THREAD_SEM_H_
#define TURBO_SYNCHRONIZATION_INTERNAL_PER_THREAD_SEM_H_

#include <atomic>

#include "turbo/platform/internal/thread_identity.h"
#include "turbo/synchronization/internal/create_thread_identity.h"
#include "turbo/synchronization/internal/kernel_timeout.h"

namespace turbo {
TURBO_NAMESPACE_BEGIN

class Mutex;

namespace synchronization_internal {

class PerThreadSem {
 public:
  PerThreadSem() = delete;
  PerThreadSem(const PerThreadSem&) = delete;
  PerThreadSem& operator=(const PerThreadSem&) = delete;

  // Routine invoked periodically (once a second) by a background thread.
  // Has no effect on user-visible state.
  static void Tick(base_internal::ThreadIdentity* identity);

  // ---------------------------------------------------------------------------
  // Routines used by autosizing threadpools to detect when threads are
  // blocked.  Each thread has a counter pointer, initially zero.  If non-zero,
  // the implementation atomically increments the counter when it blocks on a
  // semaphore, a decrements it again when it wakes.  This allows a threadpool
  // to keep track of how many of its threads are blocked.
  // SetThreadBlockedCounter() should be used only by threadpool
  // implementations.  GetThreadBlockedCounter() should be used by modules that
  // block threads; if the pointer returned is non-zero, the location should be
  // incremented before the thread blocks, and decremented after it wakes.
  static void SetThreadBlockedCounter(std::atomic<int> *counter);
  static std::atomic<int> *GetThreadBlockedCounter();

 private:
  // Create the PerThreadSem associated with "identity".  Initializes count=0.
  // REQUIRES: May only be called by ThreadIdentity.
  static void Init(base_internal::ThreadIdentity* identity);

  // Increments "identity"'s count.
  static inline void Post(base_internal::ThreadIdentity* identity);

  // Waits until either our count > 0 or t has expired.
  // If count > 0, decrements count and returns true.  Otherwise returns false.
  // !t.has_timeout() => Wait(t) will return true.
  static inline bool Wait(KernelTimeout t);

  // Permitted callers.
  friend class PerThreadSemTest;
  friend class turbo::Mutex;
  friend void OneTimeInitThreadIdentity(turbo::base_internal::ThreadIdentity*);
};

}  // namespace synchronization_internal
TURBO_NAMESPACE_END
}  // namespace turbo

// In some build configurations we pass --detect-odr-violations to the
// gold linker.  This causes it to flag weak symbol overrides as ODR
// violations.  Because ODR only applies to C++ and not C,
// --detect-odr-violations ignores symbols not mangled with C++ names.
// By changing our extension points to be extern "C", we dodge this
// check.
extern "C" {
void TURBO_INTERNAL_C_SYMBOL(TurboInternalPerThreadSemPost)(
    turbo::base_internal::ThreadIdentity* identity);
bool TURBO_INTERNAL_C_SYMBOL(TurboInternalPerThreadSemWait)(
    turbo::synchronization_internal::KernelTimeout t);
}  // extern "C"

void turbo::synchronization_internal::PerThreadSem::Post(
    turbo::base_internal::ThreadIdentity* identity) {
  TURBO_INTERNAL_C_SYMBOL(TurboInternalPerThreadSemPost)(identity);
}

bool turbo::synchronization_internal::PerThreadSem::Wait(
    turbo::synchronization_internal::KernelTimeout t) {
  return TURBO_INTERNAL_C_SYMBOL(TurboInternalPerThreadSemWait)(t);
}

#endif  // TURBO_SYNCHRONIZATION_INTERNAL_PER_THREAD_SEM_H_
