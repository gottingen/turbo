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
// This example demonstrates how to use different methods to
// run a taskflow.
#include "turbo/taskflow/taskflow.h"

int main(){

  // create an executor and a taskflow
  turbo::Executor executor(1);
  turbo::Taskflow taskflow("Demo");

  auto A = taskflow.emplace([&](){ std::cout << "TaskA\n"; }).name("A");
  auto B = taskflow.emplace([&](turbo::Subflow& subflow){
    std::cout << "TaskB\n";
    auto B1 = subflow.emplace([&](){ std::cout << "TaskB1\n"; }).name("B1");
    auto B2 = subflow.emplace([&](){ std::cout << "TaskB2\n"; }).name("B2");
    auto B3 = subflow.emplace([&](){ std::cout << "TaskB3\n"; }).name("B3");
    B1.precede(B3);
    B2.precede(B3);
  }).name("B");

  auto C = taskflow.emplace([&](){ std::cout << "TaskC\n"; }).name("C");
  auto D = taskflow.emplace([&](){ std::cout << "TaskD\n"; }).name("D");

  A.precede(B, C);
  B.precede(D);
  C.precede(D);

  // dumpping a taskflow before execution won't visualize subflow tasks
  std::cout << "Dump the taskflow before execution:\n";
  taskflow.dump(std::cout);

  std::cout << "Run the taskflow once without callback\n" << std::endl;
  executor.run(taskflow).get();
  std::cout << std::endl;

  // after execution, we can visualize subflow tasks
  std::cout << "Dump the taskflow after execution:\n";
  taskflow.dump(std::cout);
  std::cout << std::endl;

  std::cout << "Use wait_for_all to wait for the execution to finish\n";
  executor.run(taskflow).get();
  executor.wait_for_all();
  std::cout << std::endl;

  std::cout << "Execute the taskflow two times without a callback\n";
  executor.run(taskflow).get();
  std::cout << "Dump after two executions:\n";
  taskflow.dump(std::cout);
  std::cout << std::endl;

  std::cout << "Execute the taskflow four times with a callback\n";
  executor.run_n(taskflow, 4, [] () { std::cout << "finishes 4 runs\n"; })
          .get();
  std::cout << std::endl;

  std::cout << "Run the taskflow until the predicate returns true\n";
  executor.run_until(taskflow, [counter=3]() mutable {
    std::cout << "Counter = " << counter << std::endl;
    return counter -- == 0;
  }).get();

  taskflow.dump(std::cout);

  return 0;
}
