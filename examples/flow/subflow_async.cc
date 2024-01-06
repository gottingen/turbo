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
// The program demonstrates how to create asynchronous task
// from a running subflow.
#include "turbo/taskflow/taskflow.h"

int main() {

  turbo::Taskflow taskflow("Subflow Async");
  turbo::Executor executor;

  std::atomic<int> counter{0};

  taskflow.emplace([&](turbo::Subflow& sf){
    for(int i=0; i<10; i++) {
      // Here, we use "silent_async" instead of "async" because we do
      // not care the return value. The method "silent_async" gives us
      // less overhead compared to "async".
      // The 10 asynchronous tasks run concurrently.
      sf.silent_async([&](){
        std::cout << "async task from the subflow\n";
        counter.fetch_add(1, std::memory_order_relaxed);
      });
    }
    sf.join();
    std::cout << counter << " = 10\n";
  });

  executor.run(taskflow).wait();

  return 0;
}

