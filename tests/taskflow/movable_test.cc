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
#include <turbo/taskflow/taskflow.h>

// increments a counter only on destruction
struct CountOnDestruction {

  CountOnDestruction(const CountOnDestruction& rhs) : counter {rhs.counter} {
    rhs.counter = nullptr;
  }

  CountOnDestruction(CountOnDestruction&& rhs) : counter{rhs.counter} {
    rhs.counter = nullptr;
  }

  CountOnDestruction(std::atomic<int>& c) : counter {&c} {}

  ~CountOnDestruction() {
    if(counter) {
      //std::cout << "destroying\n";
      counter->fetch_add(1, std::memory_order_relaxed);
    }
  }

  mutable std::atomic<int>* counter {nullptr};
};

// ----------------------------------------------------------------------------
// test move constructor
// ----------------------------------------------------------------------------

TEST_CASE("moved_run") {

  int N = 10000;

  std::atomic<int> counter {0};

  turbo::Taskflow taskflow;

  auto make_taskflow = [&](){
    for(int i=0; i<N; i++) {
      taskflow.emplace([&, c=CountOnDestruction{counter}](){
        counter.fetch_add(1, std::memory_order_relaxed);
      });
    }
  };

  // run the moved taskflow
  make_taskflow();
  turbo::Executor().run_until(
    std::move(taskflow),
    [repeat=2]() mutable { return repeat-- == 0; },
    [](){}
  ).wait();

  REQUIRE(taskflow.num_tasks() == 0);
  REQUIRE(counter == 3*N);

  // run the original empty taskflow
  turbo::Executor().run(taskflow).wait();
  REQUIRE(counter == 3*N);

  // remake the taskflow and run it again
  make_taskflow();
  REQUIRE(taskflow.num_tasks() == N);
  turbo::Executor().run(taskflow).wait();
  REQUIRE(counter == 4*N);
  REQUIRE(taskflow.num_tasks() == N);

  // run the moved taskflow
  turbo::Executor().run(std::move(taskflow)).wait();
  REQUIRE(counter == 6*N);
  REQUIRE(taskflow.num_tasks() == 0);

  // run the moved empty taskflow
  turbo::Executor().run(std::move(taskflow)).wait();
  REQUIRE(counter == 6*N);
  REQUIRE(taskflow.num_tasks() == 0);

  // remake the taskflow and run it with moved ownership
  make_taskflow();
  REQUIRE(taskflow.num_tasks() == N);
  turbo::Executor().run_n(std::move(taskflow), 3).wait();
  REQUIRE(counter == 10*N);
  REQUIRE(taskflow.num_tasks() == 0);

  // run the moved empty taskflow with callable
  turbo::Executor().run(std::move(taskflow), [&](){
    counter.fetch_add(N, std::memory_order_relaxed);
  }).wait();
  REQUIRE(counter == 11*N);
  REQUIRE(taskflow.num_tasks() == 0);

  // remake the taskflow and run it with moved ownership
  make_taskflow();
  turbo::Executor().run(std::move(taskflow), [&](){
    counter.fetch_add(N, std::memory_order_relaxed);
  }).wait();
  REQUIRE(counter == 14*N);
  REQUIRE(taskflow.num_tasks() == 0);
}

// ----------------------------------------------------------------------------
// test move assignment operator
// ----------------------------------------------------------------------------

TEST_CASE("moved_taskflows") {

  int N = 10000;

  std::atomic<int> counter {0};

  auto make_taskflow = [&counter](turbo::Taskflow& taskflow, int N){
    for(int i=0; i<N; i++) {
      taskflow.emplace([&counter, c=CountOnDestruction{counter}](){
        counter.fetch_add(1, std::memory_order_relaxed);
      });
    }
  };

  {
    turbo::Taskflow taskflow1;
    turbo::Taskflow taskflow2;

    make_taskflow(taskflow1, N);
    make_taskflow(taskflow2, N/2);

    REQUIRE(taskflow1.num_tasks() == N);
    REQUIRE(taskflow2.num_tasks() == N/2);

    taskflow1 = std::move(taskflow2);

    REQUIRE(counter == N);
    REQUIRE(taskflow1.num_tasks() == N/2);
    REQUIRE(taskflow2.num_tasks() == 0);

    {
      turbo::Executor executor;
      executor.run(std::move(taskflow1));  // N/2
      executor.run(std::move(taskflow2));  // 0
      REQUIRE(taskflow1.num_tasks() == 0);
      REQUIRE(taskflow2.num_tasks() == 0);

      make_taskflow(taskflow1, N);
      make_taskflow(taskflow2, N);
      REQUIRE(taskflow1.num_tasks() == N);
      REQUIRE(taskflow2.num_tasks() == N);
      executor.wait_for_all();
    }
    REQUIRE(counter == 2*N);
  }

  // now both taskflow1 and taskflow2 die
  REQUIRE(counter == 4*N);

  // move constructor
  {
    turbo::Taskflow taskflow1;
    turbo::Taskflow taskflow2(std::move(taskflow1));

    REQUIRE(taskflow1.num_tasks() == 0);
    REQUIRE(taskflow2.num_tasks() == 0);

    make_taskflow(taskflow1, N);
    turbo::Taskflow taskflow3(std::move(taskflow1));

    REQUIRE(counter == 4*N);
    REQUIRE(taskflow1.num_tasks() == 0);
    REQUIRE(taskflow3.num_tasks() == N);

    taskflow3 = std::move(taskflow1);

    REQUIRE(counter == 5*N);
    REQUIRE(taskflow1.num_tasks() == 0);
    REQUIRE(taskflow2.num_tasks() == 0);
    REQUIRE(taskflow3.num_tasks() == 0);
  }

  REQUIRE(counter == 5*N);
}

// ----------------------------------------------------------------------------
// test multithreaded run
// ----------------------------------------------------------------------------

TEST_CASE("parallel_moved_runs") {

  int N = 10000;

  std::atomic<int> counter {0};

  auto make_taskflow = [&counter](turbo::Taskflow& taskflow, int N){
    for(int i=0; i<N; i++) {
      taskflow.emplace([&counter, c=CountOnDestruction{counter}](){
        counter.fetch_add(1, std::memory_order_relaxed);
      });
    }
  };

  {
    turbo::Executor executor;

    std::vector<std::thread> threads;
    for(int i=0; i<64; i++) {
      threads.emplace_back([&](){
        turbo::Taskflow taskflow;
        make_taskflow(taskflow, N);
        executor.run(std::move(taskflow));
      });
    }

    for(auto& thread : threads) thread.join();

    executor.wait_for_all();
  }

  REQUIRE(counter == 64*N*2);

  counter = 0;

  {
    turbo::Executor executor;

    std::vector<std::thread> threads;
    for(int i=0; i<32; i++) {
      threads.emplace_back([&](){
        turbo::Taskflow taskflow1;
        make_taskflow(taskflow1, N);
        turbo::Taskflow taskflow2(std::move(taskflow1));
        executor.run(std::move(taskflow1), [&](){ counter++; });
        executor.run(std::move(taskflow2), [&](){ counter++; });
        executor.run(std::move(taskflow1), [&](){ counter++; });
        executor.run(std::move(taskflow2), [&](){ counter++; });
      });
    }

    for(auto& thread : threads) thread.join();

    executor.wait_for_all();
  }

  REQUIRE(counter == 32*(N*2 + 4));
}




