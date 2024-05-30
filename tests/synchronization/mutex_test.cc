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

#include <turbo/synchronization/mutex.h>

#ifdef _WIN32
#include <windows.h>
#endif

#include <algorithm>
#include <atomic>
#include <cstdlib>
#include <functional>
#include <memory>
#include <random>
#include <string>
#include <thread>  // NOLINT(build/c++11)
#include <type_traits>
#include <vector>

#include <gtest/gtest.h>
#include <turbo/base/attributes.h>
#include <turbo/base/config.h>
#include <turbo/base/internal/sysinfo.h>
#include <turbo/log/check.h>
#include <turbo/log/log.h>
#include <turbo/memory/memory.h>
#include <turbo/synchronization/internal/create_thread_identity.h>
#include <turbo/synchronization/internal/thread_pool.h>
#include <turbo/times/clock.h>
#include <turbo/times/time.h>

#ifdef TURBO_HAVE_PTHREAD_GETSCHEDPARAM
#include <pthread.h>
#include <string.h>
#endif

namespace {

// TODO(dmauro): Replace with a commandline flag.
static constexpr bool kExtendedTest = false;

std::unique_ptr<turbo::synchronization_internal::ThreadPool> CreatePool(
    int threads) {
  return turbo::make_unique<turbo::synchronization_internal::ThreadPool>(threads);
}

std::unique_ptr<turbo::synchronization_internal::ThreadPool>
CreateDefaultPool() {
  return CreatePool(kExtendedTest ? 32 : 10);
}

// Hack to schedule a function to run on a thread pool thread after a
// duration has elapsed.
static void ScheduleAfter(turbo::synchronization_internal::ThreadPool *tp,
                          turbo::Duration after,
                          const std::function<void()> &func) {
  tp->Schedule([func, after] {
    turbo::SleepFor(after);
    func();
  });
}

struct ScopedInvariantDebugging {
  ScopedInvariantDebugging() { turbo::EnableMutexInvariantDebugging(true); }
  ~ScopedInvariantDebugging() { turbo::EnableMutexInvariantDebugging(false); }
};

struct TestContext {
  int iterations;
  int threads;
  int g0;  // global 0
  int g1;  // global 1
  turbo::Mutex mu;
  turbo::CondVar cv;
};

// To test whether the invariant check call occurs
static std::atomic<bool> invariant_checked;

static bool GetInvariantChecked() {
  return invariant_checked.load(std::memory_order_relaxed);
}

static void SetInvariantChecked(bool new_value) {
  invariant_checked.store(new_value, std::memory_order_relaxed);
}

static void CheckSumG0G1(void *v) {
  TestContext *cxt = static_cast<TestContext *>(v);
  CHECK_EQ(cxt->g0, -cxt->g1) << "Error in CheckSumG0G1";
  SetInvariantChecked(true);
}

static void TestMu(TestContext *cxt, int c) {
  for (int i = 0; i != cxt->iterations; i++) {
    turbo::MutexLock l(&cxt->mu);
    int a = cxt->g0 + 1;
    cxt->g0 = a;
    cxt->g1--;
  }
}

static void TestTry(TestContext *cxt, int c) {
  for (int i = 0; i != cxt->iterations; i++) {
    do {
      std::this_thread::yield();
    } while (!cxt->mu.TryLock());
    int a = cxt->g0 + 1;
    cxt->g0 = a;
    cxt->g1--;
    cxt->mu.Unlock();
  }
}

static void TestR20ms(TestContext *cxt, int c) {
  for (int i = 0; i != cxt->iterations; i++) {
    turbo::ReaderMutexLock l(&cxt->mu);
    turbo::SleepFor(turbo::Milliseconds(20));
    cxt->mu.AssertReaderHeld();
  }
}

static void TestRW(TestContext *cxt, int c) {
  if ((c & 1) == 0) {
    for (int i = 0; i != cxt->iterations; i++) {
      turbo::WriterMutexLock l(&cxt->mu);
      cxt->g0++;
      cxt->g1--;
      cxt->mu.AssertHeld();
      cxt->mu.AssertReaderHeld();
    }
  } else {
    for (int i = 0; i != cxt->iterations; i++) {
      turbo::ReaderMutexLock l(&cxt->mu);
      CHECK_EQ(cxt->g0, -cxt->g1) << "Error in TestRW";
      cxt->mu.AssertReaderHeld();
    }
  }
}

struct MyContext {
  int target;
  TestContext *cxt;
  bool MyTurn();
};

bool MyContext::MyTurn() {
  TestContext *cxt = this->cxt;
  return cxt->g0 == this->target || cxt->g0 == cxt->iterations;
}

static void TestAwait(TestContext *cxt, int c) {
  MyContext mc;
  mc.target = c;
  mc.cxt = cxt;
  turbo::MutexLock l(&cxt->mu);
  cxt->mu.AssertHeld();
  while (cxt->g0 < cxt->iterations) {
    cxt->mu.Await(turbo::Condition(&mc, &MyContext::MyTurn));
    CHECK(mc.MyTurn()) << "Error in TestAwait";
    cxt->mu.AssertHeld();
    if (cxt->g0 < cxt->iterations) {
      int a = cxt->g0 + 1;
      cxt->g0 = a;
      mc.target += cxt->threads;
    }
  }
}

static void TestSignalAll(TestContext *cxt, int c) {
  int target = c;
  turbo::MutexLock l(&cxt->mu);
  cxt->mu.AssertHeld();
  while (cxt->g0 < cxt->iterations) {
    while (cxt->g0 != target && cxt->g0 != cxt->iterations) {
      cxt->cv.Wait(&cxt->mu);
    }
    if (cxt->g0 < cxt->iterations) {
      int a = cxt->g0 + 1;
      cxt->g0 = a;
      cxt->cv.SignalAll();
      target += cxt->threads;
    }
  }
}

static void TestSignal(TestContext *cxt, int c) {
  CHECK_EQ(cxt->threads, 2) << "TestSignal should use 2 threads";
  int target = c;
  turbo::MutexLock l(&cxt->mu);
  cxt->mu.AssertHeld();
  while (cxt->g0 < cxt->iterations) {
    while (cxt->g0 != target && cxt->g0 != cxt->iterations) {
      cxt->cv.Wait(&cxt->mu);
    }
    if (cxt->g0 < cxt->iterations) {
      int a = cxt->g0 + 1;
      cxt->g0 = a;
      cxt->cv.Signal();
      target += cxt->threads;
    }
  }
}

static void TestCVTimeout(TestContext *cxt, int c) {
  int target = c;
  turbo::MutexLock l(&cxt->mu);
  cxt->mu.AssertHeld();
  while (cxt->g0 < cxt->iterations) {
    while (cxt->g0 != target && cxt->g0 != cxt->iterations) {
      cxt->cv.WaitWithTimeout(&cxt->mu, turbo::Seconds(100));
    }
    if (cxt->g0 < cxt->iterations) {
      int a = cxt->g0 + 1;
      cxt->g0 = a;
      cxt->cv.SignalAll();
      target += cxt->threads;
    }
  }
}

static bool G0GE2(TestContext *cxt) { return cxt->g0 >= 2; }

static void TestTime(TestContext *cxt, int c, bool use_cv) {
  CHECK_EQ(cxt->iterations, 1) << "TestTime should only use 1 iteration";
  CHECK_GT(cxt->threads, 2) << "TestTime should use more than 2 threads";
  const bool kFalse = false;
  turbo::Condition false_cond(&kFalse);
  turbo::Condition g0ge2(G0GE2, cxt);
  if (c == 0) {
    turbo::MutexLock l(&cxt->mu);

    turbo::Time start = turbo::Time::current_time();
    if (use_cv) {
      cxt->cv.WaitWithTimeout(&cxt->mu, turbo::Seconds(1));
    } else {
      CHECK(!cxt->mu.AwaitWithTimeout(false_cond, turbo::Seconds(1)))
          << "TestTime failed";
    }
    turbo::Duration elapsed = turbo::Time::current_time() - start;
    CHECK(turbo::Seconds(0.9) <= elapsed && elapsed <= turbo::Seconds(2.0))
        << "TestTime failed";
    CHECK_EQ(cxt->g0, 1) << "TestTime failed";

    start = turbo::Time::current_time();
    if (use_cv) {
      cxt->cv.WaitWithTimeout(&cxt->mu, turbo::Seconds(1));
    } else {
      CHECK(!cxt->mu.AwaitWithTimeout(false_cond, turbo::Seconds(1)))
          << "TestTime failed";
    }
    elapsed = turbo::Time::current_time() - start;
    CHECK(turbo::Seconds(0.9) <= elapsed && elapsed <= turbo::Seconds(2.0))
        << "TestTime failed";
    cxt->g0++;
    if (use_cv) {
      cxt->cv.Signal();
    }

    start = turbo::Time::current_time();
    if (use_cv) {
      cxt->cv.WaitWithTimeout(&cxt->mu, turbo::Seconds(4));
    } else {
      CHECK(!cxt->mu.AwaitWithTimeout(false_cond, turbo::Seconds(4)))
          << "TestTime failed";
    }
    elapsed = turbo::Time::current_time() - start;
    CHECK(turbo::Seconds(3.9) <= elapsed && elapsed <= turbo::Seconds(6.0))
        << "TestTime failed";
    CHECK_GE(cxt->g0, 3) << "TestTime failed";

    start = turbo::Time::current_time();
    if (use_cv) {
      cxt->cv.WaitWithTimeout(&cxt->mu, turbo::Seconds(1));
    } else {
      CHECK(!cxt->mu.AwaitWithTimeout(false_cond, turbo::Seconds(1)))
          << "TestTime failed";
    }
    elapsed = turbo::Time::current_time() - start;
    CHECK(turbo::Seconds(0.9) <= elapsed && elapsed <= turbo::Seconds(2.0))
        << "TestTime failed";
    if (use_cv) {
      cxt->cv.SignalAll();
    }

    start = turbo::Time::current_time();
    if (use_cv) {
      cxt->cv.WaitWithTimeout(&cxt->mu, turbo::Seconds(1));
    } else {
      CHECK(!cxt->mu.AwaitWithTimeout(false_cond, turbo::Seconds(1)))
          << "TestTime failed";
    }
    elapsed = turbo::Time::current_time() - start;
    CHECK(turbo::Seconds(0.9) <= elapsed && elapsed <= turbo::Seconds(2.0))
        << "TestTime failed";
    CHECK_EQ(cxt->g0, cxt->threads) << "TestTime failed";

  } else if (c == 1) {
    turbo::MutexLock l(&cxt->mu);
    const turbo::Time start = turbo::Time::current_time();
    if (use_cv) {
      cxt->cv.WaitWithTimeout(&cxt->mu, turbo::Milliseconds(500));
    } else {
      CHECK(!cxt->mu.AwaitWithTimeout(false_cond, turbo::Milliseconds(500)))
          << "TestTime failed";
    }
    const turbo::Duration elapsed = turbo::Time::current_time() - start;
    CHECK(turbo::Seconds(0.4) <= elapsed && elapsed <= turbo::Seconds(0.9))
        << "TestTime failed";
    cxt->g0++;
  } else if (c == 2) {
    turbo::MutexLock l(&cxt->mu);
    if (use_cv) {
      while (cxt->g0 < 2) {
        cxt->cv.WaitWithTimeout(&cxt->mu, turbo::Seconds(100));
      }
    } else {
      CHECK(cxt->mu.AwaitWithTimeout(g0ge2, turbo::Seconds(100)))
          << "TestTime failed";
    }
    cxt->g0++;
  } else {
    turbo::MutexLock l(&cxt->mu);
    if (use_cv) {
      while (cxt->g0 < 2) {
        cxt->cv.Wait(&cxt->mu);
      }
    } else {
      cxt->mu.Await(g0ge2);
    }
    cxt->g0++;
  }
}

static void TestMuTime(TestContext *cxt, int c) { TestTime(cxt, c, false); }

static void TestCVTime(TestContext *cxt, int c) { TestTime(cxt, c, true); }

static void EndTest(int *c0, int *c1, turbo::Mutex *mu, turbo::CondVar *cv,
                    const std::function<void(int)> &cb) {
  mu->Lock();
  int c = (*c0)++;
  mu->Unlock();
  cb(c);
  turbo::MutexLock l(mu);
  (*c1)++;
  cv->Signal();
}

// Code common to RunTest() and RunTestWithInvariantDebugging().
static int RunTestCommon(TestContext *cxt, void (*test)(TestContext *cxt, int),
                         int threads, int iterations, int operations) {
  turbo::Mutex mu2;
  turbo::CondVar cv2;
  int c0 = 0;
  int c1 = 0;
  cxt->g0 = 0;
  cxt->g1 = 0;
  cxt->iterations = iterations;
  cxt->threads = threads;
  turbo::synchronization_internal::ThreadPool tp(threads);
  for (int i = 0; i != threads; i++) {
    tp.Schedule(std::bind(
        &EndTest, &c0, &c1, &mu2, &cv2,
        std::function<void(int)>(std::bind(test, cxt, std::placeholders::_1))));
  }
  mu2.Lock();
  while (c1 != threads) {
    cv2.Wait(&mu2);
  }
  mu2.Unlock();
  return cxt->g0;
}

// Basis for the parameterized tests configured below.
static int RunTest(void (*test)(TestContext *cxt, int), int threads,
                   int iterations, int operations) {
  TestContext cxt;
  return RunTestCommon(&cxt, test, threads, iterations, operations);
}

// Like RunTest(), but sets an invariant on the tested Mutex and
// verifies that the invariant check happened. The invariant function
// will be passed the TestContext* as its arg and must call
// SetInvariantChecked(true);
#if !defined(TURBO_MUTEX_ENABLE_INVARIANT_DEBUGGING_NOT_IMPLEMENTED)
static int RunTestWithInvariantDebugging(void (*test)(TestContext *cxt, int),
                                         int threads, int iterations,
                                         int operations,
                                         void (*invariant)(void *)) {
  ScopedInvariantDebugging scoped_debugging;
  SetInvariantChecked(false);
  TestContext cxt;
  cxt.mu.EnableInvariantDebugging(invariant, &cxt);
  int ret = RunTestCommon(&cxt, test, threads, iterations, operations);
  CHECK(GetInvariantChecked()) << "Invariant not checked";
  return ret;
}
#endif

// --------------------------------------------------------
// Test for fix of bug in TryRemove()
struct TimeoutBugStruct {
  turbo::Mutex mu;
  bool a;
  int a_waiter_count;
};

static void WaitForA(TimeoutBugStruct *x) {
  x->mu.LockWhen(turbo::Condition(&x->a));
  x->a_waiter_count--;
  x->mu.Unlock();
}

static bool NoAWaiters(TimeoutBugStruct *x) { return x->a_waiter_count == 0; }

// Test that a CondVar.Wait(&mutex) can un-block a call to mutex.Await() in
// another thread.
TEST(Mutex, CondVarWaitSignalsAwait) {
  // Use a struct so the lock annotations apply.
  struct {
    turbo::Mutex barrier_mu;
    bool barrier TURBO_GUARDED_BY(barrier_mu) = false;

    turbo::Mutex release_mu;
    bool release TURBO_GUARDED_BY(release_mu) = false;
    turbo::CondVar released_cv;
  } state;

  auto pool = CreateDefaultPool();

  // Thread A.  Sets barrier, waits for release using Mutex::Await, then
  // signals released_cv.
  pool->Schedule([&state] {
    state.release_mu.Lock();

    state.barrier_mu.Lock();
    state.barrier = true;
    state.barrier_mu.Unlock();

    state.release_mu.Await(turbo::Condition(&state.release));
    state.released_cv.Signal();
    state.release_mu.Unlock();
  });

  state.barrier_mu.LockWhen(turbo::Condition(&state.barrier));
  state.barrier_mu.Unlock();
  state.release_mu.Lock();
  // Thread A is now blocked on release by way of Mutex::Await().

  // Set release.  Calling released_cv.Wait() should un-block thread A,
  // which will signal released_cv.  If not, the test will hang.
  state.release = true;
  state.released_cv.Wait(&state.release_mu);
  state.release_mu.Unlock();
}

// Test that a CondVar.WaitWithTimeout(&mutex) can un-block a call to
// mutex.Await() in another thread.
TEST(Mutex, CondVarWaitWithTimeoutSignalsAwait) {
  // Use a struct so the lock annotations apply.
  struct {
    turbo::Mutex barrier_mu;
    bool barrier TURBO_GUARDED_BY(barrier_mu) = false;

    turbo::Mutex release_mu;
    bool release TURBO_GUARDED_BY(release_mu) = false;
    turbo::CondVar released_cv;
  } state;

  auto pool = CreateDefaultPool();

  // Thread A.  Sets barrier, waits for release using Mutex::Await, then
  // signals released_cv.
  pool->Schedule([&state] {
    state.release_mu.Lock();

    state.barrier_mu.Lock();
    state.barrier = true;
    state.barrier_mu.Unlock();

    state.release_mu.Await(turbo::Condition(&state.release));
    state.released_cv.Signal();
    state.release_mu.Unlock();
  });

  state.barrier_mu.LockWhen(turbo::Condition(&state.barrier));
  state.barrier_mu.Unlock();
  state.release_mu.Lock();
  // Thread A is now blocked on release by way of Mutex::Await().

  // Set release.  Calling released_cv.Wait() should un-block thread A,
  // which will signal released_cv.  If not, the test will hang.
  state.release = true;
  EXPECT_TRUE(
      !state.released_cv.WaitWithTimeout(&state.release_mu, turbo::Seconds(10)))
      << "; Unrecoverable test failure: CondVar::WaitWithTimeout did not "
         "unblock the turbo::Mutex::Await call in another thread.";

  state.release_mu.Unlock();
}

// Test for regression of a bug in loop of TryRemove()
TEST(Mutex, MutexTimeoutBug) {
  auto tp = CreateDefaultPool();

  TimeoutBugStruct x;
  x.a = false;
  x.a_waiter_count = 2;
  tp->Schedule(std::bind(&WaitForA, &x));
  tp->Schedule(std::bind(&WaitForA, &x));
  turbo::SleepFor(turbo::Seconds(1));  // Allow first two threads to hang.
  // The skip field of the second will point to the first because there are
  // only two.

  // Now cause a thread waiting on an always-false to time out
  // This would deadlock when the bug was present.
  bool always_false = false;
  x.mu.LockWhenWithTimeout(turbo::Condition(&always_false),
                           turbo::Milliseconds(500));

  // if we get here, the bug is not present.   Cleanup the state.

  x.a = true;                                    // wakeup the two waiters on A
  x.mu.Await(turbo::Condition(&NoAWaiters, &x));  // wait for them to exit
  x.mu.Unlock();
}

struct CondVarWaitDeadlock : testing::TestWithParam<int> {
  turbo::Mutex mu;
  turbo::CondVar cv;
  bool cond1 = false;
  bool cond2 = false;
  bool read_lock1;
  bool read_lock2;
  bool signal_unlocked;

