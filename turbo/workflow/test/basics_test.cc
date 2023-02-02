#define DOCTEST_CONFIG_IMPLEMENT_WITH_MAIN

#include "doctest.h"
#include <turbo/workflow/workflow.h>
#include <turbo/workflow/algorithm/reduce.h>

// --------------------------------------------------------
// Testcase: Type
// --------------------------------------------------------
TEST_CASE("Type" * doctest::timeout(300)) {

  turbo::Workflow taskflow, taskflow2;

  auto t1 = taskflow.emplace([](){});
  auto t2 = taskflow.emplace([](){ return 1; });
  auto t3 = taskflow.emplace([](turbo::Subflow&){ });
  auto t4 = taskflow.composed_of(taskflow2);
  auto t5 = taskflow.emplace([](){ return turbo::InlinedVector{1, 2}; });
  auto t6 = taskflow.emplace([](turbo::Runtime&){});

  REQUIRE(t1.type() == turbo::TaskType::STATIC);
  REQUIRE(t2.type() == turbo::TaskType::CONDITION);
  REQUIRE(t3.type() == turbo::TaskType::DYNAMIC);
  REQUIRE(t4.type() == turbo::TaskType::MODULE);
  REQUIRE(t5.type() == turbo::TaskType::CONDITION);
  REQUIRE(t6.type() == turbo::TaskType::RUNTIME);
}

// --------------------------------------------------------
// Testcase: Builder
// --------------------------------------------------------
TEST_CASE("Builder" * doctest::timeout(300)) {

  const size_t num_tasks = 100;

  turbo::Workflow taskflow;
  turbo::Executor executor;

  std::atomic<int> counter {0};
  std::vector<turbo::Task> silent_tasks;
  std::vector<turbo::Task> tasks;

  SUBCASE("EmptyFlow") {
    for(unsigned W=1; W<32; ++W) {
      turbo::Executor executor(W);
      turbo::Workflow taskflow;
      REQUIRE(taskflow.num_tasks() == 0);
      REQUIRE(taskflow.empty() == true);
      executor.run(taskflow).wait();
    }
  }

  SUBCASE("Placeholder") {

    for(size_t i=0; i<num_tasks; ++i) {
      silent_tasks.emplace_back(taskflow.placeholder().name(std::to_string(i)));
    }

    for(size_t i=0; i<num_tasks; ++i) {
      REQUIRE(silent_tasks[i].name() == std::to_string(i));
      REQUIRE(silent_tasks[i].num_dependents() == 0);
      REQUIRE(silent_tasks[i].num_successors() == 0);
    }

    for(auto& task : silent_tasks) {
      task.work([&counter](){ counter++; });
    }

    executor.run(taskflow).get();
    REQUIRE(counter == num_tasks);
  }

  SUBCASE("EmbarrassinglyParallel"){

    for(size_t i=0;i<num_tasks;i++) {
      tasks.emplace_back(taskflow.emplace([&counter]() {counter += 1;}));
    }

    REQUIRE(taskflow.num_tasks() == num_tasks);
    executor.run(taskflow).get();
    REQUIRE(counter == num_tasks);
    REQUIRE(taskflow.num_tasks() == 100);

    counter = 0;

    for(size_t i=0;i<num_tasks;i++){
      silent_tasks.emplace_back(taskflow.emplace([&counter]() {counter += 1;}));
    }

    REQUIRE(taskflow.num_tasks() == num_tasks * 2);
    executor.run(taskflow).get();
    REQUIRE(counter == num_tasks * 2);
    REQUIRE(taskflow.num_tasks() == 200);
  }

  SUBCASE("BinarySequence"){
    for(size_t i=0;i<num_tasks;i++){
      if(i%2 == 0){
        tasks.emplace_back(
          taskflow.emplace([&counter]() { REQUIRE(counter == 0); counter += 1;})
        );
      }
      else{
        tasks.emplace_back(
          taskflow.emplace([&counter]() { REQUIRE(counter == 1); counter -= 1;})
        );
      }
      if(i>0){
        //tasks[i-1].first.precede(tasks[i].first);
        tasks[i-1].precede(tasks[i]);
      }

      if(i==0) {
        //REQUIRE(tasks[i].first.num_dependents() == 0);
        REQUIRE(tasks[i].num_dependents() == 0);
      }
      else {
        //REQUIRE(tasks[i].first.num_dependents() == 1);
        REQUIRE(tasks[i].num_dependents() == 1);
      }
    }
    executor.run(taskflow).get();
  }

  SUBCASE("LinearCounter"){
    for(size_t i=0;i<num_tasks;i++){
      tasks.emplace_back(
        taskflow.emplace([&counter, i]() {
          REQUIRE(counter == i); counter += 1;}
        )
      );
      if(i>0) {
        tasks[i-1].precede(tasks[i]);
      }
    }
    executor.run(taskflow).get();
    REQUIRE(counter == num_tasks);
    REQUIRE(taskflow.num_tasks() == num_tasks);
  }

  SUBCASE("Broadcast"){
    auto src = taskflow.emplace([&counter]() {counter -= 1;});
    for(size_t i=1; i<num_tasks; i++){
      silent_tasks.emplace_back(
        taskflow.emplace([&counter]() {REQUIRE(counter == -1);})
      );
      src.precede(silent_tasks.back());
    }
    executor.run(taskflow).get();
    REQUIRE(counter == - 1);
    REQUIRE(taskflow.num_tasks() == num_tasks);
  }

  SUBCASE("Succeed"){
    auto dst = taskflow.emplace([&]() { REQUIRE(counter == num_tasks - 1);});
    for(size_t i=1;i<num_tasks;i++){
      silent_tasks.emplace_back(
        taskflow.emplace([&counter]() {counter += 1;})
      );
      dst.succeed(silent_tasks.back());
    }
    executor.run(taskflow).get();
    REQUIRE(counter == num_tasks - 1);
    REQUIRE(taskflow.num_tasks() == num_tasks);
  }

  SUBCASE("MapReduce"){

    auto src = taskflow.emplace([&counter]() {counter = 0;});

    auto dst = taskflow.emplace(
      [&counter, num_tasks]() { REQUIRE(counter == num_tasks);}
    );

    for(size_t i=0;i<num_tasks;i++){
      silent_tasks.emplace_back(
        taskflow.emplace([&counter]() {counter += 1;})
      );
      src.precede(silent_tasks.back());
      dst.succeed(silent_tasks.back());
    }
    executor.run(taskflow).get();
    REQUIRE(taskflow.num_tasks() == num_tasks + 2);
  }

  SUBCASE("Linearize"){
    for(size_t i=0;i<num_tasks;i++){
      silent_tasks.emplace_back(
        taskflow.emplace([&counter, i]() {
          REQUIRE(counter == i); counter += 1;}
        )
      );
    }
    taskflow.linearize(silent_tasks);
    executor.run(taskflow).get();
    REQUIRE(counter == num_tasks);
    REQUIRE(taskflow.num_tasks() == num_tasks);
  }

  SUBCASE("Kite"){
    auto src = taskflow.emplace([&counter]() {counter = 0;});
    for(size_t i=0;i<num_tasks;i++){
      silent_tasks.emplace_back(
        taskflow.emplace([&counter, i]() {
          REQUIRE(counter == i); counter += 1; }
        )
      );
      src.precede(silent_tasks.back());
    }
    taskflow.linearize(silent_tasks);
    auto dst = taskflow.emplace(
      [&counter, num_tasks]() { REQUIRE(counter == num_tasks);}
    );

    for(auto task : silent_tasks) dst.succeed(task);
    executor.run(taskflow).get();
    REQUIRE(taskflow.num_tasks() == num_tasks + 2);
  }
}

