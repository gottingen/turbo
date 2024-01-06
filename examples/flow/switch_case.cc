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
// This program demonstrates how to implement switch-case control flow
// using condition tasks.
#include "turbo/taskflow/taskflow.h"

int main() {

  turbo::Executor executor;
  turbo::Taskflow taskflow;

  auto [source, swcond, case1, case2, case3, target] = taskflow.emplace(
    [](){ std::cout << "source\n"; },
    [](){ std::cout << "switch\n"; return rand()%3; },
    [](){ std::cout << "case 1\n"; return 0; },
    [](){ std::cout << "case 2\n"; return 0; },
    [](){ std::cout << "case 3\n"; return 0; },
    [](){ std::cout << "target\n"; }
  );

  source.precede(swcond);
  swcond.precede(case1, case2, case3);
  target.succeed(case1, case2, case3);

  executor.run(taskflow).wait();

  return 0;
}
