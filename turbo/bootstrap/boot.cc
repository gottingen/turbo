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
#include <turbo/bootstrap/boot.h>
#include <turbo/base/call_once.h>
#include <turbo/synchronization/mutex.h>
#include <vector>
#include <iostream>
#include <memory>

namespace turbo {

    struct BootTaskRegistration {
        BootTaskRegistration()
        : boot_tasks(BOOT_TASK_PRIORITY_SLOTS),
          boot_tasks_mutex(){
            for (uint32_t i = 0; i < BOOT_TASK_PRIORITY_SLOTS; i++) {
                boot_tasks[i].reserve(INITIAL_BOOT_TASK_CAPACITY_PER_PRIORITY);
            }
            boot_tasks[get_boot_task_priority(INVALID_BOOT_TASK_ID)].push_back(nullptr);
        }

        static constexpr uint32_t INITIAL_BOOT_TASK_CAPACITY_PER_PRIORITY = 16;
        static constexpr uint32_t ONLY_EXIT_TASK_SLOT = 7;

        std::vector<std::vector<std::shared_ptr<BootTask>>> boot_tasks;
        turbo::Mutex boot_tasks_mutex;

        bool is_initialized{false};
        bool is_finalized{false};
        static BootTaskRegistration& get_instance() {
            static BootTaskRegistration instance;
            return instance;
        }
    };

    BootTaskId register_boot_task(std::unique_ptr<BootTask> task, uint32_t priority) {
        if (priority > MAX_BOOT_TASK_PRIORITY) {
            return INVALID_BOOT_TASK_ID;
        }
        auto &instance = BootTaskRegistration::get_instance();
        turbo::MutexLock lock(&instance.boot_tasks_mutex);
        TURBO_RAW_CHECK(!instance.is_initialized, "register_boot_task should be called before bootstrap_initialize"
                                         "if you want to register a task exit callback, you should use register_exit_task instead");
        auto id = make_boot_task_id(priority, instance.boot_tasks[priority].size());
        instance.boot_tasks[priority].push_back(std::move(task));
        return id;
    }

    void cancel_boot_task(BootTaskId id) {
        if (!is_valid_boot_task_id(id)) {
            return;
        }
        auto priority = get_boot_task_priority(id);
        auto index = get_boot_task_index(id);
        if (priority > MAX_BOOT_TASK_PRIORITY) {
            return;
        }
        auto &instance = BootTaskRegistration::get_instance();
        turbo::MutexLock lock(&instance.boot_tasks_mutex);
        TURBO_RAW_CHECK(instance.is_initialized, "cancel_boot_task should be called after bootstrap_initialize"
                                        "this time, assume your program is still running before the  bootstrap_initialize");
        if (index >= instance.boot_tasks[priority].size()) {
            return;
        }
        instance.boot_tasks[priority][index].reset();
    }

    bool exists_boot_task(BootTaskId id) {
        if (!is_valid_boot_task_id(id)) {
            return false;
        }
        auto priority = get_boot_task_priority(id);
        auto index = get_boot_task_index(id);
        if (priority > MAX_BOOT_TASK_PRIORITY) {
            return false;
        }
        auto &instance = BootTaskRegistration::get_instance();
        turbo::MutexLock lock(&instance.boot_tasks_mutex);
        if (index >= instance.boot_tasks[priority].size()) {
            return false;
        }
        return instance.boot_tasks[priority][index] != nullptr;
    }

    uint32_t get_boot_priority_task_count(uint32_t priority) {
        if (priority > MAX_BOOT_TASK_PRIORITY) {
            return 0;
        }
        auto &instance = BootTaskRegistration::get_instance();
        turbo::MutexLock lock(&instance.boot_tasks_mutex);
        return instance.boot_tasks[priority].size();
    }

    uint32_t get_boot_task_count() {
        auto &instance = BootTaskRegistration::get_instance();
        turbo::MutexLock lock(&instance.boot_tasks_mutex);
        uint32_t count = 0;
        for (uint32_t i = 0; i <= MAX_BOOT_TASK_PRIORITY; i++) {
            count += instance.boot_tasks[i].size();
        }
        return count;
    }

    uint32_t bootstrap_initialize() {
        auto &instance = BootTaskRegistration::get_instance();
        turbo::MutexLock lock(&instance.boot_tasks_mutex);
        if (instance.is_initialized) {
            std::cerr << "bootstrap_initialize has been called before" << std::endl;
            std::cerr << "bootstrap_initialize should only be called once" << std::endl;
            std::cerr << "do nothing and return" << std::endl;
            return 0;
        }
        uint32_t count = 0;
        for (uint32_t i = 0; i <= MAX_BOOT_TASK_PRIORITY; i++) {
            auto &tasks = instance.boot_tasks[MAX_BOOT_TASK_PRIORITY - i];
            for (auto &task: tasks) {
                if (task) {
                    task->run_boot();
                    count++;
                }
            }
        }
        instance.is_initialized = true;
        return count;
    }

    void bootstrap_finalize() {
        auto &instance = BootTaskRegistration::get_instance();
        turbo::MutexLock lock(&instance.boot_tasks_mutex);

        TURBO_RAW_CHECK(!instance.is_finalized, "bootstrap_finalize has been called before"
                                        "bootstrap_finalize should only be called once");
        if (instance.is_initialized) {
            std::cerr << "bootstrap_initialize has been called before, call bootstrap_finalize" << std::endl;
            for (uint32_t i = 0; i <= MAX_BOOT_TASK_PRIORITY; i++) {
                auto &tasks = instance.boot_tasks[i];
                auto it = tasks.rbegin();
                while (it != tasks.rend()) {
                    if (*it) {
                        (*it)->run_shutdown();
                    }
                    it++;
                }
            }
        } else {
            std::cerr << "bootstrap_initialize has not been called before, do nothing about it" << std::endl;
        }
        auto &tasks = instance.boot_tasks[BootTaskRegistration::ONLY_EXIT_TASK_SLOT];
        auto it = tasks.rbegin();
        while (it != tasks.rend()) {
            if (*it) {
                (*it)->run_shutdown();
            }
            it++;
        }
        instance.is_finalized = true;
    }

    BootTaskId register_exit_task(std::function<void()> shutdown_func) {
        return register_boot_task(nullptr, std::move(shutdown_func), BootTaskRegistration::ONLY_EXIT_TASK_SLOT);
    }

    uint32_t get_exit_task_count() {
        return get_boot_priority_task_count(BootTaskRegistration::ONLY_EXIT_TASK_SLOT);
    }

    void cancel_exit_task(BootTaskId id) {
        if (!is_valid_boot_task_id(id)) {
            return;
        }
        auto priority = get_boot_task_priority(id);
        auto index = get_boot_task_index(id);
        TURBO_RAW_CHECK(priority != BootTaskRegistration::ONLY_EXIT_TASK_SLOT, "cancel_exit_task should be called with a exit task id");
        auto &instance = BootTaskRegistration::get_instance();
        turbo::MutexLock lock(&instance.boot_tasks_mutex);
        TURBO_RAW_CHECK(index < instance.boot_tasks[priority].size(), "cancel_exit_task should be called with a valid exit task id");
        instance.boot_tasks[priority][index].reset();
    }

}  // namespace turbo