// Copyright 2023 The Turbo Authors.
//
// Licensed under the Apache License, Version 2.0 (the "License);
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

#include "gtest/gtest.h"

#include "turbo/workflow//workflow.h"

// --------------------------------------------------------
// Testcase: Conditional Tasking
// --------------------------------------------------------

TEST(Cond, Types) {

  turbo::Workflow taskflow;

  auto explicit_c = [](){
    return 1;
  };

  auto implicit_c = []() -> int {
    return 2;
  };

  auto explicit_task = taskflow.emplace(explicit_c);
  auto implicit_task = taskflow.emplace(implicit_c);

  static_assert(turbo::is_condition_task_v<decltype(explicit_c)>);
  static_assert(turbo::is_condition_task_v<decltype(implicit_c)>);

  EXPECT_TRUE(explicit_task.type() == turbo::TaskType::CONDITION);
  EXPECT_TRUE(implicit_task.type() == turbo::TaskType::CONDITION);
}


void conditional_spawn(
  std::atomic<int>& counter,
  const int max_depth,
  int depth,
  turbo::Subflow& subflow
)  {
  if(depth < max_depth) {
    for(int i=0; i<2; i++) {
      auto A = subflow.emplace([&](){ counter++; });
      auto B = subflow.emplace(
        [&, max_depth, depth=depth+1](turbo::Subflow& subflow){
          conditional_spawn(counter, max_depth, depth, subflow);
      });
      auto C = subflow.emplace(
        [&, max_depth, depth=depth+1](turbo::Subflow& subflow){
          conditional_spawn(counter, max_depth, depth, subflow);
        }
      );

      auto cond = subflow.emplace([depth](){
        if(depth%2) return 1;
        else return 0;
      }).precede(B, C);
      A.precede(cond);
    }
  }
}

void loop_cond(unsigned w) {

  turbo::Executor executor(w);
  turbo::Workflow taskflow;

  int counter = -1;
  int state   = 0;

  auto A = taskflow.emplace([&] () { counter = 0; });
  auto B = taskflow.emplace([&] () mutable {
      EXPECT_TRUE((++counter % 100) == (++state % 100));
      return counter < 100 ? 0 : 1;
  });
  auto C = taskflow.emplace(
    [&] () {
      EXPECT_TRUE(counter == 100);
      counter = 0;
  });

  A.precede(B);
  B.precede(B, C);

  EXPECT_TRUE(A.num_strong_dependents() == 0);
  EXPECT_TRUE(A.num_weak_dependents() == 0);
  EXPECT_TRUE(A.num_dependents() == 0);

  EXPECT_TRUE(B.num_strong_dependents() == 1);
  EXPECT_TRUE(B.num_weak_dependents() == 1);
  EXPECT_TRUE(B.num_dependents() == 2);

  executor.run(taskflow).wait();
  EXPECT_TRUE(counter == 0);
  EXPECT_TRUE(state == 100);

  executor.run(taskflow);
  executor.run(taskflow);
  executor.run(taskflow);
  executor.run(taskflow);
  executor.run_n(taskflow, 10);
  executor.wait_for_all();

  EXPECT_TRUE(state == 1500);
}

TEST(LoopCond, 1thread) {
  loop_cond(1);
}

TEST(LoopCond, 2threads) {
  loop_cond(2);
}

TEST(LoopCond, 3threads) {
  loop_cond(3);
}

TEST(LoopCond, 4threads) {
  loop_cond(4);
}

