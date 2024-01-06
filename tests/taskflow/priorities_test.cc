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

TEST_CASE("SimplePriority.Sequential" * doctest::timeout(300)) {
  
  turbo::Executor executor(1);
  turbo::Taskflow taskflow;

  int counter = 0;

  auto [A, B, C, D, E] = taskflow.emplace(
    [&] () { counter = 0; },
    [&] () { REQUIRE(counter == 0); counter++; },
    [&] () { REQUIRE(counter == 2); counter++; },
    [&] () { REQUIRE(counter == 1); counter++; },
    [&] () { }
  );

  A.precede(B, C, D); 
  E.succeed(B, C, D);
  
  REQUIRE(B.priority() == turbo::TaskPriority::HIGH);
  REQUIRE(C.priority() == turbo::TaskPriority::HIGH);
  REQUIRE(D.priority() == turbo::TaskPriority::HIGH);

  B.priority(turbo::TaskPriority::HIGH);
  C.priority(turbo::TaskPriority::LOW);
  D.priority(turbo::TaskPriority::NORMAL);

  REQUIRE(B.priority() == turbo::TaskPriority::HIGH);
  REQUIRE(C.priority() == turbo::TaskPriority::LOW);
  REQUIRE(D.priority() == turbo::TaskPriority::NORMAL);

  executor.run_n(taskflow, 100).wait();
}

TEST_CASE("RandomPriority.Sequential" * doctest::timeout(300)) {
  
  turbo::Executor executor(1);
  turbo::Taskflow taskflow;

  const auto MAX_P = static_cast<unsigned>(turbo::TaskPriority::MAX);

  auto beg = taskflow.emplace([](){});
  auto end = taskflow.emplace([](){});

  size_t counters[MAX_P];
  size_t priorities[MAX_P];

  for(unsigned p=0; p<MAX_P; p++) {
    counters[p] = 0;
    priorities[p] = 0;
  }

  for(size_t i=0; i<10000; i++) {
    unsigned p = ::rand() % MAX_P;
    taskflow.emplace([p, &counters](){ counters[p]++; })
            .priority(static_cast<turbo::TaskPriority>(p))
            .succeed(beg)
            .precede(end);
    priorities[p]++;
  }

  executor.run(taskflow).wait();

  for(unsigned p=0; p<MAX_P; p++) {
    REQUIRE(priorities[p] == counters[p]);
  }

}

TEST_CASE("RandomPriority.Parallel" * doctest::timeout(300)) {
  
  turbo::Executor executor;
  turbo::Taskflow taskflow;

  const auto MAX_P = static_cast<unsigned>(turbo::TaskPriority::MAX);

  auto beg = taskflow.emplace([](){});
  auto end = taskflow.emplace([](){});

  std::atomic<size_t> counters[MAX_P];
  size_t priorities[MAX_P];

  for(unsigned p=0; p<MAX_P; p++) {
    counters[p] = 0;
    priorities[p] = 0;
  }

  for(size_t i=0; i<10000; i++) {
    unsigned p = ::rand() % MAX_P;
    taskflow.emplace([p, &counters](){ counters[p]++; })
            .priority(static_cast<turbo::TaskPriority>(p))
            .succeed(beg)
            .precede(end);
    priorities[p]++;
  }

  executor.run_n(taskflow, 2).wait();

  for(unsigned p=0; p<MAX_P; p++) {
    REQUIRE(counters[p]!=0);
    //std::cout << priorities[p] << ' ' << counters[p] << '\n';
  }

}