// --------------------------------------------------------
// Testcase: Creation
// --------------------------------------------------------
TEST_CASE("Creation" * doctest::timeout(300)) {

  std::vector<int> dummy(1000, -1);

  auto create_taskflow = [&] () {
    for(int i=0; i<10; ++i) {
      turbo::Workflow tf;
      tf.for_each(dummy.begin(), dummy.end(), [] (int) {});
    }
  };

  SUBCASE("One") {
    create_taskflow();
    REQUIRE(dummy.size() == 1000);
    for(auto item : dummy) {
      REQUIRE(item == -1);
    }
  }

  SUBCASE("Two") {
    std::thread t1(create_taskflow);
    std::thread t2(create_taskflow);
    t1.join();
    t2.join();
    REQUIRE(dummy.size() == 1000);
    for(auto item : dummy) {
      REQUIRE(item == -1);
    }
  }

  SUBCASE("Four") {
    std::thread t1(create_taskflow);
    std::thread t2(create_taskflow);
    std::thread t3(create_taskflow);
    std::thread t4(create_taskflow);
    t1.join();
    t2.join();
    t3.join();
    t4.join();
    REQUIRE(dummy.size() == 1000);
    for(auto item : dummy) {
      REQUIRE(item == -1);
    }
  }

}

// --------------------------------------------------------
// Testcase: STDFunction
// --------------------------------------------------------
TEST_CASE("STDFunction" * doctest::timeout(300)) {

  turbo::Workflow taskflow;
  turbo::Executor executor;

  int counter = 0;

  std::function<void()> func1  = [&] () { ++counter; };
  std::function<int()>  func2  = [&] () { ++counter; return 0; };
  std::function<void()> func3  = [&] () { };
  std::function<void()> func4  = [&] () { ++counter;};

  // scenario 1
  auto A = taskflow.emplace(func1);
  auto B = taskflow.emplace(func2);
  auto C = taskflow.emplace(func3);
  auto D = taskflow.emplace(func4);
  A.precede(B);
  B.precede(C, D);
  executor.run(taskflow).wait();
  REQUIRE(counter == 2);

  // scenario 2
  counter = 0;
  A.work(func1);
  B.work(func2);
  C.work(func4);
  D.work(func3);
  executor.run(taskflow).wait();
  REQUIRE(counter == 3);

  // scenario 3
  taskflow.clear();
  std::tie(A, B, C, D) = taskflow.emplace(
    std::move(func1), std::move(func2), std::move(func3), std::move(func4)
  );
  A.precede(B);
  B.precede(C, D);
  counter = 0;
  executor.run(taskflow).wait();
  REQUIRE(counter == 2);
}

