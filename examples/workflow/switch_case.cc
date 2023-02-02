// This program demonstrates how to implement switch-case control flow
// using condition tasks.
#include <turbo/workflow/workflow.h>

int main() {

  turbo::Executor executor;
  turbo::Workflow workflow;

  auto [source, swcond, case1, case2, case3, target] = workflow.emplace(
    [](){ std::cout << "source\n"; },
    [](){ std::cout << "switch\n"; return rand()%3; },
    [](){ std::cout << "case 1\n"; return 0; },
    [](){ std::cout << "case 2\n"; return 0; },
    [](){ std::cout << "case 3\n"; return 0; },
    [](){ std::cout << "target\n"; }
  );

  source.precede(swcond);
  swcond.precede(case1, case2, case3);
  target.succeed(case1, case2, case3);

  executor.run(workflow).wait();

  return 0;
}
