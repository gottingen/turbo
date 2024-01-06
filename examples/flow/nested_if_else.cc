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
// This program demonstrates how to create nested if-else control flow
// using condition tasks.
#include "turbo/taskflow/taskflow.h"

int main() {

  turbo::Executor executor;
  turbo::Taskflow taskflow;

  int i;

  // create three condition tasks for nested control flow
  auto initi = taskflow.emplace([&](){ i=3; });
  auto cond1 = taskflow.emplace([&](){ return i>1 ? 1 : 0; });
  auto cond2 = taskflow.emplace([&](){ return i>2 ? 1 : 0; });
  auto cond3 = taskflow.emplace([&](){ return i>3 ? 1 : 0; });
  auto equl1 = taskflow.emplace([&](){ std::cout << "i=1\n"; });
  auto equl2 = taskflow.emplace([&](){ std::cout << "i=2\n"; });
  auto equl3 = taskflow.emplace([&](){ std::cout << "i=3\n"; });
  auto grtr3 = taskflow.emplace([&](){ std::cout << "i>3\n"; });

  initi.precede(cond1);
  cond1.precede(equl1, cond2);
  cond2.precede(equl2, cond3);
  cond3.precede(equl3, grtr3);

  // dump the conditioned flow
  taskflow.dump(std::cout);

  executor.run(taskflow).wait();

  return 0;
}