// --------------------------------------------------------
// Testcase: Iterators
// --------------------------------------------------------
TEST_CASE("Iterators" * doctest::timeout(300)) {

  SUBCASE("Order") {
    turbo::Workflow taskflow;

    auto A = taskflow.emplace([](){}).name("A");
    auto B = taskflow.emplace([](){}).name("B");
    auto C = taskflow.emplace([](){}).name("C");
    auto D = taskflow.emplace([](){}).name("D");
    auto E = taskflow.emplace([](){}).name("E");

    A.precede(B, C, D, E);
    E.succeed(B, C, D);

    A.for_each_successor([&, i=0] (turbo::Task s) mutable {
      switch(i++) {
        case 0:
          REQUIRE(s == B);
        break;
        case 1:
          REQUIRE(s == C);
        break;
        case 2:
          REQUIRE(s == D);
        break;
        case 3:
          REQUIRE(s == E);
        break;
        default:
        break;
      }
    });

    E.for_each_dependent([&, i=0](turbo::Task s) mutable {
      switch(i++) {
        case 0:
          REQUIRE(s == A);
        break;
        case 1:
          REQUIRE(s == B);
        break;
        case 2:
          REQUIRE(s == C);
        break;
        case 3:
          REQUIRE(s == D);
        break;
      }
    });
  }

  SUBCASE("Generic") {
    turbo::Workflow taskflow;

    auto A = taskflow.emplace([](){}).name("A");
    auto B = taskflow.emplace([](){}).name("B");
    auto C = taskflow.emplace([](){}).name("C");
    auto D = taskflow.emplace([](){}).name("D");
    auto E = taskflow.emplace([](){}).name("E");

    std::vector<turbo::Task> tasks;

    taskflow.for_each_task([&tasks](turbo::Task s){
      tasks.push_back(s);
    });

    REQUIRE(std::find(tasks.begin(), tasks.end(), A) != tasks.end());

    A.precede(B);

    A.for_each_successor([B](turbo::Task s){ REQUIRE(s==B); });
    B.for_each_dependent([A](turbo::Task s){ REQUIRE(s==A); });

    A.precede(C);
    A.precede(D);
    A.precede(E);
    C.precede(B);
    D.precede(B);
    E.precede(B);

    int counter{0}, a{0}, b{0}, c{0}, d{0}, e{0};
    A.for_each_successor([&](turbo::Task s) {
      counter++;
      if(s == A) ++a;
      if(s == B) ++b;
      if(s == C) ++c;
      if(s == D) ++d;
      if(s == E) ++e;
    });
    REQUIRE(counter == A.num_successors());
    REQUIRE(a==0);
    REQUIRE(b==1);
    REQUIRE(c==1);
    REQUIRE(d==1);
    REQUIRE(e==1);

    counter = a = b = c = d = e = 0;
    B.for_each_dependent([&](turbo::Task s) {
      counter++;
      if(s == A) ++a;
      if(s == B) ++b;
      if(s == C) ++c;
      if(s == D) ++d;
      if(s == E) ++e;
    });

    REQUIRE(counter == B.num_dependents());
    REQUIRE(a == 1);
    REQUIRE(b == 0);
    REQUIRE(c == 1);
    REQUIRE(d == 1);
    REQUIRE(e == 1);

    A.for_each_successor([](turbo::Task s){
      s.name("A");
    });

    REQUIRE(A.name() == "A");
    REQUIRE(B.name() == "A");
    REQUIRE(C.name() == "A");
    REQUIRE(D.name() == "A");
    REQUIRE(E.name() == "A");

    B.for_each_dependent([](turbo::Task s){
      s.name("B");
    });

    REQUIRE(A.name() == "B");
    REQUIRE(B.name() == "A");
    REQUIRE(C.name() == "B");
    REQUIRE(D.name() == "B");
    REQUIRE(E.name() == "B");

  }
}

// --------------------------------------------------------
// Testcase: Hash
// --------------------------------------------------------
TEST_CASE("Hash" * doctest::timeout(300)) {

  std::hash<turbo::Task> hash;

  // empty hash
  turbo::Task t1, t2;

  REQUIRE(hash(t1) == hash(t2));

  turbo::Workflow taskflow;

  t1 = taskflow.emplace([](){});

  REQUIRE(((hash(t1) != hash(t2)) || (hash(t1) == hash(t2) && t1 != t2)));

  t2 = taskflow.emplace([](){});

  REQUIRE(((hash(t1) != hash(t2)) || (hash(t1) == hash(t2) && t1 != t2)));

  t2 = t1;

  REQUIRE(hash(t1) == hash(t2));
}

