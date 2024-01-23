// Copyright 2023 The Turbo Authors.
//
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


#include <pthread.h>
#include <cstddef>
#include <memory>
#include <list>
#include <iostream>
#include <sstream>
#include "turbo/times/stop_watcher.h"
#include <gtest/gtest.h>
#include "turbo/var/var.h"
#include "turbo/var/window.h"

class WindowTest : public testing::Test {
protected:
    void SetUp() {}
    void TearDown() {}
};

TEST_F(WindowTest, window) {
    const int window_size = 2;
    // test turbo::Adder
    turbo::Adder<int> adder;
    turbo::Window<turbo::Adder<int> > window_adder(&adder, window_size);
    turbo::PerSecond<turbo::Adder<int> > per_second_adder(&adder, window_size);
    turbo::WindowEx<turbo::Adder<int>, 2> window_ex_adder("window_ex_adder");
    turbo::PerSecondEx<turbo::Adder<int>, window_size> per_second_ex_adder("per_second_ex_adder");

    // test turbo::Maxer
    turbo::Maxer<int> maxer;
    turbo::Window<turbo::Maxer<int> > window_maxer(&maxer, window_size);
    turbo::WindowEx<turbo::Maxer<int>, window_size> window_ex_maxer;

    // test turbo::Miner
    turbo::Miner<int> miner;
    turbo::Window<turbo::Miner<int> > window_miner(&miner, window_size);
    turbo::WindowEx<turbo::Miner<int>, window_size> window_ex_miner;

    // test turbo::IntRecorder
    turbo::IntRecorder recorder;
    turbo::Window<turbo::IntRecorder> window_int_recorder(&recorder, window_size);
    turbo::WindowEx<turbo::IntRecorder, window_size> window_ex_int_recorder("window_ex_int_recorder");

    adder << 10;
    window_ex_adder << 10;
    per_second_ex_adder << 10;

    maxer << 10;
    window_ex_maxer << 10;
    miner << 10;
    window_ex_miner << 10;

    recorder << 10;
    window_ex_int_recorder << 10;

    sleep(1);
    adder << 2;
    window_ex_adder << 2;
    per_second_ex_adder << 2;

    maxer << 2;
    window_ex_maxer << 2;
    miner << 2;
    window_ex_miner << 2;

    recorder << 2;
    window_ex_int_recorder << 2;
    sleep(1);

    ASSERT_EQ(window_adder.get_value(), window_ex_adder.get_value())<<window_ex_adder.get_value();
    TURBO_RAW_LOG(INFO, "window_adder.get_value()=%d, window_ex_adder.get_value()=%d", window_adder.get_value(), window_ex_adder.get_value());
    ASSERT_EQ(per_second_adder.get_value(), per_second_ex_adder.get_value());

    ASSERT_EQ(window_maxer.get_value(), window_ex_maxer.get_value());
    ASSERT_EQ(window_miner.get_value(), window_ex_miner.get_value());

    turbo::Stat recorder_stat = window_int_recorder.get_value();
    turbo::Stat window_ex_recorder_stat = window_ex_int_recorder.get_value();
    ASSERT_EQ(recorder_stat.get_average_int(), window_ex_recorder_stat.get_average_int());
    ASSERT_DOUBLE_EQ(recorder_stat.get_average_double(), window_ex_recorder_stat.get_average_double());
}