// ----------------------------------------------------------------------------
// Testcase: FlipCoinCond
// ----------------------------------------------------------------------------
void flip_coin_cond(unsigned w) {

  turbo::Workflow taskflow;

  size_t rounds = 10000;
  size_t steps = 0;
  size_t total_steps = 0;
  double average_steps = 0.0;

  auto A = taskflow.emplace( [&](){ steps = 0; } );
  auto B = taskflow.emplace( [&](){ ++steps; return std::rand()%2; } );
  auto C = taskflow.emplace( [&](){ return std::rand()%2; } );
  auto D = taskflow.emplace( [&](){ return std::rand()%2; } );
  auto E = taskflow.emplace( [&](){ return std::rand()%2; } );
  auto F = taskflow.emplace( [&](){ return std::rand()%2; } );
  auto G = taskflow.emplace( [&]() mutable {
      //++N;  // a new round
      total_steps += steps;
      //avg = (double)accu/N;
      //std::cout << "round " << N << ": steps=" << steps
      //                           << " accumulated_steps=" << accu
      //                           << " average_steps=" << avg << '\n';
    }
  );

  A.precede(B).name("init");
  B.precede(C, B).name("flip-coin-1");
  C.precede(D, B).name("flip-coin-2");
  D.precede(E, B).name("flip-coin-3");
  E.precede(F, B).name("flip-coin-4");
  F.precede(G, B).name("flip-coin-5");

  //taskflow.dump(std::cout);

  turbo::Executor executor(w);

  executor.run_n(taskflow, rounds).wait();

  average_steps = total_steps / (double)rounds;

  EXPECT_TRUE(std::fabs(average_steps-32.0)<1.0);

  //taskflow.dump(std::cout);
}

TEST(FlipCoinCond, 1thread) {
  flip_coin_cond(1);
}

TEST(FlipCoinCond, 2threads) {
  flip_coin_cond(2);
}

TEST(FlipCoinCond, 3threads) {
  flip_coin_cond(3);
}

TEST(FlipCoinCond, 4threads) {
  flip_coin_cond(4);
}

// ----------------------------------------------------------------------------
// Testcase: CyclicCondition
// ----------------------------------------------------------------------------
void cyclic_cond(unsigned w) {
  turbo::Executor executor(w);

  //      ____________________
  //      |                  |
  //      v                  |
  // S -> A -> Branch -> many branches -> T
  //
  // Make sure each branch will be passed through exactly once
  // and the T (target) node will also be passed

  turbo::Workflow flow;
  auto S = flow.emplace([](){});

  int num_iterations = 0;
  const int total_iteration = 1000;
  auto A = flow.emplace([&](){ num_iterations ++; });
  S.precede(A);

  int sel = 0;
  bool pass_T = false;
  std::vector<bool> pass(total_iteration, false);
  auto T = flow.emplace([&](){
    EXPECT_TRUE(num_iterations == total_iteration); pass_T=true; }
  );
  auto branch = flow.emplace([&](){ return sel++; });
  A.precede(branch);
  for(size_t i=0; i<total_iteration; i++) {
    auto t = flow.emplace([&, i](){
      if(num_iterations < total_iteration) {
        EXPECT_TRUE(!pass[i]);
        pass[i] = true;
        return 0;
      }
      // The last node will come to here (last iteration)
      EXPECT_TRUE(!pass[i]);
      pass[i] = true;
      return 1;
    });
    branch.precede(t);
    t.precede(A);
    t.precede(T);
  }

  executor.run(flow).get();

  EXPECT_TRUE(pass_T);
  for(size_t i=0; i<pass.size(); i++) {
    EXPECT_TRUE(pass[i]);
  }
}

TEST(CyclicCond, 1thread) {
  cyclic_cond(1);
}

TEST(CyclicCond, 2threads) {
  cyclic_cond(2);
}

TEST(CyclicCond, 3threads) {
  cyclic_cond(3);
}

TEST(CyclicCond, 4threads) {
  cyclic_cond(4);
}

TEST(CyclicCond, 5threads) {
  cyclic_cond(5);
}

TEST(CyclicCond, 6threads) {
  cyclic_cond(6);
}

TEST(CyclicCond, 7threads) {
  cyclic_cond(7);
}

TEST(CyclicCond, 8threads) {
  cyclic_cond(8);
}

