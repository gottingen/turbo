// This example demonstrates how to use different methods to
// run a workflow.
#include <turbo/workflow/workflow.h>

int main(){

  // create an executor and a workflow
  turbo::Executor executor(1);
  turbo::Workflow workflow("Demo");

  auto A = workflow.emplace([&](){ std::cout << "TaskA\n"; }).name("A");
  auto B = workflow.emplace([&](turbo::Subflow& subflow){
    std::cout << "TaskB\n";
    auto B1 = subflow.emplace([&](){ std::cout << "TaskB1\n"; }).name("B1");
    auto B2 = subflow.emplace([&](){ std::cout << "TaskB2\n"; }).name("B2");
    auto B3 = subflow.emplace([&](){ std::cout << "TaskB3\n"; }).name("B3");
    B1.precede(B3);
    B2.precede(B3);
  }).name("B");

  auto C = workflow.emplace([&](){ std::cout << "TaskC\n"; }).name("C");
  auto D = workflow.emplace([&](){ std::cout << "TaskD\n"; }).name("D");

  A.precede(B, C);
  B.precede(D);
  C.precede(D);

  // dumpping a workflow before execution won't visualize subflow tasks
  std::cout << "Dump the workflow before execution:\n";
  workflow.dump(std::cout);

  std::cout << "Run the workflow once without callback\n" << std::endl;
  executor.run(workflow).get();
  std::cout << std::endl;

  // after execution, we can visualize subflow tasks
  std::cout << "Dump the workflow after execution:\n";
  workflow.dump(std::cout);
  std::cout << std::endl;

  std::cout << "Use wait_for_all to wait for the execution to finish\n";
  executor.run(workflow).get();
  executor.wait_for_all();
  std::cout << std::endl;

  std::cout << "Execute the workflow two times without a callback\n";
  executor.run(workflow).get();
  std::cout << "Dump after two executions:\n";
  workflow.dump(std::cout);
  std::cout << std::endl;

  std::cout << "Execute the workflow four times with a callback\n";
  executor.run_n(workflow, 4, [] () { std::cout << "finishes 4 runs\n"; })
          .get();
  std::cout << std::endl;

  std::cout << "Run the workflow until the predicate returns true\n";
  executor.run_until(workflow, [counter=3]() mutable {
    std::cout << "Counter = " << counter << std::endl;
    return counter -- == 0;
  }).get();

  workflow.dump(std::cout);

  return 0;
}
