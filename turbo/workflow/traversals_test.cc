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
#include "turbo/workflow/workflow.h"

// --------------------------------------------------------
// Graph generation
// --------------------------------------------------------

struct Node {

  std::string name;
  size_t idx   {0};
  size_t level {0};
  bool visited {false};

  std::atomic<size_t> dependents {0};
  std::vector<Node*> successors;

  void precede(Node& n) {
    successors.emplace_back(&n);
    n.dependents ++;
  }
};

std::unique_ptr<Node[]> make_dag(size_t num_nodes, size_t max_degree) {

  std::unique_ptr<Node[]> nodes(new Node[num_nodes]);

  // Make sure nodes are in clean state
  for(size_t i=0; i<num_nodes; i++) {
    nodes[i].idx = i;
    nodes[i].name = std::to_string(i);
    EXPECT_TRUE(!nodes[i].visited);
    EXPECT_TRUE(nodes[i].successors.empty());
    EXPECT_TRUE(nodes[i].dependents == 0);
  }

  // Create a DAG
  for(size_t i=0; i<num_nodes; i++) {
    size_t degree {0};
    for(size_t j=i+1; j<num_nodes && degree < max_degree; j++) {
      if(j%2 == 1) {
        nodes[i].precede(nodes[j]);
        degree ++;
      }
    }
  }

  return nodes;
}

std::unique_ptr<Node[]> make_chain(size_t num_nodes) {

  std::unique_ptr<Node[]> nodes(new Node[num_nodes]);

  // Make sure nodes are in clean state
  for(size_t i=0; i<num_nodes; i++) {
    nodes[i].idx = i;
    nodes[i].name = std::to_string(i);
    EXPECT_TRUE(!nodes[i].visited);
    EXPECT_TRUE(nodes[i].successors.empty());
    EXPECT_TRUE(nodes[i].dependents == 0);
  }

  // Create a DAG
  for(size_t i=1; i<num_nodes; i++) {
    nodes[i-1].precede(nodes[i]);
  }

  return nodes;
}

// --------------------------------------------------------
// Testcase: StaticTraversal
// --------------------------------------------------------
TEST(Static, Traversal) {

  size_t max_degree = 4;
  size_t num_nodes = 1000;

  for(unsigned w=1; w<=4; w++) {

    auto nodes = make_dag(num_nodes, max_degree);

    turbo::Workflow tf;
    turbo::Executor executor(w);

    std::atomic<size_t> level(0);
    std::vector<turbo::Task> tasks;

    for(size_t i=0; i<num_nodes; ++i) {
      auto task = tf.emplace([&level, v=&(nodes[i])](){
        v->level = ++level;
        v->visited = true;
        for(size_t j=0; j<v->successors.size(); ++j) {
          v->successors[j]->dependents.fetch_sub(1);
        }
      }).name(nodes[i].name);

      tasks.push_back(task);
    }

    for(size_t i=0; i<num_nodes; ++i) {
      for(size_t j=0; j<nodes[i].successors.size(); ++j) {
        tasks[i].precede(tasks[nodes[i].successors[j]->idx]);
      }
    }

    executor.run(tf).wait();  // block until finished

    for(size_t i=0; i<num_nodes; i++) {
      EXPECT_TRUE(nodes[i].visited);
      EXPECT_TRUE(nodes[i].dependents == 0);
      for(size_t j=0; j<nodes[i].successors.size(); ++j) {
        EXPECT_TRUE(nodes[i].level < nodes[i].successors[j]->level);
      }
    }
  }
}

// --------------------------------------------------------
// Testcase: DynamicTraversal
// --------------------------------------------------------
TEST(Dynamic, Traversal) {

  std::atomic<size_t> level;

  std::function<void(Node*, turbo::Subflow&)> traverse;

  traverse = [&] (Node* n, turbo::Subflow& subflow) {
    EXPECT_TRUE(!n->visited);
    n->visited = true;
    size_t S = n->successors.size();
    for(size_t i=0; i<S; i++) {
      if(n->successors[i]->dependents.fetch_sub(1) == 1) {
        n->successors[i]->level = ++level;
        subflow.emplace([s=n->successors[i], &traverse](turbo::Subflow &subflow){
          traverse(s, subflow);
        });
      }
    }
  };

  size_t max_degree = 4;
  size_t num_nodes = 1000;

  for(unsigned w=1; w<=4; w++) {

    auto nodes = make_dag(num_nodes, max_degree);

    std::vector<Node*> src;
    for(size_t i=0; i<num_nodes; i++) {
      if(nodes[i].dependents == 0) {
        src.emplace_back(&(nodes[i]));
      }
    }

    level = 0;

    turbo::Workflow tf;
    turbo::Executor executor(w);

    for(size_t i=0; i<src.size(); i++) {
      tf.emplace([s=src[i], &traverse](turbo::Subflow& subflow){
        traverse(s, subflow);
      });
    }

    executor.run(tf).wait();  // block until finished

    for(size_t i=0; i<num_nodes; i++) {
      EXPECT_TRUE(nodes[i].visited);
      EXPECT_TRUE(nodes[i].dependents == 0);
      for(size_t j=0; j<nodes[i].successors.size(); ++j) {
        EXPECT_TRUE(nodes[i].level < nodes[i].successors[j]->level);
      }
    }
  }
}

