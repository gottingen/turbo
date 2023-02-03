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

#include <stdlib.h>     /* srand, rand */
#include <time.h>       /* time */

#include <mutex>
#include <algorithm>



// ----------------------------------------------------------------------------
// one pipe (S), L lines, W workers, defer to the previous token
// ----------------------------------------------------------------------------

void pipeline_1P_S_DeferPreviousToken(size_t L, unsigned w) {

  turbo::Executor executor(w);

  const size_t maxN = 3;

  for(size_t N = 0; N <= maxN; N++) {
    
    std::vector<size_t> collection1;
    std::vector<size_t> deferrals;

    turbo::Workflow taskflow;

    turbo::Pipeline pl(
      L,
      turbo::Pipe{turbo::PipeType::SERIAL, [N, &collection1, L, &deferrals](auto& pf) mutable {
        if(pf.token() == N) {
          pf.stop();
          return;
        }
        else {
          switch(pf.num_deferrals()) {
            case 0:
              if (pf.token() == 0) {
                //printf("Stage 1 : token %zu on line %zu\n", pf.token() ,pf.line());
                collection1.push_back(pf.token());
                deferrals.push_back(pf.num_deferrals());
              }
              else {
                pf.defer(pf.token()-1);
              }
            break;

            case 1:
              //printf("Stage 1 : token %zu on line %zu\n", pf.token(), pf.line());
              collection1.push_back(pf.token());
              deferrals.push_back(pf.num_deferrals());
            break;
          }
          EXPECT_TRUE(pf.token() % L == pf.line());
        }
      }}
    );

    auto pipeline = taskflow.composed_of(pl).name("module_of_pipeline");
    auto test = taskflow.emplace([&](){
      EXPECT_TRUE(collection1.size() == N);

      for (size_t i = 0; i < N; ++i) {
        EXPECT_TRUE(collection1[i] == i);
      }

      EXPECT_TRUE(deferrals.size() == N);
      for (size_t i = 0; i < deferrals.size(); ++i) {
        if (i == 0) {
          EXPECT_TRUE(deferrals[i] == 0);
        }
        else {
          EXPECT_TRUE(deferrals[i] == 1);
        }
      }
    }).name("test");

    pipeline.precede(test);

    executor.run_n(taskflow, 1, [&]() mutable {
      collection1.clear();
      deferrals.clear();
    }).get();
  }
}

// one pipe (S)
TEST(Pipeline, 1P_S_DeferPreviousToken_1L_1W) {
  pipeline_1P_S_DeferPreviousToken(1, 1);
}

TEST(Pipeline, 1P_S_DeferPreviousToken_1L_2W) {
  pipeline_1P_S_DeferPreviousToken(1, 2);
}

TEST(Pipeline, 1P_S_DeferPreviousToken_1L_3W) {
  pipeline_1P_S_DeferPreviousToken(1, 3);
}

TEST(Pipeline, 1P_S_DeferPreviousToken_1L_4W) {
  pipeline_1P_S_DeferPreviousToken(1, 4);
}

TEST(Pipeline, 1P_S_DeferPreviousToken_2L_1W) {
  pipeline_1P_S_DeferPreviousToken(2, 1);
}

TEST(Pipeline, 1P_S_DeferPreviousToken_2L_2W) {
  pipeline_1P_S_DeferPreviousToken(2, 2);
}

TEST(Pipeline, 1P_S_DeferPreviousToken_2L_3W) {
  pipeline_1P_S_DeferPreviousToken(2, 3);
}

TEST(Pipeline, 1P_S_DeferPreviousToken_2L_4W) {
  pipeline_1P_S_DeferPreviousToken(2, 4);
}

TEST(Pipeline, 1P_S_DeferPreviousToken_3L_1W) {
  pipeline_1P_S_DeferPreviousToken(3, 1);
}

TEST(Pipeline, 1P_S_DeferPreviousToken_3L_2W) {
  pipeline_1P_S_DeferPreviousToken(3, 2);
}

TEST(Pipeline, 1P_S_DeferPreviousToken_3L_3W) {
  pipeline_1P_S_DeferPreviousToken(3, 3);
}

TEST(Pipeline, 1P_S_DeferPreviousToken_3L_4W) {
  pipeline_1P_S_DeferPreviousToken(3, 4);
}

TEST(Pipeline, 1P_S_DeferPreviousToken_4L_1W) {
  pipeline_1P_S_DeferPreviousToken(4, 1);
}

TEST(Pipeline, 1P_S_DeferPreviousToken_4L_2W) {
  pipeline_1P_S_DeferPreviousToken(4, 2);
}

TEST(Pipeline, 1P_S_DeferPreviousToken_4L_3W) {
  pipeline_1P_S_DeferPreviousToken(4, 3);
}

TEST(Pipeline, 1P_S_DeferPreviousToken_4L_4W) {
  pipeline_1P_S_DeferPreviousToken(4, 4);
}


// ----------------------------------------------------------------------------
// two pipes (SS), L lines, W workers, defer to the previous token
// ----------------------------------------------------------------------------

void pipeline_2P_SS_DeferPreviousToken(size_t L, unsigned w) {

  turbo::Executor executor(w);

  const size_t maxN = 100;

  std::vector<std::array<size_t, 2>> mybuffer(L);

  for(size_t N = 0; N <= maxN; N++) {
    
    std::vector<size_t> collection1;
    std::vector<size_t> collection2;
    std::vector<size_t> deferrals1;
    std::vector<size_t> deferrals2;

    std::mutex mutex;

    turbo::Workflow taskflow;

    turbo::Pipeline pl(
      L,
      turbo::Pipe{turbo::PipeType::SERIAL, [N, &collection1, &mybuffer, L, &deferrals1](auto& pf) mutable {
        if(pf.token() == N) {
          pf.stop();
          return;
        }
        else {
          switch(pf.num_deferrals()) {
            case 0:
              if (pf.token() == 0) {
                //printf("Stage 1 : token %zu on line %zu\n", pf.token() ,pf.line());
                collection1.push_back(pf.token());
                mybuffer[pf.line()][pf.pipe()] = pf.token(); 
                deferrals1.push_back(pf.num_deferrals());          
              }
              else {
                pf.defer(pf.token()-1);
              }
            break;

            case 1:
              //printf("Stage 1 : token %zu on line %zu\n", pf.token(), pf.line());
              collection1.push_back(pf.token());
              mybuffer[pf.line()][pf.pipe()] = pf.token();           
              deferrals1.push_back(pf.num_deferrals());          
            break;
          }
          EXPECT_TRUE(pf.token() % L == pf.line());
        }
      }},

      turbo::Pipe{turbo::PipeType::SERIAL, [&mybuffer, &mutex, &collection2, L, &deferrals2](auto& pf) mutable {
        EXPECT_TRUE(pf.token() % L == pf.line());
        {
          std::scoped_lock<std::mutex> lock(mutex);
          collection2.push_back(mybuffer[pf.line()][pf.pipe() - 1]);
          deferrals2.push_back(pf.num_deferrals());
        }

        if (pf.token() == 0) {
          EXPECT_TRUE(pf.num_deferrals() == 0);
        }
        else {
          EXPECT_TRUE(pf.num_deferrals() == 1);
        }
        //printf("Stage 2 : token %zu at line %zu\n", pf.token(), pf.line());
      }}
    );

    auto pipeline = taskflow.composed_of(pl).name("module_of_pipeline");
    auto test = taskflow.emplace([&](){
      EXPECT_TRUE(collection1.size() == N);
      EXPECT_TRUE(collection2.size() == N);
      for (size_t i = 0; i < N; ++i) {
        EXPECT_TRUE(collection1[i] == i);
        EXPECT_TRUE(collection2[i] == i);
      }

      EXPECT_TRUE(deferrals1.size() == N);
      EXPECT_TRUE(deferrals2.size() == N);
      for (size_t i = 0; i < deferrals1.size(); ++i) {
        if (i == 0) {
          EXPECT_TRUE(deferrals1[i] == 0);
          EXPECT_TRUE(deferrals2[i] == 0);
        }
        else {
          EXPECT_TRUE(deferrals1[i] == 1);
          EXPECT_TRUE(deferrals2[i] == 1);
        }
      }
    }).name("test");

    pipeline.precede(test);

    executor.run_n(taskflow, 1, [&]() mutable {
      collection1.clear();
      collection2.clear();
      deferrals1.clear();
      deferrals2.clear();
    }).get();
  }
}

// two pipes (SS)
TEST(Pipeline, 2p_SS_DeferPreviousToken_1L_1W) {
  pipeline_2P_SS_DeferPreviousToken(1, 1);
}

