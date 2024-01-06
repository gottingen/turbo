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
#define DOCTEST_CONFIG_IMPLEMENT_WITH_MAIN

#include "turbo/testing/test.h"
#include <turbo/taskflow/taskflow.h>
#include <turbo/taskflow/algorithm/for_each.h>

// --------------------------------------------------------
// Testcase: for_each
// --------------------------------------------------------

template <typename P>
void for_each(unsigned W) {

  turbo::Executor executor(W);
  turbo::Taskflow taskflow;

  std::vector<int> vec(1024);
  for(int n = 0; n <= 150; n++) {

    std::fill_n(vec.begin(), vec.size(), -1);

    int beg = ::rand()%300 - 150;
    int end = beg + n;

    for(int s=1; s<=16; s*=2) {
      for(size_t c : {0, 1, 3, 7, 99}) {
        taskflow.clear();
        std::atomic<int> counter {0};

        taskflow.for_each_index(
          beg, end, s, [&](int i){ counter++; vec[i-beg] = i;}, P(c)
        );

        executor.run(taskflow).wait();
        REQUIRE(counter == (n + s - 1) / s);

        for(int i=beg; i<end; i+=s) {
          REQUIRE(vec[i-beg] == i);
          vec[i-beg] = -1;
        }

        for(const auto i : vec) {
          REQUIRE(i == -1);
        }
      }
    }
  }

  for(size_t n = 0; n < 150; n++) {
    for(size_t c : {0, 1, 3, 7, 99}) {

      std::fill_n(vec.begin(), vec.size(), -1);

      taskflow.clear();
      std::atomic<int> counter {0};

      taskflow.for_each(
        vec.begin(), vec.begin() + n, [&](int& i){
        counter++;
        i = 1;
      }, P(c));

      executor.run(taskflow).wait();
      REQUIRE(counter == n);

      for(size_t i=0; i<n; ++i) {
        REQUIRE(vec[i] == 1);
      }

      for(size_t i=n; i<vec.size(); ++i) {
        REQUIRE(vec[i] == -1);
      }
    }
  }
}

// guided
TEST_CASE("ParallelFor.Guided.1thread" * doctest::timeout(300)) {
  for_each<turbo::GuidedPartitioner>(1);
}

TEST_CASE("ParallelFor.Guided.2threads" * doctest::timeout(300)) {
  for_each<turbo::GuidedPartitioner>(2);
}

TEST_CASE("ParallelFor.Guided.3threads" * doctest::timeout(300)) {
  for_each<turbo::GuidedPartitioner>(3);
}

TEST_CASE("ParallelFor.Guided.4threads" * doctest::timeout(300)) {
  for_each<turbo::GuidedPartitioner>(4);
}

TEST_CASE("ParallelFor.Guided.5threads" * doctest::timeout(300)) {
  for_each<turbo::GuidedPartitioner>(5);
}

TEST_CASE("ParallelFor.Guided.6threads" * doctest::timeout(300)) {
  for_each<turbo::GuidedPartitioner>(6);
}

TEST_CASE("ParallelFor.Guided.7threads" * doctest::timeout(300)) {
  for_each<turbo::GuidedPartitioner>(7);
}

TEST_CASE("ParallelFor.Guided.8threads" * doctest::timeout(300)) {
  for_each<turbo::GuidedPartitioner>(8);
}

TEST_CASE("ParallelFor.Guided.9threads" * doctest::timeout(300)) {
  for_each<turbo::GuidedPartitioner>(9);
}

TEST_CASE("ParallelFor.Guided.10threads" * doctest::timeout(300)) {
  for_each<turbo::GuidedPartitioner>(10);
}

TEST_CASE("ParallelFor.Guided.11threads" * doctest::timeout(300)) {
  for_each<turbo::GuidedPartitioner>(11);
}

TEST_CASE("ParallelFor.Guided.12threads" * doctest::timeout(300)) {
  for_each<turbo::GuidedPartitioner>(12);
}

// dynamic
TEST_CASE("ParallelFor.Dynamic.1thread" * doctest::timeout(300)) {
  for_each<turbo::DynamicPartitioner>(1);
}

