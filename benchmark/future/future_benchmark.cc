
/****************************************************************
 * Copyright (c) 2022, liyinbin
 * All rights reserved.
 * Author by liyinbin (jeff.li) lijippy@163.com
 *****************************************************************/


#include <benchmark/benchmark.h>
#include "flare/future/future.h"

// This is not the fairest of tests.
static void BM_std_future_reference(benchmark::State &state) {
    for (auto _ : state) {
        std::vector<std::promise<int>> proms(2000);
        std::vector<std::future<int>> futs;
        futs.reserve(proms.size());
        for (auto &p : proms) {
            futs.push_back(p.get_future());
        }

        std::thread worker([ps = std::move(proms)]() mutable {
            int i = 0;
            for (auto &p : ps) {
                p.set_value(++i);
            }
        });
        worker.detach();

        int total = 0;
        for (auto &f : futs) {
            total += f.get();
        }

        benchmark::DoNotOptimize(total);
    }
}

// This uses flare::future in a weird way, but that matches better with how std futures
// work.
static void BM_using_flare_future_fair(benchmark::State &state) {
    for (auto _ : state) {
        std::vector<flare::promise<int>> proms(2000);
        std::vector<flare::future<int>> futs;
        futs.reserve(proms.size());

        for (auto &p : proms) {
            futs.push_back(p.get_future());
        }

        std::thread worker([ps = std::move(proms)]() mutable {
            int i = 0;
            for (auto &p : ps) {
                p.set_value(++i);
            }
        });

        int total = 0;
        for (auto &f : futs) {
            f.finally([&total](flare::expected<int> v) { total += *v; });
        }
        worker.join();

        benchmark::DoNotOptimize(total);
    }
}

// This is more representative of how you would use flare::future to tackle that
// specific problem.
static void BM_using_flare_future_normal(benchmark::State &state) {
    for (auto _ : state) {
        std::vector<flare::promise<int>> proms(2000);

        int total = 0;
        for (auto &p : proms) {
            p.get_future().finally([&total](flare::expected<int> v) { total += *v; });
        }

        std::thread worker([ps = std::move(proms)]() mutable {
            int i = 0;
            for (auto &p : ps) {
                p.set_value(++i);
            }
        });

        worker.join();

        benchmark::DoNotOptimize(total);
    }
}
// Register the function as a benchmark
BENCHMARK(BM_std_future_reference);
BENCHMARK(BM_using_flare_future_normal);
BENCHMARK(BM_using_flare_future_fair);
// Run the benchmark
BENCHMARK_MAIN();