// --------------------------------------------------------
// Testcase: SequentialRuns
// --------------------------------------------------------
void sequential_runs(unsigned W) {

  using namespace std::chrono_literals;

  size_t num_tasks = 100;

  turbo::Executor executor(W);
  turbo::Workflow taskflow;

  std::atomic<int> counter {0};
  std::vector<turbo::Task> silent_tasks;

  for(size_t i=0;i<num_tasks;i++){
    silent_tasks.emplace_back(
      taskflow.emplace([&counter]() {counter += 1;})
    );
  }

  SUBCASE("RunOnce"){
    auto fu = executor.run(taskflow);
    REQUIRE(taskflow.num_tasks() == num_tasks);
    fu.get();
    REQUIRE(counter == num_tasks);
  }

  SUBCASE("WaitForAll") {
    executor.run(taskflow);
    executor.wait_for_all();
    REQUIRE(counter == num_tasks);
  }

  SUBCASE("RunWithFuture") {

    std::atomic<size_t> count {0};
    turbo::Workflow f;
    auto A = f.emplace([&](){ count ++; });
    auto B = f.emplace([&](turbo::Subflow& subflow){
      count ++;
      auto B1 = subflow.emplace([&](){ count++; });
      auto B2 = subflow.emplace([&](){ count++; });
      auto B3 = subflow.emplace([&](){ count++; });
      B1.precede(B3); B2.precede(B3);
    });
    auto C = f.emplace([&](){ count ++; });
    auto D = f.emplace([&](){ count ++; });

    A.precede(B, C);
    B.precede(D);
    C.precede(D);

    std::list<turbo::Future<void>> fu_list;
    for(size_t i=0; i<500; i++) {
      if(i == 499) {
        executor.run(f).get();   // Synchronize the first 500 runs
        executor.run_n(f, 500);  // Run 500 times more
      }
      else if(i % 2) {
        fu_list.push_back(executor.run(f));
      }
      else {
        fu_list.push_back(executor.run(f, [&, i=i](){
          REQUIRE(count == (i+1)*7); })
        );
      }
    }

    executor.wait_for_all();

    for(auto& fu: fu_list) {
      REQUIRE(fu.valid());
      REQUIRE(fu.wait_for(std::chrono::seconds(1)) == std::future_status::ready);
    }

    REQUIRE(count == 7000);

  }

  SUBCASE("RunWithChange") {
    std::atomic<size_t> count {0};
    turbo::Workflow f;
    auto A = f.emplace([&](){ count ++; });
    auto B = f.emplace([&](turbo::Subflow& subflow){
      count ++;
      auto B1 = subflow.emplace([&](){ count++; });
      auto B2 = subflow.emplace([&](){ count++; });
      auto B3 = subflow.emplace([&](){ count++; });
      B1.precede(B3); B2.precede(B3);
    });
    auto C = f.emplace([&](){ count ++; });
    auto D = f.emplace([&](){ count ++; });

    A.precede(B, C);
    B.precede(D);
    C.precede(D);

    executor.run_n(f, 10).get();
    REQUIRE(count == 70);

    auto E = f.emplace([](){});
    D.precede(E);
    executor.run_n(f, 10).get();
    REQUIRE(count == 140);

    auto F = f.emplace([](){});
    E.precede(F);
    executor.run_n(f, 10);
    executor.wait_for_all();
    REQUIRE(count == 210);

  }

  SUBCASE("RunWithPred") {
    std::atomic<size_t> count {0};
    turbo::Workflow f;
    auto A = f.emplace([&](){ count ++; });
    auto B = f.emplace([&](turbo::Subflow& subflow){
      count ++;
      auto B1 = subflow.emplace([&](){ count++; });
      auto B2 = subflow.emplace([&](){ count++; });
      auto B3 = subflow.emplace([&](){ count++; });
      B1.precede(B3); B2.precede(B3);
    });
    auto C = f.emplace([&](){ count ++; });
    auto D = f.emplace([&](){ count ++; });

    A.precede(B, C);
    B.precede(D);
    C.precede(D);

    executor.run_until(f, [run=10]() mutable { return run-- == 0; },
      [&](){
        REQUIRE(count == 70);
        count = 0;
      }
    ).get();


    executor.run_until(f, [run=10]() mutable { return run-- == 0; },
      [&](){
        REQUIRE(count == 70);
        count = 0;
    });

    executor.run_until(f, [run=10]() mutable { return run-- == 0; },
      [&](){
        REQUIRE(count == 70);
        count = 0;
      }
    ).get();

  }

  SUBCASE("MultipleRuns") {
    std::atomic<size_t> counter(0);

    turbo::Workflow tf1, tf2, tf3, tf4;

    for(size_t n=0; n<16; ++n) {
      tf1.emplace([&](){counter.fetch_add(1, std::memory_order_relaxed);});
    }

    for(size_t n=0; n<1024; ++n) {
      tf2.emplace([&](){counter.fetch_add(1, std::memory_order_relaxed);});
    }

    for(size_t n=0; n<32; ++n) {
      tf3.emplace([&](){counter.fetch_add(1, std::memory_order_relaxed);});
    }

    for(size_t n=0; n<128; ++n) {
      tf4.emplace([&](){counter.fetch_add(1, std::memory_order_relaxed);});
    }

    for(int i=0; i<200; ++i) {
      executor.run(tf1);
      executor.run(tf2);
      executor.run(tf3);
      executor.run(tf4);
    }
    executor.wait_for_all();
    REQUIRE(counter == 240000);

  }
}


