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
// This program demonstrates how to perform parallel inclusive scan.

#include "turbo/taskflow/taskflow.h"
#include "turbo/taskflow/algorithm/scan.h"

int main(int argc, char* argv[]) {

  if(argc != 3) {
    std::cerr << "usage: ./inclusive_scan num_workers num_elements\n"; 
    std::exit(EXIT_FAILURE);
  }

  size_t W = std::atoi(argv[1]);
  size_t N = std::atoi(argv[2]);

  turbo::Executor executor(W);
  turbo::Taskflow taskflow;

  std::vector<int> elements(N), scan_seq(N), scan_par(N);
  for(size_t i=0; i<N; i++) {
    elements[i] = static_cast<int>(i);
  }
  
  // sequential inclusive scan
  {
    std::cout << "sequential inclusive scan ... ";
    auto beg = std::chrono::steady_clock::now();
    std::inclusive_scan(
      elements.begin(), elements.end(), scan_seq.begin(), std::multiplies<int>{}
    );
    auto end = std::chrono::steady_clock::now();
    std::cout << std::chrono::duration_cast<std::chrono::nanoseconds>(end-beg).count()
              << "ns\n";
  }

  // create a parallel inclusive scan task
  {
    std::cout << "parallel   inclusive scan ... ";
    auto beg = std::chrono::steady_clock::now();
    taskflow.inclusive_scan(
      elements.begin(), elements.end(), scan_par.begin(), std::multiplies<int>{}
    );
    executor.run(taskflow).wait();
    auto end = std::chrono::steady_clock::now();
    std::cout << std::chrono::duration_cast<std::chrono::nanoseconds>(end-beg).count()
              << "ns\n";
  }

  // verify the result
  for(size_t i=0; i<N; i++) {
    if(scan_seq[i] != scan_par[i]) {
      printf(
        "scan_seq[%zu]=%d..., scan_par[%zu]=%d...\n",
        i, scan_seq[i], i, scan_par[i]
      );
      throw std::runtime_error("incorrect result");
    }
  }

  printf("correct result\n");

  return 0;
}
