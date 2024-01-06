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
#include "turbo/taskflow/taskflow.h"

int spawn(int n, turbo::Subflow& sbf) {
  if (n < 2) return n;
  int res1, res2;

  // compute f(n-1)
  sbf.emplace([&res1, n] (turbo::Subflow& sbf) { res1 = spawn(n - 1, sbf); } )
     .name(std::to_string(n-1));

  // compute f(n-2)
  sbf.emplace([&res2, n] (turbo::Subflow& sbf) { res2 = spawn(n - 2, sbf); } )
     .name(std::to_string(n-2));

  sbf.join();
  return res1 + res2;
}

int main(int argc, char* argv[]) {

  if(argc != 2) {
    std::cerr << "usage: ./fibonacci N\n";
    std::exit(EXIT_FAILURE);
  }

  int N = std::atoi(argv[1]);

  if(N < 0) {
    throw std::runtime_error("N must be non-negative");
  }

  int res;  // result

  turbo::Executor executor;
  turbo::Taskflow taskflow("fibonacci");

  taskflow.emplace([&res, N] (turbo::Subflow& sbf) {
    res = spawn(N, sbf);
  }).name(std::to_string(N));

  executor.run(taskflow).wait();

  //taskflow.dump(std::cout);

  std::cout << "Fib[" << N << "]: " << res << std::endl;

  return 0;
}