TEST_CASE("SerialRuns.1thread" * doctest::timeout(300)) {
  sequential_runs(1);
}

TEST_CASE("SerialRuns.2threads" * doctest::timeout(300)) {
  sequential_runs(2);
}

TEST_CASE("SerialRuns.3threads" * doctest::timeout(300)) {
  sequential_runs(3);
}

TEST_CASE("SerialRuns.4threads" * doctest::timeout(300)) {
  sequential_runs(4);
}

TEST_CASE("SerialRuns.5threads" * doctest::timeout(300)) {
  sequential_runs(5);
}

TEST_CASE("SerialRuns.6threads" * doctest::timeout(300)) {
  sequential_runs(6);
}

TEST_CASE("SerialRuns.7threads" * doctest::timeout(300)) {
  sequential_runs(7);
}

TEST_CASE("SerialRuns.8threads" * doctest::timeout(300)) {
  sequential_runs(8);
}

// --------------------------------------------------------
// Testcase:: RunAndWait
// --------------------------------------------------------

TEST_CASE("RunAndWait.Simple") {
  
  // create an executor and a taskflow
  turbo::Executor executor(2);
  turbo::Workflow taskflow("Demo");

  REQUIRE_THROWS(executor.run_and_wait(taskflow));

  int counter{0};
  
  // taskflow to run by the main taskflow
  turbo::Workflow others;
  turbo::Task A = others.emplace([&](){ counter++; });
  turbo::Task B = others.emplace([&](){ counter++; });
  A.precede(B);

  // main taskflow
  turbo::Task C = taskflow.emplace([&](){
    executor.run_and_wait(others);
    REQUIRE(counter == 2);
  });
  turbo::Task D = taskflow.emplace([&](){
    executor.run_and_wait(others);
    REQUIRE(counter == 4);
  });
  C.precede(D);

  executor.run(taskflow).wait();

  // run others again
  executor.run(others).wait();

  REQUIRE(counter == 6);
}

TEST_CASE("RunAndWait.Complex") {

  const size_t N = 100;
  const size_t T = 1000;
  
  // create an executor and a taskflow
  turbo::Executor executor(2);
  turbo::Workflow taskflow;

  std::array<turbo::Workflow, N> taskflows;

  std::atomic<size_t> counter{0};
  
  for(size_t n=0; n<N; n++) {
    for(size_t i=0; i<T; i++) {
      taskflows[n].emplace([&](){ counter++; });
    }
    taskflow.emplace([&executor, &tf=taskflows[n]](){
      executor.run_and_wait(tf);
      //executor.run(tf).wait();
    });
  }

  executor.run(taskflow).wait();

  REQUIRE(counter == T*N);
}

// --------------------------------------------------------
// Testcase: WorkerID
// --------------------------------------------------------
void worker_id(unsigned w) {

  turbo::Workflow taskflow;
  turbo::Executor executor(w);

  for(int i=0; i<1000; i++) {
    auto A = taskflow.emplace([&](){
      auto id = executor.this_worker_id();
      REQUIRE(id>=0);
      REQUIRE(id< w);
    });

    auto B = taskflow.emplace([&](turbo::Subflow& sf){
      auto id = executor.this_worker_id();
      REQUIRE(id>=0);
      REQUIRE(id< w);
      sf.emplace([&](){
        auto id = executor.this_worker_id();
        REQUIRE(id>=0);
        REQUIRE(id< w);
      });
      sf.emplace([&](turbo::Subflow&){
        auto id = executor.this_worker_id();
        REQUIRE(id>=0);
        REQUIRE(id< w);
      });
    });

    A.precede(B);
  }

  executor.run_n(taskflow, 100).wait();
}

TEST_CASE("WorkerID.1thread") {
  worker_id(1);
}

TEST_CASE("WorkerID.2threads") {
  worker_id(2);
}

TEST_CASE("WorkerID.3threads") {
  worker_id(3);
}

TEST_CASE("WorkerID.4threads") {
  worker_id(4);
}

TEST_CASE("WorkerID.5threads") {
  worker_id(5);
}

TEST_CASE("WorkerID.6threads") {
  worker_id(6);
}

TEST_CASE("WorkerID.7threads") {
  worker_id(7);
}

TEST_CASE("WorkerID.8threads") {
  worker_id(8);
}