TEST(Pipeline, 2p_SS_DeferPreviousToken_1L_2W) {
  pipeline_2P_SS_DeferPreviousToken(1, 2);
}

TEST(Pipeline, 2p_SS_DeferPreviousToken_1L_3W) {
  pipeline_2P_SS_DeferPreviousToken(1, 3);
}

TEST(Pipeline, 2p_SS_DeferPreviousToken_1L_4W) {
  pipeline_2P_SS_DeferPreviousToken(1, 4);
}

TEST(Pipeline, 2p_SS_DeferPreviousToken_2L_1W) {
  pipeline_2P_SS_DeferPreviousToken(2, 1);
}

TEST(Pipeline, 2p_SS_DeferPreviousToken_2L_2W) {
  pipeline_2P_SS_DeferPreviousToken(2, 2);
}

TEST(Pipeline, 2p_SS_DeferPreviousToken_2L_3W) {
  pipeline_2P_SS_DeferPreviousToken(2, 3);
}

TEST(Pipeline, 2p_SS_DeferPreviousToken_2L_4W) {
  pipeline_2P_SS_DeferPreviousToken(2, 4);
}

TEST(Pipeline, 2p_SS_DeferPreviousToken_3L_1W) {
  pipeline_2P_SS_DeferPreviousToken(3, 1);
}

TEST(Pipeline, 2p_SS_DeferPreviousToken_3L_2W) {
  pipeline_2P_SS_DeferPreviousToken(3, 2);
}

TEST(Pipeline, 2p_SS_DeferPreviousToken_3L_3W) {
  pipeline_2P_SS_DeferPreviousToken(3, 3);
}

TEST(Pipeline, 2p_SS_DeferPreviousToken_3L_4W) {
  pipeline_2P_SS_DeferPreviousToken(3, 4);
}

TEST(Pipeline, 2p_SS_DeferPreviousToken_4L_1W) {
  pipeline_2P_SS_DeferPreviousToken(4, 1);
}

TEST(Pipeline, 2p_SS_DeferPreviousToken_4L_2W) {
  pipeline_2P_SS_DeferPreviousToken(4, 2);
}

TEST(Pipeline, 2p_SS_DeferPreviousToken_4L_3W) {
  pipeline_2P_SS_DeferPreviousToken(4, 3);
}

TEST(Pipeline, 2p_SS_DeferPreviousToken_4L_4W) {
  pipeline_2P_SS_DeferPreviousToken(4, 4);
}


// ----------------------------------------------------------------------------
// two pipes (SP), L lines, W workers, defer to the previous token
// ----------------------------------------------------------------------------

void pipeline_2P_SP_DeferPreviousToken(size_t L, unsigned w) {

  turbo::Executor executor(w);

  const size_t maxN = 100;

  std::vector<std::array<size_t, 2>> mybuffer(L);

  for(size_t N = 0; N <= maxN; N++) {
    
    std::vector<size_t> collection1;
    std::vector<size_t> collection2;
    std::vector<size_t> deferrals1;
    std::vector<size_t> deferrals2(N);
    std::mutex mutex;

    turbo::Workflow taskflow;

    turbo::Pipeline pl(
      L,
      turbo::Pipe{turbo::PipeType::SERIAL, [N, &collection1, &mybuffer, L, &deferrals1](auto& pf) mutable {
        if(pf.token() == N) {
          pf.stop();
          return;
        }
        else {
          switch(pf.num_deferrals()) {
            case 0:
              if (pf.token() == 0) {
                //printf("Stage 1 : token %zu on line %zu\n", pf.token() ,pf.line());
                collection1.push_back(pf.token());
                deferrals1.push_back(pf.num_deferrals());
                mybuffer[pf.line()][pf.pipe()] = pf.token();           
              }
              else {
                pf.defer(pf.token()-1);
              }
            break;

            case 1:
              //printf("Stage 1 : token %zu on line %zu\n", pf.token(), pf.line());
              collection1.push_back(pf.token());
              deferrals1.push_back(pf.num_deferrals());
              mybuffer[pf.line()][pf.pipe()] = pf.token();           
            break;
          }
          EXPECT_TRUE(pf.token() % L == pf.line());
        }
      }},

      turbo::Pipe{turbo::PipeType::PARALLEL, [&mybuffer, &mutex, &collection2, L, &deferrals2](auto& pf) mutable {
        EXPECT_TRUE(pf.token() % L == pf.line());
        {
          std::scoped_lock<std::mutex> lock(mutex);
          collection2.push_back(mybuffer[pf.line()][pf.pipe() - 1]);
          deferrals2[pf.token()] = pf.num_deferrals();
        }

        if (pf.token() == 0) {
          EXPECT_TRUE(pf.num_deferrals() == 0);
        }
        else {
          EXPECT_TRUE(pf.num_deferrals() == 1);
        }
    
        //printf("Stage 2 : token %zu at line %zu\n", pf.token(), pf.line());
      }}
    );

    auto pipeline = taskflow.composed_of(pl).name("module_of_pipeline");
    auto test = taskflow.emplace([&](){
      EXPECT_TRUE(collection1.size() == N);
      EXPECT_TRUE(collection2.size() == N);
      sort(collection2.begin(), collection2.end());
  
      for (size_t i = 0; i < N; ++i) {
        EXPECT_TRUE(collection1[i] == i);
        EXPECT_TRUE(collection2[i] == i);
      }

      EXPECT_TRUE(deferrals1.size() == N);
      EXPECT_TRUE(deferrals2.size() == N);
      for (size_t i = 0; i < N; ++i) {
        if (i == 0) {
          EXPECT_TRUE(deferrals1[0] == 0);
          EXPECT_TRUE(deferrals2[0] == 0);
        }
        else {
          EXPECT_TRUE(deferrals1[i] == 1);
          EXPECT_TRUE(deferrals2[i] == 1);
        }
      }
    }).name("test");

    pipeline.precede(test);

    executor.run_n(taskflow, 1, [&]() mutable {
      collection1.clear();
      collection2.clear();
      deferrals1.clear();
      deferrals2.clear();
    }).get();
  }
}

// two pipes (SP)
TEST(Pipeline, 2p_PS_DeferPreviousToken_1L_1W) {
  pipeline_2P_SP_DeferPreviousToken(1, 1);
}

TEST(Pipeline, 2p_PS_DeferPreviousToken_1L_2W) {
  pipeline_2P_SP_DeferPreviousToken(1, 2);
}

TEST(Pipeline, 2p_PS_DeferPreviousToken_1L_3W) {
  pipeline_2P_SP_DeferPreviousToken(1, 3);
}

TEST(Pipeline, 2p_PS_DeferPreviousToken_1L_4W) {
  pipeline_2P_SP_DeferPreviousToken(1, 4);
}

TEST(Pipeline, 2p_PS_DeferPreviousToken_2L_1W) {
  pipeline_2P_SP_DeferPreviousToken(2, 1);
}

TEST(Pipeline, 2p_PS_DeferPreviousToken_2L_2W) {
  pipeline_2P_SP_DeferPreviousToken(2, 2);
}

TEST(Pipeline, 2p_PS_DeferPreviousToken_2L_3W) {
  pipeline_2P_SP_DeferPreviousToken(2, 3);
}

TEST(Pipeline, 2p_PS_DeferPreviousToken_2L_4W) {
  pipeline_2P_SP_DeferPreviousToken(2, 4);
}

TEST(Pipeline, 2p_PS_DeferPreviousToken_3L_1W) {
  pipeline_2P_SP_DeferPreviousToken(3, 1);
}

TEST(Pipeline, 2p_PS_DeferPreviousToken_3L_2W) {
  pipeline_2P_SP_DeferPreviousToken(3, 2);
}

TEST(Pipeline, 2p_PS_DeferPreviousToken_3L_3W) {
  pipeline_2P_SP_DeferPreviousToken(3, 3);
}

TEST(Pipeline, 2p_PS_DeferPreviousToken_3L_4W) {
  pipeline_2P_SP_DeferPreviousToken(3, 4);
}

TEST(Pipeline, 2p_PS_DeferPreviousToken_4L_1W) {
  pipeline_2P_SP_DeferPreviousToken(4, 1);
}

TEST(Pipeline, 2p_PS_DeferPreviousToken_4L_2W) {
  pipeline_2P_SP_DeferPreviousToken(4, 2);
}

TEST(Pipeline, 2p_PS_DeferPreviousToken_4L_3W) {
  pipeline_2P_SP_DeferPreviousToken(4, 3);
}

TEST(Pipeline, 2p_PS_DeferPreviousToken_4L_4W) {
  pipeline_2P_SP_DeferPreviousToken(4, 4);
}


