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

#include "turbo/taskflow/taskflow.h"

int main() {

  turbo::Executor executor;

  // create asynchronous tasks from the executor
  // (using executor as a thread pool)
  std::future<int> fu = executor.async([](){
    std::cout << "async task 1 returns 1\n";
    return 1;
  });

  executor.silent_async([](){  // silent async task doesn't return
    std::cout << "async task 2 does not return (silent)\n";
  });

  // create asynchronous tasks with names (for profiling)
  executor.async("async_task", [](){
    std::cout << "named async task returns 1\n";
    return 1;
  });

  executor.silent_async("silent_async_task", [](){
    std::cout << "named silent async task does not return\n";
  });

  executor.wait_for_all();  // wait for the two async tasks to finish

  // create asynchronous tasks from a subflow
  // all asynchronous tasks are guaranteed to finish when the subflow joins
  turbo::Taskflow taskflow;

  std::atomic<int> counter {0};

  taskflow.emplace([&](turbo::Subflow& sf){
    for(int i=0; i<100; i++) {
      sf.silent_async([&](){ counter.fetch_add(1, std::memory_order_relaxed); });
    }
    sf.join();

    // when subflow joins, all spawned tasks from the subflow will finish
    if(counter == 100) {
      std::cout << "async tasks spawned from the subflow all finish\n";
    }
    else {
      throw std::runtime_error("this should not happen");
    }
  });

  executor.run(taskflow).wait();

  return 0;
}