// --------------------------------------------------------
// Testcase: ParallelRuns
// --------------------------------------------------------
void parallel_runs(unsigned w) {

  std::atomic<int> counter;
  std::vector<std::thread> threads;

  auto make_taskflow = [&] (turbo::Workflow& tf) {
    for(int i=0; i<1024; i++) {
      auto A = tf.emplace([&] () {
        counter.fetch_add(1, std::memory_order_relaxed);
      });
      auto B = tf.emplace([&] () {
        counter.fetch_add(1, std::memory_order_relaxed);
      });
      A.precede(B);
    }
  };

  SUBCASE("RunAndWait") {
    turbo::Executor executor(w);
    counter = 0;
    for(int t=0; t<32; t++) {
      threads.emplace_back([&] () {
        turbo::Workflow taskflow;
        make_taskflow(taskflow);
        executor.run(taskflow).wait();
      });
    }

    for(auto& t : threads) {
      t.join();
    }
    threads.clear();

    REQUIRE(counter.load() == 32*1024*2);

  }

  SUBCASE("RunAndWaitForAll") {
    turbo::Executor executor(w);
    counter = 0;
    std::vector<std::unique_ptr<turbo::Workflow>> taskflows(32);
    std::atomic<size_t> barrier(0);
    for(int t=0; t<32; t++) {
      threads.emplace_back([&, t=t] () {
        taskflows[t] = std::make_unique<turbo::Workflow>();
        make_taskflow(*taskflows[t]);
        executor.run(*taskflows[t]);
        ++barrier;    // make sure all runs are issued
      });
    }

    while(barrier != 32);
    executor.wait_for_all();
    REQUIRE(counter.load() == 32*1024*2);

    for(auto& t : threads) {
      t.join();
    }
    threads.clear();
  }
}


TEST_CASE("ParallelRuns.1thread" * doctest::timeout(300)) {
  parallel_runs(1);
}

TEST_CASE("ParallelRuns.2threads" * doctest::timeout(300)) {
  parallel_runs(2);
}

TEST_CASE("ParallelRuns.3threads" * doctest::timeout(300)) {
  parallel_runs(3);
}

TEST_CASE("ParallelRuns.4threads" * doctest::timeout(300)) {
  parallel_runs(4);
}

TEST_CASE("ParallelRuns.5threads" * doctest::timeout(300)) {
  parallel_runs(5);
}

TEST_CASE("ParallelRuns.6threads" * doctest::timeout(300)) {
  parallel_runs(6);
}

TEST_CASE("ParallelRuns.7threads" * doctest::timeout(300)) {
  parallel_runs(7);
}

TEST_CASE("ParallelRuns.8threads" * doctest::timeout(300)) {
  parallel_runs(8);
}

// --------------------------------------------------------
// Testcase: NestedRuns
// --------------------------------------------------------
void nested_runs(unsigned w) {

  int counter {0};

  struct A {

    turbo::Executor executor;
    turbo::Workflow taskflow;

    int& counter;

    A(unsigned w, int& c) : executor{w}, counter{c} { }

    void run()
    {
      taskflow.clear();
      auto A1 = taskflow.emplace([&]() { counter++; });
      auto A2 = taskflow.emplace([&]() { counter++; });
      A1.precede(A2);
      executor.run_n(taskflow, 10).wait();
    }

  };

  struct B {

    turbo::Workflow taskflow;
    turbo::Executor executor;

    int& counter;

    A a_sim;

    B(unsigned w, int& c) : executor{w}, counter{c}, a_sim{w, c} { }

    void run()
    {
      taskflow.clear();
      auto B1 = taskflow.emplace([&] () { ++counter; });
      auto B2 = taskflow.emplace([&] () { ++counter; a_sim.run(); });
      B1.precede(B2);
      executor.run_n(taskflow, 100).wait();
    }
  };

  struct C {

    turbo::Workflow taskflow;
    turbo::Executor executor;

    int& counter;

    B b_sim;

    C(unsigned w, int& c) : executor{w}, counter{c}, b_sim{w, c} { }

    void run()
    {
      taskflow.clear();
      auto C1 = taskflow.emplace([&] () { ++counter; });
      auto C2 = taskflow.emplace([&] () { ++counter; b_sim.run(); });
      C1.precede(C2);
      executor.run_n(taskflow, 100).wait();
    }
  };

  C c(w, counter);
  c.run();

  REQUIRE(counter == 220200);
}

TEST_CASE("NestedRuns.1thread") {
  nested_runs(1);
}

TEST_CASE("NestedRuns.2threads") {
  nested_runs(2);
}

TEST_CASE("NestedRuns.3threads") {
  nested_runs(3);
}

TEST_CASE("NestedRuns.4threads") {
  nested_runs(4);
}

TEST_CASE("NestedRuns.8threads") {
  nested_runs(8);
}

TEST_CASE("NestedRuns.16threads") {
  nested_runs(16);
}

// --------------------------------------------------------
// Testcase: ParallelFor
// --------------------------------------------------------

