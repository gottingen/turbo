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
// Created by jeff on 24-5-30.
//
#include <turbo/times/time.h>
#include <iostream>

int main () {
    turbo::Time t = turbo::Time::current_time();
    std::cout << "current time is " << t << std::endl;
    std::cout<< "current nano time is " << turbo::Time::current_nanoseconds() << std::endl;
    std::cout<< "current micro time is " << turbo::Time::current_microseconds() << std::endl;
    std::cout<< "current milli time is " << turbo::Time::current_milliseconds() << std::endl;
    std::cout<< "current seconds time is " << turbo::Time::current_seconds() << std::endl;
    std::cout<< "current double micro time is " << turbo::Time::current_microseconds<double>() << std::endl;
    std::cout<< "current double milli time is " << turbo::Time::current_milliseconds<double>() << std::endl;
    std::cout<< "current double seconds time is " << turbo::Time::current_seconds<double>() << std::endl;
    return 0;
}