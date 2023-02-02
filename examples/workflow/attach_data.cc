// This example demonstrates how to attach data to a task and run
// the task iteratively with changing data.

#include <turbo/workflow/workflow.h>

int main(){

  turbo::Executor executor;
  turbo::Workflow workflow("attach data to a task");

  int data;

  // create a task and attach it the data
  auto A = workflow.placeholder();
  A.data(&data).work([A](){
    auto d = *static_cast<int*>(A.data());
    std::cout << "data is " << d << std::endl;
  });

  // run the workflow iteratively with changing data
  for(data = 0; data<10; data++){
    executor.run(workflow).wait();
  }

  return 0;
}