void for_each(unsigned W) {

  using namespace std::chrono_literals;

  const auto mapper = [](unsigned w, size_t num_data){
    turbo::Executor executor(w);
    turbo::Workflow tf;
    std::vector<int> vec(num_data, 0);
    tf.for_each(
      vec.begin(), vec.end(), [] (int& v) { v = 64; } /*, group ? ::rand()%17 : 0*/
    );
    for(const auto v : vec) {
      REQUIRE(v == 0);
    }
    executor.run(tf);
    executor.wait_for_all();
    for(const auto v : vec) {
      REQUIRE(v == 64);
    }
  };

  const auto reducer = [](unsigned w, size_t num_data){
    turbo::Executor executor(w);
    turbo::Workflow tf;
    std::vector<int> vec(num_data, 0);
    std::atomic<int> sum(0);
    tf.for_each(vec.begin(), vec.end(), [&](auto) { ++sum; }/*, group ? ::rand()%17 : 0*/);
    REQUIRE(sum == 0);
    executor.run(tf);
    executor.wait_for_all();
    REQUIRE(sum == vec.size());
  };

  // map
  SUBCASE("Map") {
    for(size_t num_data=1; num_data<=59049; num_data *= 3){
      mapper(W, num_data);
    }
  }

  // reduce
  SUBCASE("Reduce") {
    for(size_t num_data=1; num_data<=59049; num_data *= 3){
      reducer(W, num_data);
    }
  }
}

TEST_CASE("ParallelFor.1thread" * doctest::timeout(300)) {
  for_each(1);
}

TEST_CASE("ParallelFor.2threads" * doctest::timeout(300)) {
  for_each(2);
}

TEST_CASE("ParallelFor.3threads" * doctest::timeout(300)) {
  for_each(3);
}

TEST_CASE("ParallelFor.4threads" * doctest::timeout(300)) {
  for_each(4);
}

TEST_CASE("ParallelFor.5threads" * doctest::timeout(300)) {
  for_each(5);
}

TEST_CASE("ParallelFor.6threads" * doctest::timeout(300)) {
  for_each(6);
}

TEST_CASE("ParallelFor.7threads" * doctest::timeout(300)) {
  for_each(7);
}

TEST_CASE("ParallelFor.8threads" * doctest::timeout(300)) {
  for_each(8);
}

// --------------------------------------------------------
// Testcase: ParallelForOnIndex
// --------------------------------------------------------
void for_each_index(unsigned w) {

  using namespace std::chrono_literals;

  auto positive_integer_step = [] (unsigned w) {
    turbo::Executor executor(w);
    for(int beg=-10; beg<=10; ++beg) {
      for(int end=beg; end<=10; ++end) {
        for(int s=1; s<=end-beg; ++s) {
          int n = 0;
          for(int b = beg; b<end; b+=s) {
            ++n;
          }
          //for(size_t c=0; c<10; c++) {
            turbo::Workflow tf;
            std::atomic<int> counter {0};
            tf.for_each_index(beg, end, s, [&] (auto) {
              counter.fetch_add(1, std::memory_order_relaxed);
            }/*, c*/);
            executor.run(tf);
            executor.wait_for_all();
            REQUIRE(n == counter);
          //}
        }
      }
    }
  };

  auto negative_integer_step = [] (unsigned w) {
    turbo::Executor executor(w);
    for(int beg=10; beg>=-10; --beg) {
      for(int end=beg; end>=-10; --end) {
        for(int s=1; s<=beg-end; ++s) {
          int n = 0;
          for(int b = beg; b>end; b-=s) {
            ++n;
          }
          //for(size_t c=0; c<10; c++) {
            turbo::Workflow tf;
            std::atomic<int> counter {0};
            tf.for_each_index(beg, end, -s, [&] (auto) {
              counter.fetch_add(1, std::memory_order_relaxed);
            }/*, c*/);
            executor.run(tf);
            executor.wait_for_all();
            REQUIRE(n == counter);
          //}
        }
      }
    }
  };

  SUBCASE("PositiveIntegerStep") {
    positive_integer_step(w);
  }

  SUBCASE("NegativeIntegerStep") {
    negative_integer_step(w);
  }

  //SUBCASE("PositiveFloatingStep") {
  //  positive_floating_step(w);
  //}
  //
  //SUBCASE("NegativeFloatingStep") {
  //  negative_floating_step(w);
  //}
}

TEST_CASE("ParallelForIndex.1thread" * doctest::timeout(300)) {
  for_each_index(1);
}

TEST_CASE("ParallelForIndex.2threads" * doctest::timeout(300)) {
  for_each_index(2);
}

TEST_CASE("ParallelForIndex.3threads" * doctest::timeout(300)) {
  for_each_index(3);
}

TEST_CASE("ParallelForIndex.4threads" * doctest::timeout(300)) {
  for_each_index(4);
}

TEST_CASE("ParallelForIndex.5threads" * doctest::timeout(300)) {
  for_each_index(5);
}

TEST_CASE("ParallelForIndex.6threads" * doctest::timeout(300)) {
  for_each_index(6);
}

TEST_CASE("ParallelForIndex.7threads" * doctest::timeout(300)) {
  for_each_index(7);
}

TEST_CASE("ParallelForIndex.8threads" * doctest::timeout(300)) {
  for_each_index(8);
}