// ----------------------------------------------------------------------------
// BTreeCond
// ----------------------------------------------------------------------------
TEST(BTreeCond, BTreeCondition) {
  for(unsigned w=1; w<=8; ++w) {
    for(int l=1; l<12; l++) {
      turbo::Workflow flow;
      std::vector<turbo::Task> prev_tasks;
      std::vector<turbo::Task> tasks;

      std::atomic<int> counter {0};
      int level = l;

      for(int i=0; i<level; i++) {
        tasks.clear();
        for(int j=0; j< (1<<i); j++) {
          if(i % 2 == 0) {
            tasks.emplace_back(flow.emplace([&](){ counter++; }) );
          }
          else {
            if(j%2) {
              tasks.emplace_back(flow.emplace([](){ return 1; }));
            }
            else {
              tasks.emplace_back(flow.emplace([](){ return 0; }));
            }
          }
        }

        for(size_t j=0; j<prev_tasks.size(); j++) {
          prev_tasks[j].precede(tasks[2*j]    );
          prev_tasks[j].precede(tasks[2*j + 1]);
        }
        tasks.swap(prev_tasks);
      }

      turbo::Executor executor(w);
      executor.run(flow).wait();

      EXPECT_TRUE(counter == (1<<((level+1)/2)) - 1);
    }
  }
}

//             ---- > B
//             |
//  A -> Cond -
//             |
//             ---- > C

TEST(Cond, DynamicBTreeCondition) {
  for(unsigned w=1; w<=8; ++w) {
    std::atomic<int> counter {0};
    constexpr int max_depth = 6;
    turbo::Workflow flow;
    flow.emplace([&](turbo::Subflow& subflow) {
      counter++;
      conditional_spawn(counter, max_depth, 0, subflow); }
    );
    turbo::Executor executor(w);
    executor.run_n(flow, 4).get();
    // Each run increments the counter by (2^(max_depth+1) - 1)
    EXPECT_TRUE(counter.load() == ((1<<(max_depth+1)) - 1)*4);
  }
}

//        ______
//       |      |
//       v      |
//  S -> A -> cond

void nested_cond(unsigned w) {

  const int outer_loop = 3;
  const int mid_loop = 4;
  const int inner_loop = 5;

  int counter {0};
  turbo::Workflow flow;
  auto S = flow.emplace([](){});
  auto A = flow.emplace([&] (turbo::Subflow& subflow) mutable {
    //         ___________
    //        |           |
    //        v           |
    //   S -> A -> B -> cond
    auto S = subflow.emplace([](){ });
    auto A = subflow.emplace([](){ }).succeed(S);
    auto B = subflow.emplace([&](turbo::Subflow& subflow){

      //         ___________
      //        |           |
      //        v           |
      //   S -> A -> B -> cond
      //        |
      //        -----> C
      //        -----> D
      //        -----> E

      auto S = subflow.emplace([](){});
      auto A = subflow.emplace([](){}).succeed(S);
      auto B = subflow.emplace([&](){ counter++; }).succeed(A);
      subflow.emplace([&, repeat=0]() mutable {
        if(repeat ++ < inner_loop)
          return 0;

        repeat = 0;
        return 1;
      }).succeed(B).precede(A).name("cond");

      // Those are redundant tasks
      subflow.emplace([](){}).succeed(A).name("C");
      subflow.emplace([](){}).succeed(A).name("D");
      subflow.emplace([](){}).succeed(A).name("E");
    }).succeed(A);
    subflow.emplace([&, repeat=0]() mutable {
      if(repeat ++ < mid_loop)
        return 0;

      repeat = 0;
      return 1;
    }).succeed(B).precede(A).name("cond");

  }).succeed(S);

  flow.emplace(
    [&, repeat=0]() mutable {
      if(repeat ++ < outer_loop) {
        return 0;
      }

      repeat = 0;
      return 1;
    }
  ).succeed(A).precede(A);

  turbo::Executor executor(w);
  const int repeat = 10;
  executor.run_n(flow, repeat).get();

  EXPECT_TRUE(counter == (inner_loop+1)*(mid_loop+1)*(outer_loop+1)*repeat);
}

