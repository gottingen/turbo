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

// ============================================================================
// Test without Priority
// ============================================================================

// Procedure: tsq_owner
void tsq_owner() {

  for(size_t N=1; N<=777777; N=N*2+1) {
    turbo::TaskQueue<void*> queue;
    std::vector<void*> gold(N);

    EXPECT_TRUE(queue.empty());

    // push and pop
    for(size_t i=0; i<N; ++i) {
      gold[i] = &i;
      queue.push(gold[i], 0);
    }
    for(size_t i=0; i<N; ++i) {
      auto ptr = queue.pop();
      EXPECT_TRUE(ptr != nullptr);
      EXPECT_TRUE(gold[N-i-1] == ptr);
    }
    EXPECT_TRUE(queue.pop() == nullptr);

    // push and steal
    for(size_t i=0; i<N; ++i) {
      queue.push(gold[i], 0);
    }
    // i starts from 1 to avoid cache effect
    for(size_t i=1; i<N; ++i) {
      auto ptr = queue.steal();
      EXPECT_TRUE(ptr != nullptr);
      EXPECT_TRUE(gold[i] == ptr);
    }
  }
}

// Procedure: tsq_n_thieves
void tsq_n_thieves(size_t M) {

  for(size_t N=1; N<=777777; N=N*2+1) {
    turbo::TaskQueue<void*> queue;
    std::vector<void*> gold(N);
    std::atomic<size_t> consumed {0};

    for(size_t i=0; i<N; ++i) {
      gold[i] = &i;
    }

    // thieves
    std::vector<std::thread> threads;
    std::vector<std::vector<void*>> stolens(M);
    for(size_t i=0; i<M; ++i) {
      threads.emplace_back([&, i](){
        while(consumed != N) {
          auto ptr = queue.steal();
          if(ptr != nullptr) {
            stolens[i].push_back(ptr);
            consumed.fetch_add(1, std::memory_order_relaxed);
          }
        }
        EXPECT_TRUE(queue.steal() == nullptr);
      });
    }

    // master thread
    for(size_t i=0; i<N; ++i) {
      queue.push(gold[i], 0);
    }

    std::vector<void*> items;
    while(consumed != N) {
      auto ptr = queue.pop();
      if(ptr != nullptr) {
        items.push_back(ptr);
        consumed.fetch_add(1, std::memory_order_relaxed);
      }
    }
    EXPECT_TRUE(queue.steal() == nullptr);
    EXPECT_TRUE(queue.pop() == nullptr);
    EXPECT_TRUE(queue.empty());

    // join thieves
    for(auto& thread : threads) thread.join();

    // merge items
    for(size_t i=0; i<M; ++i) {
      for(auto s : stolens[i]) {
        items.push_back(s);
      }
    }

    std::sort(items.begin(), items.end());
    std::sort(gold.begin(), gold.end());

    EXPECT_TRUE(items.size() == N);
    EXPECT_TRUE(items == gold);
  }

}

// ----------------------------------------------------------------------------
// Testcase: TSQTest.Owner
// ----------------------------------------------------------------------------
TEST(WorkStealing, QueueOwner) {
  tsq_owner();
}

// ----------------------------------------------------------------------------
// Testcase: TSQTest.1Thief
// ----------------------------------------------------------------------------
TEST(WorkStealing, Queue1Thief) {
  tsq_n_thieves(1);
}

// ----------------------------------------------------------------------------
// Testcase: TSQTest.2Thieves
// ----------------------------------------------------------------------------
TEST(WorkStealing, Queue2Thieves) {
  tsq_n_thieves(2);
}

// ----------------------------------------------------------------------------
// Testcase: TSQTest.3Thieves
// ----------------------------------------------------------------------------
TEST(WorkStealing, Queue3Thieves) {
  tsq_n_thieves(3);
}

// ----------------------------------------------------------------------------
// Testcase: TSQTest.4Thieves
// ----------------------------------------------------------------------------
TEST(WorkStealing, Queue4Thieves) {
  tsq_n_thieves(4);
}

