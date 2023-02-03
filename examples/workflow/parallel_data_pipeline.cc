// This program demonstrates how to use turbo::DataPipeline to create
// a pipeline with in-pipe data automatically managed by the Workflow
// library.

#include <turbo/workflow/algorithm/data_pipeline.h>
#include <turbo/workflow/workflow.h>

int main() {

  // dataflow => void -> int -> std::string -> float -> void 
  turbo::Workflow workflow("pipeline");
  turbo::Executor executor;

  const size_t num_lines = 3;
  
  // create a pipeline graph
  turbo::DataPipeline pl(num_lines,
    turbo::make_data_pipe<void, int>(turbo::PipeType::SERIAL, [&](turbo::Pipeflow& pf) -> int{
      if(pf.token() == 5) {
        pf.stop();
        return 0;
      }
      else {
        printf("first pipe returns %lu\n", pf.token());
        return pf.token();
      }
    }),

    turbo::make_data_pipe<int, std::string>(turbo::PipeType::SERIAL, [](int& input) {
      printf("second pipe returns a strong of %d\n", input + 100);
      return std::to_string(input + 100);
    }),

    turbo::make_data_pipe<std::string, void>(turbo::PipeType::SERIAL, [](std::string& input) {
      printf("third pipe receives the input string %s\n", input.c_str());
    })
  );

  // build the pipeline graph using composition
  workflow.composed_of(pl).name("pipeline");

  // dump the pipeline graph structure (with composition)
  workflow.dump(std::cout);

  // run the pipeline
  executor.run(workflow).wait();

  return 0;
}

