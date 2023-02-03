//
// Copyright 2023 The Turbo Authors.
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


#include "turbo/platform/thread/attribute.h"
#if defined(TURBO_PLATFORM_LINUX)
#include <pthread.h>
#include <chrono>
#include <thread>
#include <vector>
#include "gtest/gtest.h"

#include "turbo/base/internal/cpu.h"
#include "turbo/log/logging.h"
#include "turbo/base/random.h"

using namespace std::literals;

namespace turbo::platform_internal {

TEST(Thread, SetCurrentAffinity) {
  auto nprocs = internal::GetNumberOfProcessorsConfigured();
  for (int j = 0; j != 1000; ++j) {
    for (int i = 0; i != nprocs; ++i) {
      if (internal::IsProcessorAccessible(i)) {
        SetCurrentThreadAffinity(std::vector<int>{i});
        if (Random(100) < 10) {
          std::this_thread::sleep_for(1ms);
        }
        ASSERT_EQ(i, internal::GetCurrentProcessorId());
      }
    }
  }
}

TEST(Thread, SetCurrentName) {
  SetCurrentThreadName("asdf");
  char buffer[30] = {};
  pthread_getname_np(pthread_self(), buffer, sizeof(buffer));
  ASSERT_EQ("asdf", std::string(buffer));
}

}  // namespace turbo::platform_internal

#endif  // TURBO_PLATFORM_LINUX