// ----------------------------------------------------------------------------
// one pipe (S), L lines, W workers
//
// defer to the next token, pf.defer(pf.token()+1) except the max token
// ----------------------------------------------------------------------------

void pipeline_1P_S_DeferNextToken(size_t L, unsigned w, turbo::PipeType) {

  turbo::Executor executor(w);

  const size_t maxN = 100;

  std::vector<int> source(maxN);
  std::iota(source.begin(), source.end(), 0);

  std::vector<size_t> collection1;
  std::vector<size_t> deferrals;

  for(size_t N = 1; N <= maxN; N++) {

    turbo::Workflow taskflow;
    deferrals.resize(N);

    turbo::Pipeline pl(
      L,
      turbo::Pipe{turbo::PipeType::SERIAL, [N, &collection1, &deferrals](auto& pf) mutable {
        if(pf.token() == N) {
          pf.stop();
          return;
        }
        else {
          switch(pf.num_deferrals()) {
            case 0:
              if (pf.token() < N-1) {
                pf.defer(pf.token()+1);
              }
              else {
                deferrals[pf.token()] = pf.num_deferrals();
                collection1.push_back(pf.token());
              }
            break;

            case 1:
              collection1.push_back(pf.token());
              deferrals[pf.token()] = pf.num_deferrals();
            break;
          }
        }
      }}
    );

    taskflow.composed_of(pl).name("module_of_pipeline");
    executor.run(taskflow).wait();
 
    EXPECT_TRUE(deferrals.size() == N); 
    for (size_t i = 0; i < deferrals.size()-1;++i) {
      EXPECT_TRUE(deferrals[i] == 1);
    }
    EXPECT_TRUE(deferrals[deferrals.size()-1] == 0);

    for (size_t i = 0; i < collection1.size(); ++i) {
      EXPECT_TRUE(i + collection1[i] == N-1);
    }
    
    collection1.clear();
    deferrals.clear();
  }
}

// one pipe 
TEST(Pipeline, 1P_S_DeferNextToken_1L_1W) {
  pipeline_1P_S_DeferNextToken(1, 1, turbo::PipeType::SERIAL);
}

TEST(Pipeline, 1P_S_DeferNextToken_1L_2W) {
  pipeline_1P_S_DeferNextToken(1, 2, turbo::PipeType::SERIAL);
}

TEST(Pipeline, 1P_S_DeferNextToken_1L_3W) {
  pipeline_1P_S_DeferNextToken(1, 3, turbo::PipeType::SERIAL);
}

TEST(Pipeline, 1P_S_DeferNextToken_1L_4W) {
  pipeline_1P_S_DeferNextToken(1, 4, turbo::PipeType::SERIAL);
}

TEST(Pipeline, 1P_S_DeferNextToken_2L_1W) {
  pipeline_1P_S_DeferNextToken(2, 1, turbo::PipeType::SERIAL);
}

TEST(Pipeline, 1P_S_DeferNextToken_2L_2W) {
  pipeline_1P_S_DeferNextToken(2, 2, turbo::PipeType::SERIAL);
}

TEST(Pipeline, 1P_S_DeferNextToken_2L_3W) {
  pipeline_1P_S_DeferNextToken(2, 3, turbo::PipeType::SERIAL);
}

TEST(Pipeline, 1P_S_DeferNextToken_2L_4W) {
  pipeline_1P_S_DeferNextToken(2, 4, turbo::PipeType::SERIAL);
}

TEST(Pipeline, 1P_S_DeferNextToken_3L_1W) {
  pipeline_1P_S_DeferNextToken(3, 1, turbo::PipeType::SERIAL);
}

TEST(Pipeline, 1P_S_DeferNextToken_3L_2W) {
  pipeline_1P_S_DeferNextToken(3, 2, turbo::PipeType::SERIAL);
}

TEST(Pipeline, 1P_S_DeferNextToken_3L_3W) {
  pipeline_1P_S_DeferNextToken(3, 3, turbo::PipeType::SERIAL);
}

TEST(Pipeline, 1P_S_DeferNextToken_3L_4W) {
  pipeline_1P_S_DeferNextToken(3, 4, turbo::PipeType::SERIAL);
}

TEST(Pipeline, 1P_S_DeferNextToken_4L_1W) {
  pipeline_1P_S_DeferNextToken(4, 1, turbo::PipeType::SERIAL);
}

TEST(Pipeline, 1P_S_DeferNextToken_4L_2W) {
  pipeline_1P_S_DeferNextToken(4, 2, turbo::PipeType::SERIAL);
}

TEST(Pipeline, 1P_S_DeferNextToken_4L_3W) {
  pipeline_1P_S_DeferNextToken(4, 3, turbo::PipeType::SERIAL);
}

TEST(Pipeline, 1P_S_DeferNextToken_4L_4W) {
  pipeline_1P_S_DeferNextToken(4, 4, turbo::PipeType::SERIAL);
}


// ----------------------------------------------------------------------------
// two pipes (SS), L lines, W workers
//
// defer to the next token, pf.defer(pf.token()+1) except the max token
// ----------------------------------------------------------------------------

void pipeline_2P_SS_DeferNextToken(size_t L, unsigned w, turbo::PipeType second_type) {

  turbo::Executor executor(w);

  const size_t maxN = 100;

  std::vector<int> source(maxN);
  std::iota(source.begin(), source.end(), 0);
  std::vector<size_t> mybuffer(L);

  std::vector<size_t> collection1;
  std::vector<size_t> collection2;
  std::vector<size_t> deferrals1;
  std::vector<size_t> deferrals2;

  for(size_t N = 1; N <= maxN; N++) {

    turbo::Workflow taskflow;
    deferrals1.resize(N);
    deferrals2.resize(N);

    //size_t value = (N-1)%L;
    //std::cout << "N = " << N << ", value = " << value << ", L = " << L << ", W = " << w << '\n';    
    turbo::Pipeline pl(
      L,
      turbo::Pipe{turbo::PipeType::SERIAL, [N, &mybuffer, &collection1, &deferrals1](auto& pf) mutable {
        if(pf.token() == N) {
          pf.stop();
          return;
        }
        else {
          switch(pf.num_deferrals()) {
            case 0:
              if (pf.token() < N-1) {
                pf.defer(pf.token()+1);
              }
              else {
                collection1.push_back(pf.token());
                deferrals1[pf.token()] = pf.num_deferrals();
                mybuffer[pf.line()] = pf.token();              
              }
            break;

            case 1:
              collection1.push_back(pf.token());
              deferrals1[pf.token()] = pf.num_deferrals();
              mybuffer[pf.line()] = pf.token();              
            break;
          }
        }
      }},

      turbo::Pipe{second_type, [&mybuffer, &collection2, &deferrals2](auto& pf) mutable {
        collection2.push_back(mybuffer[pf.line()]);
        deferrals2[pf.token()] = pf.num_deferrals();
      }}
    );

    taskflow.composed_of(pl).name("module_of_pipeline");
    executor.run(taskflow).wait();
   
    for (size_t i = 0; i < collection1.size(); ++i) {
      EXPECT_TRUE(i + collection1[i] == N-1);
      EXPECT_TRUE(i + collection2[i] == N-1);
    }
    
    EXPECT_TRUE(deferrals1.size() == N);
    EXPECT_TRUE(deferrals2.size() == N);
    for (size_t i = 0; i < deferrals1.size()-1; ++i) {
      EXPECT_TRUE(deferrals1[i] == 1);
      EXPECT_TRUE(deferrals2[i] == 1);
    }
    EXPECT_TRUE(deferrals1[deferrals1.size()-1] == 0);
    EXPECT_TRUE(deferrals2[deferrals2.size()-1] == 0);

    collection1.clear();
    collection2.clear();
    deferrals1.clear();
    deferrals2.clear();
  }
}

// two pipes 
TEST(Pipeline, 2p_SS_DeferNextToken_1L_1W) {
  pipeline_2P_SS_DeferNextToken(1, 1, turbo::PipeType::SERIAL);
}

TEST(Pipeline, 2p_SS_DeferNextToken_1L_2W) {
  pipeline_2P_SS_DeferNextToken(1, 2, turbo::PipeType::SERIAL);
}

TEST(Pipeline, 2p_SS_DeferNextToken_1L_3W) {
  pipeline_2P_SS_DeferNextToken(1, 3, turbo::PipeType::SERIAL);
}

TEST(Pipeline, 2p_SS_DeferNextToken_1L_4W) {
  pipeline_2P_SS_DeferNextToken(1, 4, turbo::PipeType::SERIAL);
}

TEST(Pipeline, 2p_SS_DeferNextToken_2L_1W) {
  pipeline_2P_SS_DeferNextToken(2, 1, turbo::PipeType::SERIAL);
}

