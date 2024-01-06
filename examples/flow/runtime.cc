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
// This program demonstrates how to use a runtime task to forcefully
// schedule an active task that would never be scheduled.

#include "turbo/taskflow/taskflow.h"

int main() {

  turbo::Taskflow taskflow("Runtime Tasking");
  turbo::Executor executor;

  turbo::Task A, B, C, D;

  std::tie(A, B, C, D) = taskflow.emplace(
    [] () { return 0; },
    [&C] (turbo::Runtime& rt) {  // C must be captured by reference
      std::cout << "B\n";
      rt.schedule(C);
    },
    [] () { std::cout << "C\n"; },
    [] () { std::cout << "D\n"; }
  );

  // name tasks
  A.name("A");
  B.name("B");
  C.name("C");
  D.name("D");

  // create conditional dependencies
  A.precede(B, C, D);

  // dump the graph structure
  taskflow.dump(std::cout);

  // we will see both B and C in the output
  executor.run(taskflow).wait();

  return 0;
}
