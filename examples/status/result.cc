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
// Created by jeff on 24-5-31.
//

#include <turbo/utility/status.h>

turbo::Result<int> not_ok() {
    return turbo::internal_error("not ok");
}

turbo::Result<int> ok() {
    return 1;
}

turbo::Status call_ok() {
    RESULT_ASSIGN_OR_RETURN(auto r, ok());
    std::cout << "this should be 1: " << r << std::endl;
    return turbo::OkStatus();
}

turbo::Status call_not_ok() {
    RESULT_ASSIGN_OR_RETURN(auto r, not_ok());
    (void)r;
    std::cout << "this should not be printed" << std::endl;
    return turbo::OkStatus();
}

turbo::Status call_not_ok1() {
    STATUS_RETURN_IF_ERROR(call_not_ok())<<" stream message call_not_ok1";
    std::cout << "this should not be printed" << std::endl;
    return turbo::OkStatus();
}

turbo::Status call_not_ok2() {
    STATUS_RETURN_IF_ERROR(call_not_ok1())<<" stream message call_not_ok2";
    std::cout << "this should not be printed" << std::endl;
    return turbo::OkStatus();
}

int main() {
    auto r = call_ok();
    std::cout << "this should be ok: " << r << std::endl;
    r = call_not_ok();
    std::cout << "this should be printed: " << r << std::endl;
    r = call_not_ok1();
    std::cout << "this should be printed: " << r << std::endl;
    r = call_not_ok2();
    std::cout << "this should be printed: " << r << std::endl;
    return 0;
}