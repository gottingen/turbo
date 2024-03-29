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

// --------------------------------------------------------
// Testcase: Async
// --------------------------------------------------------

void async(unsigned W) {

  turbo::Executor executor(W);

  std::vector<std::future<int>> fus;

  std::atomic<int> counter(0);

  int N = 100000;

  for(int i=0; i<N; ++i) {
    fus.emplace_back(executor.async([&](){
      counter.fetch_add(1, std::memory_order_relaxed);
      return -2;
    }));
  }

  executor.wait_for_all();

  REQUIRE(counter == N);

  int c = 0;
  for(auto& fu : fus) {
    c += fu.get();
  }

  REQUIRE(-c == 2*N);
}

TEST_CASE("Async.1thread" * doctest::timeout(300)) {
  async(1);
}

TEST_CASE("Async.2threads" * doctest::timeout(300)) {
  async(2);
}

TEST_CASE("Async.4threads" * doctest::timeout(300)) {
  async(4);
}

TEST_CASE("Async.8threads" * doctest::timeout(300)) {
  async(8);
}

TEST_CASE("Async.16threads" * doctest::timeout(300)) {
  async(16);
}

// --------------------------------------------------------
// Testcase: NestedAsync
// --------------------------------------------------------

void nested_async(unsigned W) {

  turbo::Executor executor(W);

  std::vector<std::future<int>> fus;

  std::atomic<int> counter(0);

  int N = 100000;

  for(int i=0; i<N; ++i) {
    fus.emplace_back(executor.async([&](){
      counter.fetch_add(1, std::memory_order_relaxed);
      executor.async([&](){
        counter.fetch_add(1, std::memory_order_relaxed);
        executor.async([&](){
          counter.fetch_add(1, std::memory_order_relaxed);
          executor.async("inner-most", [&](){
            counter.fetch_add(1, std::memory_order_relaxed);
          });
        });
      });
      return -2;
    }));
  }

  executor.wait_for_all();

  REQUIRE(counter == 4*N);

  int c = 0;
  for(auto& fu : fus) {
    c += fu.get();
  }

  REQUIRE(-c == 2*N);
}

TEST_CASE("NestedAsync.1thread" * doctest::timeout(300)) {
  nested_async(1);
}

TEST_CASE("NestedAsync.2threads" * doctest::timeout(300)) {
  nested_async(2);
}

TEST_CASE("NestedAsync.4threads" * doctest::timeout(300)) {
  nested_async(4);
}

TEST_CASE("NestedAsync.8threads" * doctest::timeout(300)) {
  nested_async(8);
}

TEST_CASE("NestedAsync.16threads" * doctest::timeout(300)) {
  nested_async(16);
}

// --------------------------------------------------------
// Testcase: MixedAsync
// --------------------------------------------------------

void mixed_async(unsigned W) {

  turbo::Taskflow taskflow;
  turbo::Executor executor(W);

  std::atomic<int> counter(0);

  int N = 1000;

  for(int i=0; i<N; i=i+1) {
    turbo::Task A, B, C, D;
    std::tie(A, B, C, D) = taskflow.emplace(
      [&] () {
        executor.async([&](){
          counter.fetch_add(1, std::memory_order_relaxed);
        });
      },
      [&] () {
        executor.async([&](){
          counter.fetch_add(1, std::memory_order_relaxed);
        });
      },
      [&] () {
        executor.silent_async([&](){
          counter.fetch_add(1, std::memory_order_relaxed);
        });
      },
      [&] () {
        executor.silent_async([&](){
          counter.fetch_add(1, std::memory_order_relaxed);
        });
      }
    );

    A.precede(B, C);
    D.succeed(B, C);
  }

  executor.run(taskflow);
  executor.wait_for_all();

  REQUIRE(counter == 4*N);

}

TEST_CASE("MixedAsync.1thread" * doctest::timeout(300)) {
  mixed_async(1);
}

TEST_CASE("MixedAsync.2threads" * doctest::timeout(300)) {
  mixed_async(2);
}

TEST_CASE("MixedAsync.4threads" * doctest::timeout(300)) {
  mixed_async(4);
}

TEST_CASE("MixedAsync.8threads" * doctest::timeout(300)) {
  mixed_async(8);
}

TEST_CASE("MixedAsync.16threads" * doctest::timeout(300)) {
  mixed_async(16);
}

// --------------------------------------------------------
// Testcase: SubflowAsync
// --------------------------------------------------------