TEST(Pipeline, 2p_SS_DeferNextToken_2L_2W) {
  pipeline_2P_SS_DeferNextToken(2, 2, turbo::PipeType::SERIAL);
}

TEST(Pipeline, 2p_SS_DeferNextToken_2L_3W) {
  pipeline_2P_SS_DeferNextToken(2, 3, turbo::PipeType::SERIAL);
}

TEST(Pipeline, 2p_SS_DeferNextToken_2L_4W) {
  pipeline_2P_SS_DeferNextToken(2, 4, turbo::PipeType::SERIAL);
}

TEST(Pipeline, 2p_SS_DeferNextToken_3L_1W) {
  pipeline_2P_SS_DeferNextToken(3, 1, turbo::PipeType::SERIAL);
}

TEST(Pipeline, 2p_SS_DeferNextToken_3L_2W) {
  pipeline_2P_SS_DeferNextToken(3, 2, turbo::PipeType::SERIAL);
}

TEST(Pipeline, 2p_SS_DeferNextToken_3L_3W) {
  pipeline_2P_SS_DeferNextToken(3, 3, turbo::PipeType::SERIAL);
}

TEST(Pipeline, 2p_SS_DeferNextToken_3L_4W) {
  pipeline_2P_SS_DeferNextToken(3, 4, turbo::PipeType::SERIAL);
}

TEST(Pipeline, 2p_SS_DeferNextToken_4L_1W) {
  pipeline_2P_SS_DeferNextToken(4, 1, turbo::PipeType::SERIAL);
}

TEST(Pipeline, 2p_SS_DeferNextToken_4L_2W) {
  pipeline_2P_SS_DeferNextToken(4, 2, turbo::PipeType::SERIAL);
}

TEST(Pipeline, 2p_SS_DeferNextToken_4L_3W) {
  pipeline_2P_SS_DeferNextToken(4, 3, turbo::PipeType::SERIAL);
}

TEST(Pipeline, 2p_SS_DeferNextToken_4L_4W) {
  pipeline_2P_SS_DeferNextToken(4, 4, turbo::PipeType::SERIAL);
}

// ----------------------------------------------------------------------------
// two pipes (SP), L lines, W workers
//
// defer to the next token, pf.defer(pf.token()+1) except the max token
// ----------------------------------------------------------------------------

void pipeline_2P_SP_DeferNextToken(size_t L, unsigned w) {

  turbo::Executor executor(w);

  const size_t maxN = 100;

  std::vector<int> source(maxN);
  std::iota(source.begin(), source.end(), 0);
  std::vector<size_t> mybuffer(L);

  std::vector<size_t> collection1;
  std::vector<size_t> collection2;
  std::vector<size_t> deferrals1;
  std::vector<size_t> deferrals2;
  std::mutex mtx;

  for(size_t N = 1; N <= maxN; N++) {

    turbo::Workflow taskflow;
    deferrals1.resize(N);
    deferrals2.resize(N);

    //std::cout << "N = " << N << ", value = " << value << ", L = " << L << ", W = " << w << '\n';    
    turbo::Pipeline pl(
      L,
      turbo::Pipe{turbo::PipeType::SERIAL, [N, &mybuffer, &collection1, &deferrals1](auto& pf) mutable {
        if(pf.token() == N) {
          pf.stop();
          return;
        }
        else {
          switch(pf.num_deferrals()) {
            case 0:
              if (pf.token() < N-1) {
                pf.defer(pf.token()+1);
              }
              else {
                collection1.push_back(pf.token());
                deferrals1[pf.token()] = pf.num_deferrals();
                mybuffer[pf.line()] = pf.token();              
              }
            break;

            case 1:
              collection1.push_back(pf.token());
              deferrals1[pf.token()] = pf.num_deferrals();
              mybuffer[pf.line()] = pf.token();              
            break;
          }
        }
      }},

      turbo::Pipe{turbo::PipeType::PARALLEL, [&mybuffer, &collection2, &mtx, &deferrals2](auto& pf) mutable {
        {
          std::unique_lock lk(mtx);
          collection2.push_back(mybuffer[pf.line()]);
          deferrals2[pf.token()] = pf.num_deferrals();
        }
      }}
    );

    taskflow.composed_of(pl).name("module_of_pipeline");
    executor.run(taskflow).wait();
  
    sort(collection2.begin(), collection2.end()); 
    for (size_t i = 0; i < collection1.size(); ++i) {
      EXPECT_TRUE(i + collection1[i] == N-1);
      EXPECT_TRUE(collection2[i] == i);
    }
    
    EXPECT_TRUE(deferrals1.size() == N);
    EXPECT_TRUE(deferrals2.size() == N);
    for (size_t i = 0; i < deferrals1.size()-1; ++i) {
      EXPECT_TRUE(deferrals1[i] == 1);
      EXPECT_TRUE(deferrals2[i] == 1);
    }
    EXPECT_TRUE(deferrals1[deferrals1.size()-1] == 0);
    EXPECT_TRUE(deferrals2[deferrals2.size()-1] == 0);
    
    collection1.clear();
    collection2.clear();
    deferrals1.clear();
    deferrals2.clear();
  }
}

// two pipes 
TEST(Pipeline, 2p_PS_DeferNextToken_1L_1W) {
  pipeline_2P_SP_DeferNextToken(1, 1);
}

TEST(Pipeline, 2p_PS_DeferNextToken_1L_2W) {
  pipeline_2P_SP_DeferNextToken(1, 2);
}

TEST(Pipeline, 2p_PS_DeferNextToken_1L_3W) {
  pipeline_2P_SP_DeferNextToken(1, 3);
}

TEST(Pipeline, 2p_PS_DeferNextToken_1L_4W) {
  pipeline_2P_SP_DeferNextToken(1, 4);
}

TEST(Pipeline, 2p_PS_DeferNextToken_2L_1W) {
  pipeline_2P_SP_DeferNextToken(2, 1);
}

TEST(Pipeline, 2p_PS_DeferNextToken_2L_2W) {
  pipeline_2P_SP_DeferNextToken(2, 2);
}

TEST(Pipeline, 2p_PS_DeferNextToken_2L_3W) {
  pipeline_2P_SP_DeferNextToken(2, 3);
}

TEST(Pipeline, 2p_PS_DeferNextToken_2L_4W) {
  pipeline_2P_SP_DeferNextToken(2, 4);
}

TEST(Pipeline, 2p_PS_DeferNextToken_3L_1W) {
  pipeline_2P_SP_DeferNextToken(3, 1);
}

TEST(Pipeline, 2p_PS_DeferNextToken_3L_2W) {
  pipeline_2P_SP_DeferNextToken(3, 2);
}

TEST(Pipeline, 2p_PS_DeferNextToken_3L_3W) {
  pipeline_2P_SP_DeferNextToken(3, 3);
}

TEST(Pipeline, 2p_PS_DeferNextToken_3L_4W) {
  pipeline_2P_SP_DeferNextToken(3, 4);
}

TEST(Pipeline, 2p_PS_DeferNextToken_4L_1W) {
  pipeline_2P_SP_DeferNextToken(4, 1);
}

TEST(Pipeline, 2p_PS_DeferNextToken_4L_2W) {
  pipeline_2P_SP_DeferNextToken(4, 2);
}

TEST(Pipeline, 2p_PS_DeferNextToken_4L_3W) {
  pipeline_2P_SP_DeferNextToken(4, 3);
}

TEST(Pipeline, 2p_PS_DeferNextToken_4L_4W) {
  pipeline_2P_SP_DeferNextToken(4, 4);
}



// ----------------------------------------------------------------------------
// two pipes (SS), L lines, W workers, mimic 264 frame patterns
// ----------------------------------------------------------------------------

struct Frames {
  char type;
  size_t id;
  bool b_defer = false;
  std::vector<size_t> defers;
  Frames(char t, size_t i, std::vector<size_t>&& d) : type{t}, id{i}, defers{std::move(d)} {}
};

