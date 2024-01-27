// Copyright 2023 The Elastic-AI Authors.
// part of Elastic AI Search
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//      https://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.
//
//
// Created by jeff on 24-1-27.
//
#define DOCTEST_CONFIG_IMPLEMENT_WITH_MAIN
#include "turbo/testing/test.h"

#include <iostream>
#include "turbo/times/stop_watcher.h"
#include "turbo/fiber/fiber.h"
#include "turbo/fiber/timer.h"

/*
TEST_CASE("FiberTest, abort") {
    turbo::FiberTimer timer;
    timer.run_at(turbo::time_now() + turbo::Duration::seconds(1), [](void *arg) {
        std::cout << "timer triggered" << std::endl;
    }, nullptr);
}*/


TEST_CASE("FiberTest, detach") {
    turbo::FiberTimer timer;
    timer.run_at(turbo::time_now() + turbo::Duration::seconds(1), [](void *arg) {
        std::cout << "timer triggered" << std::endl;
    }, nullptr);
    timer.detach();
}


TEST_CASE("FiberTest, cancel") {
    turbo::FiberTimer timer;
    timer.run_at(turbo::time_now() + turbo::Duration::seconds(1), [](void *arg) {
        std::cout << "timer triggered" << std::endl;
    }, nullptr);
    timer.cancel();
}

TEST_CASE("FiberTest, sleep") {
    turbo::Fiber::usleep(2000000);
    turbo::Fiber::usleep(2000000);
    turbo::Fiber::usleep(2000000);
}

TEST_CASE("FiberTest, sleep") {
    turbo::Fiber::usleep(2000000);
    turbo::Fiber::usleep(2000000);
    turbo::Fiber::usleep(2000000);
}
TEST_CASE("FiberTest, sleep") {
    turbo::Fiber::usleep(2000000);
    turbo::Fiber::usleep(2000000);
    turbo::Fiber::usleep(2000000);
}