// --------------------------------------------------------
// Testcase: Reduce
// --------------------------------------------------------
TEST_CASE("Reduce" * doctest::timeout(300)) {

  const auto plus_test = [](unsigned num_workers, auto &&data){
    turbo::Executor executor(num_workers);
    turbo::Workflow tf;
    int result {0};
    std::iota(data.begin(), data.end(), 1);
    tf.reduce(data.begin(), data.end(), result, std::plus<int>());
    executor.run(tf).get();
    REQUIRE(result == std::accumulate(data.begin(), data.end(), 0, std::plus<int>()));
  };

  const auto multiply_test = [](unsigned num_workers, auto &&data){
    turbo::Executor executor(num_workers);
    turbo::Workflow tf;
    std::fill(data.begin(), data.end(), 1.0);
    double result {2.0};
    tf.reduce(data.begin(), data.end(), result, std::multiplies<double>());
    executor.run(tf).get();
    REQUIRE(result == std::accumulate(data.begin(), data.end(), 2.0, std::multiplies<double>()));
  };

  const auto max_test = [](unsigned num_workers, auto &&data){
    turbo::Executor executor(num_workers);
    turbo::Workflow tf;
    std::iota(data.begin(), data.end(), 1);
    int result {0};
    auto lambda = [](const auto& l, const auto& r){return std::max(l, r);};
    tf.reduce(data.begin(), data.end(), result, lambda);
    executor.run(tf).get();
    REQUIRE(result == std::accumulate(data.begin(), data.end(), 0, lambda));
  };

  const auto min_test = [](unsigned num_workers, auto &&data){
    turbo::Executor executor(num_workers);
    turbo::Workflow tf;
    std::iota(data.begin(), data.end(), 1);
    int result {std::numeric_limits<int>::max()};
    auto lambda = [](const auto& l, const auto& r){return std::min(l, r);};
    tf.reduce(data.begin(), data.end(), result, lambda);
    executor.run(tf).get();
    REQUIRE(result == std::accumulate(
      data.begin(), data.end(), std::numeric_limits<int>::max(), lambda)
    );
  };

  for(unsigned w=1; w<=4; ++w){
    for(size_t j=0; j<=256; j=j*2+1){
      plus_test(w, std::vector<int>(j));
      plus_test(w, std::list<int>(j));

      multiply_test(w, std::vector<double>(j));
      multiply_test(w, std::list<double>(j));

      max_test(w, std::vector<int>(j));
      max_test(w, std::list<int>(j));

      min_test(w, std::vector<int>(j));
      min_test(w, std::list<int>(j));
    }
  }
}

// --------------------------------------------------------
// Testcase: ReduceMin
// --------------------------------------------------------
TEST_CASE("ReduceMin" * doctest::timeout(300)) {

  for(int w=1; w<=4; w++) {
    turbo::Executor executor(w);
    for(int i=0; i<=65536; i = (i <= 1024) ? i + 1 : i*2 + 1) {
      turbo::Workflow tf;
      std::vector<int> data(i);
      int gold = std::numeric_limits<int>::max();
      int test = std::numeric_limits<int>::max();
      for(auto& d : data) {
        d = ::rand();
        gold = std::min(gold, d);
      }
      tf.reduce(data.begin(), data.end(), test, [] (int l, int r) {
        return std::min(l, r);
      });
      executor.run(tf).get();
      REQUIRE(test == gold);
    }
  }

}

// --------------------------------------------------------
// Testcase: ReduceMax
// --------------------------------------------------------
TEST_CASE("ReduceMax" * doctest::timeout(300)) {

  for(int w=1; w<=4; w++) {
    turbo::Executor executor(w);
    for(int i=0; i<=65536; i = (i <= 1024) ? i + 1 : i*2 + 1) {
      turbo::Workflow tf;
      std::vector<int> data(i);
      int gold = std::numeric_limits<int>::min();
      int test = std::numeric_limits<int>::min();
      for(auto& d : data) {
        d = ::rand();
        gold = std::max(gold, d);
      }
      tf.reduce(data.begin(), data.end(), test, [](int l, int r){
        return std::max(l, r);
      });
      executor.run(tf).get();
      REQUIRE(test == gold);
    }
  }
}

// --------------------------------------------------------
// Testcase: Observer
// --------------------------------------------------------

void observer(unsigned w) {

  turbo::Executor executor(w);

  auto observer = executor.make_observer<turbo::ChromeObserver>();

  turbo::Workflow taskflowA;
  std::vector<turbo::Task> tasks;
  // Static tasking
  for(auto i=0; i < 64; i ++) {
    tasks.emplace_back(taskflowA.emplace([](){}));
  }

  // Randomly specify dependency
  for(auto i=0; i < 64; i ++) {
    for(auto j=i+1; j < 64; j++) {
      if(rand()%2 == 0) {
        tasks[i].precede(tasks[j]);
      }
    }
  }

  executor.run_n(taskflowA, 16).get();

  REQUIRE(observer->num_tasks() == 64*16);

  observer->clear();
  REQUIRE(observer->num_tasks() == 0);
  tasks.clear();

}

TEST_CASE("Observer.1thread" * doctest::timeout(300)) {
  observer(1);
}

TEST_CASE("Observer.2threads" * doctest::timeout(300)) {
  observer(2);
}

TEST_CASE("Observer.3threads" * doctest::timeout(300)) {
  observer(3);
}

TEST_CASE("Observer.4threads" * doctest::timeout(300)) {
  observer(4);
}


