// This example shows how to compose a workflow
#include <turbo/workflow/workflow.h>

void composition_example_1() {

  std::cout << "Composition example 1\n";

  turbo::Executor executor;

  // f1 has three independent tasks
  turbo::Workflow f1("F1");
  auto f1A = f1.emplace([]() { std::cout << "F1 TaskA\n"; });
  auto f1B = f1.emplace([]() { std::cout << "F1 TaskB\n"; });
  auto f1C = f1.emplace([]() { std::cout << "F1 TaskC\n"; });
  f1A.name("f1A");
  f1B.name("f1B");
  f1C.name("f1C");
  f1A.precede(f1C);
  f1B.precede(f1C);

  // f2A ---
  //        |----> f2C ----> f1_module_task ----> f2D
  // f2B ---
  turbo::Workflow f2("F2");
  auto f2A = f2.emplace([]() { std::cout << "  F2 TaskA\n"; });
  auto f2B = f2.emplace([]() { std::cout << "  F2 TaskB\n"; });
  auto f2C = f2.emplace([]() { std::cout << "  F2 TaskC\n"; });
  auto f2D = f2.emplace([]() { std::cout << "  F2 TaskD\n"; });
  f2A.name("f2A");
  f2B.name("f2B");
  f2C.name("f2C");
  f2D.name("f2D");

  f2A.precede(f2C);
  f2B.precede(f2C);

  turbo::Task f1_module_task = f2.composed_of(f1);
  f1_module_task.name("module");
  f2C.precede(f1_module_task);
  f1_module_task.precede(f2D);

  f2.dump(std::cout);

  executor.run_n(f2, 3).get();
}

void composition_example_2() {

  std::cout << "Composition example 2\n";

  turbo::Executor executor;

  // f1 has two independent tasks
  turbo::Workflow f1("F1");
  auto f1A = f1.emplace([&]() { std::cout << "F1 TaskA\n"; });
  auto f1B = f1.emplace([&]() { std::cout << "F1 TaskB\n"; });
  f1A.name("f1A");
  f1B.name("f1B");

  //  f2A ---
  //         |----> f2C
  //  f2B ---
  //
  //  f1_module_task
  turbo::Workflow f2("F2");
  auto f2A = f2.emplace([&]() { std::cout << "  F2 TaskA\n"; });
  auto f2B = f2.emplace([&]() { std::cout << "  F2 TaskB\n"; });
  auto f2C = f2.emplace([&]() { std::cout << "  F2 TaskC\n"; });
  f2A.name("f2A");
  f2B.name("f2B");
  f2C.name("f2C");

  f2A.precede(f2C);
  f2B.precede(f2C);
  f2.composed_of(f1).name("module_of_f1");

  // f3 has a module task (f2) and a regular task
  turbo::Workflow f3("F3");
  f3.composed_of(f2).name("module_of_f2");
  f3.emplace([]() { std::cout << "      F3 TaskA\n"; }).name("f3A");

  // f4: f3_module_task -> f2_module_task
  turbo::Workflow f4;
  f4.name("F4");
  auto f3_module_task = f4.composed_of(f3).name("module_of_f3");
  auto f2_module_task = f4.composed_of(f2).name("module_of_f2");
  f3_module_task.precede(f2_module_task);

  f4.dump(std::cout);

  executor
      .run_until(
          f4,
          [iter = 1]() mutable {
            std::cout << '\n';
            return iter-- == 0;
          },
          []() { std::cout << "First run_until finished\n"; })
      .get();

  executor.run_until(
      f4,
      [iter = 2]() mutable {
        std::cout << '\n';
        return iter-- == 0;
      },
      []() { std::cout << "Second run_until finished\n"; });

  executor
      .run_until(
          f4,
          [iter = 3]() mutable {
            std::cout << '\n';
            return iter-- == 0;
          },
          []() { std::cout << "Third run_until finished\n"; })
      .get();
}

int main() {
  composition_example_1();
  composition_example_2();
  return 0;
}