// ----------------------------------------------------------------------------
// Testcase: TSQTest.5Thieves
// ----------------------------------------------------------------------------
TEST(WorkStealing, Queue5Thieves) {
  tsq_n_thieves(5);
}

// ----------------------------------------------------------------------------
// Testcase: TSQTest.6Thieves
// ----------------------------------------------------------------------------
TEST(WorkStealing, Queue6Thieves) {
  tsq_n_thieves(6);
}

// ----------------------------------------------------------------------------
// Testcase: TSQTest.7Thieves
// ----------------------------------------------------------------------------
TEST(WorkStealing, Queue7Thieves) {
  tsq_n_thieves(7);
}

// ----------------------------------------------------------------------------
// Testcase: TSQTest.8Thieves
// ----------------------------------------------------------------------------
TEST(WorkStealing, Queue8Thieves) {
  tsq_n_thieves(8);
}

// ============================================================================
// Test with Priority
// ============================================================================

// Procedure: priority_tsq_owner
void priority_tsq_owner() {

  const unsigned P = 5;

  turbo::TaskQueue<void*, P> queue;

  //for(unsigned p=0; p<P; p++) {
  //  EXPECT_TRUE(queue.push(nullptr, p) == true);
  //  EXPECT_TRUE(queue.push(nullptr, p) == false);
  //  EXPECT_TRUE(queue.push(nullptr, p) == false);
  //  EXPECT_TRUE(queue.push(nullptr, p) == false);

  //  EXPECT_TRUE(queue.pop(p) == nullptr);
  //  EXPECT_TRUE(queue.pop(p) == nullptr);
  //  EXPECT_TRUE(queue.pop(p) == nullptr);
  //  EXPECT_TRUE(queue.pop(p) == nullptr);
  //  EXPECT_TRUE(queue.pop(p) == nullptr);
  //  
  //  EXPECT_TRUE(queue.push(nullptr, p) == true);
  //  EXPECT_TRUE(queue.push(nullptr, p) == false);
  //  
  //  EXPECT_TRUE(queue.pop(p) == nullptr);
  //  EXPECT_TRUE(queue.pop(p) == nullptr);

  //  EXPECT_TRUE(queue.empty(p) == true);
  //}

  for(size_t N=1; N<=777777; N=N*2+1) {

    std::vector<std::pair<void*, unsigned>> gold(N);

    EXPECT_TRUE(queue.empty());
    EXPECT_TRUE(queue.pop() == nullptr);

    for(unsigned p=0; p<P; p++) {
      EXPECT_TRUE(queue.empty(p));
      EXPECT_TRUE(queue.pop(p) == nullptr);
      EXPECT_TRUE(queue.steal(p) == nullptr);
    }
    EXPECT_TRUE(queue.empty());

    // push 
    for(size_t i=0; i<N; ++i) {
      auto p = rand() % P;
      gold[i] = {&i, p};
      queue.push(&i, p);
    }

    // pop
    for(size_t i=0; i<N; ++i) {
      auto [g_ptr, g_pri]= gold[N-i-1];
      auto ptr = queue.pop(g_pri);
      EXPECT_TRUE(ptr != nullptr);
      EXPECT_TRUE(ptr == g_ptr);
    }
    EXPECT_TRUE(queue.pop() == nullptr);

    // push and steal
    for(size_t i=0; i<N; ++i) {
      queue.push(gold[i].first, gold[i].second);
    }

    // i starts from 1 to avoid cache effect
    for(size_t i=0; i<N; ++i) {
      auto [g_ptr, g_pri] = gold[i];
      auto ptr = queue.steal(g_pri);
      EXPECT_TRUE(ptr != nullptr);
      EXPECT_TRUE(g_ptr == ptr);
    }
    
    for(unsigned p=0; p<P; p++) {
      EXPECT_TRUE(queue.empty(p));
      EXPECT_TRUE(queue.pop(p) == nullptr);
      EXPECT_TRUE(queue.steal(p) == nullptr);
    }
    EXPECT_TRUE(queue.empty());
  }
}

TEST(WorkStealing, PriorityQueue_Owner) {
  priority_tsq_owner();
}

