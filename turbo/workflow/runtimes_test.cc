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
#include "turbo/workflow/algorithm/pipeline.h"

// --------------------------------------------------------
// Testcase: RuntimeTasking
// --------------------------------------------------------

TEST(Runtime, Basics) {

  turbo::Workflow taskflow;
  turbo::Executor executor;

  int a = 0;
  int b = 0;

  taskflow.emplace([&](turbo::Runtime& rt){
    rt.run_and_wait([&](turbo::Subflow& sf){
      EXPECT_TRUE(&rt.executor() == &executor);
      auto task1 = sf.emplace([&](){a++;});
      auto task2 = sf.emplace([&](){a++;});
      auto task3 = sf.emplace([&](){a++;});
      auto task4 = sf.emplace([&](){a++;});
      auto task5 = sf.emplace([&](){a++;});
      task1.precede(task2);
      task2.precede(task3);
      task3.precede(task4);
      task4.precede(task5);
    });
  });

  taskflow.emplace([&](turbo::Subflow& sf){
    sf.emplace([&](turbo::Runtime& rt){
      EXPECT_TRUE(&rt.executor() == &executor);
      rt.run_and_wait([&](turbo::Subflow& sf){
        auto task1 = sf.emplace([&](){b++;});
        auto task2 = sf.emplace([&](){b++;});
        auto task3 = sf.emplace([&](){b++;});
        auto task4 = sf.emplace([&](){b++;});
        auto task5 = sf.emplace([&](){b++;});
        task1.precede(task2);
        task2.precede(task3);
        task3.precede(task4);
        task4.precede(task5);
        sf.detach();
      });
    });
  });

  executor.run(taskflow).wait();

  EXPECT_TRUE(a == 5);
  EXPECT_TRUE(b == 5);
}

// --------------------------------------------------------
// Testcase: PipelineSP_ExternalGraph.Simple
// --------------------------------------------------------

TEST(Runtime, ExternalGraph_Simple) {

  const size_t N = 100;

  turbo::Executor executor;
  turbo::Workflow taskflow;
  
  std::vector<int> results(N, 0);
  std::vector<turbo::Workflow> graphs(N);

  for(size_t i=0; i<N; i++) {

    auto& fb = graphs[i];

    auto A = fb.emplace([&res=results[i]]()mutable{ ++res; });
    auto B = fb.emplace([&res=results[i]]()mutable{ ++res; });
    auto C = fb.emplace([&res=results[i]]()mutable{ ++res; });
    auto D = fb.emplace([&res=results[i]]()mutable{ ++res; });

    A.precede(B);
    B.precede(C);
    C.precede(D);

    taskflow.emplace([&res=results[i], &graph=graphs[i]](turbo::Runtime& rt)mutable{
      rt.run_and_wait(graph);
    });
  }
  
  executor.run_n(taskflow, 100).wait();

  for(size_t i=0; i<N; i++) {
    EXPECT_TRUE(results[i] == 400);
  }

}

// --------------------------------------------------------
// Testcase: PipelineSP_Subflow
// --------------------------------------------------------