TEST_CASE("ParallelFor.Dynamic.2threads" * doctest::timeout(300)) {
  for_each<turbo::DynamicPartitioner>(2);
}

TEST_CASE("ParallelFor.Dynamic.3threads" * doctest::timeout(300)) {
  for_each<turbo::DynamicPartitioner>(3);
}

TEST_CASE("ParallelFor.Dynamic.4threads" * doctest::timeout(300)) {
  for_each<turbo::DynamicPartitioner>(4);
}

TEST_CASE("ParallelFor.Dynamic.5threads" * doctest::timeout(300)) {
  for_each<turbo::DynamicPartitioner>(5);
}

TEST_CASE("ParallelFor.Dynamic.6threads" * doctest::timeout(300)) {
  for_each<turbo::DynamicPartitioner>(6);
}

TEST_CASE("ParallelFor.Dynamic.7threads" * doctest::timeout(300)) {
  for_each<turbo::DynamicPartitioner>(7);
}

TEST_CASE("ParallelFor.Dynamic.8threads" * doctest::timeout(300)) {
  for_each<turbo::DynamicPartitioner>(8);
}

TEST_CASE("ParallelFor.Dynamic.9threads" * doctest::timeout(300)) {
  for_each<turbo::DynamicPartitioner>(9);
}

TEST_CASE("ParallelFor.Dynamic.10threads" * doctest::timeout(300)) {
  for_each<turbo::DynamicPartitioner>(10);
}

TEST_CASE("ParallelFor.Dynamic.11threads" * doctest::timeout(300)) {
  for_each<turbo::DynamicPartitioner>(11);
}

TEST_CASE("ParallelFor.Dynamic.12threads" * doctest::timeout(300)) {
  for_each<turbo::DynamicPartitioner>(12);
}

// static
TEST_CASE("ParallelFor.Static.1thread" * doctest::timeout(300)) {
  for_each<turbo::StaticPartitioner>(1);
}

TEST_CASE("ParallelFor.Static.2threads" * doctest::timeout(300)) {
  for_each<turbo::StaticPartitioner>(2);
}

TEST_CASE("ParallelFor.Static.3threads" * doctest::timeout(300)) {
  for_each<turbo::StaticPartitioner>(3);
}

TEST_CASE("ParallelFor.Static.4threads" * doctest::timeout(300)) {
  for_each<turbo::StaticPartitioner>(4);
}

TEST_CASE("ParallelFor.Static.5threads" * doctest::timeout(300)) {
  for_each<turbo::StaticPartitioner>(5);
}

TEST_CASE("ParallelFor.Static.6threads" * doctest::timeout(300)) {
  for_each<turbo::StaticPartitioner>(6);
}

TEST_CASE("ParallelFor.Static.7threads" * doctest::timeout(300)) {
  for_each<turbo::StaticPartitioner>(7);
}

TEST_CASE("ParallelFor.Static.8threads" * doctest::timeout(300)) {
  for_each<turbo::StaticPartitioner>(8);
}

TEST_CASE("ParallelFor.Static.9threads" * doctest::timeout(300)) {
  for_each<turbo::StaticPartitioner>(9);
}

TEST_CASE("ParallelFor.Static.10threads" * doctest::timeout(300)) {
  for_each<turbo::StaticPartitioner>(10);
}

TEST_CASE("ParallelFor.Static.11threads" * doctest::timeout(300)) {
  for_each<turbo::StaticPartitioner>(11);
}

TEST_CASE("ParallelFor.Static.12threads" * doctest::timeout(300)) {
  for_each<turbo::StaticPartitioner>(12);
}

// random
TEST_CASE("ParallelFor.Random.1thread" * doctest::timeout(300)) {
  for_each<turbo::RandomPartitioner>(1);
}

TEST_CASE("ParallelFor.Random.2threads" * doctest::timeout(300)) {
  for_each<turbo::RandomPartitioner>(2);
}

TEST_CASE("ParallelFor.Random.3threads" * doctest::timeout(300)) {
  for_each<turbo::RandomPartitioner>(3);
}

TEST_CASE("ParallelFor.Random.4threads" * doctest::timeout(300)) {
  for_each<turbo::RandomPartitioner>(4);
}

