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
// This program demonstrates how to implement do-while control flow
// using condition tasks.
#include "turbo/taskflow/taskflow.h"

int main() {

  turbo::Executor executor;
  turbo::Taskflow taskflow;

  int i;

  auto [init, body, cond, done] = taskflow.emplace(
    [&](){ std::cout << "i=0\n"; i=0; },
    [&](){ std::cout << "i++ => i="; i++; },
    [&](){ std::cout << i << '\n'; return i<5 ? 0 : 1; },
    [&](){ std::cout << "done\n"; }
  );

  init.name("init");
  body.name("do i++");
  cond.name("while i<5");
  done.name("done");

  init.precede(body);
  body.precede(cond);
  cond.precede(body, done);

  //taskflow.dump(std::cout);

  executor.run(taskflow).wait();

  return 0;
}

