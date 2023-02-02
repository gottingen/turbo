// This program demonstrates how to implement while-loop control flow
// using condition tasks.
#include <turbo/workflow/workflow.h>

int main() {

  turbo::Executor executor;
  turbo::Workflow workflow;

  int i;

  auto [init, cond, body, back, done] = workflow.emplace(
    [&](){ std::cout << "i=0\n"; i=0; },
    [&](){ std::cout << "while i<5\n"; return i < 5 ? 0 : 1; },
    [&](){ std::cout << "i++=" << i++ << '\n'; },
    [&](){ std::cout << "back\n"; return 0; },
    [&](){ std::cout << "done\n"; }
  );

  init.name("init");
  cond.name("while i<5");
  body.name("i++");
  back.name("back");
  done.name("done");

  init.precede(cond);
  cond.precede(body, done);
  body.precede(back);
  back.precede(cond);

  workflow.dump(std::cout);

  executor.run(workflow).wait();

  return 0;
}

