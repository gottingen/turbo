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
// Created by jeff on 24-1-26.
//
#include "turbo/flags/flag.h"
#include "turbo/fiber/config.h"

TURBO_FLAG(int32_t, stack_size_small, 32768, "size of small stacks");
TURBO_FLAG(int32_t, stack_size_normal, 1048576, "size of normal stacks");
TURBO_FLAG(int32_t, stack_size_large, 8388608, "size of large stacks");
TURBO_FLAG(int32_t, guard_page_size, 4096, "size of guard page, allocate stacks by malloc if it's 0(not recommended)");
TURBO_FLAG(int32_t, tc_stack_small, 32, "maximum small stacks cached by each thread");
TURBO_FLAG(int32_t, tc_stack_normal, 8, "maximum normal stacks cached by each thread");

TURBO_FLAG(int32_t, task_group_delete_delay, 1, "delay deletion of TaskGroup for so many seconds");
TURBO_FLAG(int32_t, task_group_runqueue_capacity, 4096, "capacity of runqueue in each TaskGroup");
TURBO_FLAG(int32_t, task_group_yield_before_idle, 0, "TaskGroup yields so many times before idle");

TURBO_FLAG(int32_t, fiber_concurrency, 8 + turbo::fiber_config::FIBER_EPOLL_THREAD_NUM, "Number of fiber workers");
TURBO_FLAG(int32_t, fiber_min_concurrency, 0, "Initial number of fiber workers which will be added on-demand.");
