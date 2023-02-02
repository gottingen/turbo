// A simple example with a semaphore constraint that only one task can
// execute at a time.

#include <turbo/workflow/workflow.h>

void sl() {
  std::this_thread::sleep_for(std::chrono::seconds(1));
}

int main() {

  turbo::Executor executor(4);
  turbo::Workflow workflow;

  // define a critical region of 1 worker
  turbo::Semaphore semaphore(1);

  // create give tasks in workflow
  std::vector<turbo::Task> tasks {
    workflow.emplace([](){ sl(); std::cout << "A" << std::endl; }),
    workflow.emplace([](){ sl(); std::cout << "B" << std::endl; }),
    workflow.emplace([](){ sl(); std::cout << "C" << std::endl; }),
    workflow.emplace([](){ sl(); std::cout << "D" << std::endl; }),
    workflow.emplace([](){ sl(); std::cout << "E" << std::endl; })
  };

  for(auto & task : tasks) {
    task.acquire(semaphore);
    task.release(semaphore);
  }

  executor.run(workflow);
  executor.wait_for_all();

  return 0;
}

