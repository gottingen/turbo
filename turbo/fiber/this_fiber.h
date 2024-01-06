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

#ifndef TURBO_FIBER_THIS_FIBER_H_
#define TURBO_FIBER_THIS_FIBER_H_

#include <cstdint>
#include "turbo/fiber/fiber.h"
#include "turbo/times/time.h"
#include "turbo/platform/port.h"

namespace turbo {

    /**
     * @ingroup turbo_fiber
     * @brief yield cpu to other fibers
     * @return 0 if success, -1 if error
     */
    int fiber_yield();

    /**
     * @ingroup turbo_fiber
     * @brief sleep until deadline
     * @param deadline
     * @return status.ok() if success, otherwise error
     */
    turbo::Status fiber_sleep_until(const turbo::Time& deadline);

    /**
     * @ingroup turbo_fiber
     * @brief sleep for a duration
     * @param span
     * @return status.ok() if success, otherwise error
     */
    turbo::Status fiber_sleep_for(const turbo::Duration& span);

    /**
     * @ingroup turbo_fiber
     * @brief get current fiber id
     * @return current fiber id
     */
    fiber_id_t get_fiber_id();

}  // namespace turbo
#endif  // TURBO_FIBER_THIS_FIBER_H_