TEST(NestedCond, 1thread) {
  nested_cond(1);
}

TEST(NestedCond, 2threads) {
  nested_cond(2);
}

TEST(NestedCond, 3threads) {
  nested_cond(3);
}

TEST(NestedCond, 4threads) {
  nested_cond(4);
}

TEST(NestedCond, 5threads) {
  nested_cond(5);
}

TEST(NestedCond, 6threads) {
  nested_cond(6);
}

TEST(NestedCond, 7threads) {
  nested_cond(7);
}

TEST(NestedCond, 8threads) {
  nested_cond(8);
}

//         ________________
//        |  ___   ______  |
//        | |   | |      | |
//        v v   | v      | |
//   S -> A -> cond1 -> cond2 -> D
//               |
//                ----> B

void cond2cond(unsigned w) {

  const int repeat = 10;
  turbo::Workflow flow;

  int num_visit_A {0};
  int num_visit_C1 {0};
  int num_visit_C2 {0};

  int iteration_C1 {0};
  int iteration_C2 {0};

  auto S = flow.emplace([](){});
  auto A = flow.emplace([&](){ num_visit_A++; }).succeed(S);
  auto cond1 = flow.emplace([&]() mutable {
    num_visit_C1++;
    iteration_C1++;
    if(iteration_C1 == 1) return 0;
    return 1;
  }).succeed(A).precede(A);

  auto cond2 = flow.emplace([&]() mutable {
    num_visit_C2 ++;
    return iteration_C2++;
  }).succeed(cond1).precede(cond1, A);

  flow.emplace([](){ EXPECT_TRUE(false); }).succeed(cond1).name("B");
  flow.emplace([&](){
    iteration_C1 = 0;
    iteration_C2 = 0;
  }).succeed(cond2).name("D");

  turbo::Executor executor(w);
  executor.run_n(flow, repeat).get();

  EXPECT_TRUE(num_visit_A  == 3*repeat);
  EXPECT_TRUE(num_visit_C1 == 4*repeat);
  EXPECT_TRUE(num_visit_C2 == 3*repeat);

}

TEST(Cond2Cond, 1thread) {
  cond2cond(1);
}

TEST(Cond2Cond, 2threads) {
  cond2cond(2);
}

TEST(Cond2Cond, 3threads) {
  cond2cond(3);
}

TEST(Cond2Cond, 4threads) {
  cond2cond(4);
}

TEST(Cond2Cond, 5threads) {
  cond2cond(5);
}

TEST(Cond2Cond, 6threads) {
  cond2cond(6);
}

TEST(Cond2Cond, 7threads) {
  cond2cond(7);
}

TEST(Cond2Cond, 8threads) {
  cond2cond(8);
}


