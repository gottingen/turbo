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
// The program demonstrates how to cancel a submitted taskflow
// graph and wait until the cancellation completes.

#include "turbo/taskflow/taskflow.h"

int main() {

  turbo::Executor executor;
  turbo::Taskflow taskflow("cancel");

  // We create a taskflow graph of 1000 tasks each of 1 second.
  // Ideally, the taskflow completes in 1000/P seconds, where P
  // is the number of workers.
  for(int i=0; i<1000; i++) {
    taskflow.emplace([](){
      std::this_thread::sleep_for(std::chrono::seconds(1));
    });
  }

  // submit the taskflow
  auto beg = std::chrono::steady_clock::now();
  turbo::Future fu = executor.run(taskflow);

  // submit a cancel request to cancel all 1000 tasks.
  fu.cancel();

  // wait until the cancellation finishes
  fu.get();
  auto end = std::chrono::steady_clock::now();

  // the duration should be much less than 1000 seconds
  std::cout << "taskflow completes in "
            << std::chrono::duration_cast<std::chrono::milliseconds>(end-beg).count()
            << " milliseconds\n";

  return 0;
}