void subflow_async(size_t W) {

  turbo::Taskflow taskflow;
  turbo::Executor executor(W);

  std::atomic<int> counter{0};

  auto A = taskflow.emplace(
    [&](){ counter.fetch_add(1, std::memory_order_relaxed); }
  );
  auto B = taskflow.emplace(
    [&](){ counter.fetch_add(1, std::memory_order_relaxed); }
  );

  taskflow.emplace(
    [&](){ counter.fetch_add(1, std::memory_order_relaxed); }
  );

  auto S1 = taskflow.emplace([&] (turbo::Subflow& sf){
    for(int i=0; i<1000; i++) {
      sf.async([&](){ counter.fetch_add(1, std::memory_order_relaxed); });
    }
  });

  auto S2 = taskflow.emplace([&] (turbo::Subflow& sf){
    sf.emplace([&](){ counter.fetch_add(1, std::memory_order_relaxed); });
    for(int i=0; i<1000; i++) {
      sf.async([&](){ counter.fetch_add(1, std::memory_order_relaxed); });
    }
  });

  taskflow.emplace([&] (turbo::Subflow& sf){
    sf.emplace([&](){ counter.fetch_add(1, std::memory_order_relaxed); });
    for(int i=0; i<1000; i++) {
      sf.async([&](){ counter.fetch_add(1, std::memory_order_relaxed); });
    }
    sf.join();
  });

  taskflow.emplace([&] (turbo::Subflow& sf){
    for(int i=0; i<1000; i++) {
      sf.async([&](){ counter.fetch_add(1, std::memory_order_relaxed); });
    }
    sf.join();
  });

  A.precede(S1, S2);
  B.succeed(S1, S2);

  executor.run(taskflow).wait();

  REQUIRE(counter == 4005);
}

TEST_CASE("SubflowAsync.1thread") {
  subflow_async(1);
}

TEST_CASE("SubflowAsync.3threads") {
  subflow_async(3);
}

TEST_CASE("SubflowAsync.11threads") {
  subflow_async(11);
}

// --------------------------------------------------------
// Testcase: NestedSubflowAsync
// --------------------------------------------------------

void nested_subflow_async(size_t W) {

  turbo::Taskflow taskflow;
  turbo::Executor executor(W);

  std::atomic<int> counter{0};

  taskflow.emplace([&](turbo::Subflow& sf1){

    for(int i=0; i<100; i++) {
      sf1.async([&](){ counter.fetch_add(1, std::memory_order_relaxed); });
    }

    sf1.emplace([&](turbo::Subflow& sf2){
      for(int i=0; i<100; i++) {
        sf2.async([&](){ counter.fetch_add(1, std::memory_order_relaxed); });
        sf1.async(
          "named", [&](){ counter.fetch_add(1, std::memory_order_relaxed); }
        );
      }

      sf2.emplace([&](turbo::Subflow& sf3){
        for(int i=0; i<100; i++) {
          sf3.silent_async(
            "named", [&](){ counter.fetch_add(1, std::memory_order_relaxed); }
          );
          sf2.silent_async([&](){ counter.fetch_add(1, std::memory_order_relaxed); });
          sf1.silent_async([&](){ counter.fetch_add(1, std::memory_order_relaxed); });
        }
      });
    });

    sf1.join();
    REQUIRE(counter == 600);
  });

  executor.run(taskflow).wait();
  REQUIRE(counter == 600);
}

TEST_CASE("NestedSubflowAsync.1thread") {
  nested_subflow_async(1);
}

TEST_CASE("NestedSubflowAsync.3threads") {
  nested_subflow_async(3);
}

TEST_CASE("NestedSubflowAsync.11threads") {
  nested_subflow_async(11);
}

// --------------------------------------------------------
// Testcase: RuntimeAsync
// --------------------------------------------------------

void runtime_async(size_t W) {

  turbo::Taskflow taskflow;
  turbo::Executor executor(W);

  std::atomic<int> counter{0};

  auto A = taskflow.emplace(
    [&](){ counter.fetch_add(1, std::memory_order_relaxed); }
  );
  auto B = taskflow.emplace(
    [&](){ counter.fetch_add(1, std::memory_order_relaxed); }
  );

  taskflow.emplace(
    [&](){ counter.fetch_add(1, std::memory_order_relaxed); }
  );

  auto S1 = taskflow.emplace([&] (turbo::Runtime& sf){
    for(int i=0; i<1000; i++) {
      sf.silent_async(
        "named_silent_async", [&](){counter.fetch_add(1, std::memory_order_relaxed);}
      );
    }
    sf.corun_all();
  });

  auto S2 = taskflow.emplace([&] (turbo::Runtime& sf){
    sf.silent_async([&](){ counter.fetch_add(1, std::memory_order_relaxed); });
    for(int i=0; i<1000; i++) {
      sf.silent_async([&](){ counter.fetch_add(1, std::memory_order_relaxed); });
    }
    sf.corun_all();
  });

  taskflow.emplace([&] (turbo::Runtime& sf){
    sf.silent_async([&](){ counter.fetch_add(1, std::memory_order_relaxed); });
    for(int i=0; i<1000; i++) {
      sf.async(
        "named_async", [&](){ counter.fetch_add(1, std::memory_order_relaxed); }
      );
    }
    sf.corun_all();
  });

  taskflow.emplace([&] (turbo::Runtime& sf){
    for(int i=0; i<1000; i++) {
      sf.async([&](){ counter.fetch_add(1, std::memory_order_relaxed); });
    }
    sf.corun_all();
  });

  A.precede(S1, S2);
  B.succeed(S1, S2);

  executor.run(taskflow).wait();

  REQUIRE(counter == 4005);
}

TEST_CASE("RuntimeAsync.1thread") {
  runtime_async(1);
}

TEST_CASE("RuntimeAsync.3threads") {
  runtime_async(3);
}

TEST_CASE("RuntimeAsync.11threads") {
  runtime_async(11);
}
