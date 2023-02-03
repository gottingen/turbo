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

#include "gtest/gtest.h"

#include "turbo/workflow/workflow.h"

// EmptyFuture
TEST(Future, EmptyFuture) {
  turbo::Future<void> fu;
  EXPECT_TRUE(fu.valid() == false);
  EXPECT_TRUE(fu.cancel() == false);
}

// Future
TEST(Future,Future) {

  turbo::Workflow taskflow;
  turbo::Executor executor(4);

  std::atomic<int> counter{0};

  for(int i=0; i<100; i++) {
    taskflow.emplace([&](){
      counter.fetch_add(1, std::memory_order_relaxed);
    });
  }

  auto fu = executor.run(taskflow);

  fu.get();

  EXPECT_TRUE(counter == 100);
}

// Cancel
TEST(Future,Cancel) {

  turbo::Workflow taskflow;
  turbo::Executor executor(4);

  std::atomic<int> counter{0};

  // artificially long (possible larger than 300 seconds)
  for(int i=0; i<10000; i++) {
    taskflow.emplace([&](){
      std::this_thread::sleep_for(std::chrono::milliseconds(100));
      counter.fetch_add(1, std::memory_order_relaxed);
    });
  }

  // a new round
  counter = 0;
  auto fu = executor.run(taskflow);
  EXPECT_TRUE(fu.cancel() == true);
  fu.get();
  EXPECT_TRUE(counter < 10000);

  // a new round
  counter = 0;
  fu = executor.run_n(taskflow, 100);
  EXPECT_TRUE(fu.cancel() == true);
  fu.get();
  EXPECT_TRUE(counter < 10000);
}

// multiple cnacels
TEST(Future,MultipleCancels) {

  turbo::Workflow taskflow1, taskflow2, taskflow3, taskflow4;
  turbo::Executor executor(4);

  std::atomic<int> counter{0};

  // artificially long (possible larger than 300 seconds)
  for(int i=0; i<10000; i++) {
    taskflow1.emplace([&](){
      std::this_thread::sleep_for(std::chrono::milliseconds(100));
      counter.fetch_add(1, std::memory_order_relaxed);
    });
    taskflow2.emplace([&](){
      std::this_thread::sleep_for(std::chrono::milliseconds(100));
      counter.fetch_add(1, std::memory_order_relaxed);
    });
    taskflow3.emplace([&](){
      std::this_thread::sleep_for(std::chrono::milliseconds(100));
      counter.fetch_add(1, std::memory_order_relaxed);
    });
    taskflow4.emplace([&](){
      std::this_thread::sleep_for(std::chrono::milliseconds(100));
      counter.fetch_add(1, std::memory_order_relaxed);
    });
  }

  // a new round
  counter = 0;
  auto fu1 = executor.run(taskflow1);
  auto fu2 = executor.run(taskflow2);
  auto fu3 = executor.run(taskflow3);
  auto fu4 = executor.run(taskflow4);
  EXPECT_TRUE(fu1.cancel() == true);
  EXPECT_TRUE(fu2.cancel() == true);
  EXPECT_TRUE(fu3.cancel() == true);
  EXPECT_TRUE(fu4.cancel() == true);
  executor.wait_for_all();
  EXPECT_TRUE(counter < 10000);
  EXPECT_TRUE(fu1.wait_for(std::chrono::milliseconds(0)) == std::future_status::ready);
  EXPECT_TRUE(fu2.wait_for(std::chrono::milliseconds(0)) == std::future_status::ready);
  EXPECT_TRUE(fu3.wait_for(std::chrono::milliseconds(0)) == std::future_status::ready);
  EXPECT_TRUE(fu4.wait_for(std::chrono::milliseconds(0)) == std::future_status::ready);
}



