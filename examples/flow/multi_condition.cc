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
// This program demonstrates how to use multi-condition task
// to jump to multiple successor tasks
//
// A ----> B
//   |
//   |---> C
//   |
//   |---> D
//
#include "turbo/taskflow/taskflow.h"

int main() {

  turbo::Executor executor;
  turbo::Taskflow taskflow("Multi-Conditional Tasking Demo");

  auto A = taskflow.emplace([&]() -> turbo::SmallVector<int> {
    std::cout << "A\n";
    return {0, 2};
  }).name("A");
  auto B = taskflow.emplace([&](){ std::cout << "B\n"; }).name("B");
  auto C = taskflow.emplace([&](){ std::cout << "C\n"; }).name("C");
  auto D = taskflow.emplace([&](){ std::cout << "D\n"; }).name("D");

  A.precede(B, C, D);

  // visualizes the taskflow
  taskflow.dump(std::cout);

  // executes the taskflow
  executor.run(taskflow).wait();

  return 0;
}

