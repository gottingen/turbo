// Copyright 2022 The Turbo Authors.
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

#include <cstdlib>
#include <thread>  // NOLINT(build/c++11), Turbo test
#include <type_traits>

#include "turbo/base/internal/raw_logging.h"
#include "turbo/platform/port.h"
#include "turbo/platform/const_init.h"
#include "turbo/platform/thread_annotations.h"
#include "turbo/synchronization/mutex.h"
#include "turbo/synchronization/notification.h"

namespace {

// A two-threaded test which checks that Mutex, CondVar, and Notification have
// correct basic functionality.  The intent is to establish that they
// function correctly in various phases of construction and destruction.
//
// Thread one acquires a lock on 'mutex', wakes thread two via 'notification',
// then waits for 'state' to be set, as signalled by 'condvar'.
//
// Thread two waits on 'notification', then sets 'state' inside the 'mutex',
// signalling the change via 'condvar'.
//
// These tests use TURBO_RAW_CHECK to validate invariants, rather than EXPECT or
// ASSERT from gUnit, because we need to invoke them during global destructors,
// when gUnit teardown would have already begun.
void ThreadOne(turbo::Mutex* mutex, turbo::CondVar* condvar,
               turbo::Notification* notification, bool* state) {
  // Test that the notification is in a valid initial state.
  TURBO_RAW_CHECK(!notification->HasBeenNotified(), "invalid Notification");
  TURBO_RAW_CHECK(*state == false, "*state not initialized");

  {
    turbo::MutexLock lock(mutex);

    notification->Notify();
    TURBO_RAW_CHECK(notification->HasBeenNotified(), "invalid Notification");

    while (*state == false) {
      condvar->Wait(mutex);
    }
  }
}

void ThreadTwo(turbo::Mutex* mutex, turbo::CondVar* condvar,
               turbo::Notification* notification, bool* state) {
  TURBO_RAW_CHECK(*state == false, "*state not initialized");

  // Wake thread one
  notification->WaitForNotification();
  TURBO_RAW_CHECK(notification->HasBeenNotified(), "invalid Notification");
  {
    turbo::MutexLock lock(mutex);
    *state = true;
    condvar->Signal();
  }
}

// Launch thread 1 and thread 2, and block on their completion.
// If any of 'mutex', 'condvar', or 'notification' is nullptr, use a locally
// constructed instance instead.
void RunTests(turbo::Mutex* mutex, turbo::CondVar* condvar) {
  turbo::Mutex default_mutex;
  turbo::CondVar default_condvar;
  turbo::Notification notification;
  if (!mutex) {
    mutex = &default_mutex;
  }
  if (!condvar) {
    condvar = &default_condvar;
  }
  bool state = false;
  std::thread thread_one(ThreadOne, mutex, condvar, &notification, &state);
  std::thread thread_two(ThreadTwo, mutex, condvar, &notification, &state);
  thread_one.join();
  thread_two.join();
}

void TestLocals() {
  turbo::Mutex mutex;
  turbo::CondVar condvar;
  RunTests(&mutex, &condvar);
}

// Normal kConstInit usage
TURBO_CONST_INIT turbo::Mutex const_init_mutex(turbo::kConstInit);
void TestConstInitGlobal() { RunTests(&const_init_mutex, nullptr); }

// Global variables during start and termination
//
// In a translation unit, static storage duration variables are initialized in
// the order of their definitions, and destroyed in the reverse order of their
// definitions.  We can use this to arrange for tests to be run on these objects
// before they are created, and after they are destroyed.

using Function = void (*)();

class OnConstruction {
 public:
  explicit OnConstruction(Function fn) { fn(); }
};

class OnDestruction {
 public:
  explicit OnDestruction(Function fn) : fn_(fn) {}
  ~OnDestruction() { fn_(); }
 private:
  Function fn_;
};

// These tests require that the compiler correctly supports C++11 constant
// initialization... but MSVC has a known regression since v19.10 till v19.25:
// https://developercommunity.visualstudio.com/content/problem/336946/class-with-constexpr-constructor-not-using-static.html
#if defined(__clang__) || \
    !(defined(_MSC_VER) && _MSC_VER > 1900 && _MSC_VER < 1925)
// kConstInit
// Test early usage.  (Declaration comes first; definitions must appear after
// the test runner.)
extern turbo::Mutex early_const_init_mutex;
// (Normally I'd write this +[], to make the cast-to-function-pointer explicit,
// but in some MSVC setups we support, lambdas provide conversion operators to
// different flavors of function pointers, making this trick ambiguous.)
OnConstruction test_early_const_init([] {
  RunTests(&early_const_init_mutex, nullptr);
});
// This definition appears before test_early_const_init, but it should be
// initialized first (due to constant initialization).  Test that the object
// actually works when constructed this way.
TURBO_CONST_INIT turbo::Mutex early_const_init_mutex(turbo::kConstInit);

// Furthermore, test that the const-init c'tor doesn't stomp over the state of
// a Mutex.  Really, this is a test that the platform under test correctly
// supports C++11 constant initialization.  (The constant-initialization
// constructors of globals "happen at link time"; memory is pre-initialized,
// before the constructors of either grab_lock or check_still_locked are run.)
extern turbo::Mutex const_init_sanity_mutex;
OnConstruction grab_lock([]() TURBO_NO_THREAD_SAFETY_ANALYSIS {
  const_init_sanity_mutex.Lock();
});
TURBO_CONST_INIT turbo::Mutex const_init_sanity_mutex(turbo::kConstInit);
OnConstruction check_still_locked([]() TURBO_NO_THREAD_SAFETY_ANALYSIS {
  const_init_sanity_mutex.AssertHeld();
  const_init_sanity_mutex.Unlock();
});
#endif  // defined(__clang__) || !(defined(_MSC_VER) && _MSC_VER > 1900)

// Test shutdown usage.  (Declarations come first; definitions must appear after
// the test runner.)
extern turbo::Mutex late_const_init_mutex;
// OnDestruction is being used here as a global variable, even though it has a
// non-trivial destructor.  This is against the style guide.  We're violating
// that rule here to check that the exception we allow for kConstInit is safe.
// NOLINTNEXTLINE
OnDestruction test_late_const_init([] {
  RunTests(&late_const_init_mutex, nullptr);
});
TURBO_CONST_INIT turbo::Mutex late_const_init_mutex(turbo::kConstInit);

}  // namespace

int main() {
  TestLocals();
  TestConstInitGlobal();
  // Explicitly call exit(0) here, to make it clear that we intend for the
  // above global object destructors to run.
  std::exit(0);
}
