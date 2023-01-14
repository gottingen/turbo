
/****************************************************************
 * Copyright (c) 2022, liyinbin
 * All rights reserved.
 * Author by liyinbin (jeff.li) lijippy@163.com
 *****************************************************************/

#ifndef TURBO_BOOTSTRAP_BOOTSTRAP_H_
#define TURBO_BOOTSTRAP_BOOTSTRAP_H_


#include <cstdint>
#include <utility>
#include <functional>
#include "turbo/base/profile.h"

// Usage:
//
// - TURBO_BOOTSTRAP(init, fini = nullptr): This overload defaults `priority` to
//   1.
// - TURBO_BOOTSTRAP(priority, init, fini = nullptr)
//
// This macro registers a callback that is called in `turbo::Start` (after
// `main` is entered`.). The user may also provide a finalizer, which is called
// before leaving `turbo::Start`, in opposite order.
//
// `priority` specifies relative order between callbacks. Callbacks with smaller
// `priority` are called earlier. Order between callbacks with same priority is
// unspecified and may not be relied on.
//
// It explicitly allowed to use this macro *without* carring a dependency to
// `//turbo:init`.
//
// For UT writers: If, for any reason, you cannot use `TURBO_TEST_MAIN` as your
// `main` in UT, you need to call `RunAllInitializers()` / `RunAllFinalizers()`
// yourself when entering / leaving `main`.

#define TURBO_BOOTSTRAP(...)                                                 \
  static ::turbo::internal::bootstrap_registration TURBO_CONCAT(        \
      turbo_bootstrap_registration_object_, __LINE__)(__FILE__, __LINE__, \
                                                       __VA_ARGS__);

// Implementation goes below.
namespace turbo {

    // Called by `OnInitRegistration`.
    void register_bootstrap_callback(std::int32_t priority, const std::function<void()> &init, const std::function<void()> &fini);

    void register_bootstrap_callback(std::int32_t priority, std::function<void()> &&init,
                                     std::function<void()> &&fini);
    // Registers a callback that's called before leaving `turbo::Start`.
    //
    // These callbacks are called after all finalizers registered via
    // `TURBO_BOOTSTRAP`.
    void set_at_exit_callback(std::function<void()> callback);

    void bootstrap_init(int argc, char**argv);
    // Called `turbo::run_bootstrap`.
    void run_bootstrap();

    void run_finalizers();

}  // namespace turbo

namespace turbo::internal {

    // Helper class for registering initialization callbacks at start-up time.
    class bootstrap_registration {
    public:
        bootstrap_registration(const char *file, std::uint32_t line,
                           std::function<void()> init, std::function<void()> fini = nullptr)
                : bootstrap_registration(file, line, 1, std::move(init), std::move(fini)) {}

        bootstrap_registration(const char *file, std::uint32_t line,
                           std::int32_t priority, std::function<void()> init,
                           std::function<void()> fini = nullptr) {
            turbo::register_bootstrap_callback(priority, std::move(init), std::move(fini));
        }
    };

}  // namespace turbo::internal


#endif // TURBO_BOOTSTRAP_BOOTSTRAP_H_
