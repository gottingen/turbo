// This example demonstrates how to use 'dump' method to visualize
// a workflow graph in DOT format.

#include <turbo/workflow/workflow.h>

int main(){

  turbo::Workflow workflow("Visualization Demo");

  // ------------------------------------------------------
  // Static Tasking
  // ------------------------------------------------------
  auto A = workflow.emplace([] () { std::cout << "Task A\n"; });
  auto B = workflow.emplace([] () { std::cout << "Task B\n"; });
  auto C = workflow.emplace([] () { std::cout << "Task C\n"; });
  auto D = workflow.emplace([] () { std::cout << "Task D\n"; });
  auto E = workflow.emplace([] () { std::cout << "Task E\n"; });

  A.precede(B, C, E);
  C.precede(D);
  B.precede(D, E);

  std::cout << "[dump without name assignment]\n";
  workflow.dump(std::cout);

  std::cout << "[dump with name assignment]\n";
  A.name("A");
  B.name("B");
  C.name("C");
  D.name("D");
  E.name("E");

  // if the graph contains solely static tasks, you can simpley dump them
  // without running the graph
  workflow.dump(std::cout);

  // ------------------------------------------------------
  // Dynamic Tasking
  // ------------------------------------------------------
  workflow.emplace([](turbo::Subflow& sf){
    sf.emplace([](){ std::cout << "subflow task1"; }).name("s1");
    sf.emplace([](){ std::cout << "subflow task2"; }).name("s2");
    sf.emplace([](){ std::cout << "subflow task3"; }).name("s3");
  });

  // in order to visualize subflow tasks, you need to run the workflow
  // to spawn the dynamic tasks first
  turbo::Executor executor;
  executor.run(workflow).wait();

  workflow.dump(std::cout);


  return 0;
}