void runtime_subflow(size_t w) {
  
  const size_t runtime_tasks_per_line = 20;
  const size_t lines = 4;
  const size_t subtasks = 4096;

  turbo::Executor executor(w);
  turbo::Workflow parent;
  turbo::Workflow taskflow;

  for (size_t subtask = 0; subtask <= subtasks; subtask = subtask == 0 ? subtask + 1 : subtask*2) {
    
    parent.clear();
    taskflow.clear();

    auto init = taskflow.emplace([](){}).name("init");
    auto end  = taskflow.emplace([](){}).name("end");

    std::vector<turbo::Task> rts;
    std::atomic<size_t> sums = 0;
    
    for (size_t i = 0; i < runtime_tasks_per_line * lines; ++i) {
      std::string rt_name = "rt-" + std::to_string(i);
      
      rts.emplace_back(taskflow.emplace([&sums, &subtask](turbo::Runtime& rt) {
        rt.run_and_wait([&sums, &subtask](turbo::Subflow& sf) {
          for (size_t j = 0; j < subtask; ++j) {
            sf.emplace([&sums]() {
              sums.fetch_add(1, std::memory_order_relaxed);
              //std::this_thread::sleep_for(std::chrono::nanoseconds(1));
            });
          }  
        });
      }).name(rt_name));
    }

    for (size_t l = 0; l < lines; ++l) {
      init.precede(rts[l*runtime_tasks_per_line]);
    }

    for (size_t l = 0; l < lines; ++l) {
      for (size_t i = 0; i < runtime_tasks_per_line-1; ++i) {
        rts[i+l*runtime_tasks_per_line].precede(rts[i+l*runtime_tasks_per_line+1]);
      }
    }

    for (size_t l = 1; l < lines+1; ++l) {
      end.succeed(rts[runtime_tasks_per_line*l-1]);
    }

    parent.composed_of(taskflow);

    executor.run(parent).wait();
    //taskflow.dump(std::cout);
    EXPECT_TRUE(sums == runtime_tasks_per_line*lines*subtask);
  }
}

TEST(Runtime, Subflow_1thread){
  runtime_subflow(1);
}

TEST(Runtime, Subflow_2threads){
  runtime_subflow(2);
}

TEST(Runtime, Subflow_3threads){
  runtime_subflow(3);
}

TEST(Runtime, Subflow_4threads){
  runtime_subflow(4);
}

TEST(Runtime, Subflow_5threads){
  runtime_subflow(5);
}

TEST(Runtime, Subflow_6threads){
  runtime_subflow(6);
}

TEST(Runtime, Subflow_7threads){
  runtime_subflow(7);
}

TEST(Runtime, Subflow_8threads){
  runtime_subflow(8);
}


// --------------------------------------------------------
// Testcase: PipelineSP,PipelineSP_Subflow
// --------------------------------------------------------

void pipeline_sp_runtime_subflow(size_t w) {
  
  size_t num_lines = 2;
  size_t subtask = 2;
  size_t max_tokens = 100000;

  turbo::Executor executor(w);
  turbo::Workflow taskflow;
 
  //for (subtask = 0; subtask <= subtasks; subtask = subtask == 0 ? subtask + 1 : subtask*2) {
   
    std::atomic<size_t> sums = 0;
    turbo::Pipeline pl(
      num_lines, 
      turbo::Pipe{
        turbo::PipeType::SERIAL, [max_tokens](turbo::Pipeflow& pf){
          //std::cout << turbo::stringify(pf.token(), '\n');
          if (pf.token() == max_tokens) {
            pf.stop();
          }
        }
      },

      turbo::Pipe{
        turbo::PipeType::PARALLEL, [subtask, &sums](turbo::Pipeflow&, turbo::Runtime& rt) {
          rt.run_and_wait([subtask, &sums](turbo::Subflow& sf) {
            for (size_t i = 0; i < subtask; ++i) {
              sf.emplace([&sums](){
                sums.fetch_add(1, std::memory_order_relaxed);  
              });
            }
          });
        }
      }
    );

    taskflow.composed_of(pl).name("pipeline");
    executor.run(taskflow).wait();
    EXPECT_TRUE(sums == subtask*max_tokens);
  //}
}


TEST(PipelineSP,PipelineSP_Subflow_1thread){
  pipeline_sp_runtime_subflow(1);
}

TEST(PipelineSP,PipelineSP_Subflow_2threads){
  pipeline_sp_runtime_subflow(2);
}

TEST(PipelineSP,PipelineSP_Subflow_3threads){
  pipeline_sp_runtime_subflow(3);
}

TEST(PipelineSP,PipelineSP_Subflow_4threads){
  pipeline_sp_runtime_subflow(4);
}

TEST(PipelineSP,PipelineSP_Subflow_5threads){
  pipeline_sp_runtime_subflow(5);
}

TEST(PipelineSP,PipelineSP_Subflow_6threads){
  pipeline_sp_runtime_subflow(6);
}