// ----------------------------------------------------------------------------
// Starvation Test
// ----------------------------------------------------------------------------

void starvation_test(size_t W) {

  turbo::Workflow taskflow;
  turbo::Executor executor(W);
  std::atomic<size_t> counter{0};

  turbo::Task prev, curr;
  
  // simple linear chain
  for(size_t l=0; l<100; l++) {

    curr = taskflow.emplace([&](){
      counter.fetch_add(1, std::memory_order_relaxed);
    });

    if(l) {
      curr.succeed(prev);
    }

    prev = curr;
  }

  // branches
  for(size_t b=W/2; b<W; b++) {
    taskflow.emplace([&](){
      counter.fetch_add(1, std::memory_order_relaxed);
    }).succeed(curr);
  }

  for(size_t b=0; b<W/2; b++) {
    taskflow.emplace([&](){
      while(counter.load(std::memory_order_relaxed) != W - W/2 + 100){
        std::this_thread::yield();
      }
    }).succeed(curr);
  }

  executor.run(taskflow).wait();

  EXPECT_TRUE(counter == W - W/2 + 100);

  // large linear chain followed by many branches
  size_t N = 1000;
  size_t target = 0;
  taskflow.clear();
  counter = 0;
  
  for(size_t l=0; l<N; l++) {
    curr = taskflow.emplace([&](){
      counter.fetch_add(1, std::memory_order_relaxed);
    });
    if(l) {
      curr.succeed(prev);
    }
    prev = curr;
  }

  const int w = rand() % W;

  for(size_t b=0; b<N; b++) {
    // wait with a probability of 0.9
    if(rand() % 10 != 0) {
      taskflow.emplace([&](){ 
        if(executor.this_worker_id() != w) {
          while(counter.load(std::memory_order_relaxed) != target + N) {
            std::this_thread::yield();
          }
        }
      }).succeed(curr);
    }
    // increment the counter with a probability of 0.1
    else {
      target++;
      taskflow.emplace([&](){ 
        counter.fetch_add(1, std::memory_order_relaxed);
      }).succeed(curr);
    }
  }

  executor.run(taskflow).wait();

  EXPECT_TRUE(counter == target + N);
  
}

TEST(WorkStealing, Starvation_1thread) {
  starvation_test(1);
}

TEST(WorkStealing, Starvation_2threads) {
  starvation_test(2);
}

TEST(WorkStealing, Starvation_3threads) {
  starvation_test(3);
}

TEST(WorkStealing, Starvation_4threads) {
  starvation_test(4);
}

TEST(WorkStealing, Starvation_5threads) {
  starvation_test(5);
}

TEST(WorkStealing, Starvation_6threads) {
  starvation_test(6);
}

TEST(WorkStealing, Starvation_7threads) {
  starvation_test(7);
}

TEST(WorkStealing, Starvation_8threads) {
  starvation_test(8);
}

// ----------------------------------------------------------------------------
// Starvation Loop Test
// ----------------------------------------------------------------------------

