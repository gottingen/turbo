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
// This program demonstrates how to create a pipeline scheduling framework
// that defers the exection of current scheuling token to the future.  
//
// The pipeline has the following structure:
//
// o -> o -> o
// |    |    |
// v    v    v
// o -> o -> o
// |    |    |
// v    v    v
// o -> o -> o
// |    |    |
// v    v    v
// o -> o -> o

// The scheduling token has the following dependencies:
//    ___________
//   |           |
//   V _____     |
//   |     |     | 
//   |     V     | 
// 1 2 3 4 5 6 7 8 9 10 
//         ^   |   |
//         |___|   |
//         ^       | 
//         |_______|
//
// 2 is deferred by 8
// 5 is dieferred by 2, 7, and 9


#include "turbo/taskflow/taskflow.h"
#include "turbo/taskflow/algorithm/pipeline.h"

int main() {

  turbo::Taskflow taskflow("deferred_pipeline");
  turbo::Executor executor;

  const size_t num_lines = 4;

  // the pipeline consists of three pipes (serial-parallel-serial)
  // and up to four concurrent scheduling tokens
  turbo::Pipeline pl(num_lines,
    turbo::Pipe{turbo::PipeType::SERIAL, [](turbo::Pipeflow& pf) {
      // generate only 15 scheduling tokens
      if(pf.token() == 15) {
        pf.stop();
      }
      else {
        // Token 5 is deferred
        if (pf.token() == 5) {
          switch(pf.num_deferrals()) {
            case 0:
              pf.defer(2);
              printf("1st-time: Token %zu is deferred by 2\n", pf.token());
              pf.defer(7);
              printf("1st-time: Token %zu is deferred by 7\n", pf.token());
              return;  
            break;

            case 1:
              pf.defer(9);
              printf("2nd-time: Token %zu is deferred by 9\n", pf.token());
              return;
            break;

            case 2:
              printf("3rd-time: Tokens 2, 7 and 9 resolved dependencies for token %zu\n", pf.token());
            break;
          }
        }
        else if (pf.token() == 2) {
          switch(pf.num_deferrals()) {
            case 0:
              pf.defer(8);
              printf("1st-time: Token %zu is deferred by 8\n", pf.token());
            break;
            case 1:
              printf("2nd-time: Token 8 resolved dependencies for token %zu\n", pf.token());
            break;
          }
        }
        else {
          printf("stage 1: Non-deferred token %zu\n", pf.token());
        }
      }
    }},

    turbo::Pipe{turbo::PipeType::SERIAL, [](turbo::Pipeflow& pf) {
      printf("stage 2: input token %zu (deferrals=%zu)\n", pf.token(), pf.num_deferrals());
    }},

    turbo::Pipe{turbo::PipeType::SERIAL, [](turbo::Pipeflow& pf) {
      printf("stage 3: input token %zu\n", pf.token());
    }}
  );
  
  // build the pipeline graph using composition
  turbo::Task init = taskflow.emplace([](){ std::cout << "ready\n"; })
                          .name("starting pipeline");
  turbo::Task task = taskflow.composed_of(pl)
                          .name("deferred_pipeline");
  turbo::Task stop = taskflow.emplace([](){ std::cout << "stopped\n"; })
                          .name("pipeline stopped");

  // create task dependency
  init.precede(task);
  task.precede(stop);
  
  // dump the pipeline graph structure (with composition)
  taskflow.dump(std::cout);

  // run the pipeline
  executor.run(taskflow).wait();
  
  return 0;
}