TEST(PipelineSP,PipelineSP_Subflow_7threads){
  pipeline_sp_runtime_subflow(7);
}

TEST(PipelineSP,PipelineSP_Subflow_8threads){
  pipeline_sp_runtime_subflow(8);
}


// --------------------------------------------------------
// Testcase: PipelineSPSPSPSP, PipelineSP_Subflow
// --------------------------------------------------------

void pipeline_spspspsp_runtime_subflow(size_t w) {
  
  size_t num_lines = 4;
  size_t subtasks = 8;
  size_t max_tokens = 4096;

  turbo::Executor executor(w);
  turbo::Workflow taskflow;
 
  for (size_t subtask = 0; subtask <= subtasks; subtask = subtask == 0 ? subtask + 1 : subtask*2) {
   
    taskflow.clear();
    
    std::atomic<size_t> sums = 0;
    turbo::Pipeline pl(
      num_lines, 
      turbo::Pipe{
        turbo::PipeType::SERIAL, [max_tokens](turbo::Pipeflow& pf){
          if (pf.token() == max_tokens) {
            pf.stop();
          }
        }
      },

      turbo::Pipe{
        turbo::PipeType::PARALLEL, [subtask, &sums](turbo::Pipeflow&, turbo::Runtime& rt) {
          rt.run_and_wait([subtask, &sums](turbo::Subflow& sf) {
            for (size_t i = 0; i < subtask; ++i) {
              sf.emplace([&sums](){
                sums.fetch_add(1, std::memory_order_relaxed);  
              });
            }
          });
        }
      },

      turbo::Pipe{
        turbo::PipeType::SERIAL, [subtask, &sums](turbo::Pipeflow&, turbo::Runtime& rt) {
          rt.run_and_wait([subtask, &sums](turbo::Subflow& sf) {
            for (size_t i = 0; i < subtask; ++i) {
              sf.emplace([&sums](){
                sums.fetch_add(1, std::memory_order_relaxed);  
              });
            }
          });
        }
      },

      turbo::Pipe{
        turbo::PipeType::PARALLEL, [subtask, &sums](turbo::Pipeflow&, turbo::Runtime& rt) {
          rt.run_and_wait([subtask, &sums](turbo::Subflow& sf) {
            for (size_t i = 0; i < subtask; ++i) {
              sf.emplace([&sums](){
                sums.fetch_add(1, std::memory_order_relaxed);  
              });
            }
          });
        }
      },

      turbo::Pipe{
        turbo::PipeType::SERIAL, [subtask, &sums](turbo::Pipeflow&, turbo::Runtime& rt) {
          rt.run_and_wait([subtask, &sums](turbo::Subflow& sf) {
            for (size_t i = 0; i < subtask; ++i) {
              sf.emplace([&sums](){
                sums.fetch_add(1, std::memory_order_relaxed);  
              });
            }
          });
        }
      },

      turbo::Pipe{
        turbo::PipeType::PARALLEL, [subtask, &sums](turbo::Pipeflow&, turbo::Runtime& rt) {
          rt.run_and_wait([subtask, &sums](turbo::Subflow& sf) {
            for (size_t i = 0; i < subtask; ++i) {
              sf.emplace([&sums](){
                sums.fetch_add(1, std::memory_order_relaxed);  
              });
            }
          });
        }
      },

      turbo::Pipe{
        turbo::PipeType::SERIAL, [subtask, &sums](turbo::Pipeflow&, turbo::Runtime& rt) {
          rt.run_and_wait([subtask, &sums](turbo::Subflow& sf) {
            for (size_t i = 0; i < subtask; ++i) {
              sf.emplace([&sums](){
                sums.fetch_add(1, std::memory_order_relaxed);  
              });
            }
          });
        }
      },

      turbo::Pipe{
        turbo::PipeType::PARALLEL, [subtask, &sums](turbo::Pipeflow&, turbo::Runtime& rt) {
          rt.run_and_wait([subtask, &sums](turbo::Subflow& sf) {
            for (size_t i = 0; i < subtask; ++i) {
              sf.emplace([&sums](){
                sums.fetch_add(1, std::memory_order_relaxed);  
              });
            }
          });
        }
      }
    );

    taskflow.composed_of(pl).name("pipeline");
    executor.run(taskflow).wait();
    EXPECT_TRUE(sums == subtask*max_tokens*7);
  }
}