void starvation_loop_test(size_t W) {

  size_t L=100, B = 1024;

  EXPECT_TRUE(B > W);
  
  turbo::Workflow taskflow;
  turbo::Executor executor(W);

  std::atomic<size_t> counter{0};
  std::atomic<size_t> barrier{0};

  // all worker must be involved
  std::mutex mutex;
  std::unordered_set<int> set;

  auto [merge, cond, stop] = taskflow.emplace(
    [&](){  
      EXPECT_TRUE(barrier.load(std::memory_order_relaxed) == B);
      EXPECT_TRUE(counter.load(std::memory_order_relaxed) == (L + B - 1));
      EXPECT_TRUE(set.size() == W);
      counter = 0;
      barrier = 0;
      set.clear();
    },
    [n=0]() mutable { 
      return ++n >= 10 ? 1 : 0;
    },
    [&](){
      EXPECT_TRUE(barrier.load(std::memory_order_relaxed) == 0);
      EXPECT_TRUE(counter.load(std::memory_order_relaxed) == 0);
      EXPECT_TRUE(set.size() == 0);
    }
  );

  turbo::Task prev, curr, second;
  
  // linear chain with delay to make workers sleep
  for(size_t l=0; l<L; l++) {

    curr = taskflow.emplace([&, l](){
      if(l) {
        counter.fetch_add(1, std::memory_order_relaxed);
      }
    });

    if(l) {
      curr.succeed(prev);
    }

    if(l==1) {
      second = curr;
    }

    prev = curr;
  }
  
  cond.precede(second, stop);


  // fork
  for(size_t b=0; b<B; b++) {
    turbo::Task task = taskflow.emplace([&](){
      // record the threads
      {
        std::scoped_lock lock(mutex);
        set.insert(executor.this_worker_id());
      }

      // all threads should be notified
      barrier.fetch_add(1, std::memory_order_relaxed);
      while(barrier.load(std::memory_order_relaxed) < W){
        std::this_thread::yield();
      }
      
      // increment the counter
      counter.fetch_add(1, std::memory_order_relaxed);
    });
    task.succeed(curr)
        .precede(merge);
  }

  merge.precede(cond);

  //taskflow.dump(std::cout);
  executor.run(taskflow).wait();
}

TEST(WorkStealing, StarvationLoop_1thread) {
  starvation_loop_test(1);
}

TEST(WorkStealing, StarvationLoop_2threads) {
  starvation_loop_test(2);
}

TEST(WorkStealing, StarvationLoop_3threads) {
  starvation_loop_test(3);
}

TEST(WorkStealing, StarvationLoop_4threads) {
  starvation_loop_test(4);
}

TEST(WorkStealing, StarvationLoop_5threads) {
  starvation_loop_test(5);
}

TEST(WorkStealing, StarvationLoop_6threads) {
  starvation_loop_test(6);
}

TEST(WorkStealing, StarvationLoop_7threads) {
  starvation_loop_test(7);
}

TEST(WorkStealing, StarvationLoop_8threads) {
  starvation_loop_test(8);
}

// ----------------------------------------------------------------------------
// Starvation Loop Test
// ----------------------------------------------------------------------------

void subflow_starvation_test(size_t W) {

  size_t L=100, B = 1024;

  EXPECT_TRUE(B > W);
  
  turbo::Workflow taskflow;
  turbo::Executor executor(W);

  std::atomic<size_t> counter{0};
  std::atomic<size_t> barrier{0};

  // all worker must be involved
  std::mutex mutex;
  std::unordered_set<int> set;

  taskflow.emplace([&](turbo::Subflow& subflow){

    auto [merge, cond, stop] = subflow.emplace(
      [&](){  
        EXPECT_TRUE(barrier.load(std::memory_order_relaxed) == B);
        EXPECT_TRUE(counter.load(std::memory_order_relaxed) == (L + B - 1));
        EXPECT_TRUE(set.size() == W);
        counter = 0;
        barrier = 0;
        set.clear();
      },
      [n=0]() mutable { 
        return ++n >= 5 ? 1 : 0;
      },
      [&](){
        EXPECT_TRUE(barrier.load(std::memory_order_relaxed) == 0);
        EXPECT_TRUE(counter.load(std::memory_order_relaxed) == 0);
        EXPECT_TRUE(set.size() == 0);
      }
    );

    turbo::Task prev, curr, second;
    
    // linear chain with delay to make workers sleep
    for(size_t l=0; l<L; l++) {

      curr = subflow.emplace([&, l](){
        if(l) {
          counter.fetch_add(1, std::memory_order_relaxed);
        }
      });

      if(l) {
        curr.succeed(prev);
      }

      if(l==1) {
        second = curr;
      }

      prev = curr;
    }
    
    cond.precede(second, stop);


    // fork
    for(size_t b=0; b<B; b++) {
      turbo::Task task = subflow.emplace([&](){
        // record the threads
        {
          std::scoped_lock lock(mutex);
          set.insert(executor.this_worker_id());
        }

        // all threads should be notified
        barrier.fetch_add(1, std::memory_order_relaxed);
        while(barrier.load(std::memory_order_relaxed) < W) {
          std::this_thread::yield();
        }
        
        // increment the counter
        counter.fetch_add(1, std::memory_order_relaxed);
      });
      task.succeed(curr)
          .precede(merge);
    }

    merge.precede(cond);
  });

  //taskflow.dump(std::cout);
  executor.run_n(taskflow, 5).wait();
}