void hierarchical_condition(unsigned w) {

  turbo::Executor executor(w);
  turbo::Workflow tf0("c0");
  turbo::Workflow tf1("c1");
  turbo::Workflow tf2("c2");
  turbo::Workflow tf3("top");

  int c1, c2, c2_repeat;

  auto c1A = tf1.emplace( [&](){ c1=0; } );
  auto c1B = tf1.emplace( [&, state=0] () mutable {
    EXPECT_TRUE(state++ % 100 == c1 % 100);
  });
  auto c1C = tf1.emplace( [&](){ return (++c1 < 100) ? 0 : 1; });

  c1A.precede(c1B);
  c1B.precede(c1C);
  c1C.precede(c1B);
  c1A.name("c1A");
  c1B.name("c1B");
  c1C.name("c1C");

  auto c2A = tf2.emplace( [&](){ EXPECT_TRUE(c2 == 100); c2 = 0; } );
  auto c2B = tf2.emplace( [&, state=0] () mutable {
      EXPECT_TRUE((state++ % 100) == (c2 % 100));
  });
  auto c2C = tf2.emplace( [&](){ return (++c2 < 100) ? 0 : 1; });

  c2A.precede(c2B);
  c2B.precede(c2C);
  c2C.precede(c2B);
  c2A.name("c2A");
  c2B.name("c2B");
  c2C.name("c2C");

  auto init = tf3.emplace([&](){
    c1=c2=c2_repeat=0;
  }).name("init");

  auto loop1 = tf3.emplace([&](){
    return (++c2 < 100) ? 0 : 1;
  }).name("loop1");

  auto loop2 = tf3.emplace([&](){
    c2 = 0;
    return ++c2_repeat < 100 ? 0 : 1;
  }).name("loop2");

  auto sync = tf3.emplace([&](){
    EXPECT_TRUE(c2==0);
    EXPECT_TRUE(c2_repeat==100);
    c2_repeat = 0;
  }).name("sync");

  auto grab = tf3.emplace([&](){
    EXPECT_TRUE(c1 == 100);
    EXPECT_TRUE(c2 == 0);
    EXPECT_TRUE(c2_repeat == 0);
  }).name("grab");

  auto mod0 = tf3.composed_of(tf0).name("module0");
  auto mod1 = tf3.composed_of(tf1).name("module1");
  auto sbf1 = tf3.emplace([&](turbo::Subflow& sbf){
    auto sbf1_1 = sbf.emplace([](){}).name("sbf1_1");
    auto module1 = sbf.composed_of(tf1).name("module1");
    auto sbf1_2 = sbf.emplace([](){}).name("sbf1_2");
    sbf1_1.precede(module1);
    module1.precede(sbf1_2);
    sbf.join();
  }).name("sbf1");
  auto mod2 = tf3.composed_of(tf2).name("module2");

  init.precede(mod0, sbf1, loop1);
  loop1.precede(loop1, mod2);
  loop2.succeed(mod2).precede(loop1, sync);
  mod0.precede(grab);
  sbf1.precede(mod1);
  mod1.precede(grab);
  sync.precede(grab);

  executor.run(tf3);
  executor.run_n(tf3, 10);
  executor.wait_for_all();

  //tf3.dump(std::cout);
}

TEST(HierCondition, 1thread) {
  hierarchical_condition(1);
}

TEST(HierCondition, 2threads) {
  hierarchical_condition(2);
}

TEST(HierCondition, 3threads) {
  hierarchical_condition(3);
}

TEST(HierCondition, 4threads) {
  hierarchical_condition(4);
}

TEST(HierCondition, 5threads) {
  hierarchical_condition(5);
}

TEST(HierCondition, 6threads) {
  hierarchical_condition(6);
}

TEST(HierCondition, 7threads) {
  hierarchical_condition(7);
}

TEST(HierCondition, 8threads) {
  hierarchical_condition(8);
}

// ----------------------------------------------------------------------------
// CondSubflow
// ----------------------------------------------------------------------------

void condition_subflow(unsigned W) {

  turbo::Workflow taskflow;
  turbo::Executor executor(W);

  const size_t I = 1000;

  std::vector<size_t> data(I);

  size_t i;

  auto init = taskflow.emplace([&](){ i = 0; }).name("init");

  auto subflow = taskflow.emplace([&](turbo::Subflow& sf){
    sf.emplace([&, i](){
      EXPECT_TRUE(i<I);
      data[i] = i*(i+1)/2*123;;
    }).name(std::to_string(i));
    sf.detach();
  }).name("subflow");

  auto cond = taskflow.emplace([&](){
    if(++i < I) return 0;
    return 1;
  }).name("cond");

  auto stop = taskflow.emplace([](){}).name("stop");

  init.precede(subflow);
  subflow.precede(cond);
  cond.precede(subflow, stop);

  executor.run(taskflow).wait();

  EXPECT_TRUE(taskflow.num_tasks() == 4 + I);

  for(size_t i=0; i<data.size(); ++i) {
    EXPECT_TRUE(data[i] == i*(i+1)/2*123);
    data[i] = 0;
  }

  executor.run_n(taskflow, 1);
  executor.run_n(taskflow, 10);
  executor.run_n(taskflow, 100);

  executor.wait_for_all();

  EXPECT_TRUE(taskflow.num_tasks() == 4 + I*100);

  for(size_t i=0; i<data.size(); ++i) {
    EXPECT_TRUE(data[i] == i*(i+1)/2*123);
  }

}