TEST(PipelineSPSPSPSP, PipelineSP_Subflow_1thread){
  pipeline_spspspsp_runtime_subflow(1);
}

TEST(PipelineSPSPSPSP, PipelineSP_Subflow_2threads){
  pipeline_spspspsp_runtime_subflow(2);
}

TEST(PipelineSPSPSPSP, PipelineSP_Subflow_3threads){
  pipeline_spspspsp_runtime_subflow(3);
}

TEST(PipelineSPSPSPSP, PipelineSP_Subflow_4threads){
  pipeline_spspspsp_runtime_subflow(4);
}

TEST(PipelineSPSPSPSP, PipelineSP_Subflow_5threads){
  pipeline_spspspsp_runtime_subflow(5);
}

TEST(PipelineSPSPSPSP, PipelineSP_Subflow_6threads){
  pipeline_spspspsp_runtime_subflow(6);
}

TEST(PipelineSPSPSPSP, PipelineSP_Subflow_7threads){
  pipeline_spspspsp_runtime_subflow(7);
}

TEST(PipelineSPSPSPSP, PipelineSP_Subflow_8threads){
  pipeline_spspspsp_runtime_subflow(8);
}


// --------------------------------------------------------
// Testcase: PipelineSPSPSPSP, PipelineSP_IrregularSubflow
// --------------------------------------------------------

