// This program demonstrates how to create a pipeline scheduling framework
// that computes the maximum occurrence of the character for each input string.
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
// o -> o -> o  (string -> unordered_map<char, size_t> -> pair<char, size_t>)
//
// Input:
//   abade
//   ddddf
//   eefge
//   xyzzd
//   ijjjj
//   jiiii
//   kkijk
//
// Output:
//   a:2
//   d:4
//   e:3
//   z:2
//   j:4
//   i:4
//   k:3

#include <turbo/workflow/algorithm/pipeline.h>
#include <turbo/workflow/workflow.h>

// Function: format the map
std::string format_map(const std::unordered_map<char, size_t>& map) {
  std::ostringstream oss;
  for(const auto& [i, j] : map) {
    oss << i << ':' << j << ' ';
  }
  return oss.str();
}

int main() {

  turbo::Workflow workflow("text-processing pipeline");
  turbo::Executor executor;

  const size_t num_lines = 2;

  // input data
  std::vector<std::string> input = {
    "abade",
    "ddddf",
    "eefge",
    "xyzzd",
    "ijjjj",
    "jiiii",
    "kkijk"
  };

  // custom data storage
  using data_type = std::variant<
    std::string, std::unordered_map<char, size_t>, std::pair<char, size_t>
  >;
  std::array<data_type, num_lines> buffer;

  // the pipeline consists of three pipes (serial-parallel-serial)
  // and up to two concurrent scheduling tokens
  turbo::Pipeline pl(num_lines,

    // first pipe processes the input data
    turbo::Pipe{turbo::PipeType::SERIAL, [&](turbo::Pipeflow& pf) {
      if(pf.token() == input.size()) {
        pf.stop();
      }
      else {
        buffer[pf.line()] = input[pf.token()];
        printf("stage 1: input token = %s\n", input[pf.token()].c_str());
      }
    }},

    // second pipe counts the frequency of each character
    turbo::Pipe{turbo::PipeType::PARALLEL, [&](turbo::Pipeflow& pf) {
      std::unordered_map<char, size_t> map;
      for(auto c : std::get<std::string>(buffer[pf.line()])) {
        map[c]++;
      }
      buffer[pf.line()] = map;
      printf("stage 2: map = %s\n", format_map(map).c_str());
    }},

    // third pipe reduces the most frequent character
    turbo::Pipe{turbo::PipeType::SERIAL, [&buffer](turbo::Pipeflow& pf) {
      auto& map = std::get<std::unordered_map<char, size_t>>(buffer[pf.line()]);
      auto sol = std::max_element(map.begin(), map.end(), [](auto& a, auto& b){
        return a.second < b.second;
      });
      printf("stage 3: %c:%zu\n", sol->first, sol->second);
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
