// The program demonstrates how to create asynchronous task
// from a running subflow.
#include <turbo/workflow/workflow.h>

int main() {

  turbo::Workflow workflow("Subflow Async");
  turbo::Executor executor;

  std::atomic<int> counter{0};

  workflow.emplace([&](turbo::Subflow& sf){
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

  executor.run(workflow).wait();

  return 0;
}

