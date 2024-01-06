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
// A simple example with a semaphore constraint that only one task can
// execute at a time.

#include "turbo/taskflow/taskflow.h"

void sl() {
  std::this_thread::sleep_for(std::chrono::seconds(1));
}

int main() {

  turbo::Executor executor(4);
  turbo::Taskflow taskflow;

  // define a critical region of 1 worker
  turbo::Semaphore semaphore(1);

  // create give tasks in taskflow
  std::vector<turbo::Task> tasks {
    taskflow.emplace([](){ sl(); std::cout << "A" << std::endl; }),
    taskflow.emplace([](){ sl(); std::cout << "B" << std::endl; }),
    taskflow.emplace([](){ sl(); std::cout << "C" << std::endl; }),
    taskflow.emplace([](){ sl(); std::cout << "D" << std::endl; }),
    taskflow.emplace([](){ sl(); std::cout << "E" << std::endl; })
  };

  for(auto & task : tasks) {
    task.acquire(semaphore);
    task.release(semaphore);
  }

  executor.run(taskflow);
  executor.wait_for_all();

  return 0;
}

