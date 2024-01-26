// Copyright 2023 The Elastic-AI Authors.
// part of Elastic AI Search
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
//
// Created by jeff on 23-12-30.
//

#ifndef TURBO_FIBER_CONFIG_H_
#define TURBO_FIBER_CONFIG_H_

#include <cstddef>
#include <cstdint>
#include "turbo/flags/declare.h"

TURBO_DECLARE_FLAG(int32_t, stack_size_small);
TURBO_DECLARE_FLAG(int32_t, stack_size_normal);
TURBO_DECLARE_FLAG(int32_t, stack_size_large);
TURBO_DECLARE_FLAG(int32_t, guard_page_size);
TURBO_DECLARE_FLAG(int32_t, tc_stack_small);
TURBO_DECLARE_FLAG(int32_t, tc_stack_normal);
TURBO_DECLARE_FLAG(int32_t, task_group_delete_delay);
TURBO_DECLARE_FLAG(int32_t, task_group_runqueue_capacity);
TURBO_DECLARE_FLAG(int32_t, task_group_yield_before_idle);
TURBO_DECLARE_FLAG(int32_t, fiber_concurrency);
TURBO_DECLARE_FLAG(int32_t, fiber_min_concurrency);

namespace turbo::fiber_config {

    static constexpr size_t FIBER_EPOLL_THREAD_NUM = 1;

    static constexpr int FIBER_MIN_CONCURRENCY = 3 + FIBER_EPOLL_THREAD_NUM;
    static constexpr int FIBER_DEFAULT_CONCURRENCY = 8 + FIBER_EPOLL_THREAD_NUM;
    static constexpr int FIBER_MAX_CONCURRENCY = 1024;

}  // namespace turbo::fiber_config

#endif  // TURBO_FIBER_CONFIG_H_
