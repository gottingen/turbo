// The program demonstrates how to create asynchronous task
// from an executor and a subflow.
#include <turbo/workflow/workflow.h>

int main() {

  turbo::Executor executor;

  // create asynchronous tasks from the executor
  // (using executor as a thread pool)
  turbo::Future<std::optional<int>> future1 = executor.async([](){
    std::cout << "async task 1 returns 1\n";
    return 1;
  });

  executor.silent_async([](){  // silent async task doesn't return
    std::cout << "async task 2 does not return (silent)\n";
  });

  // create asynchronous tasks with names (for profiling)
  executor.named_async("async_task", [](){
    std::cout << "named async task returns 1\n";
    return 1;
  });

  executor.named_silent_async("silent_async_task", [](){
    std::cout << "named silent async task does not return\n";
  });

  executor.wait_for_all();  // wait for the two async tasks to finish

  // create asynchronous tasks from a subflow
  // all asynchronous tasks are guaranteed to finish when the subflow joins
  turbo::Workflow workflow;

  std::atomic<int> counter {0};

  workflow.emplace([&](turbo::Subflow& sf){
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

  executor.run(workflow).wait();

  return 0;
}

