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
// Created by jeff on 24-1-18.
//
#define DOCTEST_CONFIG_IMPLEMENT_WITH_MAIN
#include "turbo/testing/test.h"
#include "turbo/times/stop_watcher.h"
#include "turbo/event/event_dispatcher.h"

TEST_CASE("event_dispatcher wakeup") {
    turbo::EventDispatcher dispatcher;
    turbo::FiberAttribute attr;
    turbo::Status status = dispatcher.start(&attr);
    CHECK(status.ok());
    CHECK(dispatcher.running());
    turbo::StopWatcher watcher;
    watcher.reset();
    dispatcher.stop();
    dispatcher.join();
    CHECK_LT(watcher.elapsed_mill(),10);
    CHECK(!dispatcher.running());
}

TEST_CASE("event_dispatcher wakeup") {
    turbo::EventDispatcher dispatcher;
    turbo::FiberAttribute attr;
    turbo::Status status = dispatcher.start(&attr);
    CHECK(status.ok());
    CHECK(dispatcher.running());
    turbo::sleep_for(turbo::Duration::milliseconds(1500));
    CHECK_GE(dispatcher.num_iterators(), 1);
    turbo::StopWatcher watcher;
    watcher.reset();
    dispatcher.stop();
    dispatcher.join();
    CHECK_LT(watcher.elapsed_mill(),10);
    CHECK(!dispatcher.running());
}