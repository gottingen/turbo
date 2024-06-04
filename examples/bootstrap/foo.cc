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

    struct Foo {
        Foo() {
            std::cout << "Foo::Foo" << std::endl;
        }
        ~Foo() {
            std::cout << "Foo::~Foo" << std::endl;
        }
    };

    Foo *g_foo = nullptr;
    struct FooTask : public turbo::BootTask {
        FooTask() = default;
        ~FooTask() override = default;

        void run_boot() override {
            std::cout << "Foo::run_boot" << std::endl;
            g_foo = new Foo();
        }

        void run_shutdown() override {
            std::cout << "Foo::run_shutdown" << std::endl;
            delete g_foo;
        }
    };

    struct FooRegistration {
        FooRegistration() {
            std::cout << "FooRegistration::FooRegistration" << std::endl;
            register_boot_task(std::make_unique<FooTask>(), DEFAULT_BOOT_TASK_PRIORITY);
        }
        ~FooRegistration() {
        }
    };

    TURBO_DLL static FooRegistration g_foo_registration __attribute__((used)) = FooRegistration();
}  // namespace turbo