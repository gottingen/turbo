// This program demonstrates loop-based parallelism using:
//   + STL-styled iterators
//   + plain integral indices

#include <turbo/workflow/workflow.h>

// Procedure: for_each
void for_each(int N) {

  turbo::Executor executor;
  turbo::Workflow workflow;

  std::vector<int> range(N);
  std::iota(range.begin(), range.end(), 0);

  workflow.for_each(range.begin(), range.end(), [&] (int i) {
    printf("for_each on container item: %d\n", i);
  });

  executor.run(workflow).get();
}

// Procedure: for_each_index
void for_each_index(int N) {

  turbo::Executor executor;
  turbo::Workflow workflow;

  // [0, N) with step size 2
  workflow.for_each_index(0, N, 2, [] (int i) {
    printf("for_each_index on index: %d\n", i);
  });

  executor.run(workflow).get();
}

// ----------------------------------------------------------------------------

// Function: main
int main(int argc, char* argv[]) {

  if(argc != 2) {
    std::cerr << "Usage: ./parallel_for num_iterations" << std::endl;
    std::exit(EXIT_FAILURE);
  }

  for_each(std::atoi(argv[1]));
  for_each_index(std::atoi(argv[1]));

  return 0;
}






