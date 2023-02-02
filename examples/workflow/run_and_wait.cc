// This example demonstrates how to use the run_and_wait
// method in the executor.
#include <turbo/workflow/workflow.h>

int main(){
  
  const size_t N = 100;
  const size_t T = 1000;
  
  // create an executor and a workflow
  turbo::Executor executor(2);
  turbo::Workflow workflow;

  std::array<turbo::Workflow, N> workflows;

  std::atomic<size_t> counter{0};
  
  for(size_t n=0; n<N; n++) {
    for(size_t i=0; i<T; i++) {
      workflows[n].emplace([&](){ counter++; });
    }
    workflow.emplace([&executor, &tf=workflows[n]](){
      executor.run_and_wait(tf);
      //executor.run(tf).wait();  <-- can result in deadlock
    });
  }

  executor.run(workflow).wait();

  return 0;
}