TEST_CASE("ParallelFor.Random.5threads" * doctest::timeout(300)) {
  for_each<turbo::RandomPartitioner>(5);
}

TEST_CASE("ParallelFor.Random.6threads" * doctest::timeout(300)) {
  for_each<turbo::RandomPartitioner>(6);
}

TEST_CASE("ParallelFor.Random.7threads" * doctest::timeout(300)) {
  for_each<turbo::RandomPartitioner>(7);
}

TEST_CASE("ParallelFor.Random.8threads" * doctest::timeout(300)) {
  for_each<turbo::RandomPartitioner>(8);
}

TEST_CASE("ParallelFor.Random.9threads" * doctest::timeout(300)) {
  for_each<turbo::RandomPartitioner>(9);
}

TEST_CASE("ParallelFor.Random.10threads" * doctest::timeout(300)) {
  for_each<turbo::RandomPartitioner>(10);
}

TEST_CASE("ParallelFor.Random.11threads" * doctest::timeout(300)) {
  for_each<turbo::RandomPartitioner>(11);
}

TEST_CASE("ParallelFor.Random.12threads" * doctest::timeout(300)) {
  for_each<turbo::RandomPartitioner>(12);
}

// ----------------------------------------------------------------------------
// stateful_for_each
// ----------------------------------------------------------------------------

template <typename P>
void stateful_for_each(unsigned W) {

  turbo::Executor executor(W);
  turbo::Taskflow taskflow;
  std::vector<int> vec;
  std::atomic<int> counter {0};

  for(size_t n = 0; n <= 150; n++) {
    for(size_t c : {0, 1, 3, 7, 99}) {

      std::vector<int>::iterator beg, end;
      size_t ibeg = 0, iend = 0;
      size_t half = n/2;

      taskflow.clear();

      auto init = taskflow.emplace([&](){
        vec.resize(n);
        std::fill_n(vec.begin(), vec.size(), -1);

        beg = vec.begin();
        end = beg + half;

        ibeg = half;
        iend = n;

        counter = 0;
      });

      turbo::Task pf1, pf2;

      pf1 = taskflow.for_each(
        std::ref(beg), std::ref(end), [&](int& i){
        counter++;
        i = 8;
      }, P(c));

      pf2 = taskflow.for_each_index(
        std::ref(ibeg), std::ref(iend), size_t{1}, [&] (size_t i) {
          counter++;
          vec[i] = -8;
      }, P(c));

      init.precede(pf1, pf2);

      executor.run(taskflow).wait();
      REQUIRE(counter == n);

      for(size_t i=0; i<half; ++i) {
        REQUIRE(vec[i] == 8);
        vec[i] = 0;
      }

      for(size_t i=half; i<n; ++i) {
        REQUIRE(vec[i] == -8);
        vec[i] = 0;
      }
    }
  }
}

// guided
TEST_CASE("StatefulParallelFor.Guided.1thread" * doctest::timeout(300)) {
  stateful_for_each<turbo::GuidedPartitioner>(1);
}

TEST_CASE("StatefulParallelFor.Guided.2threads" * doctest::timeout(300)) {
  stateful_for_each<turbo::GuidedPartitioner>(2);
}

TEST_CASE("StatefulParallelFor.Guided.3threads" * doctest::timeout(300)) {
  stateful_for_each<turbo::GuidedPartitioner>(3);
}

TEST_CASE("StatefulParallelFor.Guided.4threads" * doctest::timeout(300)) {
  stateful_for_each<turbo::GuidedPartitioner>(4);
}

TEST_CASE("StatefulParallelFor.Guided.5threads" * doctest::timeout(300)) {
  stateful_for_each<turbo::GuidedPartitioner>(5);
}

TEST_CASE("StatefulParallelFor.Guided.6threads" * doctest::timeout(300)) {
  stateful_for_each<turbo::GuidedPartitioner>(6);
}

TEST_CASE("StatefulParallelFor.Guided.7threads" * doctest::timeout(300)) {
  stateful_for_each<turbo::GuidedPartitioner>(7);
}

