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
// The program demonstrate how to capture an exception thrown
// from a running taskflow
#include "turbo/taskflow/taskflow.h"

int main(){

  turbo::Executor executor;
  turbo::Taskflow taskflow("exception");

  auto [A, B, C, D] = taskflow.emplace(
    []() { std::cout << "TaskA\n"; },
    []() { 
      std::cout << "TaskB\n";
      throw std::runtime_error("Exception on Task B");
    },
    []() { 
      std::cout << "TaskC\n"; 
      throw std::runtime_error("Exception on Task C");
    },
    []() { std::cout << "TaskD will not be printed due to exception\n"; }
  );

  A.precede(B, C);  // A runs before B and C
  D.succeed(B, C);  // D runs after  B and C

  try {
    executor.run(taskflow).get();
  }
  catch(const std::runtime_error& e) {
    // catched either TaskB's or TaskC's exception
    std::cout << e.what() << std::endl;
  }

  return 0;
}