TEST(WorkStealing, SubflowStarvation_1thread) {
  subflow_starvation_test(1);
}

TEST(WorkStealing, SubflowStarvation_2threads) {
  subflow_starvation_test(2);
}

TEST(WorkStealing, SubflowStarvation_3threads) {
  subflow_starvation_test(3);
}

TEST(WorkStealing, SubflowStarvation_4threads) {
  subflow_starvation_test(4);
}

TEST(WorkStealing, SubflowStarvation_5threads) {
  subflow_starvation_test(5);
}

TEST(WorkStealing, SubflowStarvation_6threads) {
  subflow_starvation_test(6);
}

TEST(WorkStealing, SubflowStarvation_7threads) {
  subflow_starvation_test(7);
}

TEST(WorkStealing, SubflowStarvation_8threads) {
  subflow_starvation_test(8);
}

// ----------------------------------------------------------------------------
// Embarrassing Starvation Test
// ----------------------------------------------------------------------------

void embarrasing_starvation_test(size_t W) {

  size_t B = 65536;

  EXPECT_TRUE(B > W);
  
  turbo::Workflow taskflow, parent;
  turbo::Executor executor(W);

  std::atomic<size_t> barrier{0};

  // all worker must be involved
  std::mutex mutex;
  std::unordered_set<int> set;

  // fork
  for(size_t b=0; b<B; b++) {
    taskflow.emplace([&](){
      // record worker
      {
        std::scoped_lock lock(mutex);
        set.insert(executor.this_worker_id());
      }

      // all threads should be notified
      barrier.fetch_add(1, std::memory_order_relaxed);
      while(barrier.load(std::memory_order_relaxed) < W) {
        std::this_thread::yield();
      }
    });
  }

  parent.composed_of(taskflow);

  executor.run(parent).wait();

  EXPECT_TRUE(set.size() == W);
}

TEST(WorkStealing, EmbarrasingStarvation_1thread) {
  embarrasing_starvation_test(1);
}

TEST(WorkStealing, EmbarrasingStarvation_2threads) {
  embarrasing_starvation_test(2);
}

TEST(WorkStealing, EmbarrasingStarvation_3threads) {
  embarrasing_starvation_test(3);
}

TEST(WorkStealing, EmbarrasingStarvation_4threads) {
  embarrasing_starvation_test(4);
}

TEST(WorkStealing, EmbarrasingStarvation_5threads) {
  embarrasing_starvation_test(5);
}

TEST(WorkStealing, EmbarrasingStarvation_6threads) {
  embarrasing_starvation_test(6);
}

TEST(WorkStealing, EmbarrasingStarvation_7threads) {
  embarrasing_starvation_test(7);
}

TEST(WorkStealing, EmbarrasingStarvation_8threads) {
  embarrasing_starvation_test(8);
}

// ----------------------------------------------------------------------------
// skewed starvation
// ----------------------------------------------------------------------------

