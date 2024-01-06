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
// stage, using a range of pipes provided by the application.
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
//
// Then, the program resets the pipeline to a new range of five pipes.
//
// o -> o -> o -> o -> o
// |    |    |    |    |
// v    v    v    v    v
// o -> o -> o -> o -> o
// |    |    |    |    |
// v    v    v    v    v
// o -> o -> o -> o -> o
// |    |    |    |    |
// v    v    v    v    v
// o -> o -> o -> o -> o

#include "turbo/taskflow/taskflow.h"
#include "turbo/taskflow/algorithm/pipeline.h"

int main() {

  turbo::Taskflow taskflow("pipeline");
  turbo::Executor executor;

  const size_t num_lines = 4;

    //1. How can I put a placeholder in the first pipe, i.e. [] (void, turbo::Pipeflow&) in order to match the pipe vector?
    auto pipe_callable1 = [] (turbo::Pipeflow& pf) mutable -> int {
        if(pf.token() == 5) {
          pf.stop();
          return 0;
        }
        else {
          printf("stage 1: input token = %zu\n", pf.token());
          return pf.token();
        }
    };
    auto pipe_callable2 = [] (int input, turbo::Pipeflow& pf) mutable -> float {
        return input + 1.0;
    };
    auto pipe_callable3 = [] (float input, turbo::Pipeflow& pf) mutable -> int {
        return input + 1;
    };

  //2. Is this ok when the type in vector definition is different from the exact types of emplaced elements?
  std::vector< ScalableDataPipeBase* > pipes;

  pipes.emplace_back(turbo::make_scalable_datapipe<void, int>(turbo::PipeType::SERIAL, pipe_callable1));
  pipes.emplace_back(turbo::make_scalable_datapipe<int, float>(turbo::PipeType::SERIAL, pipe_callable2));
  pipes.emplace_back(turbo::make_scalable_datapipe<float, int>(turbo::PipeType::SERIAL, pipe_callable3));

  // create a pipeline of four parallel lines using the given vector of pipes
  turbo::ScalablePipeline<decltype(pipes)::iterator> pl(num_lines, pipes.begin(), pipes.end());

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

  // reset the pipeline to a new range of five pipes and starts from
  // the initial state (i.e., token counts from zero)
  pipes.emplace_back(turbo::make_scalable_datapipe<int, float>(turbo::PipeType::SERIAL, pipe_callable1));
  pipes.emplace_back(turbo::make_scalable_datapipe<float, int>(turbo::PipeType::SERIAL, pipe_callable1));
  pl.reset(pipes.begin(), pipes.end());

  executor.run(taskflow).wait();

  return 0;
}



