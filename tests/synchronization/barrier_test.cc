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

#include <turbo/synchronization/barrier.h>

#include <thread>  // NOLINT(build/c++11)
#include <vector>

#include <gtest/gtest.h>
#include <turbo/synchronization/mutex.h>
#include <turbo/times/clock.h>


TEST(Barrier, SanityTest) {
  constexpr int kNumThreads = 10;
  turbo::Barrier* barrier = new turbo::Barrier(kNumThreads);

  turbo::Mutex mutex;
  int counter = 0;  // Guarded by mutex.

  auto thread_func = [&] {
    if (barrier->Block()) {
      // This thread is the last thread to reach the barrier so it is
      // responsible for deleting it.
      delete barrier;
    }

    // Increment the counter.
    turbo::MutexLock lock(&mutex);
    ++counter;
  };

  // Start (kNumThreads - 1) threads running thread_func.
  std::vector<std::thread> threads;
  for (int i = 0; i < kNumThreads - 1; ++i) {
    threads.push_back(std::thread(thread_func));
  }

  // Give (kNumThreads - 1) threads a chance to reach the barrier.
  // This test assumes at least one thread will have run after the
  // sleep has elapsed. Sleeping in a test is usually bad form, but we
  // need to make sure that we are testing the barrier instead of some
  // other synchronization method.
  turbo::SleepFor(turbo::Duration::seconds(1));

  // The counter should still be zero since no thread should have
  // been able to pass the barrier yet.
  {
    turbo::MutexLock lock(&mutex);
    EXPECT_EQ(counter, 0);
  }

  // Start 1 more thread. This should make all threads pass the barrier.
  threads.push_back(std::thread(thread_func));

  // All threads should now be able to proceed and finish.
  for (auto& thread : threads) {
    thread.join();
  }

  // All threads should now have incremented the counter.
  turbo::MutexLock lock(&mutex);
  EXPECT_EQ(counter, kNumThreads);
}