std::vector<char> types{'I','B','B','B','P','P','I','B','B','B','P','B','B','B','P','B','B','B','P','B','B','B','P','B','B','B','P','B','B','B','P','B','B','B','P','B','B','B','P','B','B','B','P','B','B','B','P','B','B','B','P','B','B','B','P','B','B','P','B','B','B','P','B','B','B','P','B','B','B','P','B','B','B','P','B','B','B','P','I','B','B','B','P','B','B','B','P','B','B','B','P','B','B','B','P','B','B','B','P','B','B','B','P','B','B','B','P','B','B','B','P','B','B','B','P','B','B','B','P','B','B','B','P','B','B','B','P','B','B','B','P','B','B','B','P','B','B','B','P','B','B','B','P','B','B','B','P','B','B','B','P','B','B','B','P','B','B','B','P','B','B','B','P','B','B','B','P','B','B','B','P','B','B','B','P','I','B','B','B','P','B','B','B','P','B','B','B','P','B','B','B','P','B','B','B','P','B','B','B','P','B','B','B','P','B','B','B','P','B','B','B','P','B','B','B','P','B','B','B','P','B','B','B','P','B','B','B','P','B','B','B','P','B','B','B','P','B','B','B','P','B','B','B','P','B','B','B','P','P','B','B','B','P','B','B','B','P','B','B','B','P','B','B','B','P','B','B','B','P','B','B','B','P','B','B','B','P','B','B','B','P','B','B','B','P','B','B','B','P','B','B','B','P','B','B','B','P','B','B','B','P','B','B','B','P','B','B','B','P','B','B','B','P','P','I','B','B','B','P','B','B','B','P','B','B','B','P','B','B','B','P','B','B','B','P','B','B','B','P','B','B','B','P','B','B','B','P','B','B','B','P','B','B','B','P','B','B','B','P','B','B','B','P','B','B','P','B','B','B','P','B','B','B','P','B','B','B','P','B','B','B','P','I','B','B','B','P','B','B','B','P','B','B','B','P','B','B','B','P','B','B','B','P','B','B','B','P','B','B','B','P','B','B','B','P','B','B','B','P','B','B','B','P','B','B','B','P','B','B','B','P','B','B','B','P','B','B','B','P','B','B','B','P','B','B','B','P','P','B','B','P','P','P','P','P','P','P','P','P','P','P','P','P','P','P','P','P','P','P','P','I','B','B','B','P','B','B','B','P','B','B','B','P','B','B','B','P','B','B','B','P','B','B','B','P','B','B','B','P','B','B','B','P','B','B','B','P','B','B','B','P','P'};


void construct_video(std::vector<Frames>& video, const size_t N) {
  for (size_t i = 0; i < N; ++i) {
    video.emplace_back(types[i], i, std::vector<size_t>{});

    if (types[i] == 'P') {
      size_t step = 1;
      size_t index;
      while (i >= step) {
        index = i - step;
        if (types[index] == 'P' || types[index] == 'I') {
          video[i].defers.push_back(index);
          break;
        }
        else {
          ++step;
        }
      }
    }
    else if (types[i] == 'B') {
      size_t step = 1;
      size_t index;
      while (i >= step) {
        index = i - step;
        if (types[index] == 'P' || types[index] == 'I') {
          video[i].defers.push_back(index);
          break;
        }
        else {
          ++step;
        }
      }
      step = 1;
      while (i+step < N) {
        index = i + step;
        if (types[index] == 'P' || types[index] == 'I') {
          video[i].defers.push_back(index);
          break;
        }
        else {
          ++step;
        }
      }
    }
  }
  //for (size_t i = 0; i < N; ++i) {
  //  std::cout << "video[" << i << "] has type = " << video[i].type
  //            << ", and id = " << video[i].id;
  //  
  //  if (video[i].defers.size()) {
  //     std::cout << ", and has depends = ";
  //     for (size_t j = 0; j < video[i].defers.size(); ++j) {
  //       std::cout << (video[i].defers[j])
  //                 << "(frame " << video[video[i].defers[j]].type << ") ";
  //     }
  //     std::cout << '\n';
  //  }
  //  else {
  //    std::cout << '\n';
  //  }
  //}
}


// ----------------------------------------------------------------------------
// one pipe (S), L lines, W workers, mimic 264 frame patterns
// ----------------------------------------------------------------------------
void pipeline_1P_S_264VideoFormat(size_t L, unsigned w) {

  turbo::Executor executor(w);

  const size_t maxN = 512;


  for(size_t N = 0; N <= maxN; N++) {
    // declare a x264 format video
    std::vector<Frames> video;
    construct_video(video, N);
    
    std::vector<size_t> collection1;
    std::vector<size_t> deferrals1(N);
    std::mutex mutex;

    turbo::Workflow taskflow;

    turbo::Pipeline pl(
      L,
      turbo::Pipe{turbo::PipeType::SERIAL, [N, &collection1, &video, &deferrals1](auto& pf) mutable {
        //printf("toekn %zu, deferred = %zu, N=%zu\n", pf.token(), pf.num_deferrals(), N);
        if(pf.token() == N) {
          //printf("Token %zu stops on line %zu\n", pf.token() ,pf.line());
          pf.stop();
          return;
        }
        else {
          switch(pf.num_deferrals()) {
            case 0:
              if (video[pf.token()].type == 'I') {
                //printf("Stage 1 : token %zu is a I frame on line %zu\n", pf.token() ,pf.line());
                collection1.push_back(pf.token());
                deferrals1[pf.token()] = 0;
              }
              else if (video[pf.token()].type == 'P') {
                //printf("Token %zu is a P frame", pf.token());
                size_t step = 1;
                size_t index = 0;
                while (pf.token() >= step) {
                  index = pf.token()-step;
                  if (video[index].type == 'P' || video[index].type == 'I') {
                    pf.defer(index);
                    //printf(" defers to token %zu which is a %c frame\n", index, video[index].type);
                    break;
                  }
                  ++step;
                }
              }
              else if (video[pf.token()].type == 'B') {
                //printf("Token %zu is a B frame", pf.token());
                size_t step = 1;
                size_t index = 0;
                
                while (pf.token() >= step) {
                  index = pf.token()-step;
                  if (video[index].type == 'P' || video[index].type == 'I') {
                    //printf(" defers to token %zu which is a %c frame\n", index, video[index].type);
                    pf.defer(index);
                    break;
                  }
                  ++step;
                }
                step = 1;
                while (pf.token()+step < N) {
                  index = pf.token()+step;
                  if (video[index].type == 'P' || video[index].type == 'I') {
                    pf.defer(index);
                    //printf(" and token %zu which is a %c frame\n", index, video[index].type);
                    break;
                  }
                  ++step;
                }
              }
            break;

            case 1:
              //printf("Stage 1 : token %zu is deferred 1 time at line %zu\n", pf.token(), pf.line());
              collection1.push_back(pf.token());
              deferrals1[pf.token()] = 1;
            break;
          }
        }
      }}
    );

    auto pipeline = taskflow.composed_of(pl).name("module_of_pipeline");
    auto test = taskflow.emplace([&](){

      for (size_t i = 0; i < N; ++i) {
        std::vector<size_t>::iterator it;
        std::vector<size_t>::iterator it_dep;

        size_t index_it, index_it_dep;
        
        if (video[i].defers.size()) {
          it = std::find(collection1.begin(), collection1.end(), i);
          index_it = std::distance(collection1.begin(), it);
          EXPECT_TRUE(it != collection1.end());
          //if (it == collection1.end()) {
          //  printf("Token %zu is missing\n", i);
          //}
          for (size_t j = 0; j < video[i].defers.size(); ++j) {
            it_dep = std::find(collection1.begin(), collection1.end(), video[i].defers[j]);
            index_it_dep = std::distance(collection1.begin(), it_dep);
            
            EXPECT_TRUE(it != collection1.end());
            EXPECT_TRUE(it_dep != collection1.end());
            EXPECT_TRUE(index_it_dep < index_it);
          }
        }
      }

      EXPECT_TRUE(deferrals1.size() == N);
      for (size_t i = 0; i < N; ++i) {
        if (video[i].type == 'I') {
          EXPECT_TRUE(deferrals1[i] == 0);
        }
        else {
          EXPECT_TRUE(deferrals1[i] == 1);
        }
      }
    }).name("test");

    pipeline.precede(test);

    executor.run_n(taskflow, 1, [&]() mutable {
      collection1.clear();
      deferrals1.clear();
    }).get();
  }
}