  CondVarWaitDeadlock() {
    read_lock1 = GetParam() & (1 << 0);
    read_lock2 = GetParam() & (1 << 1);
    signal_unlocked = GetParam() & (1 << 2);
  }

  void Waiter1() {
    if (read_lock1) {
      mu.ReaderLock();
      while (!cond1) {
        cv.Wait(&mu);
      }
      mu.ReaderUnlock();
    } else {
      mu.Lock();
      while (!cond1) {
        cv.Wait(&mu);
      }
      mu.Unlock();
    }
  }

  void Waiter2() {
    if (read_lock2) {
      mu.ReaderLockWhen(turbo::Condition(&cond2));
      mu.ReaderUnlock();
    } else {
      mu.LockWhen(turbo::Condition(&cond2));
      mu.Unlock();
    }
  }
};

// Test for a deadlock bug in Mutex::Fer().
// The sequence of events that lead to the deadlock is:
// 1. waiter1 blocks on cv in read mode (mu bits = 0).
// 2. waiter2 blocks on mu in either mode (mu bits = kMuWait).
// 3. main thread locks mu, sets cond1, unlocks mu (mu bits = kMuWait).
// 4. main thread signals on cv and this eventually calls Mutex::Fer().
// Currently Fer wakes waiter1 since mu bits = kMuWait (mutex is unlocked).
// Before the bug fix Fer neither woke waiter1 nor queued it on mutex,
// which resulted in deadlock.
TEST_P(CondVarWaitDeadlock, Test) {
  auto waiter1 = CreatePool(1);
  auto waiter2 = CreatePool(1);
  waiter1->Schedule([this] { this->Waiter1(); });
  waiter2->Schedule([this] { this->Waiter2(); });

  // Wait while threads block (best-effort is fine).
  turbo::SleepFor(turbo::Milliseconds(100));

  // Wake condwaiter.
  mu.Lock();
  cond1 = true;
  if (signal_unlocked) {
    mu.Unlock();
    cv.Signal();
  } else {
    cv.Signal();
    mu.Unlock();
  }
  waiter1.reset();  // "join" waiter1

  // Wake waiter.
  mu.Lock();
  cond2 = true;
  mu.Unlock();
  waiter2.reset();  // "join" waiter2
}

INSTANTIATE_TEST_SUITE_P(CondVarWaitDeadlockTest, CondVarWaitDeadlock,
                         ::testing::Range(0, 8),
                         ::testing::PrintToStringParamName());

// --------------------------------------------------------
// Test for fix of bug in DequeueAllWakeable()
// Bug was that if there was more than one waiting reader
// and all should be woken, the most recently blocked one
// would not be.

struct DequeueAllWakeableBugStruct {
  turbo::Mutex mu;
  turbo::Mutex mu2;       // protects all fields below
  int unfinished_count;  // count of unfinished readers; under mu2
  bool done1;            // unfinished_count == 0; under mu2
  int finished_count;    // count of finished readers, under mu2
  bool done2;            // finished_count == 0; under mu2
};

// Test for regression of a bug in loop of DequeueAllWakeable()
static void AcquireAsReader(DequeueAllWakeableBugStruct *x) {
  x->mu.ReaderLock();
  x->mu2.Lock();
  x->unfinished_count--;
  x->done1 = (x->unfinished_count == 0);
  x->mu2.Unlock();
  // make sure that both readers acquired mu before we release it.
  turbo::SleepFor(turbo::Seconds(2));
  x->mu.ReaderUnlock();

  x->mu2.Lock();
  x->finished_count--;
  x->done2 = (x->finished_count == 0);
  x->mu2.Unlock();
}

// Test for regression of a bug in loop of DequeueAllWakeable()
TEST(Mutex, MutexReaderWakeupBug) {
  auto tp = CreateDefaultPool();

  DequeueAllWakeableBugStruct x;
  x.unfinished_count = 2;
  x.done1 = false;
  x.finished_count = 2;
  x.done2 = false;
  x.mu.Lock();  // acquire mu exclusively
  // queue two thread that will block on reader locks on x.mu
  tp->Schedule(std::bind(&AcquireAsReader, &x));
  tp->Schedule(std::bind(&AcquireAsReader, &x));
  turbo::SleepFor(turbo::Seconds(1));  // give time for reader threads to block
  x.mu.Unlock();                     // wake them up

  // both readers should finish promptly
  EXPECT_TRUE(
      x.mu2.LockWhenWithTimeout(turbo::Condition(&x.done1), turbo::Seconds(10)));
  x.mu2.Unlock();

  EXPECT_TRUE(
      x.mu2.LockWhenWithTimeout(turbo::Condition(&x.done2), turbo::Seconds(10)));
  x.mu2.Unlock();
}

struct LockWhenTestStruct {
  turbo::Mutex mu1;
  bool cond = false;

