
/****************************************************************
 * Copyright (c) 2022, liyinbin
 * All rights reserved.
 * Author by liyinbin (jeff.li) lijippy@163.com
 *****************************************************************/


#ifndef FLARE_FIBER_FIBER_H_
#define FLARE_FIBER_FIBER_H_

#include "flare/fiber/internal/types.h"
#include "flare/fiber/internal/fiber.h"

namespace flare {

    enum class launch_policy {
        eImmediately,
        eLazy
    };

    struct attribute {
        launch_policy policy;
        fiber_stack_type_t stack_type;
        fiber_attribute_flag flags;
        fiber_keytable_pool_t *keytable_pool;
    };

    static const attribute kAttrPthread{.policy = launch_policy::eImmediately,
            .stack_type = FIBER_STACKTYPE_PTHREAD,
            .flags = 0,
            .keytable_pool = nullptr};

    static const attribute kAttrSmall = attribute{.policy = launch_policy::eImmediately,
            .stack_type = FIBER_STACKTYPE_SMALL,
            .flags = 0,
            .keytable_pool = nullptr};


    static const attribute kAttrNormal = attribute{.policy = launch_policy::eImmediately,
            .stack_type = FIBER_STACKTYPE_NORMAL,
            .flags = 0,
            .keytable_pool = nullptr};

    static const attribute kAttrLarge{.policy = launch_policy::eImmediately,
            .stack_type = FIBER_STACKTYPE_LARGE,
            .flags = 0,
            .keytable_pool = nullptr};

    class fiber {
    public:

    public:
        // Create an empty (invalid) fiber.
        fiber();

        explicit fiber(std::function<void *(void *)> &&fn, void *args = nullptr)
                : fiber(kAttrNormal, std::move(fn), args) {
        }

        fiber(const attribute &attr, std::function<void *(void *)> &&fn, void *args = nullptr);

        fiber(launch_policy policy, std::function<void *(void *)> &&fn, void *args = nullptr)
                : fiber(
                attribute{.policy = policy, .stack_type = FIBER_STACKTYPE_NORMAL, .flags = 0, .keytable_pool = nullptr},
                std::forward<std::function<void *(void *)>>(fn),
                args) {}

        // If a `fiber` object which owns a fiber is destructed with no prior call to
        // `join()` or `detach()`, it leads to abort.
        ~fiber();

        fiber_id_t self() const {
            return _fid;
        }

        // Wait for the fiber to exit.
        void join();

        void detach();

        bool stopped() const;

        void stop();

        int error() const { return _save_error; }

    private:

        int _save_error;
        fiber_id_t _fid;
        bool _detached;
    };
}  // namespace flare
#endif // FLARE_FIBER_FIBER_H_
