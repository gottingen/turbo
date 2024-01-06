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
#ifndef TURBO_TASKFLOW_ALGORITHM_LAUNCH_H_
#define TURBO_TASKFLOW_ALGORITHM_LAUNCH_H_

#include <turbo/taskflow/core/async.h>

namespace turbo {

    // Function: launch_loop
    template<typename P, typename Loop>
    TURBO_FORCE_INLINE void launch_loop(
            size_t N,
            size_t W,
            Runtime &rt,
            std::atomic<size_t> &next,
            P &&part,
            Loop &&loop
    ) {

        //static_assert(std::is_lvalue_reference_v<Loop>, "");

        using namespace std::string_literals;

        for (size_t w = 0; w < W; w++) {
            auto r = N - next.load(std::memory_order_relaxed);
            // no more loop work to do - finished by previous async tasks
            if (!r) {
                break;
            }
            // tail optimization
            if (r <= part.chunk_size() || w == W - 1) {
                loop();
                break;
            } else {
                rt.silent_async_unchecked("loop-"s + std::to_string(w), loop);
            }
        }

        rt.corun_all();
    }

    // Function: launch_loop
    template<typename Loop>
    TURBO_FORCE_INLINE void launch_loop(
            size_t W,
            size_t w,
            Runtime &rt,
            Loop &&loop
    ) {
        using namespace std::string_literals;
        if (w == W - 1) {
            loop();
        } else {
            rt.silent_async_unchecked("loop-"s + std::to_string(w), loop);
        }
    }

}  // namespace turbo
#endif  // TURBO_TASKFLOW_ALGORITHM_LAUNCH_H_