  turbo::Mutex mu2;
  bool waiting = false;
};

static bool LockWhenTestIsCond(LockWhenTestStruct *s) {
  s->mu2.Lock();
  s->waiting = true;
  s->mu2.Unlock();
  return s->cond;
}

static void LockWhenTestWaitForIsCond(LockWhenTestStruct *s) {
  s->mu1.LockWhen(turbo::Condition(&LockWhenTestIsCond, s));
  s->mu1.Unlock();
}

TEST(Mutex, LockWhen) {
  LockWhenTestStruct s;

  std::thread t(LockWhenTestWaitForIsCond, &s);
  s.mu2.LockWhen(turbo::Condition(&s.waiting));
  s.mu2.Unlock();

  s.mu1.Lock();
  s.cond = true;
  s.mu1.Unlock();

  t.join();
}

TEST(Mutex, LockWhenGuard) {
  turbo::Mutex mu;
  int n = 30;
  bool done = false;

  // We don't inline the lambda because the conversion is ambiguous in MSVC.
  bool (*cond_eq_10)(int *) = [](int *p) { return *p == 10; };
  bool (*cond_lt_10)(int *) = [](int *p) { return *p < 10; };

  std::thread t1([&mu, &n, &done, cond_eq_10]() {
    turbo::ReaderMutexLock lock(&mu, turbo::Condition(cond_eq_10, &n));
    done = true;
  });

  std::thread t2[10];
  for (std::thread &t : t2) {
    t = std::thread([&mu, &n, cond_lt_10]() {
      turbo::WriterMutexLock lock(&mu, turbo::Condition(cond_lt_10, &n));
      ++n;
    });
  }

  {
    turbo::MutexLock lock(&mu);
    n = 0;
  }

  for (std::thread &t : t2) t.join();
  t1.join();

  EXPECT_TRUE(done);
  EXPECT_EQ(n, 10);
}

// --------------------------------------------------------
// The following test requires Mutex::ReaderLock to be a real shared
// lock, which is not the case in all builds.
#if !defined(TURBO_MUTEX_READER_LOCK_IS_EXCLUSIVE)

// Test for fix of bug in UnlockSlow() that incorrectly decremented the reader
// count when putting a thread to sleep waiting for a false condition when the
// lock was not held.

// For this bug to strike, we make a thread wait on a free mutex with no
// waiters by causing its wakeup condition to be false.   Then the
// next two acquirers must be readers.   The bug causes the lock
// to be released when one reader unlocks, rather than both.

struct ReaderDecrementBugStruct {
  bool cond;  // to delay first thread (under mu)
  int done;   // reference count (under mu)
  turbo::Mutex mu;

