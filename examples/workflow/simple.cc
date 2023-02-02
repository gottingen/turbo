// A simple example to capture the following task dependencies.
//
//           +---+
//     +---->| B |-----+
//     |     +---+     |
//   +---+           +-v-+
//   | A |           | D |
//   +---+           +-^-+
//     |     +---+     |
//     +---->| C |-----+
//           +---+
//
#include <turbo/workflow/workflow.h> // the only include you need

int main(){

  turbo::Executor executor;
  turbo::Workflow workflow("simple");

  auto [A, B, C, D] = workflow.emplace(
    []() { std::cout << "TaskA\n"; },
    []() { std::cout << "TaskB\n"; },
    []() { std::cout << "TaskC\n"; },
    []() { std::cout << "TaskD\n"; }
  );

  A.precede(B, C);  // A runs before B and C
  D.succeed(B, C);  // D runs after  B and C

  executor.run(workflow).wait();

  return 0;
}
