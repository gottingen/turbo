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
// This example demonstrates how to attach data to a task and run
// the task iteratively with changing data.

#include "turbo/taskflow/taskflow.h"

int main(){

  turbo::Executor executor;
  turbo::Taskflow taskflow("attach data to a task");

  int data;

  // create a task and attach it the data
  auto A = taskflow.placeholder();
  A.data(&data).work([A](){
    auto d = *static_cast<int*>(A.data());
    std::cout << "data is " << d << std::endl;
  });

  // run the taskflow iteratively with changing data
  for(data = 0; data<10; data++){
    executor.run(taskflow).wait();
  }

  return 0;
}
