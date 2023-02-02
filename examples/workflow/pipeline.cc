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

#include <turbo/workflow/algorithm/pipeline.h>
#include <turbo/workflow/workflow.h>

int main() {

  turbo::Workflow workflow("pipeline");
  turbo::Executor executor;

  const size_t num_lines = 4;

  // custom data storage
  std::array<int, num_lines> buffer;

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
        "stage 2: input buffer[%zu] = %d\n", pf.line(), buffer[pf.line()]
      );
      // propagate the previous result to this pipe and increment
      // it by one
      buffer[pf.line()] = buffer[pf.line()] + 1;
    }},

    turbo::Pipe{turbo::PipeType::SERIAL, [&buffer](turbo::Pipeflow& pf) {
      printf(
        "stage 3: input buffer[%zu] = %d\n", pf.line(), buffer[pf.line()]
      );
      // propagate the previous result to this pipe and increment
      // it by one
      buffer[pf.line()] = buffer[pf.line()] + 1;
    }}
  );

  // build the pipeline graph using composition
  turbo::Task init = workflow.emplace([](){ std::cout << "ready\n"; })
                          .name("starting pipeline");
  turbo::Task task = workflow.composed_of(pl)
                          .name("pipeline");
  turbo::Task stop = workflow.emplace([](){ std::cout << "stopped\n"; })
                          .name("pipeline stopped");

  // create task dependency
  init.precede(task);
  task.precede(stop);

  // dump the pipeline graph structure (with composition)
  workflow.dump(std::cout);

  // run the pipeline
  executor.run(workflow).wait();

  return 0;
}