TEST_CASE("StatefulParallelFor.Guided.8threads" * doctest::timeout(300)) {
  stateful_for_each<turbo::GuidedPartitioner>(8);
}

TEST_CASE("StatefulParallelFor.Guided.9threads" * doctest::timeout(300)) {
  stateful_for_each<turbo::GuidedPartitioner>(9);
}

TEST_CASE("StatefulParallelFor.Guided.10threads" * doctest::timeout(300)) {
  stateful_for_each<turbo::GuidedPartitioner>(10);
}

TEST_CASE("StatefulParallelFor.Guided.11threads" * doctest::timeout(300)) {
  stateful_for_each<turbo::GuidedPartitioner>(11);
}

TEST_CASE("StatefulParallelFor.Guided.12threads" * doctest::timeout(300)) {
  stateful_for_each<turbo::GuidedPartitioner>(12);
}

// dynamic
TEST_CASE("StatefulParallelFor.Dynamic.1thread" * doctest::timeout(300)) {
  stateful_for_each<turbo::DynamicPartitioner>(1);
}

TEST_CASE("StatefulParallelFor.Dynamic.2threads" * doctest::timeout(300)) {
  stateful_for_each<turbo::DynamicPartitioner>(2);
}

TEST_CASE("StatefulParallelFor.Dynamic.3threads" * doctest::timeout(300)) {
  stateful_for_each<turbo::DynamicPartitioner>(3);
}

TEST_CASE("StatefulParallelFor.Dynamic.4threads" * doctest::timeout(300)) {
  stateful_for_each<turbo::DynamicPartitioner>(4);
}

TEST_CASE("StatefulParallelFor.Dynamic.5threads" * doctest::timeout(300)) {
  stateful_for_each<turbo::DynamicPartitioner>(5);
}

TEST_CASE("StatefulParallelFor.Dynamic.6threads" * doctest::timeout(300)) {
  stateful_for_each<turbo::DynamicPartitioner>(6);
}

TEST_CASE("StatefulParallelFor.Dynamic.7threads" * doctest::timeout(300)) {
  stateful_for_each<turbo::DynamicPartitioner>(7);
}

TEST_CASE("StatefulParallelFor.Dynamic.8threads" * doctest::timeout(300)) {
  stateful_for_each<turbo::DynamicPartitioner>(8);
}

TEST_CASE("StatefulParallelFor.Dynamic.9threads" * doctest::timeout(300)) {
  stateful_for_each<turbo::DynamicPartitioner>(9);
}

TEST_CASE("StatefulParallelFor.Dynamic.10threads" * doctest::timeout(300)) {
  stateful_for_each<turbo::DynamicPartitioner>(10);
}

TEST_CASE("StatefulParallelFor.Dynamic.11threads" * doctest::timeout(300)) {
  stateful_for_each<turbo::DynamicPartitioner>(11);
}

TEST_CASE("StatefulParallelFor.Dynamic.12threads" * doctest::timeout(300)) {
  stateful_for_each<turbo::DynamicPartitioner>(12);
}

// static
TEST_CASE("StatefulParallelFor.Static.1thread" * doctest::timeout(300)) {
  stateful_for_each<turbo::StaticPartitioner>(1);
}

TEST_CASE("StatefulParallelFor.Static.2threads" * doctest::timeout(300)) {
  stateful_for_each<turbo::StaticPartitioner>(2);
}

TEST_CASE("StatefulParallelFor.Static.3threads" * doctest::timeout(300)) {
  stateful_for_each<turbo::StaticPartitioner>(3);
}

TEST_CASE("StatefulParallelFor.Static.4threads" * doctest::timeout(300)) {
  stateful_for_each<turbo::StaticPartitioner>(4);
}

TEST_CASE("StatefulParallelFor.Static.5threads" * doctest::timeout(300)) {
  stateful_for_each<turbo::StaticPartitioner>(5);
}

TEST_CASE("StatefulParallelFor.Static.6threads" * doctest::timeout(300)) {
  stateful_for_each<turbo::StaticPartitioner>(6);
}

TEST_CASE("StatefulParallelFor.Static.7threads" * doctest::timeout(300)) {
  stateful_for_each<turbo::StaticPartitioner>(7);
}

