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
#include "turbo/taskflow/taskflow.h"  // the only include you need

int main(){

  turbo::Executor executor;
  
  // demonstration of dependent async (with future)
  printf("Dependent Async\n");
  auto [A, fuA] = executor.dependent_async([](){ printf("A\n"); });
  auto [B, fuB] = executor.dependent_async([](){ printf("B\n"); }, A);
  auto [C, fuC] = executor.dependent_async([](){ printf("C\n"); }, A);
  auto [D, fuD] = executor.dependent_async([](){ printf("D\n"); }, B, C);

  fuD.get();

  // demonstration of silent dependent async (without future)
  printf("Silent Dependent Async\n");
  A = executor.silent_dependent_async([](){ printf("A\n"); });
  B = executor.silent_dependent_async([](){ printf("B\n"); }, A);
  C = executor.silent_dependent_async([](){ printf("C\n"); }, A);
  D = executor.silent_dependent_async([](){ printf("D\n"); }, B, C);

  executor.wait_for_all();

  return 0;
}