TEST(CondSubflow, 1thread) {
  condition_subflow(1);
}

TEST(CondSubflow, 2threads) {
  condition_subflow(2);
}

TEST(CondSubflow, 3threads) {
  condition_subflow(3);
}

TEST(CondSubflow, 4threads) {
  condition_subflow(4);
}

TEST(CondSubflow, 5threads) {
  condition_subflow(5);
}

TEST(CondSubflow, 6threads) {
  condition_subflow(6);
}

TEST(CondSubflow, 7threads) {
  condition_subflow(7);
}

TEST(CondSubflow, 8threads) {
  condition_subflow(8);
}

// ----------------------------------------------------------------------------
// Multi-conditional tasking
// ----------------------------------------------------------------------------

TEST(MultiCond, Types) {

  turbo::Workflow taskflow;

  auto explicit_mc = [](){
    turbo::InlinedVector<int> v;
    return v;
  };

  auto implicit_mc = []() -> turbo::InlinedVector<int> {
    return {1, 2, 3, 9};
  };

  auto explicit_task = taskflow.emplace(explicit_mc);
  auto implicit_task = taskflow.emplace(implicit_mc);

  static_assert(turbo::is_multi_condition_task_v<decltype(explicit_mc)>);
  static_assert(turbo::is_multi_condition_task_v<decltype(implicit_mc)>);

  EXPECT_TRUE(explicit_task.type() == turbo::TaskType::CONDITION);
  EXPECT_TRUE(implicit_task.type() == turbo::TaskType::CONDITION);
}

// ----------------------------------------------------------------------------
// Testcase: Multiple Branches
// ----------------------------------------------------------------------------

void multiple_branches(unsigned W) {

  turbo::Executor executor(W);
  turbo::Workflow taskflow;

  std::atomic<int> counter{0};

  auto A = taskflow.placeholder();

  for(int i=0; i<100; i++) {
    auto X = taskflow.emplace([&](){ counter.fetch_add(1, std::memory_order_relaxed); });
    auto Y = taskflow.emplace([&](){ counter.fetch_add(1, std::memory_order_relaxed); });
    X.precede(Y);
    A.precede(X);
  }

  int ans = 0;
  turbo::InlinedVector<int> conds;

  for(int i=-10; i<=110; i++) {
    if(::rand() % 10 == 0) {
      conds.push_back(i);
      if(0<=i && i<100) {
        ans++;
      }
    }
  }

  A.work([&]() { return conds; });

  executor.run(taskflow).wait();

  EXPECT_TRUE(2*ans == counter);
}

TEST(MultipleBranches, 1thread) {
  multiple_branches(1);
}

TEST(MultipleBranches, 2threads) {
  multiple_branches(2);
}

TEST(MultipleBranches, 3threads) {
  multiple_branches(3);
}

TEST(MultipleBranches, 4threads) {
  multiple_branches(4);
}

TEST(MultipleBranches, 5threads) {
  multiple_branches(5);
}

TEST(MultipleBranches, 6threads) {
  multiple_branches(6);
}

TEST(MultipleBranches, 7threads) {
  multiple_branches(7);
}

TEST(MultipleBranches, 8threads) {
  multiple_branches(8);
}