void pipeline_spspspsp_runtime_irregular_subflow(size_t w) {
  
  size_t num_lines = 4;
  size_t max_tokens = 32767;

  turbo::Executor executor(w);
  turbo::Workflow taskflow;
 
  std::atomic<size_t> sums = 0;
  
  turbo::Pipeline pl(
    num_lines, 
    turbo::Pipe{
      turbo::PipeType::SERIAL, [max_tokens](turbo::Pipeflow& pf){
        if (pf.token() == max_tokens) {
          pf.stop();
        }
      }
    },

    /* subflow has the following dependency
     *    
     *     |--> B
     *  A--|
     *     |--> C
     */
    turbo::Pipe{
      turbo::PipeType::PARALLEL, [&sums](turbo::Pipeflow&, turbo::Runtime& rt) {
        rt.run_and_wait([&sums](turbo::Subflow& sf) {
          auto A = sf.emplace([&sums]() { sums.fetch_add(1, std::memory_order_relaxed); });
          auto B = sf.emplace([&sums]() { sums.fetch_add(1, std::memory_order_relaxed); });
          auto C = sf.emplace([&sums]() { sums.fetch_add(1, std::memory_order_relaxed); });
          A.precede(B, C);
        });
      }
    },

    /* subflow has the following dependency
     *
     *     |--> B--| 
     *     |       v
     *  A--|       D
     *     |       ^
     *     |--> C--|
     *
     */
    turbo::Pipe{
      turbo::PipeType::SERIAL, [&sums](turbo::Pipeflow&, turbo::Runtime& rt) {
        rt.run_and_wait([&sums](turbo::Subflow& sf) {
          auto A = sf.emplace([&sums]() { sums.fetch_add(1, std::memory_order_relaxed); });
          auto B = sf.emplace([&sums]() { sums.fetch_add(1, std::memory_order_relaxed); });
          auto C = sf.emplace([&sums]() { sums.fetch_add(1, std::memory_order_relaxed); });
          auto D = sf.emplace([&sums]() { sums.fetch_add(1, std::memory_order_relaxed); });
          A.precede(B, C);
          D.succeed(B, C);
        });
      }
    },

    /* subflow has the following dependency
     *
     *       |--> C 
     *       |       
     *  A--> B       
     *       |       
     *       |--> D 
     *
     */
    turbo::Pipe{
      turbo::PipeType::PARALLEL, [&sums](turbo::Pipeflow&, turbo::Runtime& rt) {
        rt.run_and_wait([&sums](turbo::Subflow& sf) {
          auto A = sf.emplace([&sums]() { sums.fetch_add(1, std::memory_order_relaxed); });
          auto B = sf.emplace([&sums]() { sums.fetch_add(1, std::memory_order_relaxed); });
          auto C = sf.emplace([&sums]() { sums.fetch_add(1, std::memory_order_relaxed); });
          auto D = sf.emplace([&sums]() { sums.fetch_add(1, std::memory_order_relaxed); });
          A.precede(B);
          B.precede(C, D);
        });
      }
    },

    /* subflow has the following dependency
     *
     *     |--> B--|   |--> E
     *     |       v   |
     *  A--|       D --| 
     *     |       ^   |
     *     |--> C--|   |--> F
     *
     */
    turbo::Pipe{
      turbo::PipeType::SERIAL, [&sums](turbo::Pipeflow&, turbo::Runtime& rt) {
        rt.run_and_wait([&sums](turbo::Subflow& sf) {
          auto A = sf.emplace([&sums]() { sums.fetch_add(1, std::memory_order_relaxed); });
          auto B = sf.emplace([&sums]() { sums.fetch_add(1, std::memory_order_relaxed); });
          auto C = sf.emplace([&sums]() { sums.fetch_add(1, std::memory_order_relaxed); });
          auto D = sf.emplace([&sums]() { sums.fetch_add(1, std::memory_order_relaxed); });
          auto E = sf.emplace([&sums]() { sums.fetch_add(1, std::memory_order_relaxed); });
          auto F = sf.emplace([&sums]() { sums.fetch_add(1, std::memory_order_relaxed); });
          A.precede(B, C);
          D.succeed(B, C);
          D.precede(E, F);
        });
      }
    },

    /* subflow has the following dependency
     *
     *  A --> B --> C --> D -->  E
     *
     */
    turbo::Pipe{
      turbo::PipeType::PARALLEL, [&sums](turbo::Pipeflow&, turbo::Runtime& rt) {
        rt.run_and_wait([&sums](turbo::Subflow& sf) {
          auto A = sf.emplace([&sums]() { sums.fetch_add(1, std::memory_order_relaxed); });
          auto B = sf.emplace([&sums]() { sums.fetch_add(1, std::memory_order_relaxed); });
          auto C = sf.emplace([&sums]() { sums.fetch_add(1, std::memory_order_relaxed); });
          auto D = sf.emplace([&sums]() { sums.fetch_add(1, std::memory_order_relaxed); });
          auto E = sf.emplace([&sums]() { sums.fetch_add(1, std::memory_order_relaxed); });
          A.precede(B);
          B.precede(C);
          C.precede(D);
          D.precede(E);
        });
      }
    },

    /* subflow has the following dependency
     *    
     *        |-----------|
     *        |           v
     *  A --> B --> C --> D -->  E
     *              |            ^
     *              |------------|
     *
     */
    turbo::Pipe{
      turbo::PipeType::SERIAL, [&sums](turbo::Pipeflow&, turbo::Runtime& rt) {
        rt.run_and_wait([&sums](turbo::Subflow& sf) {
          auto A = sf.emplace([&sums]() { sums.fetch_add(1, std::memory_order_relaxed); });
          auto B = sf.emplace([&sums]() { sums.fetch_add(1, std::memory_order_relaxed); });
          auto C = sf.emplace([&sums]() { sums.fetch_add(1, std::memory_order_relaxed); });
          auto D = sf.emplace([&sums]() { sums.fetch_add(1, std::memory_order_relaxed); });
          auto E = sf.emplace([&sums]() { sums.fetch_add(1, std::memory_order_relaxed); });
          A.precede(B);
          B.precede(C, D);
          C.precede(D, E);
          D.precede(E);
        });
      }
    },

    /* subflow has the following dependency
     *    
     *  |-----------|
     *  |           v
     *  A --> B --> C --> D 
     *  |                 ^
     *  |-----------------|
     *
     */
    turbo::Pipe{
      turbo::PipeType::PARALLEL, [&sums](turbo::Pipeflow&, turbo::Runtime& rt) {
        rt.run_and_wait([&sums](turbo::Subflow& sf) {
          auto A = sf.emplace([&sums]() { sums.fetch_add(1, std::memory_order_relaxed); });
          auto B = sf.emplace([&sums]() { sums.fetch_add(1, std::memory_order_relaxed); });
          auto C = sf.emplace([&sums]() { sums.fetch_add(1, std::memory_order_relaxed); });
          auto D = sf.emplace([&sums]() { sums.fetch_add(1, std::memory_order_relaxed); });
          A.precede(B, C, D);
          B.precede(C);
          C.precede(D);
        });
      }
    }
  );

  taskflow.composed_of(pl).name("pipeline");
  executor.run(taskflow).wait();

  //taskflow.dump(std::cout);
  // there are 31 spawned subtasks in total
  EXPECT_TRUE(sums == 31*max_tokens);
}