// cancel subflow
TEST(Future,CancelSubflow) {

  turbo::Workflow taskflow;
  turbo::Executor executor(4);

  std::atomic<int> counter{0};

  // artificially long (possible larger than 300 seconds)
  for(int i=0; i<100; i++) {
    taskflow.emplace([&, i](turbo::Subflow& sf){
      for(int j=0; j<100; j++) {
        sf.emplace([&](){
          std::this_thread::sleep_for(std::chrono::milliseconds(100));
          counter.fetch_add(1, std::memory_order_relaxed);
        });
      }
      if(i % 2) {
        sf.join();
      }
      else {
        sf.detach();
      }
    });
  }

  // a new round
  counter = 0;
  auto fu = executor.run(taskflow);
  EXPECT_TRUE(fu.cancel() == true);
  fu.get();
  EXPECT_TRUE(counter < 10000);

  // a new round
  counter = 0;
  auto fu1 = executor.run(taskflow);
  auto fu2 = executor.run(taskflow);
  auto fu3 = executor.run(taskflow);
  EXPECT_TRUE(fu1.cancel() == true);
  EXPECT_TRUE(fu2.cancel() == true);
  EXPECT_TRUE(fu3.cancel() == true);
  fu1.get();
  fu2.get();
  fu3.get();
  EXPECT_TRUE(counter < 10000);
}

// cancel asynchronous tasks in subflow
TEST(Future,CancelSubflowAsyncTasks) {

  turbo::Workflow taskflow;
  turbo::Executor executor(4);

  std::atomic<int> counter{0};

  // artificially long (possible larger than 300 seconds)
  for(int i=0; i<100; i++) {
    taskflow.emplace([&](turbo::Subflow& sf){
      for(int j=0; j<100; j++) {
        auto a = sf.emplace([&](){
          std::this_thread::sleep_for(std::chrono::milliseconds(100));
          counter.fetch_add(1, std::memory_order_relaxed);
        });
        auto b = sf.emplace([&](){
          std::this_thread::sleep_for(std::chrono::milliseconds(100));
          counter.fetch_add(1, std::memory_order_relaxed);
        });
        a.precede(b);
        sf.async([&](){
          std::this_thread::sleep_for(std::chrono::milliseconds(100));
          counter.fetch_add(1, std::memory_order_relaxed);
        });
        sf.silent_async([&](){
          std::this_thread::sleep_for(std::chrono::milliseconds(100));
          counter.fetch_add(1, std::memory_order_relaxed);
        });
      }
    });
  }

  // a new round
  counter = 0;
  auto fu = executor.run(taskflow);
  EXPECT_TRUE(fu.cancel() == true);
  fu.get();
  EXPECT_TRUE(counter < 10000);
}

// cancel infinite loop
TEST(Future,CancelInfiniteLoop) {

  turbo::Workflow taskflow;
  turbo::Executor executor(4);

  for(int i=0; i<100; i++) {
    auto a = taskflow.emplace([](){});
    auto b = taskflow.emplace([](){ return 0; });
    a.precede(b);
    b.precede(b);
  }

  auto fu = executor.run(taskflow);
  EXPECT_TRUE(fu.cancel() == true);
  fu.get();
}

// cancel from another
TEST(Future,CancelFromAnother) {

  turbo::Workflow taskflow, another;
  turbo::Executor executor(4);

  // create a single inifnite loop
  auto a = taskflow.emplace([](){});
  auto b = taskflow.emplace([](){ return 0; });
  a.precede(b);
  b.precede(b);

  auto fu = executor.run(taskflow);

  EXPECT_TRUE(fu.wait_for(
    std::chrono::milliseconds(100)) == std::future_status::timeout
  );

  // create a task to cancel another flow
  another.emplace([&]() { EXPECT_TRUE(fu.cancel() == true); });

  executor.run(another).wait();
}