void skewed_starvation(size_t W) {
  
  turbo::Workflow taskflow;
  turbo::Executor executor(W);

  std::atomic<size_t> stop, count;
  
  // all worker must be involved
  std::mutex mutex;
  std::unordered_set<int> set;

  turbo::Task parent = taskflow.emplace([&](){
    set.clear();
    count.store(0, std::memory_order_relaxed);
    stop.store(false, std::memory_order_relaxed);
  }).name("root");

  turbo::Task left, right;

  for(size_t w=0; w<W; w++) {
    right = taskflow.emplace([&, w](){
      if(w) {
        // record the worker
        {
          std::scoped_lock lock(mutex);
          set.insert(executor.this_worker_id());
        }

        count.fetch_add(1, std::memory_order_release);

        // block the worker
        while(stop.load(std::memory_order_relaxed) == false) {
          std::this_thread::yield();
        }
      }
    }).name(std::string("right-") + std::to_string(w));

    left = taskflow.emplace([&](){
      std::this_thread::yield();
    }).name(std::string("left-") + std::to_string(w));
    
    // we want to remove the effect of parent stealing
    if(rand() & 1) {
      parent.precede(left, right);
    }
    else {
      parent.precede(right, left);
    }

    parent = left;
  }

  left = taskflow.emplace([&](){
    // wait for the other W-1 workers to block
    while(count.load(std::memory_order_acquire) + 1 != W) {
      std::this_thread::yield();
    }
    stop.store(true, std::memory_order_relaxed);

    EXPECT_TRUE(set.size() + 1 == W);
  }).name("stop");

  parent.precede(left);

  //taskflow.dump(std::cout);

  executor.run_n(taskflow, 1024).wait();
}

TEST(WorkStealing, SkewedStarvation_1thread) {
  skewed_starvation(1);
}

TEST(WorkStealing, SkewedStarvation_2threads) {
  skewed_starvation(2);
}

TEST(WorkStealing, SkewedStarvation_3threads) {
  skewed_starvation(3);
}

TEST(WorkStealing, SkewedStarvation_4threads) {
  skewed_starvation(4);
}

TEST(WorkStealing, SkewedStarvation_5threads) {
  skewed_starvation(5);
}

TEST(WorkStealing, SkewedStarvation_6threads) {
  skewed_starvation(6);
}

TEST(WorkStealing, SkewedStarvation_7threads) {
  skewed_starvation(7);
}

TEST(WorkStealing, SkewedStarvation_8threads) {
  skewed_starvation(8);
}

// ----------------------------------------------------------------------------
// NAryStravtion
// ----------------------------------------------------------------------------

void nary_starvation(size_t W) {
  
  size_t N = 1024;

  turbo::Workflow taskflow;
  turbo::Executor executor(W);

  std::atomic<size_t> stop, count;
  
  // all worker must be involved
  std::mutex mutex;
  std::unordered_set<int> set;

  turbo::Task parent = taskflow.emplace([&](){
    set.clear();
    count.store(0, std::memory_order_relaxed);
    stop.store(false, std::memory_order_relaxed);
  }).name("root");

  turbo::Task pivot;

  for(size_t w=0; w<W; w++) {
    
    // randomly choose a pivot to be the parent for the next level
    size_t p = rand()%N;

    for(size_t i=0; i<N; i++) {
      turbo::Task task = taskflow.emplace([&, w, p, i](){
        
        // the blocker cannot be the pivot - others simply return
        if(i != (p+1)%N) {
          std::this_thread::yield();
          return;
        }
        
        // now I need to block
        if(w) {
          // record the worker
          {
            std::scoped_lock lock(mutex);
            set.insert(executor.this_worker_id());
          }

          count.fetch_add(1, std::memory_order_release);

          // block the worker
          while(stop.load(std::memory_order_relaxed) == false) {
            std::this_thread::yield();
          }
        }
      }).name(std::to_string(w));

      parent.precede(task);

      if(p == i) {
        pivot = task;
      }
    }
    parent = pivot;
  }

  pivot = taskflow.emplace([&](){
    // wait for the other W-1 workers to block
    while(count.load(std::memory_order_acquire) + 1 != W) {
      std::this_thread::yield();
    }
    stop.store(true, std::memory_order_relaxed);
    EXPECT_TRUE(set.size() + 1 == W);
  }).name("stop");

  parent.precede(pivot);

  //taskflow.dump(std::cout);

  executor.run_n(taskflow, 5).wait();
}

TEST(WorkStealing, NAryStarvation_1thread) {
  nary_starvation(1);
}

TEST(WorkStealing, NAryStarvation_2threads) {
  nary_starvation(2);
}

TEST(WorkStealing, NAryStarvation_3threads) {
  nary_starvation(3);
}

TEST(WorkStealing, NAryStarvation_4threads) {
  nary_starvation(4);
}