TEST(Pipeline, 1P_S_264VideoFormat_1L_1W) {
  pipeline_1P_S_264VideoFormat(1,1);
}
TEST(Pipeline, 1P_S_264VideoFormat_1L_2W) {
  pipeline_1P_S_264VideoFormat(1,2);
}
TEST(Pipeline, 1P_S_264VideoFormat_1L_3W) {
  pipeline_1P_S_264VideoFormat(1,3);
}
TEST(Pipeline, 1P_S_264VideoFormat_1L_4W) {
  pipeline_1P_S_264VideoFormat(1,4);
}
TEST(Pipeline, 1P_S_264VideoFormat_2L_1W) {
  pipeline_1P_S_264VideoFormat(2,1);
}
TEST(Pipeline, 1P_S_264VideoFormat_2L_2W) {
  pipeline_1P_S_264VideoFormat(2,2);
}
TEST(Pipeline, 1P_S_264VideoFormat_2L_3W) {
  pipeline_1P_S_264VideoFormat(2,3);
}
TEST(Pipeline, 1P_S_264VideoFormat_2L_4W) {
  pipeline_1P_S_264VideoFormat(2,4);
}
TEST(Pipeline, 1P_S_264VideoFormat_3L_1W) {
  pipeline_1P_S_264VideoFormat(3,1);
}
TEST(Pipeline, 1P_S_264VideoFormat_3L_2W) {
  pipeline_1P_S_264VideoFormat(3,2);
}
TEST(Pipeline, 1P_S_264VideoFormat_3L_3W) {
  pipeline_1P_S_264VideoFormat(3,3);
}
TEST(Pipeline, 1P_S_264VideoFormat_3L_4W) {
  pipeline_1P_S_264VideoFormat(3,4);
}
TEST(Pipeline, 1P_S_264VideoFormat_4L_1W) {
  pipeline_1P_S_264VideoFormat(4,1);
}
TEST(Pipeline, 1P_S_264VideoFormat_4L_2W) {
  pipeline_1P_S_264VideoFormat(4,2);
}
TEST(Pipeline, 1P_S_264VideoFormat_4L_3W) {
  pipeline_1P_S_264VideoFormat(4,3);
}
TEST(Pipeline, 1P_S_264VideoFormat_4L_4W) {
  pipeline_1P_S_264VideoFormat(4,4);
}

// ----------------------------------------------------------------------------
// two pipes (SS), L lines, W workers, mimic 264 frame patterns
// ----------------------------------------------------------------------------
void pipeline_2P_SS_264VideoFormat(size_t L, unsigned w) {

  turbo::Executor executor(w);

  const size_t maxN = 512;

  std::vector<std::array<size_t, 2>> mybuffer(L);

  for(size_t N = 0; N <= maxN; N++) {
    // declare a x264 format video
    std::vector<Frames> video;
    construct_video(video, N);
    
    std::vector<size_t> collection1;
    std::vector<size_t> collection2;
    std::vector<size_t> deferrals1(N);
    std::vector<size_t> deferrals2(N);
    
    std::mutex mutex;

    turbo::Workflow taskflow;

    turbo::Pipeline pl(
      L,
      turbo::Pipe{turbo::PipeType::SERIAL, [N, &collection1, &mybuffer, &video, &deferrals1](auto& pf) mutable {
        if(pf.token() == N) {
          //printf("Token %zu stops on line %zu\n", pf.token() ,pf.line());
          pf.stop();
          return;
        }
        else {
          switch(pf.num_deferrals()) {
            case 0:
              if (video[pf.token()].type == 'I') {
                //printf("Stage 1 : token %zu is a I frame on line %zu\n", pf.token() ,pf.line());
                collection1.push_back(pf.token());
                mybuffer[pf.line()][pf.pipe()] = pf.token();
                deferrals1[pf.token()] = pf.num_deferrals();           
              }
              else if (video[pf.token()].type == 'P') {
                //printf("Token %zu is a P frame", pf.token());
                size_t step = 1;
                size_t index = 0;
                while (pf.token() >= step) {
                  index = pf.token()-step;
                  if (video[index].type == 'P' || video[index].type == 'I') {
                    pf.defer(index);
                    //printf(" defers to token %zu which is a %c frame\n", index, video[index].type);
                    break;
                  }
                  ++step;
                }
              }
              else if (video[pf.token()].type == 'B') {
                //printf("Token %zu is a B frame", pf.token());
                size_t step = 1;
                size_t index = 0;
                
                while (pf.token() >= step) {
                  index = pf.token()-step;
                  if (video[index].type == 'P' || video[index].type == 'I') {
                    //printf(" defers to token %zu which is a %c frame\n", index, video[index].type);
                    pf.defer(index);
                    break;
                  }
                  ++step;
                }
                step = 1;
                while (pf.token()+step < N) {
                  index = pf.token()+step;
                  if (video[index].type == 'P' || video[index].type == 'I') {
                    pf.defer(index);
                    //printf(" and token %zu which is a %c frame\n", index, video[index].type);
                    break;
                  }
                  ++step;
                }
              }
            break;

            case 1:
              //printf("Stage 1 : token %zu is deferred 1 time at line %zu\n", pf.token(), pf.line());
              collection1.push_back(pf.token());
              mybuffer[pf.line()][pf.pipe()] = pf.token();           
              deferrals1[pf.token()] = pf.num_deferrals();
            break;
          }
        }
      }},

      turbo::Pipe{turbo::PipeType::SERIAL, [&mybuffer, &mutex, &collection2, &deferrals2](auto& pf) mutable {
        {
          std::scoped_lock<std::mutex> lock(mutex);
          collection2.push_back(mybuffer[pf.line()][pf.pipe() - 1]);
          deferrals2[pf.token()] = pf.num_deferrals();
        }
        //printf("Stage 2 : token %zu at line %zu\n", pf.token(), pf.line());
      }}
    );

    auto pipeline = taskflow.composed_of(pl).name("module_of_pipeline");
    auto test = taskflow.emplace([&](){
      //printf("N = %zu and collection1.size() = %zu\n", N, collection1.size());
      for (size_t i = 0; i < collection1.size(); ++i) {
        //printf("collection1[%zu]=%zu, collection2[%zu]=%zu\n", i, collection1[i], i, collection2[i]);
        EXPECT_TRUE(collection1[i] == collection2[i]);
      }

      for (size_t i = 0; i < N; ++i) {
        std::vector<size_t>::iterator it;
        std::vector<size_t>::iterator it_dep;

        size_t index_it, index_it_dep;
        
        if (video[i].defers.size()) {
          it = std::find(collection1.begin(), collection1.end(), i);
          index_it = std::distance(collection1.begin(), it);
          //if (it == collection1.end()) {
          //  printf("Token %zu is missing\n", i);
          //}
          for (size_t j = 0; j < video[i].defers.size(); ++j) {
            it_dep = std::find(collection1.begin(), collection1.end(), video[i].defers[j]);
            index_it_dep = std::distance(collection1.begin(), it_dep);
            
            EXPECT_TRUE(it != collection1.end());
            EXPECT_TRUE(it_dep != collection1.end());
            EXPECT_TRUE(index_it_dep < index_it);
          }
        }
      }

      EXPECT_TRUE(deferrals1.size() == N);
      EXPECT_TRUE(deferrals2.size() == N);
      for (size_t i = 0; i < N; ++i) {
        if (video[i].type == 'I') {
          EXPECT_TRUE(deferrals1[i] == 0);
          EXPECT_TRUE(deferrals2[i] == 0);
        }
        else {
          EXPECT_TRUE(deferrals1[i] == 1);
          EXPECT_TRUE(deferrals2[i] == 1);
        }
      }
    }).name("test");

    pipeline.precede(test);

    executor.run_n(taskflow, 1, [&]() mutable {
      collection1.clear();
      collection2.clear();
      deferrals1.clear();
      deferrals2.clear();
    }).get();
  }
}

TEST(Pipeline, 2p_SS_264VideoFormat_1L_1W) {
  pipeline_2P_SS_264VideoFormat(1,1);
}
TEST(Pipeline, 2p_SS_264VideoFormat_1L_2W) {
  pipeline_2P_SS_264VideoFormat(1,2);
}
TEST(Pipeline, 2p_SS_264VideoFormat_1L_3W) {
  pipeline_2P_SS_264VideoFormat(1,3);
}
TEST(Pipeline, 2p_SS_264VideoFormat_1L_4W) {
  pipeline_2P_SS_264VideoFormat(1,4);
}
TEST(Pipeline, 2p_SS_264VideoFormat_2L_1W) {
  pipeline_2P_SS_264VideoFormat(2,1);
}
TEST(Pipeline, 2p_SS_264VideoFormat_2L_2W) {
  pipeline_2P_SS_264VideoFormat(2,2);
}
TEST(Pipeline, 2p_SS_264VideoFormat_2L_3W) {
  pipeline_2P_SS_264VideoFormat(2,3);
}
TEST(Pipeline, 2p_SS_264VideoFormat_2L_4W) {
  pipeline_2P_SS_264VideoFormat(2,4);
}
TEST(Pipeline, 2p_SS_264VideoFormat_3L_1W) {
  pipeline_2P_SS_264VideoFormat(3,1);
}
TEST(Pipeline, 2p_SS_264VideoFormat_3L_2W) {
  pipeline_2P_SS_264VideoFormat(3,2);
}
TEST(Pipeline, 2p_SS_264VideoFormat_3L_3W) {
  pipeline_2P_SS_264VideoFormat(3,3);
}
TEST(Pipeline, 2p_SS_264VideoFormat_3L_4W) {
  pipeline_2P_SS_264VideoFormat(3,4);
}
TEST(Pipeline, 2p_SS_264VideoFormat_4L_1W) {
  pipeline_2P_SS_264VideoFormat(4,1);
}
TEST(Pipeline, 2p_SS_264VideoFormat_4L_2W) {
  pipeline_2P_SS_264VideoFormat(4,2);
}
TEST(Pipeline, 2p_SS_264VideoFormat_4L_3W) {
  pipeline_2P_SS_264VideoFormat(4,3);
}
TEST(Pipeline, 2p_SS_264VideoFormat_4L_4W) {
  pipeline_2P_SS_264VideoFormat(4,4);
}