// ----------------------------------------------------------------------------
// Testcase: Multiple Loops
// ----------------------------------------------------------------------------

void multiple_loops(unsigned W) {

  turbo::Executor executor(W);
  turbo::Workflow taskflow;

  std::atomic<int> counter{0};

  auto A = taskflow.emplace([](){});
  auto B = taskflow.emplace([&, i=bool{true}, c = int(0)]() mutable -> turbo::InlinedVector<int> {
    if(i) {
      i = false;
      return {0, 1};
    }
    else {
      counter.fetch_add(1, std::memory_order_relaxed);
      return {++c < 10 ? 0 : -1};
    }
  });
  auto C = taskflow.emplace([&, i=bool{true}, c = int(0)]() mutable -> turbo::InlinedVector<int> {
    if(i) {
      i = false;
      return {0, 1};
    }
    else {
      counter.fetch_add(1, std::memory_order_relaxed);
      return {++c < 10 ? 0 : -1};
    }
  });
  auto D = taskflow.emplace([&, i=bool{true}, c = int(0)]() mutable -> turbo::InlinedVector<int> {
    if(i) {
      i = false;
      return {0, 1};
    }
    else {
      counter.fetch_add(1, std::memory_order_relaxed);
      return {++c < 10 ? 0 : -1};
    }
  });
  auto E = taskflow.emplace([&, i=bool{true}, c = int(0)]() mutable -> turbo::InlinedVector<int> {
    if(i) {
      i = false;
      return {0, 1};
    }
    else {
      counter.fetch_add(1, std::memory_order_relaxed);
      return {++c < 10 ? 0 : -1};
    }
  });

  A.precede(B);
  B.precede(B, C);
  C.precede(C, D);
  D.precede(D, E);
  E.precede(E);

  executor.run(taskflow).wait();

  //taskflow.dump(std::cout);

  EXPECT_TRUE(counter == 40);
}

TEST(MultipleLoops, 1thread) {
  multiple_loops(1);
}

TEST(MultipleLoops, 2threads) {
  multiple_loops(2);
}

TEST(MultipleLoops, 3threads) {
  multiple_loops(3);
}

TEST(MultipleLoops, 4threads) {
  multiple_loops(4);
}

TEST(MultipleLoops, 5threads) {
  multiple_loops(5);
}

TEST(MultipleLoops, 6threads) {
  multiple_loops(6);
}

TEST(MultipleLoops, 7threads) {
  multiple_loops(7);
}

TEST(MultipleLoops, 8threads) {
  multiple_loops(8);
}

// ----------------------------------------------------------------------------
// binary tree
// ----------------------------------------------------------------------------

void binary_tree(unsigned w) {

  const int N = 10;

  turbo::Workflow taskflow;
  turbo::Executor executor(w);

  std::atomic<int> counter{0};
  std::vector<turbo::Task> tasks;

  for(int i=1; i<(1<<N); i++) {
    tasks.emplace_back(taskflow.emplace([&counter]() -> turbo::InlinedVector<int>{
      counter.fetch_add(1, std::memory_order_relaxed);
      return {0, 1};
    }));
  }

  for(size_t i=0; i<tasks.size(); i++) {
    size_t l = i*2+1;
    size_t r = l + 1;
    if(l < tasks.size()) tasks[i].precede(tasks[l]);
    if(r < tasks.size()) tasks[i].precede(tasks[r]);
  }

  executor.run_n(taskflow, N).wait();

  EXPECT_TRUE(((1<<N)-1)*N == counter);
}

TEST(MultiCondBinaryTree, 1thread) {
  binary_tree(1);
}

TEST(MultiCondBinaryTree, 2threads) {
  binary_tree(2);
}

TEST(MultiCondBinaryTree, 3threads) {
  binary_tree(3);
}

TEST(MultiCondBinaryTree, 4threads) {
  binary_tree(4);
}

