// --------------------------------------------------------
// Testcase: RecursiveTraversal
// --------------------------------------------------------
//TEST("RecursiveTraversal) {
//
//  std::atomic<size_t> level;
//
//  std::function<void(Node*, turbo::Subflow&)> traverse;
//
//  traverse = [&] (Node* n, turbo::Subflow& subflow) {
//    EXPECT_TRUE(!n->visited);
//    n->visited = true;
//    size_t S = n->successors.size();
//    for(size_t i=0; i<S; i++) {
//      if(n->successors[i]->dependents.fetch_sub(1) == 1) {
//        n->successors[i]->level = ++level;
//        subflow.emplace([s=n->successors[i], &traverse](turbo::Subflow &subflow){
//          traverse(s, subflow);
//        });
//      }
//    }
//  };
//
//  size_t num_nodes = 1000;
//
//  for(unsigned w=1; w<=4; w++) {
//
//    auto nodes = make_chain(num_nodes);
//
//    std::vector<Node*> src;
//    for(size_t i=0; i<num_nodes; i++) {
//      if(nodes[i].dependents == 0) {
//        src.emplace_back(&(nodes[i]));
//      }
//    }
//
//    level = 0;
//
//    turbo::Workflow tf;
//    turbo::Executor executor(w);
//
//    for(size_t i=0; i<src.size(); i++) {
//      tf.emplace([s=src[i], &traverse](turbo::Subflow& subflow){
//        traverse(s, subflow);
//      });
//    }
//
//    executor.run(tf).wait();  // block until finished
//
//    for(size_t i=0; i<num_nodes; i++) {
//      EXPECT_TRUE(nodes[i].visited);
//      EXPECT_TRUE(nodes[i].dependents == 0);
//      for(size_t j=0; j<nodes[i].successors.size(); ++j) {
//        EXPECT_TRUE(nodes[i].level < nodes[i].successors[j]->level);
//      }
//    }
//  }
//}

// --------------------------------------------------------
// Testcase: ParallelTraversal
// --------------------------------------------------------

void parallel_traversal(unsigned num_threads) {

  turbo::Executor executor(num_threads);

  std::vector<std::thread> threads;

  for(unsigned t=0; t<num_threads; ++t) {

    threads.emplace_back([&](){

      std::atomic<size_t> level {0};

      size_t max_degree = 4;
      size_t num_nodes = 1000;

      auto nodes = make_dag(num_nodes, max_degree);

      std::vector<Node*> src;
      for(size_t i=0; i<num_nodes; i++) {
        if(nodes[i].dependents == 0) {
          src.emplace_back(&(nodes[i]));
        }
      }

      std::function<void(Node*, turbo::Subflow&)> traverse;

      traverse = [&] (Node* n, turbo::Subflow& subflow) {
        EXPECT_TRUE(!n->visited);
        n->visited = true;
        size_t S = n->successors.size();
        for(size_t i=0; i<S; i++) {
          if(n->successors[i]->dependents.fetch_sub(1) == 1) {
            n->successors[i]->level = ++level;
            subflow.emplace([s=n->successors[i], &traverse](turbo::Subflow &subflow){
              traverse(s, subflow);
            });
          }
        }
      };

      turbo::Workflow tf;

      for(size_t i=0; i<src.size(); i++) {
        tf.emplace([s=src[i], &traverse](turbo::Subflow& subflow){
          traverse(s, subflow);
        });
      }

      executor.run(tf).wait();  // block until finished

      for(size_t i=0; i<num_nodes; i++) {
        EXPECT_TRUE(nodes[i].visited);
        EXPECT_TRUE(nodes[i].dependents == 0);
        for(size_t j=0; j<nodes[i].successors.size(); ++j) {
          EXPECT_TRUE(nodes[i].level < nodes[i].successors[j]->level);
        }
      }
    });
  }

  for(auto& thread : threads) thread.join();
}

TEST(Parallel, Traversal_1) {
  parallel_traversal(1);
}

TEST(Parallel, Traversal_2) {
  parallel_traversal(2);
}

TEST(Parallel, Traversal_3) {
  parallel_traversal(3);
}

TEST(Parallel, Traversal_4) {
  parallel_traversal(4);
}

TEST(Parallel, Traversal_5) {
  parallel_traversal(5);
}

TEST(Parallel, Traversal_6) {
  parallel_traversal(6);
}

TEST(Parallel, Traversal_7) {
  parallel_traversal(7);
}

TEST(Parallel, Traversal_8) {
  parallel_traversal(8);
}



