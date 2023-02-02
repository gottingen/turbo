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

#include <turbo/workflow/algorithm/pipeline.h>
#include <turbo/workflow/workflow.h>

int main() {

  turbo::Workflow workflow("pipeline");
  turbo::Executor executor;

  const size_t num_lines = 4;

  // create data storage
  std::array<int, num_lines> buffer;

  // define the pipe callable
  auto pipe_callable = [&buffer] (turbo::Pipeflow& pf) mutable {
    switch(pf.pipe()) {
      // first stage generates only 5 scheduling tokens and saves the
      // token number into the buffer.
      case 0: {
        if(pf.token() == 5) {
          pf.stop();
        }
        else {
          printf("stage 1: input token = %zu\n", pf.token());
          buffer[pf.line()] = pf.token();
        }
        return;
      }
      break;

      // other stages propagate the previous result to this pipe and
      // increment it by one
      default: {
        printf(
          "stage %zu: input buffer[%zu] = %d\n", pf.pipe(), pf.line(), buffer[pf.line()]
        );
        buffer[pf.line()] = buffer[pf.line()] + 1;
      }
      break;
    }
  };

  // create a vector of three pipes
  std::vector< turbo::Pipe<std::function<void(turbo::Pipeflow&)>> > pipes;

  for(size_t i=0; i<3; i++) {
    pipes.emplace_back(turbo::PipeType::SERIAL, pipe_callable);
  }

  // create a pipeline of four parallel lines using the given vector of pipes
  turbo::ScalablePipeline<decltype(pipes)::iterator> pl(num_lines, pipes.begin(), pipes.end());

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

  // reset the pipeline to a new range of five pipes and starts from
  // the initial state (i.e., token counts from zero)
  for(size_t i=0; i<2; i++) {
    pipes.emplace_back(turbo::PipeType::SERIAL, pipe_callable);
  }
  pl.reset(pipes.begin(), pipes.end());

  executor.run(workflow).wait();

  return 0;
}



