// Copyright 2023 The Turbo Authors.
//
// Licensed under the Apache License, Version 2.0 (the "License);
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

#include "gtest/gtest.h"
#include "turbo/workflow/workflow.h"

// --------------------------------------------------------
// Testcase: Async
// --------------------------------------------------------

void async(unsigned W) {

  turbo::Executor executor(W);

  std::vector<turbo::Future<std::optional<int>>> fus;

  std::atomic<int> counter(0);

  int N = 100000;

  for(int i=0; i<N; ++i) {
    fus.emplace_back(executor.async([&](){
      counter.fetch_add(1, std::memory_order_relaxed);
      return -2;
    }));
  }

  executor.wait_for_all();

  EXPECT_TRUE(counter == N);

  int c = 0;
  for(auto& fu : fus) {
    c += fu.get().value();
  }

  EXPECT_TRUE(-c == 2*N);
}

TEST(Async, 1thread) {
  async(1);
}

TEST(Async, 2threads) {
  async(2);
}

TEST(Async, 4threads) {
  async(4);
}

TEST(Async, 8threads) {
  async(8);
}

TEST(Async, 16threads) {
  async(16);
}

// --------------------------------------------------------
// Testcase: NestedAsync
// --------------------------------------------------------

void nested_async(unsigned W) {

  turbo::Executor executor(W);

  std::vector<turbo::Future<std::optional<int>>> fus;

  std::atomic<int> counter(0);

  int N = 100000;

  for(int i=0; i<N; ++i) {
    fus.emplace_back(executor.async([&](){
      counter.fetch_add(1, std::memory_order_relaxed);
      executor.async([&](){
        counter.fetch_add(1, std::memory_order_relaxed);
        executor.async([&](){
          counter.fetch_add(1, std::memory_order_relaxed);
          executor.async([&](){
            counter.fetch_add(1, std::memory_order_relaxed);
          });
        });
      });
      return -2;
    }));
  }

  executor.wait_for_all();

  EXPECT_TRUE(counter == 4*N);

  int c = 0;
  for(auto& fu : fus) {
    c += fu.get().value();
  }

  EXPECT_TRUE(-c == 2*N);
}

TEST(NestedAsync, 1thread) {
  nested_async(1);
}

TEST(NestedAsync, 2threads) {
  nested_async(2);
}

TEST(NestedAsync, 4threads) {
  nested_async(4);
}

TEST(NestedAsync, 8threads) {
  nested_async(8);
}

TEST(NestedAsync, 16threads) {
  nested_async(16);
}

// --------------------------------------------------------
// Testcase: MixedAsync
// --------------------------------------------------------

void mixed_async(unsigned W) {

  turbo::Workflow taskflow;
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

  EXPECT_TRUE(counter == 4*N);

}

TEST(MixedAsync, 1thread) {
  mixed_async(1);
}

TEST(MixedAsync, 2threads) {
  mixed_async(2);
}

TEST(MixedAsync, 4threads) {
  mixed_async(4);
}

TEST(MixedAsync, 8threads) {
  mixed_async(8);
}

TEST(MixedAsync, 16threads) {
  mixed_async(16);
}

// --------------------------------------------------------
// Testcase: SubflowAsync
// --------------------------------------------------------

void subflow_async(size_t W) {

  turbo::Workflow taskflow;
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
    for(int i=0; i<100; i++) {
      sf.async([&](){ counter.fetch_add(1, std::memory_order_relaxed); });
    }
  });

  auto S2 = taskflow.emplace([&] (turbo::Subflow& sf){
    sf.emplace([&](){ counter.fetch_add(1, std::memory_order_relaxed); });
    for(int i=0; i<100; i++) {
      sf.async([&](){ counter.fetch_add(1, std::memory_order_relaxed); });
    }
  });

  taskflow.emplace([&] (turbo::Subflow& sf){
    sf.emplace([&](){ counter.fetch_add(1, std::memory_order_relaxed); });
    for(int i=0; i<100; i++) {
      sf.async([&](){ counter.fetch_add(1, std::memory_order_relaxed); });
    }
    sf.join();
  });

  taskflow.emplace([&] (turbo::Subflow& sf){
    for(int i=0; i<100; i++) {
      sf.async([&](){ counter.fetch_add(1, std::memory_order_relaxed); });
    }
    sf.join();
  });

  A.precede(S1, S2);
  B.succeed(S1, S2);

  executor.run(taskflow).wait();

  EXPECT_TRUE(counter == 405);
}

TEST(SubflowAsync, 1thread) {
  subflow_async(1);
}

TEST(SubflowAsync, 3threads) {
  subflow_async(3);
}

TEST(SubflowAsync, 11threads) {
  subflow_async(11);
}

// --------------------------------------------------------
// Testcase: NestedSubflowAsync
// --------------------------------------------------------

void nested_subflow_async(size_t W) {

  turbo::Workflow taskflow;
  turbo::Executor executor(W);

  std::atomic<int> counter{0};

  taskflow.emplace([&](turbo::Subflow& sf1){

    for(int i=0; i<100; i++) {
      sf1.async([&](){ counter.fetch_add(1, std::memory_order_relaxed); });
    }

    sf1.emplace([&](turbo::Subflow& sf2){
      for(int i=0; i<100; i++) {
        sf2.async([&](){ counter.fetch_add(1, std::memory_order_relaxed); });
        sf1.async([&](){ counter.fetch_add(1, std::memory_order_relaxed); });
      }

      sf2.emplace([&](turbo::Subflow& sf3){
        for(int i=0; i<100; i++) {
          sf3.silent_async([&](){ counter.fetch_add(1, std::memory_order_relaxed); });
          sf2.silent_async([&](){ counter.fetch_add(1, std::memory_order_relaxed); });
          sf1.silent_async([&](){ counter.fetch_add(1, std::memory_order_relaxed); });
        }
      });
    });

    sf1.join();
    EXPECT_TRUE(counter == 600);
  });

  executor.run(taskflow).wait();
  EXPECT_TRUE(counter == 600);
}

TEST(NestedSubflowAsync, 1thread) {
  nested_subflow_async(1);
}

TEST(NestedSubflowAsync, 3threads) {
  nested_subflow_async(3);
}

TEST(NestedSubflowAsync, 11threads) {
  nested_subflow_async(11);
}
