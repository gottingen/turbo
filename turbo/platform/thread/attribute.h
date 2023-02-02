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

#ifndef TURBO_PLATFORM_THREAD_ATTRIBUTE_H_
#define TURBO_PLATFORM_THREAD_ATTRIBUTE_H_

#include <cstdint>
#include <string>
#include <thread>
#include <vector>
#include "turbo/platform/macros.h"
#include "turbo/meta/type_traits.h"

namespace turbo::platform_internal {

#if defined(TURBO_PLATFORM_LINUX)

// Some helper methods for manipulating threads.

// Set affinity of the calling thread.
//
// Returns error number, or 0 on success.
int TrySetCurrentThreadAffinity(const std::vector<int>& affinity);

// Set affinity of the calling thread.
//
// Abort on failure.
void SetCurrentThreadAffinity(const std::vector<int>& affinity);

// Get affinity of the calling thread.
std::vector<int> GetCurrentThreadAffinity();

// Set name of the calling thread.
//
// Error, if any, is ignored.
void SetCurrentThreadName(const std::string& name);

#endif  // TURBO_PLATFORM_LINUX
}  // namespace turbo::platform_internal

#endif  // TURBO_PLATFORM_THREAD_ATTRIBUTE_H_
