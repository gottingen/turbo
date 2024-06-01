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
#include <iostream>

namespace turbo {

    struct Bar {
        Bar() {
            std::cout << "Bar::Bar" << std::endl;
        }
        ~Bar() {
            std::cout << "Bar::~Bar" << std::endl;
        }
    };

    Bar *g_bar = nullptr;
    struct BarTask : public turbo::BootTask {
        BarTask() = default;
        ~BarTask() override = default;

        void run_boot() override {
            std::cout << "Bar::run_boot" << std::endl;
            g_bar = new Bar();
        }

        void run_shutdown() override {
            std::cout << "Bar::run_shutdown" << std::endl;
            delete g_bar;
        }
    };

    struct BarRegistration {
        BarRegistration() {
            std::cout << "BarRegistration::BarRegistration" << std::endl;
            register_boot_task(std::make_unique<BarTask>(), DEFAULT_BOOT_TASK_PRIORITY);
        }
        ~BarRegistration() {
        }
    };

    BarRegistration g_bar_registration;
}  // namespace turbo