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
// Testcase: CriticalSection
// --------------------------------------------------------

void critical_section(size_t W) {

  turbo::Workflow taskflow;
  turbo::Executor executor(W);
  turbo::CriticalSection section(1);

  int N = 1000;
  int counter = 0;

  for(int i=0; i<N; ++i) {
    turbo::Task task = taskflow.emplace([&](){ counter++; })
                            .name(std::to_string(i));
    section.add(task);
  }

  executor.run(taskflow).wait();

  EXPECT_TRUE(counter == N);

  executor.run(taskflow);
  executor.run(taskflow);
  executor.run(taskflow);

  executor.wait_for_all();

  EXPECT_TRUE(counter == 4*N);
  EXPECT_TRUE(section.count() == 1);
}

TEST(CriticalSection, 1thread) {
  critical_section(1);
}

TEST(CriticalSection, 2threads) {
  critical_section(2);
}

TEST(CriticalSection, 3threads) {
  critical_section(3);
}

TEST(CriticalSection, 7threads) {
  critical_section(7);
}

TEST(CriticalSection, 11threads) {
  critical_section(11);
}

TEST(CriticalSection, 16threads) {
  critical_section(16);
}

// --------------------------------------------------------
// Testcase: Semaphore
// --------------------------------------------------------

void semaphore(size_t W) {

  turbo::Executor executor(W);
  turbo::Workflow taskflow;
  turbo::Semaphore semaphore(1);

  int N = 1000;
  int counter = 0;

  for(int i=0; i<N; i++) {
    auto f = taskflow.emplace([&](){ counter++; });
    auto t = taskflow.emplace([&](){ counter++; });
    f.precede(t);
    f.acquire(semaphore);
    t.release(semaphore);
  }

  executor.run(taskflow).wait();

  EXPECT_TRUE(counter == 2*N);

}

TEST(Semaphore, 1thread) {
  semaphore(1);
}

TEST(Semaphore, 2threads) {
  semaphore(2);
}

TEST(Semaphore, 4threads) {
  semaphore(4);
}

TEST(Semaphore, 8threads) {
  semaphore(8);
}

// --------------------------------------------------------
// Testcase: OverlappedSemaphore
// --------------------------------------------------------

void overlapped_semaphore(size_t W) {

  turbo::Executor executor(W);
  turbo::Workflow taskflow;
  turbo::Semaphore semaphore1(1);
  turbo::Semaphore semaphore4(4);

  int N = 1000;
  int counter = 0;

  for(int i=0; i<N; i++) {
    auto task = taskflow.emplace([&](){ counter++; });
    task.acquire(semaphore1);
    task.acquire(semaphore4);
    task.release(semaphore1);
    task.release(semaphore4);
  }

  executor.run(taskflow).wait();

  EXPECT_TRUE(counter == N);
  EXPECT_TRUE(semaphore1.count() == 1);
  EXPECT_TRUE(semaphore4.count() == 4);
}

TEST(OverlappedSemaphore, 1thread) {
  overlapped_semaphore(1);
}

TEST(OverlappedSemaphore, 2threads) {
  overlapped_semaphore(2);
}

TEST(OverlappedSemaphore, 4threads) {
  overlapped_semaphore(4);
}

TEST(OverlappedSemaphore, 8threads) {
  overlapped_semaphore(8);
}

// --------------------------------------------------------
// Testcase: Conflict Graph
// --------------------------------------------------------

void conflict_graph(size_t W) {

  turbo::Executor executor(W);
  turbo::Workflow taskflow;
  turbo::Semaphore conflict_AB(1);
  turbo::Semaphore conflict_AC(1);

  int counter {0};
  std::mutex mutex;

  turbo::Task A = taskflow.emplace([&](){ counter++; });

  // B and C can run together
  turbo::Task B = taskflow.emplace([&](){
    std::lock_guard<std::mutex> lock(mutex);
    counter++;
  });
  turbo::Task C = taskflow.emplace([&](){
    std::lock_guard<std::mutex> lock(mutex);
    counter++;
  });

  // describe the conflict between A and B
  A.acquire(conflict_AB).release(conflict_AB);
  B.acquire(conflict_AB).release(conflict_AB);

  // describe the conflict between A and C
  A.acquire(conflict_AC).release(conflict_AC);
  C.acquire(conflict_AC).release(conflict_AC);

  executor.run(taskflow).wait();

  EXPECT_TRUE(counter == 3);

  for(size_t i=0; i<10; i++) {
    executor.run_n(taskflow, 10);
  }
  executor.wait_for_all();

  EXPECT_TRUE(counter == 303);
}

TEST(ConflictGraph, 1thread) {
  conflict_graph(1);
}

TEST(ConflictGraph, 2threads) {
  conflict_graph(2);
}

TEST(ConflictGraph, 3threads) {
  conflict_graph(3);
}

TEST(ConflictGraph, 4threads) {
  conflict_graph(4);
}
