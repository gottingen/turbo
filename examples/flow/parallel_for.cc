// Copyright 2023 The Elastic-AI Authors.
// part of Elastic AI Search
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//      https://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.
//
// This program demonstrates loop-based parallelism using:
//   + STL-styled iterators
//   + plain integral indices

#include "turbo/taskflow/taskflow.h"
#include "turbo/taskflow/algorithm/for_each.h"

// Procedure: for_each
void for_each(int N) {

  turbo::Executor executor;
  turbo::Taskflow taskflow;

  std::vector<int> range(N);
  std::iota(range.begin(), range.end(), 0);

  taskflow.for_each(range.begin(), range.end(), [&] (int i) {
    printf("for_each on container item: %d\n", i);
  });

  executor.run(taskflow).get();

  taskflow.dump(std::cout);
}

// Procedure: for_each_index
void for_each_index(int N) {

  turbo::Executor executor;
  turbo::Taskflow taskflow;

  // [0, N) with step size 2
  taskflow.for_each_index(0, N, 2, [] (int i) {
    printf("for_each_index on index: %d\n", i);
  });

  executor.run(taskflow).get();
  
  taskflow.dump(std::cout);
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






