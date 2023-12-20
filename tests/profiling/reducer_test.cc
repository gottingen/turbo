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

#define DOCTEST_CONFIG_IMPLEMENT_WITH_MAIN

#include "turbo/testing/test.h"
#include "turbo/profiling/counter.h"
#include "turbo/profiling/average_gauge.h"
#include "turbo/profiling/miner_gauge.h"
#include "turbo/profiling/maxer_gauge.h"
#include "turbo/profiling/unique_gauge.h"
#include "turbo/profiling/histogram.h"
#include "turbo/profiling/prometheus_dumper.h"
#include "turbo/format/print.h"
#include "turbo/random/random.h"

TEST_CASE("reducer") {
    turbo::Counter<int> adder("counter");

    bool stop = false;
    auto thread_func = [&adder, &stop]() {
        while (!stop) {
            adder.add(1);
            turbo::sleep_for(turbo::milliseconds(20));
        }
    };

    auto t1 = std::thread(thread_func);
    auto t2 = std::thread(thread_func);

    turbo::sleep_for(turbo::seconds(1));
    size_t count = 0;
    turbo::println("counter: {}", adder.describe());
    while (count < 100) {
        turbo::sleep_for(turbo::milliseconds(50));
        turbo::println("{}", adder.dump_prometheus());
        count++;
    }
    turbo::println("{}", turbo::Variable::dump_prometheus_all());
    stop = true;
    t1.join();
    t2.join();
}

TEST_CASE("gauges") {

    static_assert(std::is_invocable_v<int()>);
    static_assert(not std::is_invocable_v<int(), int>);
    static_assert(std::is_invocable_r_v<int, int()>);
    static_assert(not std::is_invocable_r_v<int*, int()>);
    static_assert(std::is_invocable_r_v<void, void(int), int>);
    static_assert(not std::is_invocable_r_v<void, void(int), void>);

    turbo::AverageGauge<uint32_t> gauge("test");
    turbo::MaxerGauge<uint32_t> max_gauge("max_gauge");
    turbo::MinerGauge<uint32_t> min_gauge("min_gauge");

    bool stop = false;
    auto thread_func = [&gauge, &stop, &max_gauge, &min_gauge]() {
        while (!stop) {
            auto v= turbo::uniform<uint32_t>(0, 200);
            gauge<<v;
            max_gauge<<v;
            min_gauge<<v;
            turbo::sleep_for(turbo::milliseconds(20));
        }
    };

    auto t1 = std::thread(thread_func);
    auto t2 = std::thread(thread_func);

    turbo::sleep_for(turbo::seconds(1));
    size_t count = 0;
    while (count < 100) {
        turbo::sleep_for(turbo::milliseconds(50));
        turbo::println("{}{}{}", gauge.dump_prometheus(), max_gauge.dump_prometheus(), min_gauge.dump_prometheus());
        count++;
    }
    turbo::println("{}", turbo::Variable::dump_prometheus_all());
    stop = true;
    t1.join();
    t2.join();
}

TEST_CASE("funciotn") {
    turbo::UniqueGauge<std::function<int()>> gauge("funciton_test");
    gauge.set([]() { return turbo::uniform<uint32_t>(0, 200); });
    int i = 0;
    while (i < 10) {
        turbo::sleep_for(turbo::milliseconds(50));
        turbo::println("{}", gauge.dump_prometheus());
        i++;
    }
}


TEST_CASE("histogram") {
    std::array<uint32_t, 5> bins = {10, 20, 30, 40, 50};
    turbo::Histogram<uint32_t, 5> histogram("histogram");
    std::atomic<int> count(0);

    bool stop = false;
    histogram.set_boundaries(bins);
    auto func = [&histogram, &stop, &count]() {
        while (!stop) {
            auto s = histogram.scope_latency_double_milliseconds();
            turbo::sleep_for(turbo::milliseconds(turbo::uniform<uint32_t>(0, 49)));
            ++count;
        }
    };
    auto t1= std::thread(func);
    auto t2= std::thread(func);
    auto t3= std::thread(func);
    int i = 0;
    turbo::PrometheusDumper dumper;
    while (i < 1000) {
        turbo::sleep_for(turbo::milliseconds(50));
        turbo::println("{}\n{}", count.load(), dumper.dump(histogram.get_snapshot()));
        i++;
    }
    turbo::println("{}", turbo::Variable::dump_prometheus_all());
    stop = true;
    t1.join();
    t2.join();
    t3.join();
}