// The example creates the following cyclic graph of one iterative loop:
//
//       A
//       |
//       v
//       B<---|
//       |    |
//       v    |
//       C----|
//       |
//       v
//       D
//
// - A is a task that initializes a counter to zero
// - B is a task that increments the counter
// - C is a condition task that loops with B until the counter
//   reaches a breaking number
// - D is a task that finalizes the result

#include <turbo/workflow/workflow.h>

int main() {

  turbo::Executor executor;
  turbo::Workflow workflow("Conditional Tasking Demo");

  int counter;

  auto A = workflow.emplace([&](){
    std::cout << "initializes the counter to zero\n";
    counter = 0;
  }).name("A");

  auto B = workflow.emplace([&](){
    std::cout << "loops to increment the counter\n";
    counter++;
  }).name("B");

  auto C = workflow.emplace([&](){
    std::cout << "counter is " << counter << " -> ";
    if(counter != 5) {
      std::cout << "loops again (goes to B)\n";
      return 0;
    }
    std::cout << "breaks the loop (goes to D)\n";
    return 1;
  }).name("C");

  auto D = workflow.emplace([&](){
    std::cout << "done with counter equal to " << counter << '\n';
  }).name("D");

  A.precede(B);
  B.precede(C);
  C.precede(B);
  C.precede(D);

  // visualizes the workflow
  workflow.dump(std::cout);

  // executes the workflow
  executor.run(workflow).wait();

  assert(counter == 5);

  return 0;
}







