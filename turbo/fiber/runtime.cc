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
// Created by jeff on 24-1-3.
//
#include "turbo/fiber/internal/fiber.h"

namespace turbo {

    int fiber_get_concurrency(void) {
        return turbo::fiber_internal::fiber_get_concurrency_impl();
    }

    turbo::Status fiber_set_concurrency(int num) {
        return turbo::fiber_internal::fiber_set_concurrency_impl(num);
    }

    void fiber_stop_world() {
        turbo::fiber_internal::fiber_stop_world_impl();
    }

}  // namespace turbo
