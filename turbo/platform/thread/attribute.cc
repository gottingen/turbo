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

#ifndef _GNU_SOURCE
#define _GNU_SOURCE
#endif

#if defined(TURBO_PLATFORM_LINUX)

#include <pthread.h>
#include <sched.h>
#include <string>
#include "turbo/log/logging.h"

#endif  // TURBO_PLATFORM_LINUX

namespace turbo::platform_internal {

#if defined(TURBO_PLATFORM_LINUX)

int TrySetCurrentThreadAffinity(const std::vector<int>& affinity) {
  TURBO_CHECK(!affinity.empty());
  cpu_set_t cpuset;

  CPU_ZERO(&cpuset);
  for (auto&& e : affinity) {
    CPU_SET(e, &cpuset);
  }
  return pthread_setaffinity_np(pthread_self(), sizeof(cpu_set_t), &cpuset);
}

void SetCurrentThreadAffinity(const std::vector<int>& affinity) {
  auto rc = TrySetCurrentThreadAffinity(affinity);
  TURBO_CHECK(rc == 0)<< "Cannot set thread affinity for thread ["
                      <<reinterpret_cast<const void*>(pthread_self())
                      <<"]: ["<<rc<<"] "<<strerror(rc)<<".";
}

std::vector<int> GetCurrentThreadAffinity() {
  cpu_set_t cpuset;
  CPU_ZERO(&cpuset);
  auto rc = pthread_getaffinity_np(pthread_self(), sizeof(cpu_set_t), &cpuset);
  TURBO_CHECK(rc == 0)<<"Cannot set name for thread ["<<reinterpret_cast<const void*>(pthread_self())
                    <<"]: ["<<rc
                  <<"] "<<strerror(rc));

  std::vector<int> result;
  for (int i = 0; i != __CPU_SETSIZE; ++i) {
    if (CPU_ISSET(i, &cpuset)) {
      result.push_back(i);
    }
  }
  return result;
}

void SetCurrentThreadName(const std::string& name) {
  auto rc = pthread_setname_np(pthread_self(), name.c_str());
  if (rc != 0) {
    TURBO_LOG(WARNING)<<"Cannot set name for thread ["<<reinterpret_cast<const void*>(pthread_self())
                      <<"]: ["<<rc
                      <<"] "<<strerror(rc));
    // Silently ignored.
  }
}

#endif  // TURBO_PLATFORM_LINUX
}  // namespace turbo::platform_internal