TEST(WorkStealing, NAryStarvation_5threads) {
  nary_starvation(5);
}

TEST(WorkStealing, NAryStarvation_6threads) {
  nary_starvation(6);
}

TEST(WorkStealing, NAryStarvation_7threads) {
  nary_starvation(7);
}

TEST(WorkStealing, NAryStarvation_8threads) {
  nary_starvation(8);
}

// ----------------------------------------------------------------------------
// Wavefront Starvation Test
// ----------------------------------------------------------------------------

void wavefront_starvation(size_t W) {
  
  turbo::Workflow taskflow;
  turbo::Executor executor(W);

  std::atomic<size_t> stop, count{0}, blocked;
  
  // all worker must be involved
  std::mutex mutex;
  std::unordered_set<int> set;

  std::vector<std::vector<turbo::Task>> G;
  G.resize(W);

  // create tasks
  for(size_t i=0; i<W; i++) {
    G[i].resize(W);
    for(size_t j=0; j<W; j++) {
      // source
      if(i + j == 0) {
        G[i][j] = taskflow.emplace([&](){
          count.fetch_add(1, std::memory_order_relaxed);
          stop.store(false, std::memory_order_relaxed);
          blocked.store(0, std::memory_order_relaxed);
          set.clear();
        });
      }
      // diagonal tasks
      else if(i + j + 1 == W) {
        
        G[i][j] = taskflow.emplace([&, i, j](){

          count.fetch_add(1, std::memory_order_relaxed);
          
          // top-right will unblock all W-1 workers
          if(i == 0 && j + 1 == W) {
            while(blocked.load(std::memory_order_acquire) + 1 != W) {
              std::this_thread::yield();
            }
            stop.store(true, std::memory_order_relaxed);
            EXPECT_TRUE(set.size() + 1 == W);
          }
          else {
            // record the worker
            {
              std::scoped_lock lock(mutex);
              set.insert(executor.this_worker_id());
            }

            blocked.fetch_add(1, std::memory_order_release);

            // block the worker
            while(stop.load(std::memory_order_relaxed) == false) {
              std::this_thread::yield();
            }
          }
        });
      }
      // other tasks
      else {
        G[i][j] = taskflow.emplace([&](){
          count.fetch_add(1, std::memory_order_relaxed);
        });
      }

      // name the task
      G[i][j].name(std::to_string(i) + ", " + std::to_string(j));
    }
  }

  // create dependency
  for(size_t i=0; i<W; i++) {
    for(size_t j=0; j<W; j++) {
      size_t next_i = i + 1;
      size_t next_j = j + 1;
      if(next_i < W) {
        G[i][j].precede(G[next_i][j]);
      }
      if(next_j < W) {
        G[i][j].precede(G[i][next_j]);
      }
    }
  }

  //taskflow.dump(std::cout);
  executor.run_n(taskflow, 1024).wait();

  EXPECT_TRUE(count == W*W*1024);
}

TEST(WorkStealing, WavefrontStarvation_1thread) {
  wavefront_starvation(1);
}

TEST(WorkStealing, WavefrontStarvation_2threads) {
  wavefront_starvation(2);
}

TEST(WorkStealing, WavefrontStarvation_3threads) {
  wavefront_starvation(3);
}

TEST(WorkStealing, WavefrontStarvation_4threads) {
  wavefront_starvation(4);
}

TEST(WorkStealing, WavefrontStarvation_5threads) {
  wavefront_starvation(5);
}

TEST(WorkStealing, WavefrontStarvation_6threads) {
  wavefront_starvation(6);
}

TEST(WorkStealing, WavefrontStarvation_7threads) {
  wavefront_starvation(7);
}

TEST(WorkStealing, WavefrontStarvation_8threads) {
  wavefront_starvation(8);
}

// ----------------------------------------------------------------------------
// Oversubscription Test
// ----------------------------------------------------------------------------