TEST_CASE("StatefulParallelFor.Static.8threads" * doctest::timeout(300)) {
  stateful_for_each<turbo::StaticPartitioner>(8);
}

TEST_CASE("StatefulParallelFor.Static.9threads" * doctest::timeout(300)) {
  stateful_for_each<turbo::StaticPartitioner>(9);
}

TEST_CASE("StatefulParallelFor.Static.10threads" * doctest::timeout(300)) {
  stateful_for_each<turbo::StaticPartitioner>(10);
}

TEST_CASE("StatefulParallelFor.Static.11threads" * doctest::timeout(300)) {
  stateful_for_each<turbo::StaticPartitioner>(11);
}

TEST_CASE("StatefulParallelFor.Static.12threads" * doctest::timeout(300)) {
  stateful_for_each<turbo::StaticPartitioner>(12);
}

// random
TEST_CASE("StatefulParallelFor.Random.1thread" * doctest::timeout(300)) {
  stateful_for_each<turbo::RandomPartitioner>(1);
}

TEST_CASE("StatefulParallelFor.Random.2threads" * doctest::timeout(300)) {
  stateful_for_each<turbo::RandomPartitioner>(2);
}

TEST_CASE("StatefulParallelFor.Random.3threads" * doctest::timeout(300)) {
  stateful_for_each<turbo::RandomPartitioner>(3);
}

TEST_CASE("StatefulParallelFor.Random.4threads" * doctest::timeout(300)) {
  stateful_for_each<turbo::RandomPartitioner>(4);
}

TEST_CASE("StatefulParallelFor.Random.5threads" * doctest::timeout(300)) {
  stateful_for_each<turbo::RandomPartitioner>(5);
}

TEST_CASE("StatefulParallelFor.Random.6threads" * doctest::timeout(300)) {
  stateful_for_each<turbo::RandomPartitioner>(6);
}

TEST_CASE("StatefulParallelFor.Random.7threads" * doctest::timeout(300)) {
  stateful_for_each<turbo::RandomPartitioner>(7);
}

TEST_CASE("StatefulParallelFor.Random.8threads" * doctest::timeout(300)) {
  stateful_for_each<turbo::RandomPartitioner>(8);
}

TEST_CASE("StatefulParallelFor.Random.9threads" * doctest::timeout(300)) {
  stateful_for_each<turbo::RandomPartitioner>(9);
}

TEST_CASE("StatefulParallelFor.Random.10threads" * doctest::timeout(300)) {
  stateful_for_each<turbo::RandomPartitioner>(10);
}

TEST_CASE("StatefulParallelFor.Random.11threads" * doctest::timeout(300)) {
  stateful_for_each<turbo::RandomPartitioner>(11);
}

TEST_CASE("StatefulParallelFor.Random.12threads" * doctest::timeout(300)) {
  stateful_for_each<turbo::RandomPartitioner>(12);
}

//// ----------------------------------------------------------------------------
//// Parallel For Exception
//// ----------------------------------------------------------------------------
//
//void parallel_for_exception(unsigned W) {
//
//  turbo::Taskflow taskflow;
//  turbo::Executor executor(W);
//
//  std::vector<int> data(1000000);
//
//  // for_each
//  taskflow.for_each(data.begin(), data.end(), [](int){
//    throw std::runtime_error("x");
//  });
//  REQUIRE_THROWS_WITH_AS(executor.run(taskflow).get(), "x", std::runtime_error);
//  
//  // for_each_index
//  taskflow.clear();
//  taskflow.for_each_index(0, 10000, 1, [](int){
//    throw std::runtime_error("y");
//  });
//  REQUIRE_THROWS_WITH_AS(executor.run(taskflow).get(), "y", std::runtime_error);
//}
//
//TEST_CASE("ParallelFor.Exception.1thread") {
//  parallel_for_exception(1);
//}
//
//TEST_CASE("ParallelFor.Exception.2threads") {
//  parallel_for_exception(2);
//}
//
//TEST_CASE("ParallelFor.Exception.3threads") {
//  parallel_for_exception(3);
//}
//
//TEST_CASE("ParallelFor.Exception.4threads") {
//  parallel_for_exception(4);
//}