// ----------------------------------------------------------------------------
// two pipes (SP), L lines, W workers, mimic 264 frame patterns
// ----------------------------------------------------------------------------

void pipeline_2P_SP_264VideoFormat(size_t L, unsigned w) {

  turbo::Executor executor(w);

  const size_t maxN = 512;

  std::vector<std::array<size_t, 2>> mybuffer(L);

  for(size_t N = 0; N <= maxN; N++) {
    // declare a x264 format video
    std::vector<Frames> video;
    construct_video(video, N);
    
    std::vector<size_t> collection1;
    std::vector<size_t> collection2;
    std::vector<size_t> deferrals1(N);
    std::vector<size_t> deferrals2(N);
    
    std::mutex mutex;

    turbo::Workflow taskflow;

    turbo::Pipeline pl(
      L,
      turbo::Pipe{turbo::PipeType::SERIAL, [N, &collection1, &mybuffer, &video, &deferrals1](auto& pf) mutable {
        if(pf.token() == N) {
          pf.stop();
          return;
        }
        else {
          switch(pf.num_deferrals()) {
            case 0:
              if (video[pf.token()].type == 'I') {
                //printf("Stage 1 : token %zu is a I frame on line %zu\n", pf.token() ,pf.line());
                collection1.push_back(pf.token());
                mybuffer[pf.line()][pf.pipe()] = pf.token();           
                deferrals1[pf.token()] = pf.num_deferrals();
              }
              else if (video[pf.token()].type == 'P') {
                //printf("Token %zu is a P frame", pf.token());
                size_t step = 1;
                size_t index = 0;
                while (pf.token() >= step) {
                  index = pf.token()-step;
                  if (video[index].type == 'P' || video[index].type == 'I') {
                    pf.defer(index);
                    //printf(" defers to token %zu which is a %c frame\n", index, video[index].type);
                    break;
                  }
                  ++step;
                }
              }
              else if (video[pf.token()].type == 'B') {
                //printf("Token %zu is a B frame", pf.token());
                size_t step = 1;
                size_t index = 0;
                
                while (pf.token() >= step) {
                  index = pf.token()-step;
                  if (video[index].type == 'P' || video[index].type == 'I') {
                    //printf(" defers to token %zu which is a %c frame\n", index, video[index].type);
                    pf.defer(index);
                    break;
                  }
                  ++step;
                }
                step = 1;
                while (pf.token()+step < N) {
                  index = pf.token()+step;
                  if (video[index].type == 'P' || video[index].type == 'I') {
                    pf.defer(index);
                    //printf(" and token %zu which is a %c frame\n", index, video[index].type);
                    break;
                  }
                  ++step;
                }
              }
            break;

            case 1:
              //printf("Stage 1 : token %zu is deferred 1 time at line %zu\n", pf.token(), pf.line());
              collection1.push_back(pf.token());
              mybuffer[pf.line()][pf.pipe()] = pf.token();           
              deferrals1[pf.token()] = pf.num_deferrals();
            break;
          }
        }
      }},

      turbo::Pipe{turbo::PipeType::PARALLEL, [&mybuffer, &mutex, &collection2, &deferrals2](auto& pf) mutable {
        {
          std::scoped_lock<std::mutex> lock(mutex);
          collection2.push_back(mybuffer[pf.line()][pf.pipe() - 1]);
          deferrals2[pf.token()] = pf.num_deferrals();
        }
        //printf("Stage 2 : token %zu at line %zu\n", pf.token(), pf.line());
      }}
    );

    auto pipeline = taskflow.composed_of(pl).name("module_of_pipeline");
    auto test = taskflow.emplace([&](){
      EXPECT_TRUE(collection1.size() == N);
      EXPECT_TRUE(collection2.size() == N);

      for (size_t i = 0; i < N; ++i) {
        std::vector<size_t>::iterator it;
        std::vector<size_t>::iterator it_dep;

        if (video[i].defers.size()) {
          it = std::find(collection1.begin(), collection1.end(), i);

          for (size_t j = 0; j < video[i].defers.size(); ++j) {
            it_dep = std::find(collection1.begin(), collection1.end(), video[i].defers[j]);
            
            EXPECT_TRUE(it != collection1.end());
            EXPECT_TRUE(it_dep != collection1.end());
          }
        }
      }

      EXPECT_TRUE(deferrals1.size() == N);
      EXPECT_TRUE(deferrals2.size() == N);
      for (size_t i = 0; i < N; ++i) {
        if (video[i].type == 'I') {
          EXPECT_TRUE(deferrals1[i] == 0);
          EXPECT_TRUE(deferrals2[i] == 0);
        }
        else {
          EXPECT_TRUE(deferrals1[i] == 1);
          EXPECT_TRUE(deferrals2[i] == 1);
        }
      }

    }).name("test");

    pipeline.precede(test);

    executor.run_n(taskflow, 1, [&]() mutable {
      collection1.clear();
      collection2.clear();
      deferrals1.clear();
      deferrals2.clear();
    }).get();
  }
}

TEST(Pipeline, 2p_PS_264VideoFormat_1L_1W) {
  pipeline_2P_SP_264VideoFormat(1,1);
}
TEST(Pipeline, 2p_PS_264VideoFormat_1L_2W) {
  pipeline_2P_SP_264VideoFormat(1,2);
}
TEST(Pipeline, 2p_PS_264VideoFormat_1L_3W) {
  pipeline_2P_SP_264VideoFormat(1,3);
}
TEST(Pipeline, 2p_PS_264VideoFormat_1L_4W) {
  pipeline_2P_SP_264VideoFormat(1,4);
}
TEST(Pipeline, 2p_PS_264VideoFormat_2L_1W) {
  pipeline_2P_SP_264VideoFormat(2,1);
}
TEST(Pipeline, 2p_PS_264VideoFormat_2L_2W) {
  pipeline_2P_SP_264VideoFormat(2,2);
}
TEST(Pipeline, 2p_PS_264VideoFormat_2L_3W) {
  pipeline_2P_SP_264VideoFormat(2,3);
}
TEST(Pipeline, 2p_PS_264VideoFormat_2L_4W) {
  pipeline_2P_SP_264VideoFormat(2,4);
}
TEST(Pipeline, 2p_PS_264VideoFormat_3L_1W) {
  pipeline_2P_SP_264VideoFormat(3,1);
}
TEST(Pipeline, 2p_PS_264VideoFormat_3L_2W) {
  pipeline_2P_SP_264VideoFormat(3,2);
}
TEST(Pipeline, 2p_PS_264VideoFormat_3L_3W) {
  pipeline_2P_SP_264VideoFormat(3,3);
}
TEST(Pipeline, 2p_PS_264VideoFormat_3L_4W) {
  pipeline_2P_SP_264VideoFormat(3,4);
}
TEST(Pipeline, 2p_PS_264VideoFormat_4L_1W) {
  pipeline_2P_SP_264VideoFormat(4,1);
}
TEST(Pipeline, 2p_PS_264VideoFormat_4L_2W) {
  pipeline_2P_SP_264VideoFormat(4,2);
}
TEST(Pipeline, 2p_PS_264VideoFormat_4L_3W) {
  pipeline_2P_SP_264VideoFormat(4,3);
}
TEST(Pipeline, 2p_PS_264VideoFormat_4L_4W) {
  pipeline_2P_SP_264VideoFormat(4,4);
}

// ----------------------------------------------------------------------------
// three pipes (SPP), L lines, W workers, mimic 264 frame patterns
// mainly test pf.num_deferrals()
// ----------------------------------------------------------------------------