// cancel from async task
TEST(Future,CancelFromAsync) {

  turbo::Workflow taskflow;
  turbo::Executor executor(4);

  // create a single inifnite loop
  auto a = taskflow.emplace([](){});
  auto b = taskflow.emplace([&](){ return 0; });
  a.precede(b);
  b.precede(b);

  executor.async([&](){
    auto fu = executor.run_n(taskflow, 100);
    std::this_thread::sleep_for(std::chrono::milliseconds(100));
    EXPECT_TRUE(fu.cancel() == true);
  });

  executor.wait_for_all();
}

// cancel async tasks
TEST(Future,CancelAsync) {

  turbo::Executor executor(2);

  std::vector<turbo::Future<void>> futures;

  for(int i=0; i<10000; i++) {
    futures.push_back(executor.async([](){
      std::this_thread::sleep_for(std::chrono::milliseconds(100));
    }));
  }

  size_t n_success = 0, n_failure = 0;

  for(auto& fu : futures) {
    if(fu.cancel() == true) n_success++;
    else n_failure++;
  }

  executor.wait_for_all();

  EXPECT_TRUE(n_success > n_failure);

  for(auto& fu : futures) {
    EXPECT_TRUE(fu.valid());
    EXPECT_NO_THROW(fu.get());
  }
}

// cancel subflow async tasks
TEST(Future,CancelSubflowAsync) {

  turbo::Workflow taskflow;
  turbo::Executor executor(2);

  std::atomic<bool> futures_ready {false};
  std::vector<turbo::Future<void>> futures;

  taskflow.emplace([&](turbo::Subflow& sf){
    for(int i=0; i<10000; i++) {
      futures.push_back(sf.async([](){
        std::this_thread::sleep_for(std::chrono::milliseconds(100));
      }));
    }
    futures_ready = true;
  });

  executor.run(taskflow);

  while(!futures_ready);

  size_t n_success = 0, n_failure = 0;

  for(auto& fu : futures) {
    if(fu.cancel() == true) n_success++;
    else n_failure++;
  }

  executor.wait_for_all();
  EXPECT_TRUE(n_success > n_failure);

  for(auto& fu : futures) {
    EXPECT_TRUE(fu.valid());
    EXPECT_NO_THROW(fu.get());
  }
}

// cancel composition tasks
TEST(Future,CancelComposition) {

  turbo::Executor executor(4);

  // f1 has two independent tasks
  turbo::Workflow f1("F1");
  auto f1A = f1.emplace([&](){ });
  auto f1B = f1.emplace([&](){ });
  f1A.name("f1A");
  f1B.name("f1B");

  //  f2A ---
  //         |----> f2C
  //  f2B ---
  //
  //  f1_module_task
  turbo::Workflow f2("F2");
  auto f2A = f2.emplace([&](){ });
  auto f2B = f2.emplace([&](){ });
  auto f2C = f2.emplace([&](){ });
  f2A.name("f2A");
  f2B.name("f2B");
  f2C.name("f2C");

  f2A.precede(f2C);
  f2B.precede(f2C);
  f2.composed_of(f1).name("module_of_f1");

  // f3 has a module task (f2) and a regular task
  turbo::Workflow f3("F3");
  f3.composed_of(f2).name("module_of_f2");
  f3.emplace([](){ }).name("f3A");

  // f4: f3_module_task -> f2_module_task
  turbo::Workflow f4;
  f4.name("F4");
  auto f3_module_task = f4.composed_of(f3).name("module_of_f3");
  auto f2_module_task = f4.composed_of(f2).name("module_of_f2");
  f3_module_task.precede(f2_module_task);

  for(int r=0; r<100; r++) {

    size_t N = 100;
    size_t success = 0;

    std::vector<turbo::Future<void>> futures;

    for(int i=0; i<100; i++) {
      futures.emplace_back(executor.run(f4));
    }

    for(auto& fu: futures) {
      success += (fu.cancel() ? 1 : 0);
    }

    executor.wait_for_all();

    EXPECT_TRUE(success <= N);
  }
}

