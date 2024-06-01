//
// Copyright (C) 2024 EA group inc.
// Author: Jeff.li lijippy@163.com
// All rights reserved.
// This program is free software: you can redistribute it and/or modify
// it under the terms of the GNU Affero General Public License as published
// by the Free Software Foundation, either version 3 of the License, or
// (at your option) any later version.
//
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU Affero General Public License for more details.
//
// You should have received a copy of the GNU Affero General Public License
// along with this program.  If not, see <https://www.gnu.org/licenses/>.
//
//
// Created by jeff on 24-6-1.
//

#pragma once

#include <memory>
#include <type_traits>
#include <functional>
#include <turbo/base/macros.h>
#include <turbo/base/internal/raw_logging.h>

namespace turbo {

    static constexpr uint32_t BOOT_TASK_PRIORITY_SLOTS = 8;
    // there are 8 slots priority from 0 to 6
    // the priority 7 is reserved for the exit task
    static constexpr uint32_t MAX_BOOT_TASK_PRIORITY = 6;

    // the default priority is 3
    // if you want to run the task before the default task, you can set the priority to 4-6
    // if you want to run the task lower priority, you can set the priority to 0-2
    static constexpr uint32_t DEFAULT_BOOT_TASK_PRIORITY = 3;

    struct BootTask {
        virtual ~BootTask() = default;

        virtual void run_boot() = 0;

        virtual void run_shutdown() = 0;
    };

    // Register a boot task to be run at startup or shutdown.
    // A task id contains two parts: the priority and the index.
    // The higher the priority, the earlier the task is run.
    typedef uint64_t BootTaskId;

    static constexpr BootTaskId INVALID_BOOT_TASK_ID = 0;

    constexpr BootTaskId make_boot_task_id(uint32_t priority, uint32_t index) {
        return (static_cast<uint64_t>(priority) << 32) | index;
    }

    constexpr uint32_t get_boot_task_priority(BootTaskId id) {
        return id >> 32;
    }

    constexpr uint32_t get_boot_task_index(BootTaskId id) {
        return id & 0xFFFFFFFF;
    }

    constexpr bool is_valid_boot_task_id(BootTaskId id) {
        return id != INVALID_BOOT_TASK_ID;
    }

    // Register a boot task to be run at startup or shutdown.
    // if the priority is greater than MAX_BOOT_TASK_PRIORITY, the task will be registered with MAX_BOOT_TASK_PRIORITY.
    BootTaskId register_boot_task(std::unique_ptr<BootTask> task, uint32_t priority = DEFAULT_BOOT_TASK_PRIORITY);

    TURBO_DLL void cancel_boot_task(BootTaskId id);

    TURBO_DLL bool exists_boot_task(BootTaskId id);

    TURBO_DLL uint32_t get_boot_priority_task_count(uint32_t priority = DEFAULT_BOOT_TASK_PRIORITY);

    TURBO_DLL uint32_t get_boot_task_count();

    TURBO_DLL uint32_t bootstrap_initialize();

    TURBO_DLL void bootstrap_finalize();

    struct DefaultBootTask : public BootTask {
        DefaultBootTask(std::function<void()> boot_func, std::function<void()> shutdown_func)
                : boot_func(std::move(boot_func)), shutdown_func(std::move(shutdown_func)) {}

        void run_boot() override {
            if (boot_func) {
                boot_func();
            }

        }

        void run_shutdown() override {
            if (shutdown_func) {
                shutdown_func();
            }
        }

    private:
        std::function<void()> boot_func;
        std::function<void()> shutdown_func;
    };

    // Register a boot task to be run at startup or shutdown.
    // if the priority is greater than MAX_BOOT_TASK_PRIORITY, the task will be registered with MAX_BOOT_TASK_PRIORITY.
    TURBO_DLL inline BootTaskId register_boot_task(std::function<void()> boot_func, std::function<void()> shutdown_func, uint32_t priority = DEFAULT_BOOT_TASK_PRIORITY) {
        return register_boot_task(std::make_unique<DefaultBootTask>(std::move(boot_func), std::move(shutdown_func)), priority);
    }

    // Register a boot task to be run at startup or shutdown.
    // if the priority is greater than MAX_BOOT_TASK_PRIORITY, the task will be registered with MAX_BOOT_TASK_PRIORITY.
    TURBO_DLL inline BootTaskId register_boot_task(std::function<void()> boot_func, uint32_t priority = DEFAULT_BOOT_TASK_PRIORITY) {
        return register_boot_task(std::make_unique<DefaultBootTask>(std::move(boot_func), nullptr), priority);
    }
    // exit task call always at the end of the program
    // so when program exit, the exit task will be called at first and then call the task registered with the priority
    BootTaskId register_exit_task(std::function<void()> shutdown_func);

    uint32_t get_exit_task_count();

    void cancel_exit_task(BootTaskId id);

}  // namespace turbo
