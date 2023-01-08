

/****************************************************************
 * Copyright (c) 2022, liyinbin
 * All rights reserved.
 * Author by liyinbin (jeff.li) lijippy@163.com
 *****************************************************************/

#include "flare/bootstrap/bootstrap.h"

#include <algorithm>
#include <atomic>
#include <map>
#include <mutex>
#include <random>
#include <utility>
#include <vector>
#include "flare/log/logging.h"
#include "flare/memory/resident.h"
#include "flare/bootstrap/flags.h"

namespace flare {

    namespace {

        void log_config_init(char* argv0);
        // The below two registries are filled by `prepare_for_running_callbacks()` (by
        // moving callbacks from the registry above.)
        std::vector<std::function<void()>> initializer_registry;
        std::vector<std::function<void()>> finalizer_registry;

        struct at_exit_callback_registry {
            std::mutex lock;
            std::vector<std::function<void()>> registry;
        };

        at_exit_callback_registry *get_at_exit_callback_registry() {
            static flare::resident<at_exit_callback_registry> registry;
            return registry.get();
        }

        void log_config_init(char* argv0) {
            flare::log::init_logging(argv0);
            //distable info level single file
            if(!FLAGS_flare_logtostderr) {
                flare::log::set_log_destination(flare::log::FLARE_INFO, "");
                //distable debug level single file
                flare::log::set_log_destination(flare::log::FLARE_DEBUG, "");
                flare::log::enable_log_cleaner(FLAGS_flare_log_save_days);
            }
        }
    }  // namespace

}  // namespace flare


namespace flare {

    void set_at_exit_callback(std::function<void()> callback) {
        auto &&registry = get_at_exit_callback_registry();
        std::scoped_lock _(registry->lock);
        registry->registry.push_back(std::move(callback));
    }

    std::atomic<bool> registry_prepared{false};

    std::random_device r;
    std::seed_seq seeds{r(), r(), r(), r(), r(), r(), r(), r()};
    std::mt19937_64 engine(seeds);

    // This registry is filled by `RegisterCallback()`.
    auto get_staging_registry() {
        // Must be `std::map`, order (of key) matters.
        static std::map<std::int32_t,
                std::vector<std::pair<std::function<void()>, std::function<void()>>>>
                registry;
        return &registry;
    }

    void prepare_for_running_callbacks() {
        auto &&temp_registry = *get_staging_registry();
        auto &&init_registry = initializer_registry;
        auto &&fini_registry = finalizer_registry;

        // Loop from lowest priority to highest.
        for (auto&&[k, v] : temp_registry) {
            // Force a shuffle here, so that the user cannot rely on relative call order
            // between same priority callbacks.

            std::shuffle(v.begin(), v.end(), engine);
            for (auto &&f : v) {
                init_registry.push_back(std::move(f.first));
                if (f.second) {
                    fini_registry.push_back(std::move(f.second));
                }
            }
        }

        // Finalizers are called in opposite order.
        std::reverse(fini_registry.begin(), fini_registry.end());
        registry_prepared.store(true, std::memory_order_relaxed);
    }

    void run_bootstrap() {
        prepare_for_running_callbacks();

        auto &&registry = initializer_registry;
        for (auto &&c : registry) {
            c();
        }
        // In case the initializer holds some resource (unlikely), free it now.
        registry.clear();
    }

    void run_finalizers() {
        auto &&registry = finalizer_registry;
        for (auto &&c : registry) {
            c();
        }
        registry.clear();
        for (auto &&e : get_at_exit_callback_registry()->registry) {
            e();
        }
    }


    void register_bootstrap_callback(std::int32_t priority, const std::function<void()> &init,
                                     const std::function<void()> &fini) {
        FLARE_CHECK(!registry_prepared.load(std::memory_order_relaxed)) <<
                                                                  "Callbacks may only be registered before `flare::Start` is called.";

        auto &&registry = *get_staging_registry();
        registry[priority].emplace_back(init, fini);
    }

    void register_bootstrap_callback(std::int32_t priority, std::function<void()> &&init,
                                     std::function<void()> &&fini) {
        FLARE_CHECK(!registry_prepared.load(std::memory_order_relaxed)) <<
                                                                  "Callbacks may only be registered before `flare::Start` is called.";

        auto &&registry = *get_staging_registry();
        registry[priority].emplace_back(std::move(init), std::move(fini));
    }

    void bootstrap_init(int argc, char **argv) {
        google::ParseCommandLineFlags(&argc, &argv, true);
        flare::detail::apply_flags_overrider();
        log_config_init(argv[0]);
    }

}  // namespace flare
