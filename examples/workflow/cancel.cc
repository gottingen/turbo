// The program demonstrates how to cancel a submitted workflow
// graph and wait until the cancellation completes.

#include <turbo/workflow/workflow.h>

int main() {

  turbo::Executor executor;
  turbo::Workflow workflow("cancel");

  // We create a workflow graph of 1000 tasks each of 1 second.
  // Ideally, the workflow completes in 1000/P seconds, where P
  // is the number of workers.
  for(int i=0; i<1000; i++) {
    workflow.emplace([](){
      std::this_thread::sleep_for(std::chrono::seconds(1));
    });
  }

  // submit the workflow
  auto beg = std::chrono::steady_clock::now();
  turbo::Future fu = executor.run(workflow);

  // submit a cancel request to cancel all 1000 tasks.
  fu.cancel();

  // wait until the cancellation finishes
  fu.get();
  auto end = std::chrono::steady_clock::now();

  // the duration should be much less than 1000 seconds
  std::cout << "workflow completes in "
            << std::chrono::duration_cast<std::chrono::milliseconds>(end-beg).count()
            << " milliseconds\n";

  return 0;
}