TEST(PipelineSPSPSPSP, PipelineSP_Irregular_Subflow_1thread){
  pipeline_spspspsp_runtime_irregular_subflow(1);
}

TEST(PipelineSPSPSPSP, PipelineSP_Irregular_Subflow_2threads){
  pipeline_spspspsp_runtime_irregular_subflow(2);
}

TEST(PipelineSPSPSPSP, PipelineSP_Irregular_Subflow_3threads){
  pipeline_spspspsp_runtime_irregular_subflow(3);
}

TEST(PipelineSPSPSPSP, PipelineSP_Irregular_Subflow_4threads){
  pipeline_spspspsp_runtime_irregular_subflow(4);
}

TEST(PipelineSPSPSPSP, PipelineSP_Irregular_Subflow_5threads){
  pipeline_spspspsp_runtime_irregular_subflow(5);
}

TEST(PipelineSPSPSPSP, PipelineSP_Irregular_Subflow_6threads){
  pipeline_spspspsp_runtime_irregular_subflow(6);
}

TEST(PipelineSPSPSPSP, PipelineSP_Irregular_Subflow_7threads){
  pipeline_spspspsp_runtime_irregular_subflow(7);
}

TEST(PipelineSPSPSPSP, PipelineSP_Irregular_Subflow_8threads){
  pipeline_spspspsp_runtime_irregular_subflow(8);
}

// --------------------------------------------------------
// Testcase: ScalablePipelineSPSPSPSP, PipelineSP_Subflow
// --------------------------------------------------------

