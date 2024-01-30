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
#ifndef TURBO_FIBER_RUNTIME_H_
#define TURBO_FIBER_RUNTIME_H_

#include "turbo/fiber/config.h"
#include "turbo/status/status.h"

namespace turbo {

    /**
     * @ingroup turbo_fiber
     * @brief get the number of worker pthread
     * @return the number of pthread
     */
    int fiber_get_concurrency(void);

    /**
     * @ingroup turbo_fiber
     * @brief set number of worker pthreads to `num'. After a successful call,
     *        fiber_get_concurrency() shall return new set number, but workers may
     *        take some time to quit or create.
     * @note currently concurrency cannot be reduced after any fiber created.
     * @param num
     * @return
     */
    turbo::Status fiber_set_concurrency(int num);

    /**
     * @ingroup turbo_fiber
     * @brief  stop all the fiber workers, and wait for all the workers to quit.
     * @note   you should as possible as you can to avoid calling this function.
     * @return
     */
    void fiber_stop_world();

}  // namespace turbo
#endif // TURBO_FIBER_RUNTIME_H_
