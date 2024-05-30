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

#include <turbo/log/logging.h>

void call0() {
    int x = 3, y = 5;
    CHECK_EQ(2 * x, y) << "oops!";
}

void call1() {
    call0();
}

void call2() {
    call1();
}

void call3() {
    call2();
}

void call4() {
    call3();
}

void call5() {
    call4();
}

void call6() {
    call5();
}

int main() {

    //turbo::initialize_log();
    // default rotation size 100MB
    turbo::setup_rotating_file_sink("logs/rotating.txt");
    turbo::set_stderr_threshold(turbo::LogSeverityAtLeast::kInfinity);
    auto limit = 10 * 1024 * 1024;
    std::string msg = "hello world";
    for (int i = 0; i < limit; i++) {
        LOG(INFO) << "hello world";
        LOG(ERROR) << "error hello world";
    }
}