void scalable_pipeline_spspspsp_runtime_subflow(size_t w) {
  
  size_t num_lines = 4;
  size_t subtasks = 8;
  size_t max_tokens = 4096;

  turbo::Executor executor(w);
  turbo::Workflow taskflow;

  using pipe_t = turbo::Pipe<std::function<void(turbo::Pipeflow&, turbo::Runtime&)>>;
  std::vector<pipe_t> pipes;

  turbo::ScalablePipeline<std::vector<pipe_t>::iterator> sp;
 
  for (size_t subtask = 0; subtask <= subtasks; subtask = subtask == 0 ? subtask + 1 : subtask*2) {
   
    taskflow.clear();
    pipes.clear();
    
    std::atomic<size_t> sums = 0;

    pipes.emplace_back(turbo::PipeType::SERIAL, [max_tokens](turbo::Pipeflow& pf, turbo::Runtime&){
      if (pf.token() == max_tokens) {
        pf.stop();
      }
    });


    pipes.emplace_back(turbo::PipeType::PARALLEL, [subtask, &sums](turbo::Pipeflow&, turbo::Runtime& rt) {
      rt.run_and_wait([subtask, &sums](turbo::Subflow& sf) {
        for (size_t i = 0; i < subtask; ++i) {
          sf.emplace([&sums](){
            sums.fetch_add(1, std::memory_order_relaxed);  
          });
        }
      });
    });

    pipes.emplace_back(turbo::PipeType::SERIAL, [subtask, &sums](turbo::Pipeflow&, turbo::Runtime& rt) {
      rt.run_and_wait([subtask, &sums](turbo::Subflow& sf) {
        for (size_t i = 0; i < subtask; ++i) {
          sf.emplace([&sums](){
            sums.fetch_add(1, std::memory_order_relaxed);  
          });
        }
      });
    });
    
    pipes.emplace_back(turbo::PipeType::PARALLEL, [subtask, &sums](turbo::Pipeflow&, turbo::Runtime& rt) {
      rt.run_and_wait([subtask, &sums](turbo::Subflow& sf) {
        for (size_t i = 0; i < subtask; ++i) {
          sf.emplace([&sums](){
            sums.fetch_add(1, std::memory_order_relaxed);  
          });
        }
      });
    });
    
    pipes.emplace_back(turbo::PipeType::SERIAL, [subtask, &sums](turbo::Pipeflow&, turbo::Runtime& rt) {
      rt.run_and_wait([subtask, &sums](turbo::Subflow& sf) {
        for (size_t i = 0; i < subtask; ++i) {
          sf.emplace([&sums](){
            sums.fetch_add(1, std::memory_order_relaxed);  
          });
        }
      });
    });

    pipes.emplace_back(turbo::PipeType::PARALLEL, [subtask, &sums](turbo::Pipeflow&, turbo::Runtime& rt) {
      rt.run_and_wait([subtask, &sums](turbo::Subflow& sf) {
        for (size_t i = 0; i < subtask; ++i) {
          sf.emplace([&sums](){
            sums.fetch_add(1, std::memory_order_relaxed);  
          });
        }
      });
    });
    

    pipes.emplace_back(turbo::PipeType::SERIAL, [subtask, &sums](turbo::Pipeflow&, turbo::Runtime& rt) {
      rt.run_and_wait([subtask, &sums](turbo::Subflow& sf) {
        for (size_t i = 0; i < subtask; ++i) {
          sf.emplace([&sums](){
            sums.fetch_add(1, std::memory_order_relaxed);  
          });
        }
      });
    });

    pipes.emplace_back(turbo::PipeType::PARALLEL, [subtask, &sums](turbo::Pipeflow&, turbo::Runtime& rt) {
      rt.run_and_wait([subtask, &sums](turbo::Subflow& sf) {
        for (size_t i = 0; i < subtask; ++i) {
          sf.emplace([&sums](){
            sums.fetch_add(1, std::memory_order_relaxed);  
          });
        }
      });
    });
    //
    
    sp.reset(num_lines, pipes.begin(), pipes.end());

    taskflow.composed_of(sp).name("pipeline");
    executor.run(taskflow).wait();
    EXPECT_TRUE(sums == subtask*max_tokens*7);
  }
}


TEST(ScalablePipelineSPSPSPSP, PipelineSP_Subflow_1thread){
  scalable_pipeline_spspspsp_runtime_subflow(1);
}

TEST(ScalablePipelineSPSPSPSP, PipelineSP_Subflow_2threads){
  scalable_pipeline_spspspsp_runtime_subflow(2);
}

TEST(ScalablePipelineSPSPSPSP, PipelineSP_Subflow_3threads){
  scalable_pipeline_spspspsp_runtime_subflow(3);
}

TEST(ScalablePipelineSPSPSPSP, PipelineSP_Subflow_4threads){
  scalable_pipeline_spspspsp_runtime_subflow(4);
}

TEST(ScalablePipelineSPSPSPSP, PipelineSP_Subflow_5threads){
  scalable_pipeline_spspspsp_runtime_subflow(5);
}

TEST(ScalablePipelineSPSPSPSP, PipelineSP_Subflow_6threads){
  scalable_pipeline_spspspsp_runtime_subflow(6);
}

TEST(ScalablePipelineSPSPSPSP, PipelineSP_Subflow_7threads){
  scalable_pipeline_spspspsp_runtime_subflow(7);
}

TEST(ScalablePipelineSPSPSPSP, PipelineSP_Subflow_8threads){
  scalable_pipeline_spspspsp_runtime_subflow(8);
}

