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

#include <turbo/base/internal/sysinfo.h>

#ifndef _WIN32
#include <sys/types.h>
#include <unistd.h>
#endif

#include <thread>  // NOLINT(build/c++11)
#include <unordered_set>
#include <vector>

#include <gtest/gtest.h>
#include <turbo/synchronization/barrier.h>
#include <turbo/synchronization/mutex.h>

namespace turbo {
TURBO_NAMESPACE_BEGIN
namespace base_internal {
namespace {

TEST(SysinfoTest, NumCPUs) {
  EXPECT_NE(NumCPUs(), 0)
      << "NumCPUs() should not have the default value of 0";
}

TEST(SysinfoTest, GetTID) {
  EXPECT_EQ(GetTID(), GetTID());  // Basic compile and equality test.
#ifdef __native_client__
  // Native Client has a race condition bug that leads to memory
  // exaustion when repeatedly creating and joining threads.
  // https://bugs.chromium.org/p/nativeclient/issues/detail?id=1027
  return;
#endif
  // Test that TIDs are unique to each thread.
  // Uses a few loops to exercise implementations that reallocate IDs.
  for (int i = 0; i < 10; ++i) {
    constexpr int kNumThreads = 10;
    Barrier all_threads_done(kNumThreads);
    std::vector<std::thread> threads;

    Mutex mutex;
    std::unordered_set<pid_t> tids;

    for (int j = 0; j < kNumThreads; ++j) {
      threads.push_back(std::thread([&]() {
        pid_t id = GetTID();
        {
          MutexLock lock(&mutex);
          ASSERT_TRUE(tids.find(id) == tids.end());
          tids.insert(id);
        }
        // We can't simply join the threads here. The threads need to
        // be alive otherwise the TID might have been reallocated to
        // another live thread.
        all_threads_done.Block();
      }));
    }
    for (auto& thread : threads) {
      thread.join();
    }
  }
}

#ifdef __linux__
TEST(SysinfoTest, LinuxGetTID) {
  // On Linux, for the main thread, GetTID()==getpid() is guaranteed by the API.
  EXPECT_EQ(GetTID(), getpid());
}
#endif

}  // namespace
}  // namespace base_internal
TURBO_NAMESPACE_END
}  // namespace turbo
