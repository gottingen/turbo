
/****************************************************************
 * Copyright (c) 2022, liyinbin
 * All rights reserved.
 * Author by liyinbin (jeff.li) lijippy@163.com
 *****************************************************************/

#ifndef FLARE_FIBER_FIBER_ASYNC_H_
#define FLARE_FIBER_FIBER_ASYNC_H_


#include <type_traits>
#include <utility>
#include "flare/future/future.h"
#include "flare/fiber/fiber.h"

namespace flare {

    // Runs `f` with `args...` asynchronously.
    //
    // It's unspecified in which fiber (except the caller's own one) `f` is called.
    //
    // Note that this method is only available in fiber runtime. If you want to
    // start a fiber from pthread, use `start_fiber_from_pthread` (@sa: `fiber.h`)
    // instead.
    template<class F, class... Args>
    inline void fiber_async(attribute policy, F &&f, Args &&... args) {

        auto proc = [f = std::forward<F>(f),
                args = std::make_tuple(std::forward<Args>(args)...)](void *) mutable -> void * {
            std::apply(f, std::move(args));
            return nullptr;
        };
        fiber fb = fiber(policy, std::move(proc), nullptr);
        fb.detach();
    }

    template<class F>
    void fiber_async(attribute policy, F &&f) {
        auto proc = [f = std::forward<F>(f)](void *) mutable -> void * {
            f();
            return nullptr;
        };
        fiber fb = fiber(policy, std::move(proc), nullptr);
        fb.detach();
    }

}  // namespace flare

#endif  // FLARE_FIBER_FIBER_ASYNC_H_
