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
// This program demonstrates how to create if-else control flow
// using condition tasks.
#include "turbo/taskflow/taskflow.h"

int main() {

  turbo::Executor executor;
  turbo::Taskflow taskflow;

  // create three static tasks and one condition task
  auto [init, cond, yes, no] = taskflow.emplace(
    [] () { },
    [] () { return 0; },
    [] () { std::cout << "yes\n"; },
    [] () { std::cout << "no\n"; }
  );

  init.name("init");
  cond.name("cond");
  yes.name("yes");
  no.name("no");

  cond.succeed(init);

  // With this order, when cond returns 0, execution
  // moves on to yes. When cond returns 1, execution
  // moves on to no.
  cond.precede(yes, no);

  // dump the conditioned flow
  taskflow.dump(std::cout);

  executor.run(taskflow).wait();

  return 0;
}