void oversubscription_test(size_t W) {

  turbo::Workflow taskflow;
  turbo::Executor executor(W);

  std::atomic<size_t> counter{0};
  
  // all worker must be involved
  std::mutex mutex;
  std::unordered_set<int> set;

  for(size_t n = 0; n<W/2; n++) { 
  
    turbo::Task prev, curr;

    for(size_t l=0; l<100; l++) {

      curr = taskflow.emplace([&](){
        // record worker
        {
          std::scoped_lock lock(mutex);
          set.insert(executor.this_worker_id());
        }
        counter.fetch_add(1, std::memory_order_relaxed);
      });

      if(l) {
        curr.succeed(prev);
      }

      prev = curr;
    }
  }

  for(size_t t=1; t<=100; t++) {
    set.clear();
    executor.run(taskflow).wait();
    EXPECT_TRUE(counter == 100*(W/2)*t);
    EXPECT_TRUE(set.size() <= W/2);
  }
}

TEST(WorkStealing, Oversubscription_2threads) {
  oversubscription_test(2);
}

TEST(WorkStealing, Oversubscription_3threads) {
  oversubscription_test(3);
}

TEST(WorkStealing, Oversubscription_4threads) {
  oversubscription_test(4);
}

TEST(WorkStealing, Oversubscription_5threads) {
  oversubscription_test(5);
}

TEST(WorkStealing, Oversubscription_6threads) {
  oversubscription_test(6);
}

TEST(WorkStealing, Oversubscription_7threads) {
  oversubscription_test(7);
}

TEST(WorkStealing, Oversubscription_8threads) {
  oversubscription_test(8);
}

//TEST(WorkStealing, Oversubscription_16threads) {
//  oversubscription_test(16);
//}
//
//TEST(WorkStealing, Oversubscription_32threads) {
//  oversubscription_test(32);
//}

// ----------------------------------------------------------------------------

void ws_broom(size_t W) {
  
  turbo::Workflow taskflow;
  turbo::Executor executor(W);
  
  turbo::Task task, prev;
  for(size_t i=0; i<10; i++) {
    task = taskflow.emplace([&](){
      //std::cout << executor.this_worker() << std::endl;
      printf("linear by worker %d\n", executor.this_worker_id());
      std::this_thread::sleep_for(std::chrono::milliseconds(100));
    });

    if(i) {
      prev.precede(task);
    }

    prev = task;
  }

  for(size_t i=0; i<10; i++) {
    taskflow.emplace([&](){
      //std::cout << executor.this_worker() << std::endl;
      printf("parallel by worker %d\n", executor.this_worker_id());
      std::this_thread::sleep_for(std::chrono::milliseconds(100));
    }).succeed(task);
  }

  executor.run(taskflow).wait();

}
 
//TEST(WS.broom.2threads) {
//  ws_broom(10);
//}

// ----------------------------------------------------------------------------
// Continuation
// ----------------------------------------------------------------------------

void continuation_test(size_t W) {
  
  turbo::Workflow taskflow;
  turbo::Executor executor(W);
  
  turbo::Task curr, prev;

  int w = executor.this_worker_id();

  EXPECT_TRUE(w == -1);

  for(size_t i=0; i<1000; i++) {
    curr = taskflow.emplace([&, i]() mutable {
      if(i == 0) {
        w = executor.this_worker_id();
      } 
      else {
        EXPECT_TRUE(w == executor.this_worker_id());
      }
    });

    if(i) {
      prev.precede(curr);
    }

    prev = curr;
  }

  executor.run(taskflow).wait();

}

TEST(WorkStealing, Continuation_1thread) {
  continuation_test(1);
}

TEST(WorkStealing, Continuation_2threads) {
  continuation_test(2);
}

TEST(WorkStealing, Continuation_3threads) {
  continuation_test(3);
}

TEST(WorkStealing, Continuation_4threads) {
  continuation_test(4);
}

TEST(WorkStealing, Continuation_5threads) {
  continuation_test(5);
}

TEST(WorkStealing, Continuation_6threads) {
  continuation_test(6);
}

TEST(WorkStealing, Continuation_7threads) {
  continuation_test(7);
}

TEST(WorkStealing, Continuation_8threads) {
  continuation_test(8);
}









