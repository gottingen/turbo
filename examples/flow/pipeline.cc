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
// that propagates a series of integers and adds one to the result at each
// stage.
//
// The pipeline has the following structure:
//
// o -> o -> o
// |         |
// v         v
// o -> o -> o
// |         |
// v         v
// o -> o -> o
// |         |
// v         v
// o -> o -> o

#include "turbo/taskflow/taskflow.h"
#include "turbo/taskflow/algorithm/pipeline.h"

int main() {

  turbo::Taskflow taskflow("pipeline");
  turbo::Executor executor;

  const size_t num_lines = 4;

  // custom data storage
  std::array<size_t, num_lines> buffer;

  // the pipeline consists of three pipes (serial-parallel-serial)
  // and up to four concurrent scheduling tokens
  turbo::Pipeline pl(num_lines,
    turbo::Pipe{turbo::PipeType::SERIAL, [&buffer](turbo::Pipeflow& pf) {
      // generate only 5 scheduling tokens
      if(pf.token() == 5) {
        pf.stop();
      }
      // save the result of this pipe into the buffer
      else {
        printf("stage 1: input token = %zu\n", pf.token());
        buffer[pf.line()] = pf.token();
      }
    }},

    turbo::Pipe{turbo::PipeType::PARALLEL, [&buffer](turbo::Pipeflow& pf) {
      printf(
        "stage 2: input buffer[%zu] = %zu\n", pf.line(), buffer[pf.line()]
      );
      // propagate the previous result to this pipe and increment
      // it by one
      buffer[pf.line()] = buffer[pf.line()] + 1;
    }},

    turbo::Pipe{turbo::PipeType::SERIAL, [&buffer](turbo::Pipeflow& pf) {
      printf(
        "stage 3: input buffer[%zu] = %zu\n", pf.line(), buffer[pf.line()]
      );
      // propagate the previous result to this pipe and increment
      // it by one
      buffer[pf.line()] = buffer[pf.line()] + 1;
    }}
  );

  // build the pipeline graph using composition
  turbo::Task init = taskflow.emplace([](){ std::cout << "ready\n"; })
                          .name("starting pipeline");
  turbo::Task task = taskflow.composed_of(pl)
                          .name("pipeline");
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
