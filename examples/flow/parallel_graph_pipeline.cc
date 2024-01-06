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
// This program demonstrates how to pipeline a sequence of linearly dependent
// tasks (stage function) over a directed acyclic graph.

#include "turbo/taskflow/taskflow.h"
#include "turbo/taskflow/algorithm/pipeline.h"

// 1st-stage function
void f1(const std::string& node) {
  printf("f1(%s)\n", node.c_str());
}

// 2nd-stage function
void f2(const std::string& node) {
  printf("f2(%s)\n", node.c_str());
}

// 3rd-stage function
void f3(const std::string& node) {
  printf("f3(%s)\n", node.c_str());
}

int main() {

  turbo::Taskflow taskflow("graph processing pipeline");
  turbo::Executor executor;

  const size_t num_lines = 2;

  // a topological order of the graph
  //    |-> B
  // A--|
  //    |-> C
  const std::vector<std::string> nodes = {"A", "B", "C"};

  // the pipeline consists of three serial pipes
  // and up to two concurrent scheduling tokens
  turbo::Pipeline pl(num_lines,

    // first pipe calls f1
    turbo::Pipe{turbo::PipeType::SERIAL, [&](turbo::Pipeflow& pf) {
      if(pf.token() == nodes.size()) {
        pf.stop();
      }
      else {
        f1(nodes[pf.token()]);
      }
    }},

    // second pipe calls f2
    turbo::Pipe{turbo::PipeType::SERIAL, [&](turbo::Pipeflow& pf) {
      f2(nodes[pf.token()]);
    }},

    // third pipe calls f3
    turbo::Pipe{turbo::PipeType::SERIAL, [&](turbo::Pipeflow& pf) {
      f3(nodes[pf.token()]);
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