  bool waiting_on_cond;   // under mu2
  bool have_reader_lock;  // under mu2
  bool complete;          // under mu2
  turbo::Mutex mu2;        // > mu
};

// L >= mu, L < mu_waiting_on_cond
static bool IsCond(void *v) {
  ReaderDecrementBugStruct *x = reinterpret_cast<ReaderDecrementBugStruct *>(v);
  x->mu2.Lock();
  x->waiting_on_cond = true;
  x->mu2.Unlock();
  return x->cond;
}

// L >= mu
static bool AllDone(void *v) {
  ReaderDecrementBugStruct *x = reinterpret_cast<ReaderDecrementBugStruct *>(v);
  return x->done == 0;
}

// L={}
static void WaitForCond(ReaderDecrementBugStruct *x) {
  turbo::Mutex dummy;
  turbo::MutexLock l(&dummy);
  x->mu.LockWhen(turbo::Condition(&IsCond, x));
  x->done--;
  x->mu.Unlock();
}

// L={}
static void GetReadLock(ReaderDecrementBugStruct *x) {
  x->mu.ReaderLock();
  x->mu2.Lock();
  x->have_reader_lock = true;
  x->mu2.Await(turbo::Condition(&x->complete));
  x->mu2.Unlock();
  x->mu.ReaderUnlock();
  x->mu.Lock();
  x->done--;
  x->mu.Unlock();
}

// Test for reader counter being decremented incorrectly by waiter
// with false condition.
TEST(Mutex, MutexReaderDecrementBug) TURBO_NO_THREAD_SAFETY_ANALYSIS {
  ReaderDecrementBugStruct x;
  x.cond = false;
  x.waiting_on_cond = false;
  x.have_reader_lock = false;
  x.complete = false;
  x.done = 2;  // initial ref count

  // Run WaitForCond() and wait for it to sleep
  std::thread thread1(WaitForCond, &x);
  x.mu2.LockWhen(turbo::Condition(&x.waiting_on_cond));
  x.mu2.Unlock();

  // Run GetReadLock(), and wait for it to get the read lock
  std::thread thread2(GetReadLock, &x);
  x.mu2.LockWhen(turbo::Condition(&x.have_reader_lock));
  x.mu2.Unlock();

  // Get the reader lock ourselves, and release it.
  x.mu.ReaderLock();
  x.mu.ReaderUnlock();

  // The lock should be held in read mode by GetReadLock().
  // If we have the bug, the lock will be free.
  x.mu.AssertReaderHeld();

  // Wake up all the threads.
  x.mu2.Lock();
  x.complete = true;
  x.mu2.Unlock();

  // TODO(delesley): turn on analysis once lock upgrading is supported.
  // (This call upgrades the lock from shared to exclusive.)
  x.mu.Lock();
  x.cond = true;
  x.mu.Await(turbo::Condition(&AllDone, &x));
  x.mu.Unlock();

  thread1.join();
  thread2.join();
}
#endif  // !TURBO_MUTEX_READER_LOCK_IS_EXCLUSIVE

// Test that we correctly handle the situation when a lock is
// held and then destroyed (w/o unlocking).
#ifdef TURBO_HAVE_THREAD_SANITIZER
// TSAN reports errors when locked Mutexes are destroyed.
TEST(Mutex, DISABLED_LockedMutexDestructionBug) TURBO_NO_THREAD_SAFETY_ANALYSIS {
#else
TEST(Mutex, LockedMutexDestructionBug) TURBO_NO_THREAD_SAFETY_ANALYSIS {
#endif
  for (int i = 0; i != 10; i++) {
    // Create, lock and destroy 10 locks.
    const int kNumLocks = 10;
    auto mu = turbo::make_unique<turbo::Mutex[]>(kNumLocks);
    for (int j = 0; j != kNumLocks; j++) {
      if ((j % 2) == 0) {
        mu[j].WriterLock();
      } else {
        mu[j].ReaderLock();
      }
    }
  }
}

// Some functions taking pointers to non-const.
bool Equals42(int *p) { return *p == 42; }
bool Equals43(int *p) { return *p == 43; }

// Some functions taking pointers to const.
bool ConstEquals42(const int *p) { return *p == 42; }
bool ConstEquals43(const int *p) { return *p == 43; }

// Some function templates taking pointers. Note it's possible for `T` to be
// deduced as non-const or const, which creates the potential for ambiguity,
// but which the implementation is careful to avoid.
template <typename T>
bool TemplateEquals42(T *p) {
  return *p == 42;
}
template <typename T>
bool TemplateEquals43(T *p) {
  return *p == 43;
}

TEST(Mutex, FunctionPointerCondition) {
  // Some arguments.
  int x = 42;
  const int const_x = 42;

  // Parameter non-const, argument non-const.
  EXPECT_TRUE(turbo::Condition(Equals42, &x).Eval());
  EXPECT_FALSE(turbo::Condition(Equals43, &x).Eval());

  // Parameter const, argument non-const.
  EXPECT_TRUE(turbo::Condition(ConstEquals42, &x).Eval());
  EXPECT_FALSE(turbo::Condition(ConstEquals43, &x).Eval());

  // Parameter const, argument const.
  EXPECT_TRUE(turbo::Condition(ConstEquals42, &const_x).Eval());
  EXPECT_FALSE(turbo::Condition(ConstEquals43, &const_x).Eval());

  // Parameter type deduced, argument non-const.
  EXPECT_TRUE(turbo::Condition(TemplateEquals42, &x).Eval());
  EXPECT_FALSE(turbo::Condition(TemplateEquals43, &x).Eval());

  // Parameter type deduced, argument const.
  EXPECT_TRUE(turbo::Condition(TemplateEquals42, &const_x).Eval());
  EXPECT_FALSE(turbo::Condition(TemplateEquals43, &const_x).Eval());

  // Parameter non-const, argument const is not well-formed.
  EXPECT_FALSE((std::is_constructible<turbo::Condition, decltype(Equals42),
                                      decltype(&const_x)>::value));
  // Validate use of is_constructible by contrasting to a well-formed case.
  EXPECT_TRUE((std::is_constructible<turbo::Condition, decltype(ConstEquals42),
                                     decltype(&const_x)>::value));
}

// Example base and derived class for use in predicates and test below. Not a
// particularly realistic example, but it suffices for testing purposes.
struct Base {
  explicit Base(int v) : value(v) {}
  int value;
};
struct Derived : Base {
  explicit Derived(int v) : Base(v) {}
};

// Some functions taking pointer to non-const `Base`.
bool BaseEquals42(Base *p) { return p->value == 42; }
bool BaseEquals43(Base *p) { return p->value == 43; }

// Some functions taking pointer to const `Base`.
bool ConstBaseEquals42(const Base *p) { return p->value == 42; }
bool ConstBaseEquals43(const Base *p) { return p->value == 43; }

TEST(Mutex, FunctionPointerConditionWithDerivedToBaseConversion) {
  // Some arguments.
  Derived derived(42);
  const Derived const_derived(42);

  // Parameter non-const base, argument derived non-const.
  EXPECT_TRUE(turbo::Condition(BaseEquals42, &derived).Eval());
  EXPECT_FALSE(turbo::Condition(BaseEquals43, &derived).Eval());

  // Parameter const base, argument derived non-const.
  EXPECT_TRUE(turbo::Condition(ConstBaseEquals42, &derived).Eval());
  EXPECT_FALSE(turbo::Condition(ConstBaseEquals43, &derived).Eval());

  // Parameter const base, argument derived const.
  EXPECT_TRUE(turbo::Condition(ConstBaseEquals42, &const_derived).Eval());
  EXPECT_FALSE(turbo::Condition(ConstBaseEquals43, &const_derived).Eval());

  // Parameter const base, argument derived const.
  EXPECT_TRUE(turbo::Condition(ConstBaseEquals42, &const_derived).Eval());
  EXPECT_FALSE(turbo::Condition(ConstBaseEquals43, &const_derived).Eval());

  // Parameter derived, argument base is not well-formed.
  bool (*derived_pred)(const Derived *) = [](const Derived *) { return true; };
  EXPECT_FALSE((std::is_constructible<turbo::Condition, decltype(derived_pred),
                                      Base *>::value));
  EXPECT_FALSE((std::is_constructible<turbo::Condition, decltype(derived_pred),
                                      const Base *>::value));
  // Validate use of is_constructible by contrasting to well-formed cases.
  EXPECT_TRUE((std::is_constructible<turbo::Condition, decltype(derived_pred),
                                     Derived *>::value));
  EXPECT_TRUE((std::is_constructible<turbo::Condition, decltype(derived_pred),
                                     const Derived *>::value));
}

struct Constable {
  bool WotsAllThisThen() const { return true; }
};

TEST(Mutex, FunctionPointerConditionWithConstMethod) {
  const Constable chapman;
  EXPECT_TRUE(turbo::Condition(&chapman, &Constable::WotsAllThisThen).Eval());
}

struct True {
  template <class... Args>
  bool operator()(Args...) const {
    return true;
  }
};

struct DerivedTrue : True {};

TEST(Mutex, FunctorCondition) {
  {  // Variadic
    True f;
    EXPECT_TRUE(turbo::Condition(&f).Eval());
  }

  {  // Inherited
    DerivedTrue g;
    EXPECT_TRUE(turbo::Condition(&g).Eval());
  }

  {  // lambda
    int value = 3;
    auto is_zero = [&value] { return value == 0; };
    turbo::Condition c(&is_zero);
    EXPECT_FALSE(c.Eval());
    value = 0;
    EXPECT_TRUE(c.Eval());
  }

  {  // bind
    int value = 0;
    auto is_positive = std::bind(std::less<int>(), 0, std::cref(value));
    turbo::Condition c(&is_positive);
    EXPECT_FALSE(c.Eval());
    value = 1;
    EXPECT_TRUE(c.Eval());
  }

  {  // std::function
    int value = 3;
    std::function<bool()> is_zero = [&value] { return value == 0; };
    turbo::Condition c(&is_zero);
    EXPECT_FALSE(c.Eval());
    value = 0;
    EXPECT_TRUE(c.Eval());
  }
}

TEST(Mutex, ConditionSwap) {
  // Ensure that Conditions can be swap'ed.
  bool b1 = true;
  turbo::Condition c1(&b1);
  bool b2 = false;
  turbo::Condition c2(&b2);
  EXPECT_TRUE(c1.Eval());
  EXPECT_FALSE(c2.Eval());
  std::swap(c1, c2);
  EXPECT_FALSE(c1.Eval());
  EXPECT_TRUE(c2.Eval());
}

// --------------------------------------------------------
// Test for bug with pattern of readers using a condvar.  The bug was that if a
// reader went to sleep on a condition variable while one or more other readers
// held the lock, but there were no waiters, the reader count (held in the
// mutex word) would be lost.  (This is because Enqueue() had at one time
// always placed the thread on the Mutex queue.  Later (CL 4075610), to
// tolerate re-entry into Mutex from a Condition predicate, Enqueue() was
// changed so that it could also place a thread on a condition-variable.  This
// introduced the case where Enqueue() returned with an empty queue, and this
// case was handled incorrectly in one place.)

static void ReaderForReaderOnCondVar(turbo::Mutex *mu, turbo::CondVar *cv,
                                     int *running) {
  std::random_device dev;
  std::mt19937 gen(dev());
  std::uniform_int_distribution<int> random_millis(0, 15);
  mu->ReaderLock();
  while (*running == 3) {
    turbo::SleepFor(turbo::Milliseconds(random_millis(gen)));
    cv->WaitWithTimeout(mu, turbo::Milliseconds(random_millis(gen)));
  }
  mu->ReaderUnlock();
  mu->Lock();
  (*running)--;
  mu->Unlock();
}

static bool IntIsZero(int *x) { return *x == 0; }

// Test for reader waiting condition variable when there are other readers
// but no waiters.
TEST(Mutex, TestReaderOnCondVar) {
  auto tp = CreateDefaultPool();
  turbo::Mutex mu;
  turbo::CondVar cv;
  int running = 3;
  tp->Schedule(std::bind(&ReaderForReaderOnCondVar, &mu, &cv, &running));
  tp->Schedule(std::bind(&ReaderForReaderOnCondVar, &mu, &cv, &running));
  turbo::SleepFor(turbo::Seconds(2));
  mu.Lock();
  running--;
  mu.Await(turbo::Condition(&IntIsZero, &running));
  mu.Unlock();
}

// --------------------------------------------------------
struct AcquireFromConditionStruct {
  turbo::Mutex mu0;   // protects value, done
  int value;         // times condition function is called; under mu0,
  bool done;         // done with test?  under mu0
  turbo::Mutex mu1;   // used to attempt to mess up state of mu0
  turbo::CondVar cv;  // so the condition function can be invoked from
                     // CondVar::Wait().
};

static bool ConditionWithAcquire(AcquireFromConditionStruct *x) {
  x->value++;  // count times this function is called

  if (x->value == 2 || x->value == 3) {
    // On the second and third invocation of this function, sleep for 100ms,
    // but with the side-effect of altering the state of a Mutex other than
    // than one for which this is a condition.  The spec now explicitly allows
    // this side effect; previously it did not.  it was illegal.
    bool always_false = false;
    x->mu1.LockWhenWithTimeout(turbo::Condition(&always_false),
                               turbo::Milliseconds(100));
    x->mu1.Unlock();
  }
  CHECK_LT(x->value, 4) << "should not be invoked a fourth time";

  // We arrange for the condition to return true on only the 2nd and 3rd calls.
  return x->value == 2 || x->value == 3;
}

static void WaitForCond2(AcquireFromConditionStruct *x) {
  // wait for cond0 to become true
  x->mu0.LockWhen(turbo::Condition(&ConditionWithAcquire, x));
  x->done = true;
  x->mu0.Unlock();
}

// Test for Condition whose function acquires other Mutexes
TEST(Mutex, AcquireFromCondition) {
  auto tp = CreateDefaultPool();

  AcquireFromConditionStruct x;
  x.value = 0;
  x.done = false;
  tp->Schedule(
      std::bind(&WaitForCond2, &x));  // run WaitForCond2() in a thread T
  // T will hang because the first invocation of ConditionWithAcquire() will
  // return false.
  turbo::SleepFor(turbo::Milliseconds(500));  // allow T time to hang

  x.mu0.Lock();
  x.cv.WaitWithTimeout(&x.mu0, turbo::Milliseconds(500));  // wake T
  // T will be woken because the Wait() will call ConditionWithAcquire()
  // for the second time, and it will return true.

  x.mu0.Unlock();

  // T will then acquire the lock and recheck its own condition.
  // It will find the condition true, as this is the third invocation,
  // but the use of another Mutex by the calling function will
  // cause the old mutex implementation to think that the outer
  // LockWhen() has timed out because the inner LockWhenWithTimeout() did.
  // T will then check the condition a fourth time because it finds a
  // timeout occurred.  This should not happen in the new
  // implementation that allows the Condition function to use Mutexes.

  // It should also succeed, even though the Condition function
  // is being invoked from CondVar::Wait, and thus this thread
  // is conceptually waiting both on the condition variable, and on mu2.

  x.mu0.LockWhen(turbo::Condition(&x.done));
  x.mu0.Unlock();
}

TEST(Mutex, DeadlockDetector) {
  turbo::SetMutexDeadlockDetectionMode(turbo::OnDeadlockCycle::kAbort);

  // check that we can call ForgetDeadlockInfo() on a lock with the lock held
  turbo::Mutex m1;
  turbo::Mutex m2;
  turbo::Mutex m3;
  turbo::Mutex m4;

  m1.Lock();  // m1 gets ID1
  m2.Lock();  // m2 gets ID2
  m3.Lock();  // m3 gets ID3
  m3.Unlock();
  m2.Unlock();
  // m1 still held
  m1.ForgetDeadlockInfo();  // m1 loses ID
  m2.Lock();                // m2 gets ID2
  m3.Lock();                // m3 gets ID3
  m4.Lock();                // m4 gets ID4
  m3.Unlock();
  m2.Unlock();
  m4.Unlock();
  m1.Unlock();
}

// Bazel has a test "warning" file that programs can write to if the
// test should pass with a warning.  This class disables the warning
// file until it goes out of scope.
class ScopedDisableBazelTestWarnings {
 public:
  ScopedDisableBazelTestWarnings() {
#ifdef _WIN32
    char file[MAX_PATH];
    if (GetEnvironmentVariableA(kVarName, file, sizeof(file)) < sizeof(file)) {
      warnings_output_file_ = file;
      SetEnvironmentVariableA(kVarName, nullptr);
    }
#else
    const char *file = getenv(kVarName);
    if (file != nullptr) {
      warnings_output_file_ = file;
      unsetenv(kVarName);
    }
#endif
  }

  ~ScopedDisableBazelTestWarnings() {
    if (!warnings_output_file_.empty()) {
#ifdef _WIN32
      SetEnvironmentVariableA(kVarName, warnings_output_file_.c_str());
#else
      setenv(kVarName, warnings_output_file_.c_str(), 0);
#endif
    }
  }

 private:
  static const char kVarName[];
  std::string warnings_output_file_;
};
const char ScopedDisableBazelTestWarnings::kVarName[] =
    "TEST_WARNINGS_OUTPUT_FILE";

#ifdef TURBO_HAVE_THREAD_SANITIZER
// This test intentionally creates deadlocks to test the deadlock detector.
TEST(Mutex, DISABLED_DeadlockDetectorBazelWarning) {
#else
TEST(Mutex, DeadlockDetectorBazelWarning) {
#endif
  turbo::SetMutexDeadlockDetectionMode(turbo::OnDeadlockCycle::kReport);

  // Cause deadlock detection to detect something, if it's
  // compiled in and enabled.  But turn off the bazel warning.
  ScopedDisableBazelTestWarnings disable_bazel_test_warnings;

  turbo::Mutex mu0;
  turbo::Mutex mu1;
  bool got_mu0 = mu0.TryLock();
  mu1.Lock();  // acquire mu1 while holding mu0
  if (got_mu0) {
    mu0.Unlock();
  }
  if (mu0.TryLock()) {  // try lock shouldn't cause deadlock detector to fire
    mu0.Unlock();
  }
  mu0.Lock();  // acquire mu0 while holding mu1; should get one deadlock
               // report here
  mu0.Unlock();
  mu1.Unlock();

  turbo::SetMutexDeadlockDetectionMode(turbo::OnDeadlockCycle::kAbort);
}

TEST(Mutex, DeadlockDetectorLongCycle) {
  turbo::SetMutexDeadlockDetectionMode(turbo::OnDeadlockCycle::kReport);

  // This test generates a warning if it passes, and crashes otherwise.
  // Cause bazel to ignore the warning.
  ScopedDisableBazelTestWarnings disable_bazel_test_warnings;

  // Check that we survive a deadlock with a lock cycle.
  std::vector<turbo::Mutex> mutex(100);
  for (size_t i = 0; i != mutex.size(); i++) {
    mutex[i].Lock();
    mutex[(i + 1) % mutex.size()].Lock();
    mutex[i].Unlock();
    mutex[(i + 1) % mutex.size()].Unlock();
  }

  turbo::SetMutexDeadlockDetectionMode(turbo::OnDeadlockCycle::kAbort);
}

// This test is tagged with NO_THREAD_SAFETY_ANALYSIS because the
// annotation-based static thread-safety analysis is not currently
// predicate-aware and cannot tell if the two for-loops that acquire and
// release the locks have the same predicates.
TEST(Mutex, DeadlockDetectorStressTest) TURBO_NO_THREAD_SAFETY_ANALYSIS {
  // Stress test: Here we create a large number of locks and use all of them.
  // If a deadlock detector keeps a full graph of lock acquisition order,
  // it will likely be too slow for this test to pass.
  const int n_locks = 1 << 17;
  auto array_of_locks = turbo::make_unique<turbo::Mutex[]>(n_locks);
  for (int i = 0; i < n_locks; i++) {
    int end = std::min(n_locks, i + 5);
    // acquire and then release locks i, i+1, ..., i+4
    for (int j = i; j < end; j++) {
      array_of_locks[j].Lock();
    }
    for (int j = i; j < end; j++) {
      array_of_locks[j].Unlock();
    }
  }
}

#ifdef TURBO_HAVE_THREAD_SANITIZER
// TSAN reports errors when locked Mutexes are destroyed.
TEST(Mutex, DISABLED_DeadlockIdBug) TURBO_NO_THREAD_SAFETY_ANALYSIS {
#else
TEST(Mutex, DeadlockIdBug) TURBO_NO_THREAD_SAFETY_ANALYSIS {
#endif
  // Test a scenario where a cached deadlock graph node id in the
  // list of held locks is not invalidated when the corresponding
  // mutex is deleted.
  turbo::SetMutexDeadlockDetectionMode(turbo::OnDeadlockCycle::kAbort);
  // Mutex that will be destroyed while being held
  turbo::Mutex *a = new turbo::Mutex;
  // Other mutexes needed by test
  turbo::Mutex b, c;

  // Hold mutex.
  a->Lock();

  // Force deadlock id assignment by acquiring another lock.
  b.Lock();
  b.Unlock();

  // Delete the mutex. The Mutex destructor tries to remove held locks,
  // but the attempt isn't foolproof.  It can fail if:
  //   (a) Deadlock detection is currently disabled.
  //   (b) The destruction is from another thread.
  // We exploit (a) by temporarily disabling deadlock detection.
  turbo::SetMutexDeadlockDetectionMode(turbo::OnDeadlockCycle::kIgnore);
  delete a;
  turbo::SetMutexDeadlockDetectionMode(turbo::OnDeadlockCycle::kAbort);

  // Now acquire another lock which will force a deadlock id assignment.
  // We should end up getting assigned the same deadlock id that was
  // freed up when "a" was deleted, which will cause a spurious deadlock
  // report if the held lock entry for "a" was not invalidated.
  c.Lock();
  c.Unlock();
}

// --------------------------------------------------------
// Test for timeouts/deadlines on condition waits that are specified using
// turbo::Duration and turbo::Time.  For each waiting function we test with
// a timeout/deadline that has already expired/passed, one that is infinite
// and so never expires/passes, and one that will expire/pass in the near
// future.

static turbo::Duration TimeoutTestAllowedSchedulingDelay() {
  // Note: we use a function here because Microsoft Visual Studio fails to
  // properly initialize constexpr static turbo::Duration variables.
  return turbo::Milliseconds(150);
}

// Returns true if `actual_delay` is close enough to `expected_delay` to pass
// the timeouts/deadlines test.  Otherwise, logs warnings and returns false.
TURBO_MUST_USE_RESULT
static bool DelayIsWithinBounds(turbo::Duration expected_delay,
                                turbo::Duration actual_delay) {
  bool pass = true;
  // Do not allow the observed delay to be less than expected.  This may occur
  // in practice due to clock skew or when the synchronization primitives use a
  // different clock than turbo::Time::current_time(), but these cases should be handled by the
  // the retry mechanism in each TimeoutTest.
  if (actual_delay < expected_delay) {
    LOG(WARNING) << "Actual delay " << actual_delay
                 << " was too short, expected " << expected_delay
                 << " (difference " << actual_delay - expected_delay << ")";
    pass = false;
  }
  // If the expected delay is <= zero then allow a small error tolerance, since
  // we do not expect context switches to occur during test execution.
  // Otherwise, thread scheduling delays may be substantial in rare cases, so
  // tolerate up to kTimeoutTestAllowedSchedulingDelay of error.
  turbo::Duration tolerance = expected_delay <= turbo::ZeroDuration()
                                 ? turbo::Milliseconds(10)
                                 : TimeoutTestAllowedSchedulingDelay();
  if (actual_delay > expected_delay + tolerance) {
    LOG(WARNING) << "Actual delay " << actual_delay
                 << " was too long, expected " << expected_delay
                 << " (difference " << actual_delay - expected_delay << ")";
    pass = false;
  }
  return pass;
}

// Parameters for TimeoutTest, below.
struct TimeoutTestParam {
  // The file and line number (used for logging purposes only).
  const char *from_file;
  int from_line;

  // Should the absolute deadline API based on turbo::Time be tested?  If false,
  // the relative deadline API based on turbo::Duration is tested.
  bool use_absolute_deadline;

  // The deadline/timeout used when calling the API being tested
  // (e.g. Mutex::LockWhenWithDeadline).
  turbo::Duration wait_timeout;

  // The delay before the condition will be set true by the test code.  If zero
  // or negative, the condition is set true immediately (before calling the API
  // being tested).  Otherwise, if infinite, the condition is never set true.
  // Otherwise a closure is scheduled for the future that sets the condition
  // true.
  turbo::Duration satisfy_condition_delay;

  // The expected result of the condition after the call to the API being
  // tested. Generally `true` means the condition was true when the API returns,
  // `false` indicates an expected timeout.
  bool expected_result;

  // The expected delay before the API under test returns.  This is inherently
  // flaky, so some slop is allowed (see `DelayIsWithinBounds` above), and the
  // test keeps trying indefinitely until this constraint passes.
  turbo::Duration expected_delay;
};

// Print a `TimeoutTestParam` to a debug log.
std::ostream &operator<<(std::ostream &os, const TimeoutTestParam &param) {
  return os << "from: " << param.from_file << ":" << param.from_line
            << " use_absolute_deadline: "
            << (param.use_absolute_deadline ? "true" : "false")
            << " wait_timeout: " << param.wait_timeout
            << " satisfy_condition_delay: " << param.satisfy_condition_delay
            << " expected_result: "
            << (param.expected_result ? "true" : "false")
            << " expected_delay: " << param.expected_delay;
}

// Like `thread::Executor::ScheduleAt` except:
// a) Delays zero or negative are executed immediately in the current thread.
// b) Infinite delays are never scheduled.
// c) Calls this test's `ScheduleAt` helper instead of using `pool` directly.
static void RunAfterDelay(turbo::Duration delay,
                          turbo::synchronization_internal::ThreadPool *pool,
                          const std::function<void()> &callback) {
  if (delay <= turbo::ZeroDuration()) {
    callback();  // immediate
  } else if (delay != turbo::InfiniteDuration()) {
    ScheduleAfter(pool, delay, callback);
  }
}

class TimeoutTest : public ::testing::Test,
                    public ::testing::WithParamInterface<TimeoutTestParam> {};

std::vector<TimeoutTestParam> MakeTimeoutTestParamValues() {
  // The `finite` delay is a finite, relatively short, delay.  We make it larger
  // than our allowed scheduling delay (slop factor) to avoid confusion when
  // diagnosing test failures.  The other constants here have clear meanings.
  const turbo::Duration finite = 3 * TimeoutTestAllowedSchedulingDelay();
  const turbo::Duration never = turbo::InfiniteDuration();
  const turbo::Duration negative = -turbo::InfiniteDuration();
  const turbo::Duration immediate = turbo::ZeroDuration();

  // Every test case is run twice; once using the absolute deadline API and once
  // using the relative timeout API.
  std::vector<TimeoutTestParam> values;
  for (bool use_absolute_deadline : {false, true}) {
    // Tests with a negative timeout (deadline in the past), which should
    // immediately return current state of the condition.

    // The condition is already true:
    values.push_back(TimeoutTestParam{
        __FILE__, __LINE__, use_absolute_deadline,
        negative,   // wait_timeout
        immediate,  // satisfy_condition_delay
        true,       // expected_result
        immediate,  // expected_delay
    });

    // The condition becomes true, but the timeout has already expired:
    values.push_back(TimeoutTestParam{
        __FILE__, __LINE__, use_absolute_deadline,
        negative,  // wait_timeout
        finite,    // satisfy_condition_delay
        false,     // expected_result
        immediate  // expected_delay
    });

    // The condition never becomes true:
    values.push_back(TimeoutTestParam{
        __FILE__, __LINE__, use_absolute_deadline,
        negative,  // wait_timeout
        never,     // satisfy_condition_delay
        false,     // expected_result
        immediate  // expected_delay
    });

    // Tests with an infinite timeout (deadline in the infinite future), which
    // should only return when the condition becomes true.

    // The condition is already true:
    values.push_back(TimeoutTestParam{
        __FILE__, __LINE__, use_absolute_deadline,
        never,      // wait_timeout
        immediate,  // satisfy_condition_delay
        true,       // expected_result
        immediate   // expected_delay
    });

    // The condition becomes true before the (infinite) expiry:
    values.push_back(TimeoutTestParam{
        __FILE__, __LINE__, use_absolute_deadline,
        never,   // wait_timeout
        finite,  // satisfy_condition_delay
        true,    // expected_result
        finite,  // expected_delay
    });

    // Tests with a (small) finite timeout (deadline soon), with the condition
    // becoming true both before and after its expiry.

    // The condition is already true:
    values.push_back(TimeoutTestParam{
        __FILE__, __LINE__, use_absolute_deadline,
        never,      // wait_timeout
        immediate,  // satisfy_condition_delay
        true,       // expected_result
        immediate   // expected_delay
    });

    // The condition becomes true before the expiry:
    values.push_back(TimeoutTestParam{
        __FILE__, __LINE__, use_absolute_deadline,
        finite * 2,  // wait_timeout
        finite,      // satisfy_condition_delay
        true,        // expected_result
        finite       // expected_delay
    });

    // The condition becomes true, but the timeout has already expired:
    values.push_back(TimeoutTestParam{
        __FILE__, __LINE__, use_absolute_deadline,
        finite,      // wait_timeout
        finite * 2,  // satisfy_condition_delay
        false,       // expected_result
        finite       // expected_delay
    });

    // The condition never becomes true:
    values.push_back(TimeoutTestParam{
        __FILE__, __LINE__, use_absolute_deadline,
        finite,  // wait_timeout
        never,   // satisfy_condition_delay
        false,   // expected_result
        finite   // expected_delay
    });
  }
  return values;
}

// Instantiate `TimeoutTest` with `MakeTimeoutTestParamValues()`.
INSTANTIATE_TEST_SUITE_P(All, TimeoutTest,
                         testing::ValuesIn(MakeTimeoutTestParamValues()));

TEST_P(TimeoutTest, Await) {
  const TimeoutTestParam params = GetParam();
  LOG(INFO) << "Params: " << params;

  // Because this test asserts bounds on scheduling delays it is flaky.  To
  // compensate it loops forever until it passes.  Failures express as test
  // timeouts, in which case the test log can be used to diagnose the issue.
  for (int attempt = 1;; ++attempt) {
    LOG(INFO) << "Attempt " << attempt;

    turbo::Mutex mu;
    bool value = false;  // condition value (under mu)

    std::unique_ptr<turbo::synchronization_internal::ThreadPool> pool =
        CreateDefaultPool();
    RunAfterDelay(params.satisfy_condition_delay, pool.get(), [&] {
      turbo::MutexLock l(&mu);
      value = true;
    });

    turbo::MutexLock lock(&mu);
    turbo::Time start_time = turbo::Time::current_time();
    turbo::Condition cond(&value);
    bool result =
        params.use_absolute_deadline
            ? mu.AwaitWithDeadline(cond, start_time + params.wait_timeout)
            : mu.AwaitWithTimeout(cond, params.wait_timeout);
    if (DelayIsWithinBounds(params.expected_delay, turbo::Time::current_time() - start_time)) {
      EXPECT_EQ(params.expected_result, result);
      break;
    }
  }
}

TEST_P(TimeoutTest, LockWhen) {
  const TimeoutTestParam params = GetParam();
  LOG(INFO) << "Params: " << params;

  // Because this test asserts bounds on scheduling delays it is flaky.  To
  // compensate it loops forever until it passes.  Failures express as test
  // timeouts, in which case the test log can be used to diagnose the issue.
  for (int attempt = 1;; ++attempt) {
    LOG(INFO) << "Attempt " << attempt;

    turbo::Mutex mu;
    bool value = false;  // condition value (under mu)

    std::unique_ptr<turbo::synchronization_internal::ThreadPool> pool =
        CreateDefaultPool();
    RunAfterDelay(params.satisfy_condition_delay, pool.get(), [&] {
      turbo::MutexLock l(&mu);
      value = true;
    });

    turbo::Time start_time = turbo::Time::current_time();
    turbo::Condition cond(&value);
    bool result =
        params.use_absolute_deadline
            ? mu.LockWhenWithDeadline(cond, start_time + params.wait_timeout)
            : mu.LockWhenWithTimeout(cond, params.wait_timeout);
    mu.Unlock();

    if (DelayIsWithinBounds(params.expected_delay, turbo::Time::current_time() - start_time)) {
      EXPECT_EQ(params.expected_result, result);
      break;
    }
  }
}

TEST_P(TimeoutTest, ReaderLockWhen) {
  const TimeoutTestParam params = GetParam();
  LOG(INFO) << "Params: " << params;

  // Because this test asserts bounds on scheduling delays it is flaky.  To
  // compensate it loops forever until it passes.  Failures express as test
  // timeouts, in which case the test log can be used to diagnose the issue.
  for (int attempt = 0;; ++attempt) {
    LOG(INFO) << "Attempt " << attempt;

    turbo::Mutex mu;
    bool value = false;  // condition value (under mu)

    std::unique_ptr<turbo::synchronization_internal::ThreadPool> pool =
        CreateDefaultPool();
    RunAfterDelay(params.satisfy_condition_delay, pool.get(), [&] {
      turbo::MutexLock l(&mu);
      value = true;
    });

    turbo::Time start_time = turbo::Time::current_time();
    bool result =
        params.use_absolute_deadline
            ? mu.ReaderLockWhenWithDeadline(turbo::Condition(&value),
                                            start_time + params.wait_timeout)
            : mu.ReaderLockWhenWithTimeout(turbo::Condition(&value),
                                           params.wait_timeout);
    mu.ReaderUnlock();

    if (DelayIsWithinBounds(params.expected_delay, turbo::Time::current_time() - start_time)) {
      EXPECT_EQ(params.expected_result, result);
      break;
    }
  }
}

TEST_P(TimeoutTest, Wait) {
  const TimeoutTestParam params = GetParam();
  LOG(INFO) << "Params: " << params;

  // Because this test asserts bounds on scheduling delays it is flaky.  To
  // compensate it loops forever until it passes.  Failures express as test
  // timeouts, in which case the test log can be used to diagnose the issue.
  for (int attempt = 0;; ++attempt) {
    LOG(INFO) << "Attempt " << attempt;

    turbo::Mutex mu;
    bool value = false;  // condition value (under mu)
    turbo::CondVar cv;    // signals a change of `value`

    std::unique_ptr<turbo::synchronization_internal::ThreadPool> pool =
        CreateDefaultPool();
    RunAfterDelay(params.satisfy_condition_delay, pool.get(), [&] {
      turbo::MutexLock l(&mu);
      value = true;
      cv.Signal();
    });

    turbo::MutexLock lock(&mu);
    turbo::Time start_time = turbo::Time::current_time();
    turbo::Duration timeout = params.wait_timeout;
    turbo::Time deadline = start_time + timeout;
    while (!value) {
      if (params.use_absolute_deadline ? cv.WaitWithDeadline(&mu, deadline)
                                       : cv.WaitWithTimeout(&mu, timeout)) {
        break;  // deadline/timeout exceeded
      }
      timeout = deadline - turbo::Time::current_time();  // recompute
    }
    bool result = value;  // note: `mu` is still held

    if (DelayIsWithinBounds(params.expected_delay, turbo::Time::current_time() - start_time)) {
      EXPECT_EQ(params.expected_result, result);
      break;
    }
  }
}

TEST(Mutex, Logging) {
  // Allow user to look at logging output
  turbo::Mutex logged_mutex;
  logged_mutex.EnableDebugLog("fido_mutex");
  turbo::CondVar logged_cv;
  logged_cv.EnableDebugLog("rover_cv");
  logged_mutex.Lock();
  logged_cv.WaitWithTimeout(&logged_mutex, turbo::Milliseconds(20));
  logged_mutex.Unlock();
  logged_mutex.ReaderLock();
  logged_mutex.ReaderUnlock();
  logged_mutex.Lock();
  logged_mutex.Unlock();
  logged_cv.Signal();
  logged_cv.SignalAll();
}

TEST(Mutex, LoggingAddressReuse) {
  // Repeatedly re-create a Mutex with debug logging at the same address.
  ScopedInvariantDebugging scoped_debugging;
  alignas(turbo::Mutex) char storage[sizeof(turbo::Mutex)];
  auto invariant =
      +[](void *alive) { EXPECT_TRUE(*static_cast<bool *>(alive)); };
  constexpr size_t kIters = 10;
  bool alive[kIters] = {};
  for (size_t i = 0; i < kIters; ++i) {
    turbo::Mutex *mu = new (storage) turbo::Mutex;
    alive[i] = true;
    mu->EnableDebugLog("Mutex");
    mu->EnableInvariantDebugging(invariant, &alive[i]);
    mu->Lock();
    mu->Unlock();
    mu->~Mutex();
    alive[i] = false;
  }
}

TEST(Mutex, LoggingBankrupcy) {
  // Test the case with too many live Mutexes with debug logging.
  ScopedInvariantDebugging scoped_debugging;
  std::vector<turbo::Mutex> mus(1 << 20);
  for (auto &mu : mus) {
    mu.EnableDebugLog("Mutex");
  }
}

TEST(Mutex, SynchEventRace) {
  // Regression test for a false TSan race report in
  // EnableInvariantDebugging/EnableDebugLog related to SynchEvent reuse.
  ScopedInvariantDebugging scoped_debugging;
  std::vector<std::thread> threads;
  for (size_t i = 0; i < 5; i++) {
    threads.emplace_back([&] {
      for (size_t j = 0; j < (1 << 17); j++) {
        {
          turbo::Mutex mu;
          mu.EnableInvariantDebugging([](void *) {}, nullptr);
          mu.Lock();
          mu.Unlock();
        }
        {
          turbo::Mutex mu;
          mu.EnableDebugLog("Mutex");
        }
      }
    });
  }
  for (auto &thread : threads) {
    thread.join();
  }
}

// --------------------------------------------------------

// Generate the vector of thread counts for tests parameterized on thread count.
static std::vector<int> AllThreadCountValues() {
  if (kExtendedTest) {
    return {2, 4, 8, 10, 16, 20, 24, 30, 32};
  }
  return {2, 4, 10};
}

// A test fixture parameterized by thread count.
class MutexVariableThreadCountTest : public ::testing::TestWithParam<int> {};

// Instantiate the above with AllThreadCountOptions().
INSTANTIATE_TEST_SUITE_P(ThreadCounts, MutexVariableThreadCountTest,
                         ::testing::ValuesIn(AllThreadCountValues()),
                         ::testing::PrintToStringParamName());

// Reduces iterations by some factor for slow platforms
// (determined empirically).
static int ScaleIterations(int x) {
  // TURBO_MUTEX_READER_LOCK_IS_EXCLUSIVE is set in the implementation
  // of Mutex that uses either std::mutex or pthread_mutex_t. Use
  // these as keys to determine the slow implementation.
#if defined(TURBO_MUTEX_READER_LOCK_IS_EXCLUSIVE)
  return x / 10;
#else
  return x;
#endif
}

TEST_P(MutexVariableThreadCountTest, Mutex) {
  int threads = GetParam();
  int iterations = ScaleIterations(10000000) / threads;
  int operations = threads * iterations;
  EXPECT_EQ(RunTest(&TestMu, threads, iterations, operations), operations);
#if !defined(TURBO_MUTEX_ENABLE_INVARIANT_DEBUGGING_NOT_IMPLEMENTED)
  iterations = std::min(iterations, 10);
  operations = threads * iterations;
  EXPECT_EQ(RunTestWithInvariantDebugging(&TestMu, threads, iterations,
                                          operations, CheckSumG0G1),
            operations);
#endif
}

TEST_P(MutexVariableThreadCountTest, Try) {
  int threads = GetParam();
  int iterations = 1000000 / threads;
  int operations = iterations * threads;
  EXPECT_EQ(RunTest(&TestTry, threads, iterations, operations), operations);
#if !defined(TURBO_MUTEX_ENABLE_INVARIANT_DEBUGGING_NOT_IMPLEMENTED)
  iterations = std::min(iterations, 10);
  operations = threads * iterations;
  EXPECT_EQ(RunTestWithInvariantDebugging(&TestTry, threads, iterations,
                                          operations, CheckSumG0G1),
            operations);
#endif
}

TEST_P(MutexVariableThreadCountTest, R20ms) {
  int threads = GetParam();
  int iterations = 100;
  int operations = iterations * threads;
  EXPECT_EQ(RunTest(&TestR20ms, threads, iterations, operations), 0);
}

TEST_P(MutexVariableThreadCountTest, RW) {
  int threads = GetParam();
  int iterations = ScaleIterations(20000000) / threads;
  int operations = iterations * threads;
  EXPECT_EQ(RunTest(&TestRW, threads, iterations, operations), operations / 2);
#if !defined(TURBO_MUTEX_ENABLE_INVARIANT_DEBUGGING_NOT_IMPLEMENTED)
  iterations = std::min(iterations, 10);
  operations = threads * iterations;
  EXPECT_EQ(RunTestWithInvariantDebugging(&TestRW, threads, iterations,
                                          operations, CheckSumG0G1),
            operations / 2);
#endif
}

TEST_P(MutexVariableThreadCountTest, Await) {
  int threads = GetParam();
  int iterations = ScaleIterations(500000);
  int operations = iterations;
  EXPECT_EQ(RunTest(&TestAwait, threads, iterations, operations), operations);
}

TEST_P(MutexVariableThreadCountTest, SignalAll) {
  int threads = GetParam();
  int iterations = 200000 / threads;
  int operations = iterations;
  EXPECT_EQ(RunTest(&TestSignalAll, threads, iterations, operations),
            operations);
}

TEST(Mutex, Signal) {
  int threads = 2;  // TestSignal must use two threads
  int iterations = 200000;
  int operations = iterations;
  EXPECT_EQ(RunTest(&TestSignal, threads, iterations, operations), operations);
}

TEST(Mutex, Timed) {
  int threads = 10;  // Use a fixed thread count of 10
  int iterations = 1000;
  int operations = iterations;
  EXPECT_EQ(RunTest(&TestCVTimeout, threads, iterations, operations),
            operations);
}

TEST(Mutex, CVTime) {
  int threads = 10;  // Use a fixed thread count of 10
  int iterations = 1;
  EXPECT_EQ(RunTest(&TestCVTime, threads, iterations, 1), threads * iterations);
}

TEST(Mutex, MuTime) {
  int threads = 10;  // Use a fixed thread count of 10
  int iterations = 1;
  EXPECT_EQ(RunTest(&TestMuTime, threads, iterations, 1), threads * iterations);
}

TEST(Mutex, SignalExitedThread) {
  // The test may expose a race when Mutex::Unlock signals a thread
  // that has already exited.
#if defined(__wasm__) || defined(__asmjs__)
  constexpr int kThreads = 1;  // OOMs under WASM
#else
  constexpr int kThreads = 100;
#endif
  std::vector<std::thread> top;
  for (unsigned i = 0; i < 2 * std::thread::hardware_concurrency(); i++) {
    top.emplace_back([&]() {
      for (int i = 0; i < kThreads; i++) {
        turbo::Mutex mu;
        std::thread t([&]() {
          mu.Lock();
          mu.Unlock();
        });
        mu.Lock();
        mu.Unlock();
        t.join();
      }
    });
  }
  for (auto &th : top) th.join();
}

TEST(Mutex, WriterPriority) {
  turbo::Mutex mu;
  bool wrote = false;
  std::atomic<bool> saw_wrote{false};
  auto readfunc = [&]() {
    for (size_t i = 0; i < 10; ++i) {
      turbo::ReaderMutexLock lock(&mu);
      if (wrote) {
        saw_wrote = true;
        break;
      }
      turbo::SleepFor(turbo::Seconds(1));
    }
  };
  std::thread t1(readfunc);
  turbo::SleepFor(turbo::Milliseconds(500));
  std::thread t2(readfunc);
  // Note: this test guards against a bug that was related to an uninit
  // PerThreadSynch::priority, so the writer intentionally runs on a new thread.
  std::thread t3([&]() {
    // The writer should be able squeeze between the two alternating readers.
    turbo::MutexLock lock(&mu);
    wrote = true;
  });
  t1.join();
  t2.join();
  t3.join();
  EXPECT_TRUE(saw_wrote.load());
}

#ifdef TURBO_HAVE_PTHREAD_GETSCHEDPARAM
TEST(Mutex, CondVarPriority) {
  // A regression test for a bug in condition variable wait morphing,
  // which resulted in the waiting thread getting priority of the waking thread.
  int err = 0;
  sched_param param;
  param.sched_priority = 7;
  std::thread test([&]() {
    err = pthread_setschedparam(pthread_self(), SCHED_FIFO, &param);
  });
  test.join();
  if (err) {
    // Setting priority usually requires special privileges.
    GTEST_SKIP() << "failed to set priority: " << strerror(err);
  }
  turbo::Mutex mu;
  turbo::CondVar cv;
  bool locked = false;
  bool notified = false;
  bool waiting = false;
  bool morph = false;
  std::thread th([&]() {
    EXPECT_EQ(0, pthread_setschedparam(pthread_self(), SCHED_FIFO, &param));
    mu.Lock();
    locked = true;
    mu.Await(turbo::Condition(&notified));
    mu.Unlock();
    EXPECT_EQ(turbo::synchronization_internal::GetOrCreateCurrentThreadIdentity()
                  ->per_thread_synch.priority,
              param.sched_priority);
    mu.Lock();
    mu.Await(turbo::Condition(&waiting));
    morph = true;
    turbo::SleepFor(turbo::Seconds(1));
    cv.Signal();
    mu.Unlock();
  });
  mu.Lock();
  mu.Await(turbo::Condition(&locked));
  notified = true;
  mu.Unlock();
  mu.Lock();
  waiting = true;
  while (!morph) {
    cv.Wait(&mu);
  }
  mu.Unlock();
  th.join();
  EXPECT_NE(turbo::synchronization_internal::GetOrCreateCurrentThreadIdentity()
                ->per_thread_synch.priority,
            param.sched_priority);
}
#endif

TEST(Mutex, LockWhenWithTimeoutResult) {
  // Check various corner cases for Await/LockWhen return value
  // with always true/always false conditions.
  turbo::Mutex mu;
  const bool kAlwaysTrue = true, kAlwaysFalse = false;
  const turbo::Condition kTrueCond(&kAlwaysTrue), kFalseCond(&kAlwaysFalse);
  EXPECT_TRUE(mu.LockWhenWithTimeout(kTrueCond, turbo::Milliseconds(1)));
  mu.Unlock();
  EXPECT_FALSE(mu.LockWhenWithTimeout(kFalseCond, turbo::Milliseconds(1)));
  EXPECT_TRUE(mu.AwaitWithTimeout(kTrueCond, turbo::Milliseconds(1)));
  EXPECT_FALSE(mu.AwaitWithTimeout(kFalseCond, turbo::Milliseconds(1)));
  std::thread th1([&]() {
    EXPECT_TRUE(mu.LockWhenWithTimeout(kTrueCond, turbo::Milliseconds(1)));
    mu.Unlock();
  });
  std::thread th2([&]() {
    EXPECT_FALSE(mu.LockWhenWithTimeout(kFalseCond, turbo::Milliseconds(1)));
    mu.Unlock();
  });
  turbo::SleepFor(turbo::Milliseconds(100));
  mu.Unlock();
  th1.join();
  th2.join();
}

}  // namespace
