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

#include <turbo/synchronization/internal/waiter.h>

#include <ctime>
#include <iostream>
#include <ostream>

#include <turbo/base/config.h>
#include <turbo/random/random.h>
#include <turbo/synchronization/internal/create_thread_identity.h>
#include <turbo/synchronization/internal/futex_waiter.h>
#include <turbo/synchronization/internal/kernel_timeout.h>
#include <turbo/synchronization/internal/pthread_waiter.h>
#include <turbo/synchronization/internal/sem_waiter.h>
#include <turbo/synchronization/internal/stdcpp_waiter.h>
#include <turbo/synchronization/internal/thread_pool.h>
#include <turbo/synchronization/internal/win32_waiter.h>
#include <turbo/time/clock.h>
#include <turbo/time/time.h>
#include <gtest/gtest.h>

// Test go/btm support by randomizing the value of clock_gettime() for
// CLOCK_MONOTONIC. This works by overriding a weak symbol in glibc.
// We should be resistant to this randomization when !SupportsSteadyClock().
#if defined(__GOOGLE_GRTE_VERSION__) &&      \
    !defined(TURBO_HAVE_ADDRESS_SANITIZER) && \
    !defined(TURBO_HAVE_MEMORY_SANITIZER) &&  \
    !defined(TURBO_HAVE_THREAD_SANITIZER)
extern "C" int __clock_gettime(clockid_t c, struct timespec* ts);

extern "C" int clock_gettime(clockid_t c, struct timespec* ts) {
  if (c == CLOCK_MONOTONIC &&
      !turbo::synchronization_internal::KernelTimeout::SupportsSteadyClock()) {
    thread_local turbo::BitGen gen;  // NOLINT
    ts->tv_sec = turbo::Uniform(gen, 0, 1'000'000'000);
    ts->tv_nsec = turbo::Uniform(gen, 0, 1'000'000'000);
    return 0;
  }
  return __clock_gettime(c, ts);
}
#endif

namespace {

TEST(Waiter, PrintPlatformImplementation) {
  // Allows us to verify that the platform is using the expected implementation.
  std::cout << turbo::synchronization_internal::Waiter::kName << std::endl;
}

template <typename T>
class WaiterTest : public ::testing::Test {
 public:
  // Waiter implementations assume that a ThreadIdentity has already been
  // created.
  WaiterTest() {
    turbo::synchronization_internal::GetOrCreateCurrentThreadIdentity();
  }
};

TYPED_TEST_SUITE_P(WaiterTest);

turbo::Duration WithTolerance(turbo::Duration d) { return d * 0.95; }

TYPED_TEST_P(WaiterTest, WaitNoTimeout) {
  turbo::synchronization_internal::ThreadPool tp(1);
  TypeParam waiter;
  tp.Schedule([&]() {
    // Include some `Poke()` calls to ensure they don't cause `waiter` to return
    // from `Wait()`.
    waiter.Poke();
    turbo::SleepFor(turbo::Seconds(1));
    waiter.Poke();
    turbo::SleepFor(turbo::Seconds(1));
    waiter.Post();
  });
  turbo::Time start = turbo::Now();
  EXPECT_TRUE(
      waiter.Wait(turbo::synchronization_internal::KernelTimeout::Never()));
  turbo::Duration waited = turbo::Now() - start;
  EXPECT_GE(waited, WithTolerance(turbo::Seconds(2)));
}

TYPED_TEST_P(WaiterTest, WaitDurationWoken) {
  turbo::synchronization_internal::ThreadPool tp(1);
  TypeParam waiter;
  tp.Schedule([&]() {
    // Include some `Poke()` calls to ensure they don't cause `waiter` to return
    // from `Wait()`.
    waiter.Poke();
    turbo::SleepFor(turbo::Milliseconds(500));
    waiter.Post();
  });
  turbo::Time start = turbo::Now();
  EXPECT_TRUE(waiter.Wait(
      turbo::synchronization_internal::KernelTimeout(turbo::Seconds(10))));
  turbo::Duration waited = turbo::Now() - start;
  EXPECT_GE(waited, WithTolerance(turbo::Milliseconds(500)));
  EXPECT_LT(waited, turbo::Seconds(2));
}

TYPED_TEST_P(WaiterTest, WaitTimeWoken) {
  turbo::synchronization_internal::ThreadPool tp(1);
  TypeParam waiter;
  tp.Schedule([&]() {
    // Include some `Poke()` calls to ensure they don't cause `waiter` to return
    // from `Wait()`.
    waiter.Poke();
    turbo::SleepFor(turbo::Milliseconds(500));
    waiter.Post();
  });
  turbo::Time start = turbo::Now();
  EXPECT_TRUE(waiter.Wait(turbo::synchronization_internal::KernelTimeout(
      start + turbo::Seconds(10))));
  turbo::Duration waited = turbo::Now() - start;
  EXPECT_GE(waited, WithTolerance(turbo::Milliseconds(500)));
  EXPECT_LT(waited, turbo::Seconds(2));
}

TYPED_TEST_P(WaiterTest, WaitDurationReached) {
  TypeParam waiter;
  turbo::Time start = turbo::Now();
  EXPECT_FALSE(waiter.Wait(
      turbo::synchronization_internal::KernelTimeout(turbo::Milliseconds(500))));
  turbo::Duration waited = turbo::Now() - start;
  EXPECT_GE(waited, WithTolerance(turbo::Milliseconds(500)));
  EXPECT_LT(waited, turbo::Seconds(1));
}

TYPED_TEST_P(WaiterTest, WaitTimeReached) {
  TypeParam waiter;
  turbo::Time start = turbo::Now();
  EXPECT_FALSE(waiter.Wait(turbo::synchronization_internal::KernelTimeout(
      start + turbo::Milliseconds(500))));
  turbo::Duration waited = turbo::Now() - start;
  EXPECT_GE(waited, WithTolerance(turbo::Milliseconds(500)));
  EXPECT_LT(waited, turbo::Seconds(1));
}

REGISTER_TYPED_TEST_SUITE_P(WaiterTest,
                            WaitNoTimeout,
                            WaitDurationWoken,
                            WaitTimeWoken,
                            WaitDurationReached,
                            WaitTimeReached);

#ifdef TURBO_INTERNAL_HAVE_FUTEX_WAITER
INSTANTIATE_TYPED_TEST_SUITE_P(Futex, WaiterTest,
                               turbo::synchronization_internal::FutexWaiter);
#endif
#ifdef TURBO_INTERNAL_HAVE_PTHREAD_WAITER
INSTANTIATE_TYPED_TEST_SUITE_P(Pthread, WaiterTest,
                               turbo::synchronization_internal::PthreadWaiter);
#endif
#ifdef TURBO_INTERNAL_HAVE_SEM_WAITER
INSTANTIATE_TYPED_TEST_SUITE_P(Sem, WaiterTest,
                               turbo::synchronization_internal::SemWaiter);
#endif
#ifdef TURBO_INTERNAL_HAVE_WIN32_WAITER
INSTANTIATE_TYPED_TEST_SUITE_P(Win32, WaiterTest,
                               turbo::synchronization_internal::Win32Waiter);
#endif
#ifdef TURBO_INTERNAL_HAVE_STDCPP_WAITER
INSTANTIATE_TYPED_TEST_SUITE_P(Stdcpp, WaiterTest,
                               turbo::synchronization_internal::StdcppWaiter);
#endif

}  // namespace
