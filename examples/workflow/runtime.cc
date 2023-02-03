// This program demonstrates how to use a runtime task to forcefully
// schedule an active task that would never be scheduled.

#include <turbo/workflow/workflow.h>

int main() {

  turbo::Workflow workflow("Runtime Tasking");
  turbo::Executor executor;

  turbo::Task A, B, C, D;

  std::tie(A, B, C, D) = workflow.emplace(
    [] () { return 0; },
    [&C] (turbo::Runtime& rt) {  // C must be captured by reference
      std::cout << "B\n";
      rt.schedule(C);
    },
    [] () { std::cout << "C\n"; },
    [] () { std::cout << "D\n"; }
  );

  // name tasks
  A.name("A");
  B.name("B");
  C.name("C");
  D.name("D");

  // create conditional dependencies
  A.precede(B, C, D);

  // dump the graph structure
  workflow.dump(std::cout);

  // we will see both B and C in the output
  executor.run(workflow).wait();

  return 0;
}
