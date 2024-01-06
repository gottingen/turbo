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
// This example demonstrates how to use the corun
// method in the executor.
#include "turbo/taskflow/taskflow.h"

int main(){
  
  const size_t N = 100;
  const size_t T = 1000;
  
  // create an executor and a taskflow
  turbo::Executor executor(2);
  turbo::Taskflow taskflow;

  std::array<turbo::Taskflow, N> taskflows;

  std::atomic<size_t> counter{0};
  
  for(size_t n=0; n<N; n++) {
    for(size_t i=0; i<T; i++) {
      taskflows[n].emplace([&](){ counter++; });
    }
    taskflow.emplace([&executor, &tf=taskflows[n]](){
      executor.corun(tf);
      //executor.run(tf).wait();  <-- can result in deadlock
    });
  }

  executor.run(taskflow).wait();

  return 0;
}
