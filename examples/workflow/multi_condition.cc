// This program demonstrates how to use multi-condition task
// to jump to multiple successor tasks
//
// A ----> B
//   |
//   |---> C
//   |
//   |---> D
//
#include <turbo/workflow/workflow.h>

int main() {

  turbo::Executor executor;
  turbo::Workflow workflow("Multi-Conditional Tasking Demo");

  auto A = workflow.emplace([&]() -> turbo::SmallVector<int> {
    std::cout << "A\n";
    return {0, 2};
  }).name("A");
  auto B = workflow.emplace([&](){ std::cout << "B\n"; }).name("B");
  auto C = workflow.emplace([&](){ std::cout << "C\n"; }).name("C");
  auto D = workflow.emplace([&](){ std::cout << "D\n"; }).name("D");

  A.precede(B, C, D);

  // visualizes the workflow
  workflow.dump(std::cout);

  // executes the workflow
  executor.run(workflow).wait();

  return 0;
}