void pipeline_3P_SPP_264VideoFormat(size_t L, unsigned w) {

  turbo::Executor executor(w);

  const size_t maxN = 512;

  std::vector<std::array<size_t, 2>> mybuffer(L);

  for(size_t N = 0; N <= maxN; N++) {
    //std::cout << "N = " << N << '\n';
    // declare a x264 format video
    std::vector<Frames> video;
    construct_video(video, N);
    
    std::vector<size_t> collection1;
    std::vector<size_t> collection2;
    std::vector<size_t> collection3;
    std::vector<size_t> deferrals1(N);
    std::vector<size_t> deferrals2(N);
    std::vector<size_t> deferrals3(N);
    
    std::mutex mutex;

    turbo::Workflow taskflow;

    turbo::Pipeline pl(
      L,
      turbo::Pipe{turbo::PipeType::SERIAL, [N, &collection1, &mybuffer, &video, &deferrals1](auto& pf) mutable {
        if(pf.token() == N) {
          pf.stop();
          return;
        }
        else {
          switch(pf.num_deferrals()) {
            case 0:
              if (video[pf.token()].type == 'I') {
                //printf("Stage 1 : token %zu is a I frame on line %zu\n", pf.token() ,pf.line());
                collection1.push_back(pf.token());
                mybuffer[pf.line()][pf.pipe()] = pf.token();           
                deferrals1[pf.token()] = pf.num_deferrals();
              }
              else if (video[pf.token()].type == 'P') {
                //printf("Token %zu is a P frame", pf.token());
                size_t step = 1;
                size_t index = 0;
                while (pf.token() >= step) {
                  index = pf.token()-step;
                  if (video[index].type == 'P' || video[index].type == 'I') {
                    pf.defer(index);
                    //printf(" defers to token %zu which is a %c frame\n", index, video[index].type);
                    break;
                  }
                  ++step;
                }
              }
              else if (video[pf.token()].type == 'B') {
                //printf("Token %zu is a B frame", pf.token());
                size_t step = 1;
                size_t index = 0;
                
                while (pf.token() >= step) {
                  index = pf.token()-step;
                  if (video[index].type == 'P' || video[index].type == 'I') {
                    //printf(" defers to token %zu which is a %c frame\n", index, video[index].type);
                    pf.defer(index);
                    break;
                  }
                  ++step;
                }
              }
            break;

            case 1:
              if (video[pf.token()].type == 'P') {
                //printf("Stage 1 : token %zu is deferred 1 time at line %zu\n", pf.token(), pf.line());
                collection1.push_back(pf.token());
                mybuffer[pf.line()][pf.pipe()] = pf.token();           
                deferrals1[pf.token()] = pf.num_deferrals();
              }
              else {
                size_t step = 1;
                size_t index = 0;
                while (pf.token()+step < N) {
                  index = pf.token()+step;
                  if (video[index].type == 'P' || video[index].type == 'I') {
                    pf.defer(index);
                    video[pf.token()].b_defer = true;
                    //printf(" and token %zu which is a %c frame\n", index, video[index].type);
                    break;
                  }
                  ++step;
                }
                if (video[pf.token()].b_defer == false) {
                  collection1.push_back(pf.token());
                  mybuffer[pf.line()][pf.pipe()] = pf.token();           
                  deferrals1[pf.token()] = pf.num_deferrals();
                }
              }
            break;

            case 2:
              collection1.push_back(pf.token());
              mybuffer[pf.line()][pf.pipe()] = pf.token();           
              deferrals1[pf.token()] = pf.num_deferrals();
            break;
          }
        }
      }},

      turbo::Pipe{turbo::PipeType::PARALLEL, [&mybuffer, &mutex, &collection2, &deferrals2](auto& pf) mutable {
        {
          std::scoped_lock<std::mutex> lock(mutex);
          collection2.push_back(mybuffer[pf.line()][pf.pipe() - 1]);
          deferrals2[pf.token()] = pf.num_deferrals();
        }
        //printf("Stage 2 : token %zu at line %zu\n", pf.token(), pf.line());
      }},

      turbo::Pipe{turbo::PipeType::PARALLEL, [&mybuffer, &mutex, &collection3, &deferrals3](auto& pf) mutable {
        {
          std::scoped_lock<std::mutex> lock(mutex);
          collection3.push_back(mybuffer[pf.line()][pf.pipe() - 1]);
          deferrals3[pf.token()] = pf.num_deferrals();
        }
        //printf("Stage 2 : token %zu at line %zu\n", pf.token(), pf.line());
      }}
    );

    auto pipeline = taskflow.composed_of(pl).name("module_of_pipeline");
    auto test = taskflow.emplace([&](){
      EXPECT_TRUE(collection1.size() == N);
      EXPECT_TRUE(collection2.size() == N);
      EXPECT_TRUE(collection3.size() == N);

      for (size_t i = 0; i < N; ++i) {
        std::vector<size_t>::iterator it;
        std::vector<size_t>::iterator it_dep;

        if (video[i].defers.size()) {
          it = std::find(collection1.begin(), collection1.end(), i);

          for (size_t j = 0; j < video[i].defers.size(); ++j) {
            it_dep = std::find(collection1.begin(), collection1.end(), video[i].defers[j]);
            
            EXPECT_TRUE(it != collection1.end());
            EXPECT_TRUE(it_dep != collection1.end());
          }
        }
      }

      EXPECT_TRUE(deferrals1.size() == N);
      EXPECT_TRUE(deferrals2.size() == N);
      EXPECT_TRUE(deferrals3.size() == N);
      for (size_t i = 0; i < N; ++i) {
        if (video[i].type == 'I') {
          EXPECT_TRUE(deferrals1[i] == 0);
          EXPECT_TRUE(deferrals2[i] == 0);
          EXPECT_TRUE(deferrals3[i] == 0);
        }
        else if (video[i].type == 'P') {
          EXPECT_TRUE(deferrals1[i] == 1);
          EXPECT_TRUE(deferrals2[i] == 1);
          EXPECT_TRUE(deferrals3[i] == 1);
        }
        else {
          if (video[i].b_defer == true) {
            EXPECT_TRUE(deferrals1[i] == 2);
            EXPECT_TRUE(deferrals2[i] == 2);
            EXPECT_TRUE(deferrals3[i] == 2);
          }
          else {
            EXPECT_TRUE(deferrals1[i] == 1);
            EXPECT_TRUE(deferrals2[i] == 1);
            EXPECT_TRUE(deferrals3[i] == 1);
          }
        }
      }

    }).name("test");

    pipeline.precede(test);

    executor.run_n(taskflow, 1, [&]() mutable {
      collection1.clear();
      collection2.clear();
      collection3.clear();
      deferrals1.clear();
      deferrals2.clear();
      deferrals3.clear();
    }).get();
  }
}

TEST(Pipeline, 3P_SPP_264VideoFormat_1L_1W) {
  pipeline_3P_SPP_264VideoFormat(1,1);
}
TEST(Pipeline, 3P_SPP_264VideoFormat_1L_2W) {
  pipeline_3P_SPP_264VideoFormat(1,2);
}
TEST(Pipeline, 3P_SPP_264VideoFormat_1L_3W) {
  pipeline_3P_SPP_264VideoFormat(1,3);
}
TEST(Pipeline, 3P_SPP_264VideoFormat_1L_4W) {
  pipeline_3P_SPP_264VideoFormat(1,4);
}
TEST(Pipeline, 3P_SPP_264VideoFormat_2L_1W) {
  pipeline_3P_SPP_264VideoFormat(2,1);
}
TEST(Pipeline, 3P_SPP_264VideoFormat_2L_2W) {
  pipeline_3P_SPP_264VideoFormat(2,2);
}
TEST(Pipeline, 3P_SPP_264VideoFormat_2L_3W) {
  pipeline_3P_SPP_264VideoFormat(2,3);
}
TEST(Pipeline, 3P_SPP_264VideoFormat_2L_4W) {
  pipeline_3P_SPP_264VideoFormat(2,4);
}
TEST(Pipeline, 3P_SPP_264VideoFormat_3L_1W) {
  pipeline_3P_SPP_264VideoFormat(3,1);
}
TEST(Pipeline, 3P_SPP_264VideoFormat_3L_2W) {
  pipeline_3P_SPP_264VideoFormat(3,2);
}
TEST(Pipeline, 3P_SPP_264VideoFormat_3L_3W) {
  pipeline_3P_SPP_264VideoFormat(3,3);
}
TEST(Pipeline, 3P_SPP_264VideoFormat_3L_4W) {
  pipeline_3P_SPP_264VideoFormat(3,4);
}
TEST(Pipeline, 3P_SPP_264VideoFormat_4L_1W) {
  pipeline_3P_SPP_264VideoFormat(4,1);
}
TEST(Pipeline, 3P_SPP_264VideoFormat_4L_2W) {
  pipeline_3P_SPP_264VideoFormat(4,2);
}
TEST(Pipeline, 3P_SPP_264VideoFormat_4L_3W) {
  pipeline_3P_SPP_264VideoFormat(4,3);
}
TEST(Pipeline, 3P_SPP_264VideoFormat_4L_4W) {
  pipeline_3P_SPP_264VideoFormat(4,4);
}

