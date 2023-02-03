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
#include "workflow.h"

#include <stdlib.h>     /* srand, rand */
#include <time.h>       /* time */

// --------------------------------------------------------
// Testcase: 1 pipe, L lines, w workers
// --------------------------------------------------------
void pipeline_1P(size_t L, unsigned w, turbo::PipeType type) {

  turbo::Executor executor(w);

  const size_t maxN = 100;

  std::vector<int> source(maxN);
  std::iota(source.begin(), source.end(), 0);

  // iterate different data amount (1, 2, 3, 4, 5, ... 1000000)
  for (size_t N = 0; N <= maxN; N++) {

    // serial direction
    if (type == turbo::PipeType::SERIAL) {
      turbo::Workflow taskflow;
      size_t j = 0;
      turbo::Pipeline pl (L, turbo::Pipe{type, [L, N, &j, &source](auto& pf) mutable {
        if (j == N) {
          pf.stop();
          return;
        }
        EXPECT_TRUE(j == source[j]);
        EXPECT_TRUE(pf.token() % L == pf.line());
        j++;
      }});

      //taskflow.pipeline(pl);
      auto pipeline = taskflow.composed_of(pl).name("module_of_pipeline");

      auto test = taskflow.emplace([&](){
        EXPECT_TRUE(j == N);
        EXPECT_TRUE(pl.num_tokens() == N);
      }).name("test");

      pipeline.precede(test);

      executor.run_until(taskflow, [counter=3, j]() mutable{
        j = 0;
        return counter --== 0;
      }).get();
    }
    // parallel pipe
    //else if(type == turbo::PipeType::PARALLEL) {
    //
    //  turbo::Workflow taskflow;

    //  std::atomic<size_t> j = 0;
    //  std::mutex mutex;
    //  std::vector<int> collection;

    //  turbo::Pipeline pl(L, turbo::Pipe{type,
    //  [N, &j, &mutex, &collection](auto& pf) mutable {

    //    auto ticket = j.fetch_add(1);

    //    if(ticket >= N) {
    //      pf.stop();
    //      return;
    //    }
    //    std::scoped_lock<std::mutex> lock(mutex);
    //    collection.push_back(ticket);
    //  }});

    //  taskflow.composed_of(pl);
    //  executor.run(taskflow).wait();
    //  EXPECT_TRUE(collection.size() == N);
    //  std::sort(collection.begin(), collection.end());
    //  for(size_t k=0; k<N; k++) {
    //    EXPECT_TRUE(collection[k] == k);
    //  }

    //  j = 0;
    //  collection.clear();
    //  executor.run(taskflow).wait();
    //  EXPECT_TRUE(collection.size() == N);
    //  std::sort(collection.begin(), collection.end());
    //  for(size_t k=0; k<N; k++) {
    //    EXPECT_TRUE(collection[k] == k);
    //  }
    //}
  }
}

// serial pipe with one line
TEST(Pipeline, 1PS_1L_1W) {
  pipeline_1P(1, 1, turbo::PipeType::SERIAL);
}

TEST(Pipeline, 1PS_1L_2W) {
  pipeline_1P(1, 2, turbo::PipeType::SERIAL);
}

TEST(Pipeline, 1PS_1L_3W) {
  pipeline_1P(1, 3, turbo::PipeType::SERIAL);
}

TEST(Pipeline, 1PS_1L_4W) {
  pipeline_1P(1, 4, turbo::PipeType::SERIAL);
}

// serial pipe with two lines
TEST(Pipeline, 1PS_2L_1W) {
  pipeline_1P(2, 1, turbo::PipeType::SERIAL);
}

TEST(Pipeline, 1PS_2L_2W) {
  pipeline_1P(2, 2, turbo::PipeType::SERIAL);
}

TEST(Pipeline, 1PS_2L_3W) {
  pipeline_1P(2, 3, turbo::PipeType::SERIAL);
}

TEST(Pipeline, 1PS_2L_4W) {
  pipeline_1P(2, 4, turbo::PipeType::SERIAL);
}

// serial pipe with three lines
TEST(Pipeline, 1PS_3L_1W) {
  pipeline_1P(3, 1, turbo::PipeType::SERIAL);
}

TEST(Pipeline, 1PS_3L_2W) {
  pipeline_1P(3, 2, turbo::PipeType::SERIAL);
}

TEST(Pipeline, 1PS_3L_3W) {
  pipeline_1P(3, 3, turbo::PipeType::SERIAL);
}

TEST(Pipeline, 1PS_3L_4W) {
  pipeline_1P(3, 4, turbo::PipeType::SERIAL);
}

// serial pipe with three lines
TEST(Pipeline, 1PS_4L_1W) {
  pipeline_1P(4, 1, turbo::PipeType::SERIAL);
}

TEST(Pipeline, 1PS_4L_2W) {
  pipeline_1P(4, 2, turbo::PipeType::SERIAL);
}

TEST(Pipeline, 1PS_4L_3W) {
  pipeline_1P(4, 3, turbo::PipeType::SERIAL);
}

TEST(Pipeline, 1PS_4L_4W) {
  pipeline_1P(4, 4, turbo::PipeType::SERIAL);
}

// ----------------------------------------------------------------------------
// two pipes (SS), L lines, W workers
// ----------------------------------------------------------------------------

void pipeline_2P_SS(size_t L, unsigned w) {

  turbo::Executor executor(w);

  const size_t maxN = 100;

  std::vector<int> source(maxN);
  std::iota(source.begin(), source.end(), 0);
  std::vector<std::array<int, 2>> mybuffer(L);

  for(size_t N = 0; N <= maxN; N++) {

    turbo::Workflow taskflow;

    size_t j1 = 0, j2 = 0;
    size_t cnt = 1;

    turbo::Pipeline pl(
      L,
      turbo::Pipe{turbo::PipeType::SERIAL, [N, &source, &j1, &mybuffer, L](auto& pf) mutable {
        if(j1 == N) {
          pf.stop();
          return;
        }
        EXPECT_TRUE(j1 == source[j1]);
        EXPECT_TRUE(pf.token() % L == pf.line());
        //*(pf.output()) = source[j1] + 1;
        mybuffer[pf.line()][pf.pipe()] = source[j1] + 1;
        j1++;
      }},

      turbo::Pipe{turbo::PipeType::SERIAL, [N, &source, &j2, &mybuffer, L](auto& pf) mutable {
        EXPECT_TRUE(j2 < N);
        EXPECT_TRUE(pf.token() % L == pf.line());
        EXPECT_TRUE(source[j2] + 1 == mybuffer[pf.line()][pf.pipe() - 1]);
        //EXPECT_TRUE(source[j2] + 1 == *(pf.input()));
        j2++;
      }}
    );

    auto pipeline = taskflow.composed_of(pl).name("module_of_pipeline");
    auto test = taskflow.emplace([&](){
      EXPECT_TRUE(j1 == N);
      EXPECT_TRUE(j2 == N);
      EXPECT_TRUE(pl.num_tokens() == cnt * N);
    }).name("test");

    pipeline.precede(test);

    executor.run_n(taskflow, 3, [&]() mutable {
      j1 = 0;
      j2 = 0;
      for(size_t i = 0; i < mybuffer.size(); ++i){
        for(size_t j = 0; j < mybuffer[0].size(); ++j){
          mybuffer[i][j] = 0;
        }
      }
      cnt++;
    }).get();
  }
}

// two pipes (SS)
TEST(Pipeline, 2PSS_1L_1W) {
  pipeline_2P_SS(1, 1);
}

TEST(Pipeline, 2PSS_1L_2W) {
  pipeline_2P_SS(1, 2);
}

TEST(Pipeline, 2PSS_1L_3W) {
  pipeline_2P_SS(1, 3);
}

TEST(Pipeline, 2PSS_1L_4W) {
  pipeline_2P_SS(1, 4);
}

TEST(Pipeline, 22PSS_2L_1W) {
  pipeline_2P_SS(2, 1);
}

TEST(Pipeline, 22PSS_2L_2W) {
  pipeline_2P_SS(2, 2);
}

TEST(Pipeline, 22PSS_2L_3W) {
  pipeline_2P_SS(2, 3);
}

TEST(Pipeline, 22PSS_2L_4W) {
  pipeline_2P_SS(2, 4);
}

TEST(Pipeline, 2PSS_3L_1W) {
  pipeline_2P_SS(3, 1);
}

TEST(Pipeline, 2PSS_3L_2W) {
  pipeline_2P_SS(3, 2);
}

TEST(Pipeline, 2PSS_3L_3W) {
  pipeline_2P_SS(3, 3);
}

TEST(Pipeline, 2PSS_3L_4W) {
  pipeline_2P_SS(3, 4);
}

TEST(Pipeline, 2PSS_4L_1W) {
  pipeline_2P_SS(4, 1);
}

TEST(Pipeline, 2PSS_4L_2W) {
  pipeline_2P_SS(4, 2);
}

TEST(Pipeline, 2PSS_4L_3W) {
  pipeline_2P_SS(4, 3);
}

TEST(Pipeline, 2PSS_4L_4W) {
  pipeline_2P_SS(4, 4);
}

// ----------------------------------------------------------------------------
// two pipes (SP), L lines, W workers
// ----------------------------------------------------------------------------
void pipeline_2P_SP(size_t L, unsigned w) {

  turbo::Executor executor(w);

  const size_t maxN = 100;

  std::vector<int> source(maxN);
  std::iota(source.begin(), source.end(), 0);
  std::vector<std::array<int, 2>> mybuffer(L);

  for(size_t N = 0; N <= maxN; N++) {

    turbo::Workflow taskflow;

    size_t j1 = 0;
    std::atomic<size_t> j2 = 0;
    std::mutex mutex;
    std::vector<int> collection;
    size_t cnt = 1;

    turbo::Pipeline pl(L,
      turbo::Pipe{turbo::PipeType::SERIAL, [N, &source, &j1, &mybuffer, L](auto& pf) mutable {
        if(j1 == N) {
          pf.stop();
          return;
        }
        EXPECT_TRUE(j1 == source[j1]);
        EXPECT_TRUE(pf.token() % L == pf.line());
        //*(pf.output()) = source[j1] + 1;
        mybuffer[pf.line()][pf.pipe()] = source[j1] + 1;
        j1++;
      }},

      turbo::Pipe{turbo::PipeType::PARALLEL,
      [N, &collection, &mutex, &j2, &mybuffer, L](auto& pf) mutable {
        EXPECT_TRUE(j2++ < N);
        {
          std::scoped_lock<std::mutex> lock(mutex);
          EXPECT_TRUE(pf.token() % L == pf.line());
          collection.push_back(mybuffer[pf.line()][pf.pipe() - 1]);
        }
      }}
    );

    auto pipeline = taskflow.composed_of(pl).name("module_of_pipeline");
    auto test = taskflow.emplace([&](){
      EXPECT_TRUE(j1 == N);
      EXPECT_TRUE(j2 == N);

      std::sort(collection.begin(), collection.end());
      for(size_t i = 0; i < N; i++) {
        EXPECT_TRUE(collection[i] == i + 1);
      }
      EXPECT_TRUE(pl.num_tokens() == cnt * N);
    }).name("test");

    pipeline.precede(test);

    executor.run_n(taskflow, 3, [&]() mutable {
      j1 = j2 = 0;
      collection.clear();
      for(size_t i = 0; i < mybuffer.size(); ++i){
        for(size_t j = 0; j < mybuffer[0].size(); ++j){
          mybuffer[i][j] = 0;
        }
      }
      cnt++;
    }).get();
  }
}

// two pipes (SP)
TEST(Pipeline, 2PSP_1L_1W) {
  pipeline_2P_SP(1, 1);
}

TEST(Pipeline, 2PSP_1L_2W) {
  pipeline_2P_SP(1, 2);
}

TEST(Pipeline, 2PSP_1L_3W) {
  pipeline_2P_SP(1, 3);
}

TEST(Pipeline, 2PSP_1L_4W) {
  pipeline_2P_SP(1, 4);
}

TEST(Pipeline, 2PSP_2L_1W) {
  pipeline_2P_SP(2, 1);
}

TEST(Pipeline, 2PSP_2L_2W) {
  pipeline_2P_SP(2, 2);
}

TEST(Pipeline, 2PSP_2L_3W) {
  pipeline_2P_SP(2, 3);
}

TEST(Pipeline, 2PSP_2L_4W) {
  pipeline_2P_SP(2, 4);
}

TEST(Pipeline, 2PSP_3L_1W) {
  pipeline_2P_SP(3, 1);
}

TEST(Pipeline, 2PSP_3L_2W) {
  pipeline_2P_SP(3, 2);
}

TEST(Pipeline, 2PSP_3L_3W) {
  pipeline_2P_SP(3, 3);
}

TEST(Pipeline, 2PSP_3L_4W) {
  pipeline_2P_SP(3, 4);
}

TEST(Pipeline, 2PSP_4L_1W) {
  pipeline_2P_SP(4, 1);
}

TEST(Pipeline, 2PSP_4L_2W) {
  pipeline_2P_SP(4, 2);
}

TEST(Pipeline, 2PSP_4L_3W) {
  pipeline_2P_SP(4, 3);
}

TEST(Pipeline, 2PSP_4L_4W) {
  pipeline_2P_SP(4, 4);
}

// ----------------------------------------------------------------------------
// three pipes (SSS), L lines, W workers
// ----------------------------------------------------------------------------
void pipeline_3P_SSS(size_t L, unsigned w) {

  turbo::Executor executor(w);

  const size_t maxN = 100;

  std::vector<int> source(maxN);
  std::iota(source.begin(), source.end(), 0);
  std::vector<std::array<int, 3>> mybuffer(L);

  for(size_t N = 0; N <= maxN; N++) {

    turbo::Workflow taskflow;

    size_t j1 = 0, j2 = 0, j3 = 0;
    size_t cnt = 1;

    turbo::Pipeline pl(L,
      turbo::Pipe{turbo::PipeType::SERIAL, [N, &source, &j1, &mybuffer, L](auto& pf) mutable {
        if(j1 == N) {
          pf.stop();
          return;
        }
        EXPECT_TRUE(j1 == source[j1]);
        EXPECT_TRUE(pf.token() % L == pf.line());
        //*(pf.output()) = source[j1] + 1;
        mybuffer[pf.line()][pf.pipe()] = source[j1] + 1;
        j1++;
      }},

      turbo::Pipe{turbo::PipeType::SERIAL, [N, &source, &j2, &mybuffer, L](auto& pf) mutable {
        EXPECT_TRUE(j2 < N);
        EXPECT_TRUE(source[j2] + 1 == mybuffer[pf.line()][pf.pipe() - 1]);
        EXPECT_TRUE(pf.token() % L == pf.line());

        //*(pf.output()) = source[j2] + 1;
        mybuffer[pf.line()][pf.pipe()] = source[j2] + 1;
        j2++;
      }},

      turbo::Pipe{turbo::PipeType::SERIAL, [N, &source, &j3, &mybuffer, L](auto& pf) mutable {
        EXPECT_TRUE(j3 < N);
        EXPECT_TRUE(source[j3] + 1 == mybuffer[pf.line()][pf.pipe() - 1]);
        EXPECT_TRUE(pf.token() % L == pf.line());
        j3++;
      }}
    );

    auto pipeline = taskflow.composed_of(pl).name("module_of_pipeline");
    auto test = taskflow.emplace([&](){
      EXPECT_TRUE(j1 == N);
      EXPECT_TRUE(j2 == N);
      EXPECT_TRUE(j3 == N);
      EXPECT_TRUE(pl.num_tokens() == cnt * N);
    }).name("test");

    pipeline.precede(test);

    executor.run_n(taskflow, 3, [&]() mutable {
      j1 = j2 = j3 = 0;
      for(size_t i = 0; i < mybuffer.size(); ++i){
        for(size_t j = 0; j < mybuffer[0].size(); ++j){
          mybuffer[i][j] = 0;
        }
      }
      cnt++;
    }).get();
  }
}

// three pipes (SSS)
TEST(Pipeline, 3PSSS_1L_1W) {
  pipeline_3P_SSS(1, 1);
}

TEST(Pipeline, 3PSSS_1L_2W) {
  pipeline_3P_SSS(1, 2);
}

TEST(Pipeline, 3PSSS_1L_3W) {
  pipeline_3P_SSS(1, 3);
}

TEST(Pipeline, 3PSSS_1L_4W) {
  pipeline_3P_SSS(1, 4);
}

TEST(Pipeline, 3PSSS_2L_1W) {
  pipeline_3P_SSS(2, 1);
}

TEST(Pipeline, 3PSSS_2L_2W) {
  pipeline_3P_SSS(2, 2);
}

TEST(Pipeline, 3PSSS_2L_3W) {
  pipeline_3P_SSS(2, 3);
}

TEST(Pipeline, 3PSSS_2L_4W) {
  pipeline_3P_SSS(2, 4);
}

TEST(Pipeline, 3PSSS_3L_1W) {
  pipeline_3P_SSS(3, 1);
}

TEST(Pipeline, 3PSSS_3L_2W) {
  pipeline_3P_SSS(3, 2);
}

TEST(Pipeline, 3PSSS_3L_3W) {
  pipeline_3P_SSS(3, 3);
}

TEST(Pipeline, 3PSSS_3L_4W) {
  pipeline_3P_SSS(3, 4);
}

TEST(Pipeline, 3PSSS_4L_1W) {
  pipeline_3P_SSS(4, 1);
}

TEST(Pipeline, 3PSSS_4L_2W) {
  pipeline_3P_SSS(4, 2);
}

TEST(Pipeline, 3PSSS_4L_3W) {
  pipeline_3P_SSS(4, 3);
}

TEST(Pipeline, 3PSSS_4L_4W) {
  pipeline_3P_SSS(4, 4);
}



// ----------------------------------------------------------------------------
// three pipes (SSP), L lines, W workers
// ----------------------------------------------------------------------------
void pipeline_3P_SSP(size_t L, unsigned w) {

  turbo::Executor executor(w);

  const size_t maxN = 100;

  std::vector<int> source(maxN);
  std::iota(source.begin(), source.end(), 0);
  std::vector<std::array<int, 3>> mybuffer(L);

  for(size_t N = 0; N <= maxN; N++) {

    turbo::Workflow taskflow;

    size_t j1 = 0, j2 = 0;
    std::atomic<size_t> j3 = 0;
    std::mutex mutex;
    std::vector<int> collection;
    size_t cnt = 1;

    turbo::Pipeline pl(L,
      turbo::Pipe{turbo::PipeType::SERIAL, [N, &source, &j1, &mybuffer, L](auto& pf) mutable {
        if(j1 == N) {
          pf.stop();
          return;
        }
        EXPECT_TRUE(j1 == source[j1]);
        EXPECT_TRUE(pf.token() % L == pf.line());
        //*(pf.output()) = source[j1] + 1;
        mybuffer[pf.line()][pf.pipe()] = source[j1] + 1;
        j1++;
      }},

      turbo::Pipe{turbo::PipeType::SERIAL, [N, &source, &j2, &mybuffer, L](auto& pf) mutable {
        EXPECT_TRUE(j2 < N);
        EXPECT_TRUE(source[j2] + 1 == mybuffer[pf.line()][pf.pipe() - 1]);
        EXPECT_TRUE(pf.token() % L == pf.line());
        //*(pf.output()) = source[j2] + 1;
        mybuffer[pf.line()][pf.pipe()] = source[j2] + 1;
        j2++;
      }},

      turbo::Pipe{turbo::PipeType::PARALLEL, [N, &j3, &mutex, &collection, &mybuffer, L](auto& pf) mutable {
        EXPECT_TRUE(j3++ < N);
        {
          std::scoped_lock<std::mutex> lock(mutex);
          EXPECT_TRUE(pf.token() % L == pf.line());
          collection.push_back(mybuffer[pf.line()][pf.pipe() - 1]);
        }
      }}
    );

    auto pipeline = taskflow.composed_of(pl).name("module_of_pipeline");
    auto test = taskflow.emplace([&](){
      EXPECT_TRUE(j1 == N);
      EXPECT_TRUE(j2 == N);
      EXPECT_TRUE(j3 == N);
      EXPECT_TRUE(collection.size() == N);

      std::sort(collection.begin(), collection.end());
      for (size_t i = 0; i < N; ++i) {
        EXPECT_TRUE(collection[i] == i + 1);
      }
      EXPECT_TRUE(pl.num_tokens() == cnt * N);
    }).name("test");

    pipeline.precede(test);

    executor.run_n(taskflow, 3, [&](){
      j1 = j2 = j3 = 0;
      collection.clear();
      for(size_t i = 0; i < mybuffer.size(); ++i){
        for(size_t j = 0; j < mybuffer[0].size(); ++j){
          mybuffer[i][j] = 0;
        }
      }

      cnt++;
    }).get();
  }
}

// three pipes (SSP)
TEST(Pipeline, 3PSSP_1L_1W) {
  pipeline_3P_SSP(1, 1);
}

TEST(Pipeline, 3PSSP_1L_2W) {
  pipeline_3P_SSP(1, 2);
}

TEST(Pipeline, 3PSSP_1L_3W) {
  pipeline_3P_SSP(1, 3);
}

TEST(Pipeline, 3PSSP_1L_4W) {
  pipeline_3P_SSP(1, 4);
}

TEST(Pipeline, 3PSSP_2L_1W) {
  pipeline_3P_SSP(2, 1);
}

TEST(Pipeline, 3PSSP_2L_2W) {
  pipeline_3P_SSP(2, 2);
}

TEST(Pipeline, 3PSSP_2L_3W) {
  pipeline_3P_SSP(2, 3);
}

TEST(Pipeline, 3PSSP_2L_4W) {
  pipeline_3P_SSP(2, 4);
}

TEST(Pipeline, 3PSSP_3L_1W) {
  pipeline_3P_SSP(3, 1);
}

TEST(Pipeline, 3PSSP_3L_2W) {
  pipeline_3P_SSP(3, 2);
}

TEST(Pipeline, 3PSSP_3L_3W) {
  pipeline_3P_SSP(3, 3);
}

TEST(Pipeline, 3PSSP_3L_4W) {
  pipeline_3P_SSP(3, 4);
}

TEST(Pipeline, 3PSSP_4L_1W) {
  pipeline_3P_SSP(4, 1);
}

TEST(Pipeline, 3PSSP_4L_2W) {
  pipeline_3P_SSP(4, 2);
}

TEST(Pipeline, 3PSSP_4L_3W) {
  pipeline_3P_SSP(4, 3);
}

TEST(Pipeline, 3PSSP_4L_4W) {
  pipeline_3P_SSP(4, 4);
}



// ----------------------------------------------------------------------------
// three pipes (SPS), L lines, W workers
// ----------------------------------------------------------------------------
void pipeline_3P_SPS(size_t L, unsigned w) {

  turbo::Executor executor(w);

  const size_t maxN = 100;

  std::vector<int> source(maxN);
  std::iota(source.begin(), source.end(), 0);
  std::vector<std::array<int, 3>> mybuffer(L);

  for(size_t N = 0; N <= maxN; N++) {

    turbo::Workflow taskflow;

    size_t j1 = 0, j3 = 0;
    std::atomic<size_t> j2 = 0;
    std::mutex mutex;
    std::vector<int> collection;
    size_t cnt = 1;

    turbo::Pipeline pl(L,
      turbo::Pipe{turbo::PipeType::SERIAL, [N, &source, &j1, &mybuffer, L](auto& pf) mutable {
        if(j1 == N) {
          pf.stop();
          return;
        }
        EXPECT_TRUE(j1 == source[j1]);
        EXPECT_TRUE(pf.token() % L == pf.line());
        //*(pf.output()) = source[j1] + 1;
        mybuffer[pf.line()][pf.pipe()] = source[j1] + 1;
        j1++;
      }},

      turbo::Pipe{turbo::PipeType::PARALLEL, [N, &j2, &mutex, &collection, &mybuffer, L](auto& pf) mutable {
        EXPECT_TRUE(j2++ < N);
        //*(pf.output()) = *(pf.input()) + 1;
        {
          std::scoped_lock<std::mutex> lock(mutex);
          mybuffer[pf.line()][pf.pipe()] = mybuffer[pf.line()][pf.pipe() - 1] + 1;
          EXPECT_TRUE(pf.token() % L == pf.line());
          collection.push_back(mybuffer[pf.line()][pf.pipe() - 1]);
        }
      }},

      turbo::Pipe{turbo::PipeType::SERIAL, [N, &source, &j3, &mybuffer, L](auto& pf) mutable {
        EXPECT_TRUE(j3 < N);
        EXPECT_TRUE(pf.token() % L == pf.line());
        EXPECT_TRUE(source[j3] + 2 == mybuffer[pf.line()][pf.pipe() - 1]);
        j3++;
      }}
    );

    auto pipeline = taskflow.composed_of(pl).name("module_of_pipeline");
    auto test = taskflow.emplace([&](){
      EXPECT_TRUE(j1 == N);
      EXPECT_TRUE(j2 == N);
      EXPECT_TRUE(j3 == N);
      EXPECT_TRUE(collection.size() == N);

      std::sort(collection.begin(), collection.end());
      for (size_t i = 0; i < N; ++i) {
        EXPECT_TRUE(collection[i] == i + 1);
      }
      EXPECT_TRUE(pl.num_tokens() == cnt * N);

    }).name("test");

    pipeline.precede(test);

    executor.run_n(taskflow, 3, [&]() mutable {
      j1 = j2 = j3 = 0;
      collection.clear();
      for(size_t i = 0; i < mybuffer.size(); ++i){
        for(size_t j = 0; j < mybuffer[0].size(); ++j){
          mybuffer[i][j] = 0;
        }
      }

      cnt++;
    }).get();
  }
}

// three pipes (SPS)
TEST(Pipeline, 3PSPS_1L_1W) {
  pipeline_3P_SPS(1, 1);
}

TEST(Pipeline, 3PSPS_1L_2W) {
  pipeline_3P_SPS(1, 2);
}

TEST(Pipeline, 3PSPS_1L_3W) {
  pipeline_3P_SPS(1, 3);
}

TEST(Pipeline, 3PSPS_1L_4W) {
  pipeline_3P_SPS(1, 4);
}

TEST(Pipeline, 3PSPS_2L_1W) {
  pipeline_3P_SPS(2, 1);
}

TEST(Pipeline, 3PSPS_2L_2W) {
  pipeline_3P_SPS(2, 2);
}

TEST(Pipeline, 3PSPS_2L_3W) {
  pipeline_3P_SPS(2, 3);
}

TEST(Pipeline, 3PSPS_2L_4W) {
  pipeline_3P_SPS(2, 4);
}

TEST(Pipeline, 3PSPS_3L_1W) {
  pipeline_3P_SPS(3, 1);
}

TEST(Pipeline, 3PSPS_3L_2W) {
  pipeline_3P_SPS(3, 2);
}

TEST(Pipeline, 3PSPS_3L_3W) {
  pipeline_3P_SPS(3, 3);
}

TEST(Pipeline, 3PSPS_3L_4W) {
  pipeline_3P_SPS(3, 4);
}

TEST(Pipeline, 3PSPS_4L_1W) {
  pipeline_3P_SPS(4, 1);
}

TEST(Pipeline, 3PSPS_4L_2W) {
  pipeline_3P_SPS(4, 2);
}

TEST(Pipeline, 3PSPS_4L_3W) {
  pipeline_3P_SPS(4, 3);
}

TEST(Pipeline, 3PSPS_4L_4W) {
  pipeline_3P_SPS(4, 4);
}


// ----------------------------------------------------------------------------
// three pipes (SPP), L lines, W workers
// ----------------------------------------------------------------------------


void pipeline_3P_SPP(size_t L, unsigned w) {

  turbo::Executor executor(w);

  const size_t maxN = 100;

  std::vector<int> source(maxN);
  std::iota(source.begin(), source.end(), 0);
  std::vector<std::array<int, 3>> mybuffer(L);

  for(size_t N = 0; N <= maxN; N++) {

    turbo::Workflow taskflow;

    size_t j1 = 0;
    std::atomic<size_t> j2 = 0;
    std::atomic<size_t> j3 = 0;
    std::mutex mutex2;
    std::mutex mutex3;
    std::vector<int> collection2;
    std::vector<int> collection3;
    size_t cnt = 1;

    turbo::Pipeline pl(L,
      turbo::Pipe{turbo::PipeType::SERIAL, [N, &source, &j1, &mybuffer, L](auto& pf) mutable {
        if(j1 == N) {
          pf.stop();
          return;
        }
        EXPECT_TRUE(j1 == source[j1]);
        EXPECT_TRUE(pf.token() % L == pf.line());
        //*(pf.output()) = source[j1] + 1;
        mybuffer[pf.line()][pf.pipe()] = source[j1] + 1;
        j1++;
      }},

      turbo::Pipe{turbo::PipeType::PARALLEL, [N, &j2, &mutex2, &collection2, &mybuffer, L](auto& pf) mutable {
        EXPECT_TRUE(j2++ < N);
        //*pf.output() = *pf.input() + 1;
        {
          std::scoped_lock<std::mutex> lock(mutex2);
          EXPECT_TRUE(pf.token() % L == pf.line());
          mybuffer[pf.line()][pf.pipe()] = mybuffer[pf.line()][pf.pipe() - 1] + 1;
          collection2.push_back(mybuffer[pf.line()][pf.pipe() - 1]);
        }
      }},

      turbo::Pipe{turbo::PipeType::PARALLEL, [N, &j3, &mutex3, &collection3, &mybuffer, L](auto& pf) mutable {
        EXPECT_TRUE(j3++ < N);
        {
          std::scoped_lock<std::mutex> lock(mutex3);
          EXPECT_TRUE(pf.token() % L == pf.line());
          collection3.push_back(mybuffer[pf.line()][pf.pipe() - 1]);
        }
      }}
    );

    auto pipeline = taskflow.composed_of(pl).name("module_of_pipeline");
    auto test = taskflow.emplace([&](){
      EXPECT_TRUE(j1 == N);
      EXPECT_TRUE(j2 == N);
      EXPECT_TRUE(j3 == N);
      EXPECT_TRUE(collection2.size() == N);
      EXPECT_TRUE(collection3.size() == N);

      std::sort(collection2.begin(), collection2.end());
      std::sort(collection3.begin(), collection3.end());
      for (size_t i = 0; i < N; ++i) {
        EXPECT_TRUE(collection2[i] == i + 1);
        EXPECT_TRUE(collection3[i] == i + 2);
      }
      EXPECT_TRUE(pl.num_tokens() == cnt * N);
    }).name("test");

    pipeline.precede(test);

    executor.run_n(taskflow, 3, [&]() mutable {
      j1 = j2 = j3 = 0;
      collection2.clear();
      collection3.clear();
      for(size_t i = 0; i < mybuffer.size(); ++i){
        for(size_t j = 0; j < mybuffer[0].size(); ++j){
          mybuffer[i][j] = 0;
        }
      }

      cnt++;
    }).get();
  }
}

// three pipes (SPP)
TEST(Pipeline, 3PSPP_1L_1W) {
  pipeline_3P_SPP(1, 1);
}

TEST(Pipeline, 3PSPP_1L_2W) {
  pipeline_3P_SPP(1, 2);
}

TEST(Pipeline, 3PSPP_1L_3W) {
  pipeline_3P_SPP(1, 3);
}

TEST(Pipeline, 3PSPP_1L_4W) {
  pipeline_3P_SPP(1, 4);
}

TEST(Pipeline, 3PSPP_2L_1W) {
  pipeline_3P_SPP(2, 1);
}

TEST(Pipeline, 3PSPP_2L_2W) {
  pipeline_3P_SPP(2, 2);
}

TEST(Pipeline, 3PSPP_2L_3W) {
  pipeline_3P_SPP(2, 3);
}

TEST(Pipeline, 3PSPP_2L_4W) {
  pipeline_3P_SPP(2, 4);
}

TEST(Pipeline, 3PSPP_3L_1W) {
  pipeline_3P_SPP(3, 1);
}

TEST(Pipeline, 3PSPP_3L_2W) {
  pipeline_3P_SPP(3, 2);
}

TEST(Pipeline, 3PSPP_3L_3W) {
  pipeline_3P_SPP(3, 3);
}

TEST(Pipeline, 3PSPP_3L_4W) {
  pipeline_3P_SPP(3, 4);
}

TEST(Pipeline, 3PSPP_4L_1W) {
  pipeline_3P_SPP(4, 1);
}

TEST(Pipeline, 3PSPP_4L_2W) {
  pipeline_3P_SPP(4, 2);
}

TEST(Pipeline, 3PSPP_4L_3W) {
  pipeline_3P_SPP(4, 3);
}

TEST(Pipeline, 3PSPP_4L_4W) {
  pipeline_3P_SPP(4, 4);
}

// ----------------------------------------------------------------------------
// three parallel pipelines. each pipeline with L lines.
// one with four pipes (SSSS), one with three pipes (SPP),
// One with two  Pipes (SP)
//
//      --> SSSS --> O --
//     |                 |
// O -> --> SSP  --> O -- --> O
//     |                 |
//      --> SP   --> O --
//
// ----------------------------------------------------------------------------

void three_parallel_pipelines(size_t L, unsigned w) {

  turbo::Executor executor(w);

  const size_t maxN = 100;

  std::vector<int> source(maxN);
  std::iota(source.begin(), source.end(), 0);
  std::vector<std::array<int, 4>> mybuffer1(L);
  std::vector<std::array<int, 3>> mybuffer2(L);
  std::vector<std::array<int, 2>> mybuffer3(L);

  for(size_t N = 0; N <= maxN; N++) {

    turbo::Workflow taskflow;

    size_t j1_1 = 0, j1_2 = 0, j1_3 = 0, j1_4 = 0;
    size_t cnt1 = 1;

    // pipeline 1 is SSSS
    turbo::Pipeline pl1(L,
      turbo::Pipe{turbo::PipeType::SERIAL, [N, &source, &j1_1, &mybuffer1, L](auto& pf) mutable {
        if(j1_1 == N) {
          pf.stop();
          return;
        }
        EXPECT_TRUE(j1_1 == source[j1_1]);
        EXPECT_TRUE(pf.token() % L == pf.line());
        mybuffer1[pf.line()][pf.pipe()] = source[j1_1] + 1;
        j1_1++;
      }},

      turbo::Pipe{turbo::PipeType::SERIAL, [N, &source, &j1_2, &mybuffer1, L](auto& pf) mutable {
        EXPECT_TRUE(j1_2 < N);
        EXPECT_TRUE(pf.token() % L == pf.line());
        EXPECT_TRUE(source[j1_2] + 1 == mybuffer1[pf.line()][pf.pipe() - 1]);
        mybuffer1[pf.line()][pf.pipe()] = source[j1_2] + 1;
        j1_2++;
      }},

      turbo::Pipe{turbo::PipeType::SERIAL, [N, &source, &j1_3, &mybuffer1, L](auto& pf) mutable {
        EXPECT_TRUE(j1_3 < N);
        EXPECT_TRUE(pf.token() % L == pf.line());
        EXPECT_TRUE(source[j1_3] + 1 == mybuffer1[pf.line()][pf.pipe() - 1]);
        mybuffer1[pf.line()][pf.pipe()] = source[j1_3] + 1;
        j1_3++;
      }},

      turbo::Pipe{turbo::PipeType::SERIAL, [N, &source, &j1_4, &mybuffer1, L](auto& pf) mutable {
        EXPECT_TRUE(j1_4 < N);
        EXPECT_TRUE(pf.token() % L == pf.line());
        EXPECT_TRUE(source[j1_4] + 1 == mybuffer1[pf.line()][pf.pipe() - 1]);
        j1_4++;
      }}
    );

    auto pipeline1 = taskflow.composed_of(pl1).name("module_of_pipeline1");
    auto test1 = taskflow.emplace([&](){
      EXPECT_TRUE(j1_1 == N);
      EXPECT_TRUE(j1_2 == N);
      EXPECT_TRUE(j1_3 == N);
      EXPECT_TRUE(j1_4 == N);
      EXPECT_TRUE(pl1.num_tokens() == cnt1 * N);
    }).name("test1");

    pipeline1.precede(test1);



    // the followings are definitions for pipeline 2
    size_t j2_1 = 0, j2_2 = 0;
    std::atomic<size_t> j2_3 = 0;
    std::mutex mutex2_3;
    std::vector<int> collection2_3;
    size_t cnt2 = 1;

    // pipeline 2 is SSP
    turbo::Pipeline pl2(L,
      turbo::Pipe{turbo::PipeType::SERIAL, [N, &source, &j2_1, &mybuffer2, L](auto& pf) mutable {
        if(j2_1 == N) {
          pf.stop();
          return;
        }
        EXPECT_TRUE(j2_1 == source[j2_1]);
        EXPECT_TRUE(pf.token() % L == pf.line());
        mybuffer2[pf.line()][pf.pipe()] = source[j2_1] + 1;
        j2_1++;
      }},

      turbo::Pipe{turbo::PipeType::SERIAL, [N, &source, &j2_2, &mybuffer2, L](auto& pf) mutable {
        EXPECT_TRUE(j2_2 < N);
        EXPECT_TRUE(source[j2_2] + 1 == mybuffer2[pf.line()][pf.pipe() - 1]);
        EXPECT_TRUE(pf.token() % L == pf.line());
        mybuffer2[pf.line()][pf.pipe()] = source[j2_2] + 1;
        j2_2++;
      }},

      turbo::Pipe{turbo::PipeType::PARALLEL, [N, &j2_3, &mutex2_3, &collection2_3, &mybuffer2, L](auto& pf) mutable {
        EXPECT_TRUE(j2_3++ < N);
        {
          std::scoped_lock<std::mutex> lock(mutex2_3);
          EXPECT_TRUE(pf.token() % L == pf.line());
          collection2_3.push_back(mybuffer2[pf.line()][pf.pipe() - 1]);
        }
      }}
    );

    auto pipeline2 = taskflow.composed_of(pl2).name("module_of_pipeline2");
    auto test2 = taskflow.emplace([&](){
      EXPECT_TRUE(j2_1 == N);
      EXPECT_TRUE(j2_2 == N);
      EXPECT_TRUE(j2_3 == N);
      EXPECT_TRUE(collection2_3.size() == N);

      std::sort(collection2_3.begin(), collection2_3.end());
      for (size_t i = 0; i < N; ++i) {
        EXPECT_TRUE(collection2_3[i] == i + 1);
      }
      EXPECT_TRUE(pl2.num_tokens() == cnt2 * N);
    }).name("test2");

    pipeline2.precede(test2);



    // the followings are definitions for pipeline 3
    size_t j3_1 = 0;
    std::atomic<size_t> j3_2 = 0;
    std::mutex mutex3_2;
    std::vector<int> collection3_2;
    size_t cnt3 = 1;

    // pipeline 3 is SP
    turbo::Pipeline pl3(L,
      turbo::Pipe{turbo::PipeType::SERIAL, [N, &source, &j3_1, &mybuffer3, L](auto& pf) mutable {
        if(j3_1 == N) {
          pf.stop();
          return;
        }
        EXPECT_TRUE(j3_1 == source[j3_1]);
        EXPECT_TRUE(pf.token() % L == pf.line());
        mybuffer3[pf.line()][pf.pipe()] = source[j3_1] + 1;
        j3_1++;
      }},

      turbo::Pipe{turbo::PipeType::PARALLEL,
      [N, &collection3_2, &mutex3_2, &j3_2, &mybuffer3, L](auto& pf) mutable {
        EXPECT_TRUE(j3_2++ < N);
        {
          std::scoped_lock<std::mutex> lock(mutex3_2);
          EXPECT_TRUE(pf.token() % L == pf.line());
          collection3_2.push_back(mybuffer3[pf.line()][pf.pipe() - 1]);
        }
      }}
    );

    auto pipeline3 = taskflow.composed_of(pl3).name("module_of_pipeline3");
    auto test3 = taskflow.emplace([&](){
      EXPECT_TRUE(j3_1 == N);
      EXPECT_TRUE(j3_2 == N);

      std::sort(collection3_2.begin(), collection3_2.end());
      for(size_t i = 0; i < N; i++) {
        EXPECT_TRUE(collection3_2[i] == i + 1);
      }
      EXPECT_TRUE(pl3.num_tokens() == cnt3 * N);
    }).name("test3");

    pipeline3.precede(test3);


    auto initial  = taskflow.emplace([](){}).name("initial");
    auto terminal = taskflow.emplace([](){}).name("terminal");

    initial.precede(pipeline1, pipeline2, pipeline3);
    terminal.succeed(test1, test2, test3);

    //taskflow.dump(std::cout);

    executor.run_n(taskflow, 3, [&]() mutable {
      // reset variables for pipeline 1
      j1_1 = j1_2 = j1_3 = j1_4 = 0;
      for(size_t i = 0; i < mybuffer1.size(); ++i){
        for(size_t j = 0; j < mybuffer1[0].size(); ++j){
          mybuffer1[i][j] = 0;
        }
      }
      cnt1++;

      // reset variables for pipeline 2
      j2_1 = j2_2 = j2_3 = 0;
      collection2_3.clear();
      for(size_t i = 0; i < mybuffer2.size(); ++i){
        for(size_t j = 0; j < mybuffer2[0].size(); ++j){
          mybuffer2[i][j] = 0;
        }
      }
      cnt2++;

      // reset variables for pipeline 3
      j3_1 = j3_2 = 0;
      collection3_2.clear();
      for(size_t i = 0; i < mybuffer3.size(); ++i){
        for(size_t j = 0; j < mybuffer3[0].size(); ++j){
          mybuffer3[i][j] = 0;
        }
      }
      cnt3++;
    }).get();


  }
}

// three parallel piplines
TEST(Three_Parallel, Pipelines_1L_1W) {
  three_parallel_pipelines(1, 1);
}

TEST(Three_Parallel, Pipelines_1L_2W) {
  three_parallel_pipelines(1, 2);
}

TEST(Three_Parallel, Pipelines_1L_3W) {
  three_parallel_pipelines(1, 3);
}

TEST(Three_Parallel, Pipelines_1L_4W) {
  three_parallel_pipelines(1, 4);
}

TEST(Three_Parallel, Pipelines_1L_5W) {
  three_parallel_pipelines(1, 5);
}

TEST(Three_Parallel, Pipelines_1L_6W) {
  three_parallel_pipelines(1, 6);
}

TEST(Three_Parallel, Pipelines_1L_7W) {
  three_parallel_pipelines(1, 7);
}

TEST(Three_Parallel, Pipelines_1L_8W) {
  three_parallel_pipelines(1, 8);
}

TEST(Three_Parallel, Pipelines_2L_1W) {
  three_parallel_pipelines(2, 1);
}

TEST(Three_Parallel, Pipelines_2L_2W) {
  three_parallel_pipelines(2, 2);
}

TEST(Three_Parallel, Pipelines_2L_3W) {
  three_parallel_pipelines(2, 3);
}

TEST(Three_Parallel, Pipelines_2L_4W) {
  three_parallel_pipelines(2, 4);
}

TEST(Three_Parallel, Pipelines_2L_5W) {
  three_parallel_pipelines(2, 5);
}

TEST(Three_Parallel, Pipelines_2L_6W) {
  three_parallel_pipelines(2, 6);
}

TEST(Three_Parallel, Pipelines_2L_7W) {
  three_parallel_pipelines(2, 7);
}

TEST(Three_Parallel, Pipelines_2L_8W) {
  three_parallel_pipelines(2, 8);
}

TEST(Three_Parallel, Pipelines_3L_1W) {
  three_parallel_pipelines(3, 1);
}

TEST(Three_Parallel, Pipelines_3L_2W) {
  three_parallel_pipelines(3, 2);
}

TEST(Three_Parallel, Pipelines_3L_3W) {
  three_parallel_pipelines(3, 3);
}

TEST(Three_Parallel, Pipelines_3L_4W) {
  three_parallel_pipelines(3, 4);
}

TEST(Three_Parallel, Pipelines_3L_5W) {
  three_parallel_pipelines(3, 5);
}

TEST(Three_Parallel, Pipelines_3L_6W) {
  three_parallel_pipelines(3, 6);
}

TEST(Three_Parallel, Pipelines_3L_7W) {
  three_parallel_pipelines(3, 7);
}

TEST(Three_Parallel, Pipelines_3L_8W) {
  three_parallel_pipelines(3, 8);
}

TEST(Three_Parallel, Pipelines_4L_1W) {
  three_parallel_pipelines(4, 1);
}

TEST(Three_Parallel, Pipelines_4L_2W) {
  three_parallel_pipelines(4, 2);
}

TEST(Three_Parallel, Pipelines_4L_3W) {
  three_parallel_pipelines(4, 3);
}

TEST(Three_Parallel, Pipelines_4L_4W) {
  three_parallel_pipelines(4, 4);
}

TEST(Three_Parallel, Pipelines_4L_5W) {
  three_parallel_pipelines(4, 5);
}

TEST(Three_Parallel, Pipelines_4L_6W) {
  three_parallel_pipelines(4, 6);
}

TEST(Three_Parallel, Pipelines_4L_7W) {
  three_parallel_pipelines(4, 7);
}

TEST(Three_Parallel, Pipelines_4L_8W) {
  three_parallel_pipelines(4, 8);
}

TEST(Three_Parallel, Pipelines_5L_1W) {
  three_parallel_pipelines(5, 1);
}

TEST(Three_Parallel, Pipelines_5L_2W) {
  three_parallel_pipelines(5, 2);
}

TEST(Three_Parallel, Pipelines_5L_3W) {
  three_parallel_pipelines(5, 3);
}

TEST(Three_Parallel, Pipelines_5L_4W) {
  three_parallel_pipelines(5, 4);
}

TEST(Three_Parallel, Pipelines_5L_5W) {
  three_parallel_pipelines(5, 5);
}

TEST(Three_Parallel, Pipelines_5L_6W) {
  three_parallel_pipelines(5, 6);
}

TEST(Three_Parallel, Pipelines_5L_7W) {
  three_parallel_pipelines(5, 7);
}

TEST(Three_Parallel, Pipelines_5L_8W) {
  three_parallel_pipelines(5, 8);
}

TEST(Three_Parallel, Pipelines_6L_1W) {
  three_parallel_pipelines(6, 1);
}

TEST(Three_Parallel, Pipelines_6L_2W) {
  three_parallel_pipelines(6, 2);
}

TEST(Three_Parallel, Pipelines_6L_3W) {
  three_parallel_pipelines(6, 3);
}

TEST(Three_Parallel, Pipelines_6L_4W) {
  three_parallel_pipelines(6, 4);
}

TEST(Three_Parallel, Pipelines_6L_5W) {
  three_parallel_pipelines(6, 5);
}

TEST(Three_Parallel, Pipelines_6L_6W) {
  three_parallel_pipelines(6, 6);
}

TEST(Three_Parallel, Pipelines_6L_7W) {
  three_parallel_pipelines(6, 7);
}

TEST(Three_Parallel, Pipelines_6L_8W) {
  three_parallel_pipelines(6, 8);
}

TEST(Three_Parallel, Pipelines_7L_1W) {
  three_parallel_pipelines(7, 1);
}

TEST(Three_Parallel, Pipelines_7L_2W) {
  three_parallel_pipelines(7, 2);
}

TEST(Three_Parallel, Pipelines_7L_3W) {
  three_parallel_pipelines(7, 3);
}

TEST(Three_Parallel, Pipelines_7L_4W) {
  three_parallel_pipelines(7, 4);
}

TEST(Three_Parallel, Pipelines_7L_5W) {
  three_parallel_pipelines(7, 5);
}

TEST(Three_Parallel, Pipelines_7L_6W) {
  three_parallel_pipelines(7, 6);
}

TEST(Three_Parallel, Pipelines_7L_7W) {
  three_parallel_pipelines(7, 7);
}

TEST(Three_Parallel, Pipelines_7L_8W) {
  three_parallel_pipelines(7, 8);
}

TEST(Three_Parallel, Pipelines_8L_1W) {
  three_parallel_pipelines(8, 1);
}

TEST(Three_Parallel, Pipelines_8L_2W) {
  three_parallel_pipelines(8, 2);
}

TEST(Three_Parallel, Pipelines_8L_3W) {
  three_parallel_pipelines(8, 3);
}

TEST(Three_Parallel, Pipelines_8L_4W) {
  three_parallel_pipelines(8, 4);
}

TEST(Three_Parallel, Pipelines_8L_5W) {
  three_parallel_pipelines(8, 5);
}

TEST(Three_Parallel, Pipelines_8L_6W) {
  three_parallel_pipelines(8, 6);
}

TEST(Three_Parallel, Pipelines_8L_7W) {
  three_parallel_pipelines(8, 7);
}

TEST(Three_Parallel, Pipelines_8L_8W) {
  three_parallel_pipelines(8, 8);
}

// ----------------------------------------------------------------------------
// three concatenated pipelines. each pipeline with L lines.
// one with four pipes (SSSS), one with three pipes (SSP),
// One with two  Pipes (SP)
//
// O -> SSSS -> O -> SSP -> O -> SP -> O
//
// ----------------------------------------------------------------------------

void three_concatenated_pipelines(size_t L, unsigned w) {

  turbo::Executor executor(w);

  const size_t maxN = 100;

  std::vector<int> source(maxN);
  std::iota(source.begin(), source.end(), 0);
  std::vector<std::array<int, 4>> mybuffer1(L);
  std::vector<std::array<int, 3>> mybuffer2(L);
  std::vector<std::array<int, 2>> mybuffer3(L);

  for(size_t N = 0; N <= maxN; N++) {

    turbo::Workflow taskflow;

    size_t j1_1 = 0, j1_2 = 0, j1_3 = 0, j1_4 = 0;
    size_t cnt1 = 1;

    // pipeline 1 is SSSS
    turbo::Pipeline pl1(L,
      turbo::Pipe{turbo::PipeType::SERIAL, [N, &source, &j1_1, &mybuffer1, L](auto& pf) mutable {
        if(j1_1 == N) {
          pf.stop();
          return;
        }
        EXPECT_TRUE(j1_1 == source[j1_1]);
        EXPECT_TRUE(pf.token() % L == pf.line());
        mybuffer1[pf.line()][pf.pipe()] = source[j1_1] + 1;
        j1_1++;
      }},

      turbo::Pipe{turbo::PipeType::SERIAL, [N, &source, &j1_2, &mybuffer1, L](auto& pf) mutable {
        EXPECT_TRUE(j1_2 < N);
        EXPECT_TRUE(pf.token() % L == pf.line());
        EXPECT_TRUE(source[j1_2] + 1 == mybuffer1[pf.line()][pf.pipe() - 1]);
        mybuffer1[pf.line()][pf.pipe()] = source[j1_2] + 1;
        j1_2++;
      }},

      turbo::Pipe{turbo::PipeType::SERIAL, [N, &source, &j1_3, &mybuffer1, L](auto& pf) mutable {
        EXPECT_TRUE(j1_3 < N);
        EXPECT_TRUE(pf.token() % L == pf.line());
        EXPECT_TRUE(source[j1_3] + 1 == mybuffer1[pf.line()][pf.pipe() - 1]);
        mybuffer1[pf.line()][pf.pipe()] = source[j1_3] + 1;
        j1_3++;
      }},

      turbo::Pipe{turbo::PipeType::SERIAL, [N, &source, &j1_4, &mybuffer1, L](auto& pf) mutable {
        EXPECT_TRUE(j1_4 < N);
        EXPECT_TRUE(pf.token() % L == pf.line());
        EXPECT_TRUE(source[j1_4] + 1 == mybuffer1[pf.line()][pf.pipe() - 1]);
        j1_4++;
      }}
    );

    auto pipeline1 = taskflow.composed_of(pl1).name("module_of_pipeline1");
    auto test1 = taskflow.emplace([&](){
      EXPECT_TRUE(j1_1 == N);
      EXPECT_TRUE(j1_2 == N);
      EXPECT_TRUE(j1_3 == N);
      EXPECT_TRUE(j1_4 == N);
      EXPECT_TRUE(pl1.num_tokens() == cnt1 * N);
    }).name("test1");



    // the followings are definitions for pipeline 2
    size_t j2_1 = 0, j2_2 = 0;
    std::atomic<size_t> j2_3 = 0;
    std::mutex mutex2_3;
    std::vector<int> collection2_3;
    size_t cnt2 = 1;

    // pipeline 2 is SSP
    turbo::Pipeline pl2(L,
      turbo::Pipe{turbo::PipeType::SERIAL, [N, &source, &j2_1, &mybuffer2, L](auto& pf) mutable {
        if(j2_1 == N) {
          pf.stop();
          return;
        }
        EXPECT_TRUE(j2_1 == source[j2_1]);
        EXPECT_TRUE(pf.token() % L == pf.line());
        mybuffer2[pf.line()][pf.pipe()] = source[j2_1] + 1;
        j2_1++;
      }},

      turbo::Pipe{turbo::PipeType::SERIAL, [N, &source, &j2_2, &mybuffer2, L](auto& pf) mutable {
        EXPECT_TRUE(j2_2 < N);
        EXPECT_TRUE(source[j2_2] + 1 == mybuffer2[pf.line()][pf.pipe() - 1]);
        EXPECT_TRUE(pf.token() % L == pf.line());
        mybuffer2[pf.line()][pf.pipe()] = source[j2_2] + 1;
        j2_2++;
      }},

      turbo::Pipe{turbo::PipeType::PARALLEL, [N, &j2_3, &mutex2_3, &collection2_3, &mybuffer2, L](auto& pf) mutable {
        EXPECT_TRUE(j2_3++ < N);
        {
          std::scoped_lock<std::mutex> lock(mutex2_3);
          EXPECT_TRUE(pf.token() % L == pf.line());
          collection2_3.push_back(mybuffer2[pf.line()][pf.pipe() - 1]);
        }
      }}
    );

    auto pipeline2 = taskflow.composed_of(pl2).name("module_of_pipeline2");
    auto test2 = taskflow.emplace([&](){
      EXPECT_TRUE(j2_1 == N);
      EXPECT_TRUE(j2_2 == N);
      EXPECT_TRUE(j2_3 == N);
      EXPECT_TRUE(collection2_3.size() == N);

      std::sort(collection2_3.begin(), collection2_3.end());
      for (size_t i = 0; i < N; ++i) {
        EXPECT_TRUE(collection2_3[i] == i + 1);
      }
      EXPECT_TRUE(pl2.num_tokens() == cnt2 * N);
    }).name("test2");



    // the followings are definitions for pipeline 3
    size_t j3_1 = 0;
    std::atomic<size_t> j3_2 = 0;
    std::mutex mutex3_2;
    std::vector<int> collection3_2;
    size_t cnt3 = 1;

    // pipeline 3 is SP
    turbo::Pipeline pl3(L,
      turbo::Pipe{turbo::PipeType::SERIAL, [N, &source, &j3_1, &mybuffer3, L](auto& pf) mutable {
        if(j3_1 == N) {
          pf.stop();
          return;
        }
        EXPECT_TRUE(j3_1 == source[j3_1]);
        EXPECT_TRUE(pf.token() % L == pf.line());
        mybuffer3[pf.line()][pf.pipe()] = source[j3_1] + 1;
        j3_1++;
      }},

      turbo::Pipe{turbo::PipeType::PARALLEL,
      [N, &collection3_2, &mutex3_2, &j3_2, &mybuffer3, L](auto& pf) mutable {
        EXPECT_TRUE(j3_2++ < N);
        {
          std::scoped_lock<std::mutex> lock(mutex3_2);
          EXPECT_TRUE(pf.token() % L == pf.line());
          collection3_2.push_back(mybuffer3[pf.line()][pf.pipe() - 1]);
        }
      }}
    );

    auto pipeline3 = taskflow.composed_of(pl3).name("module_of_pipeline3");
    auto test3 = taskflow.emplace([&](){
      EXPECT_TRUE(j3_1 == N);
      EXPECT_TRUE(j3_2 == N);

      std::sort(collection3_2.begin(), collection3_2.end());
      for(size_t i = 0; i < N; i++) {
        EXPECT_TRUE(collection3_2[i] == i + 1);
      }
      EXPECT_TRUE(pl3.num_tokens() == cnt3 * N);
    }).name("test3");


    auto initial  = taskflow.emplace([](){}).name("initial");
    auto terminal = taskflow.emplace([](){}).name("terminal");

    initial.precede(pipeline1);
    pipeline1.precede(test1);
    test1.precede(pipeline2);
    pipeline2.precede(test2);
    test2.precede(pipeline3);
    pipeline3.precede(test3);
    test3.precede(terminal);

    //taskflow.dump(std::cout);

    executor.run_n(taskflow, 3, [&]() mutable {
      // reset variables for pipeline 1
      j1_1 = j1_2 = j1_3 = j1_4 = 0;
      for(size_t i = 0; i < mybuffer1.size(); ++i){
        for(size_t j = 0; j < mybuffer1[0].size(); ++j){
          mybuffer1[i][j] = 0;
        }
      }
      cnt1++;

      // reset variables for pipeline 2
      j2_1 = j2_2 = j2_3 = 0;
      collection2_3.clear();
      for(size_t i = 0; i < mybuffer2.size(); ++i){
        for(size_t j = 0; j < mybuffer2[0].size(); ++j){
          mybuffer2[i][j] = 0;
        }
      }
      cnt2++;

      // reset variables for pipeline 3
      j3_1 = j3_2 = 0;
      collection3_2.clear();
      for(size_t i = 0; i < mybuffer3.size(); ++i){
        for(size_t j = 0; j < mybuffer3[0].size(); ++j){
          mybuffer3[i][j] = 0;
        }
      }
      cnt3++;
    }).get();


  }
}

// three concatenated piplines
TEST(Three_Concatenated, Pipelines_1L_1W) {
  three_concatenated_pipelines(1, 1);
}

TEST(Three_Concatenated, Pipelines_1L_2W) {
  three_concatenated_pipelines(1, 2);
}

TEST(Three_Concatenated, Pipelines_1L_3W) {
  three_concatenated_pipelines(1, 3);
}

TEST(Three_Concatenated, Pipelines_1L_4W) {
  three_concatenated_pipelines(1, 4);
}

TEST(Three_Concatenated, Pipelines_1L_5W) {
  three_concatenated_pipelines(1, 5);
}

TEST(Three_Concatenated, Pipelines_1L_6W) {
  three_concatenated_pipelines(1, 6);
}

TEST(Three_Concatenated, Pipelines_1L_7W) {
  three_concatenated_pipelines(1, 7);
}

TEST(Three_Concatenated, Pipelines_1L_8W) {
  three_concatenated_pipelines(1, 8);
}

TEST(Three_Concatenated, Pipelines_2L_1W) {
  three_concatenated_pipelines(2, 1);
}

TEST(Three_Concatenated, Pipelines_2L_2W) {
  three_concatenated_pipelines(2, 2);
}

TEST(Three_Concatenated, Pipelines_2L_3W) {
  three_concatenated_pipelines(2, 3);
}

TEST(Three_Concatenated, Pipelines_2L_4W) {
  three_concatenated_pipelines(2, 4);
}

TEST(Three_Concatenated, Pipelines_2L_5W) {
  three_concatenated_pipelines(2, 5);
}

TEST(Three_Concatenated, Pipelines_2L_6W) {
  three_concatenated_pipelines(2, 6);
}

TEST(Three_Concatenated, Pipelines_2L_7W) {
  three_concatenated_pipelines(2, 7);
}

TEST(Three_Concatenated, Pipelines_2L_8W) {
  three_concatenated_pipelines(2, 8);
}

TEST(Three_Concatenated, Pipelines_3L_1W) {
  three_concatenated_pipelines(3, 1);
}

TEST(Three_Concatenated, Pipelines_3L_2W) {
  three_concatenated_pipelines(3, 2);
}

TEST(Three_Concatenated, Pipelines_3L_3W) {
  three_concatenated_pipelines(3, 3);
}

TEST(Three_Concatenated, Pipelines_3L_4W) {
  three_concatenated_pipelines(3, 4);
}

TEST(Three_Concatenated, Pipelines_3L_5W) {
  three_concatenated_pipelines(3, 5);
}

TEST(Three_Concatenated, Pipelines_3L_6W) {
  three_concatenated_pipelines(3, 6);
}

TEST(Three_Concatenated, Pipelines_3L_7W) {
  three_concatenated_pipelines(3, 7);
}

TEST(Three_Concatenated, Pipelines_3L_8W) {
  three_concatenated_pipelines(3, 8);
}

TEST(Three_Concatenated, Pipelines_4L_1W) {
  three_concatenated_pipelines(4, 1);
}

TEST(Three_Concatenated, Pipelines_4L_2W) {
  three_concatenated_pipelines(4, 2);
}

TEST(Three_Concatenated, Pipelines_4L_3W) {
  three_concatenated_pipelines(4, 3);
}

TEST(Three_Concatenated, Pipelines_4L_4W) {
  three_concatenated_pipelines(4, 4);
}

TEST(Three_Concatenated, Pipelines_4L_5W) {
  three_concatenated_pipelines(4, 5);
}

TEST(Three_Concatenated, Pipelines_4L_6W) {
  three_concatenated_pipelines(4, 6);
}

TEST(Three_Concatenated, Pipelines_4L_7W) {
  three_concatenated_pipelines(4, 7);
}

TEST(Three_Concatenated, Pipelines_4L_8W) {
  three_concatenated_pipelines(4, 8);
}

TEST(Three_Concatenated, Pipelines_5L_1W) {
  three_concatenated_pipelines(5, 1);
}

TEST(Three_Concatenated, Pipelines_5L_2W) {
  three_concatenated_pipelines(5, 2);
}

TEST(Three_Concatenated, Pipelines_5L_3W) {
  three_concatenated_pipelines(5, 3);
}

TEST(Three_Concatenated, Pipelines_5L_4W) {
  three_concatenated_pipelines(5, 4);
}

TEST(Three_Concatenated, Pipelines_5L_5W) {
  three_concatenated_pipelines(5, 5);
}

TEST(Three_Concatenated, Pipelines_5L_6W) {
  three_concatenated_pipelines(5, 6);
}

TEST(Three_Concatenated, Pipelines_5L_7W) {
  three_concatenated_pipelines(5, 7);
}

TEST(Three_Concatenated, Pipelines_5L_8W) {
  three_concatenated_pipelines(5, 8);
}

TEST(Three_Concatenated, Pipelines_6L_1W) {
  three_concatenated_pipelines(6, 1);
}

TEST(Three_Concatenated, Pipelines_6L_2W) {
  three_concatenated_pipelines(6, 2);
}

TEST(Three_Concatenated, Pipelines_6L_3W) {
  three_concatenated_pipelines(6, 3);
}

TEST(Three_Concatenated, Pipelines_6L_4W) {
  three_concatenated_pipelines(6, 4);
}

TEST(Three_Concatenated, Pipelines_6L_5W) {
  three_concatenated_pipelines(6, 5);
}

TEST(Three_Concatenated, Pipelines_6L_6W) {
  three_concatenated_pipelines(6, 6);
}

TEST(Three_Concatenated, Pipelines_6L_7W) {
  three_concatenated_pipelines(6, 7);
}

TEST(Three_Concatenated, Pipelines_6L_8W) {
  three_concatenated_pipelines(6, 8);
}

TEST(Three_Concatenated, Pipelines_7L_1W) {
  three_concatenated_pipelines(7, 1);
}

TEST(Three_Concatenated, Pipelines_7L_2W) {
  three_concatenated_pipelines(7, 2);
}

TEST(Three_Concatenated, Pipelines_7L_3W) {
  three_concatenated_pipelines(7, 3);
}

TEST(Three_Concatenated, Pipelines_7L_4W) {
  three_concatenated_pipelines(7, 4);
}

TEST(Three_Concatenated, Pipelines_7L_5W) {
  three_concatenated_pipelines(7, 5);
}

TEST(Three_Concatenated, Pipelines_7L_6W) {
  three_concatenated_pipelines(7, 6);
}

TEST(Three_Concatenated, Pipelines_7L_7W) {
  three_concatenated_pipelines(7, 7);
}

TEST(Three_Concatenated, Pipelines_7L_8W) {
  three_concatenated_pipelines(7, 8);
}

TEST(Three_Concatenated, Pipelines_8L_1W) {
  three_concatenated_pipelines(8, 1);
}

TEST(Three_Concatenated, Pipelines_8L_2W) {
  three_concatenated_pipelines(8, 2);
}

TEST(Three_Concatenated, Pipelines_8L_3W) {
  three_concatenated_pipelines(8, 3);
}

TEST(Three_Concatenated, Pipelines_8L_4W) {
  three_concatenated_pipelines(8, 4);
}

TEST(Three_Concatenated, Pipelines_8L_5W) {
  three_concatenated_pipelines(8, 5);
}

TEST(Three_Concatenated, Pipelines_8L_6W) {
  three_concatenated_pipelines(8, 6);
}

TEST(Three_Concatenated, Pipelines_8L_7W) {
  three_concatenated_pipelines(8, 7);
}

TEST(Three_Concatenated, Pipelines_8L_8W) {
  three_concatenated_pipelines(8, 8);
}

// ----------------------------------------------------------------------------
// pipeline (SPSP) and conditional task.  pipeline has L lines, W workers
//
// O -> SPSP -> conditional_task
//        ^            |
//        |____________|
// ----------------------------------------------------------------------------

void looping_pipelines(size_t L, unsigned w) {

  turbo::Executor executor(w);

  const size_t maxN = 100;

  std::vector<int> source(maxN);
  std::iota(source.begin(), source.end(), 0);
  std::vector<std::array<int, 4>> mybuffer(L);

  turbo::Workflow taskflow;

  size_t j1 = 0, j3 = 0;
  std::atomic<size_t> j2 = 0;
  std::atomic<size_t> j4 = 0;
  std::mutex mutex2;
  std::mutex mutex4;
  std::vector<int> collection2;
  std::vector<int> collection4;
  size_t cnt = 0;

  size_t N = 0;

  turbo::Pipeline pl(L,
    turbo::Pipe{turbo::PipeType::SERIAL, [&N, &source, &j1, &mybuffer, L](auto& pf) mutable {
      if(j1 == N) {
        pf.stop();
        return;
      }
      EXPECT_TRUE(j1 == source[j1]);
      EXPECT_TRUE(pf.token() % L == pf.line());
      mybuffer[pf.line()][pf.pipe()] = source[j1] + 1;
      j1++;
    }},

    turbo::Pipe{turbo::PipeType::PARALLEL, [&N, &j2, &mutex2, &collection2, &mybuffer, L](auto& pf) mutable {
      EXPECT_TRUE(j2++ < N);
      {
        std::scoped_lock<std::mutex> lock(mutex2);
        EXPECT_TRUE(pf.token() % L == pf.line());
        mybuffer[pf.line()][pf.pipe()] = mybuffer[pf.line()][pf.pipe() - 1] + 1;
        collection2.push_back(mybuffer[pf.line()][pf.pipe() - 1]);
      }
    }},

    turbo::Pipe{turbo::PipeType::SERIAL, [&N, &source, &j3, &mybuffer, L](auto& pf) mutable {
      EXPECT_TRUE(j3 < N);
      EXPECT_TRUE(pf.token() % L == pf.line());
      EXPECT_TRUE(source[j3] + 2 == mybuffer[pf.line()][pf.pipe() - 1]);
      mybuffer[pf.line()][pf.pipe()] = mybuffer[pf.line()][pf.pipe() - 1] + 1;
      j3++;
    }},

    turbo::Pipe{turbo::PipeType::PARALLEL, [&N, &j4, &mutex4, &collection4, &mybuffer, L](auto& pf) mutable {
      EXPECT_TRUE(j4++ < N);
      {
        std::scoped_lock<std::mutex> lock(mutex4);
        EXPECT_TRUE(pf.token() % L == pf.line());
        collection4.push_back(mybuffer[pf.line()][pf.pipe() - 1]);
      }
    }}
  );

  auto pipeline = taskflow.composed_of(pl).name("module_of_pipeline");
  auto initial = taskflow.emplace([](){}).name("initial");

  auto conditional = taskflow.emplace([&](){
    EXPECT_TRUE(j1 == N);
    EXPECT_TRUE(j2 == N);
    EXPECT_TRUE(j3 == N);
    EXPECT_TRUE(j4 == N);
    EXPECT_TRUE(collection2.size() == N);
    EXPECT_TRUE(collection4.size() == N);
    std::sort(collection2.begin(), collection2.end());
    std::sort(collection4.begin(), collection4.end());
    for (size_t i = 0; i < N; ++i) {
      EXPECT_TRUE(collection2[i] == i + 1);
      EXPECT_TRUE(collection4[i] == i + 3);
    }
    EXPECT_TRUE(pl.num_tokens() == cnt);

    // reset variables
    j1 = j2 = j3 = j4 = 0;
    for(size_t i = 0; i < mybuffer.size(); ++i){
      for(size_t j = 0; j < mybuffer[0].size(); ++j){
        mybuffer[i][j] = 0;
      }
    }
    collection2.clear();
    collection4.clear();
    ++N;
    cnt+=N;

    return N < maxN ? 0 : 1;
  }).name("conditional");

  auto terminal = taskflow.emplace([](){}).name("terminal");

  initial.precede(pipeline);
  pipeline.precede(conditional);
  conditional.precede(pipeline, terminal);

  executor.run(taskflow).wait();
}

// looping piplines
TEST(Looping, Pipelines_1L_1W) {
  looping_pipelines(1, 1);
}

TEST(Looping, Pipelines_1L_2W) {
  looping_pipelines(1, 2);
}

TEST(Looping, Pipelines_1L_3W) {
  looping_pipelines(1, 3);
}

TEST(Looping, Pipelines_1L_4W) {
  looping_pipelines(1, 4);
}

TEST(Looping, Pipelines_1L_5W) {
  looping_pipelines(1, 5);
}

TEST(Looping, Pipelines_1L_6W) {
  looping_pipelines(1, 6);
}

TEST(Looping, Pipelines_1L_7W) {
  looping_pipelines(1, 7);
}

TEST(Looping, Pipelines_1L_8W) {
  looping_pipelines(1, 8);
}

TEST(Looping, Pipelines_2L_1W) {
  looping_pipelines(2, 1);
}

TEST(Looping, Pipelines_2L_2W) {
  looping_pipelines(2, 2);
}

TEST(Looping, Pipelines_2L_3W) {
  looping_pipelines(2, 3);
}

TEST(Looping, Pipelines_2L_4W) {
  looping_pipelines(2, 4);
}

TEST(Looping, Pipelines_2L_5W) {
  looping_pipelines(2, 5);
}

TEST(Looping, Pipelines_2L_6W) {
  looping_pipelines(2, 6);
}

TEST(Looping, Pipelines_2L_7W) {
  looping_pipelines(2, 7);
}

TEST(Looping, Pipelines_2L_8W) {
  looping_pipelines(2, 8);
}

TEST(Looping, Pipelines_3L_1W) {
  looping_pipelines(3, 1);
}

TEST(Looping, Pipelines_3L_2W) {
  looping_pipelines(3, 2);
}

TEST(Looping, Pipelines_3L_3W) {
  looping_pipelines(3, 3);
}

TEST(Looping, Pipelines_3L_4W) {
  looping_pipelines(3, 4);
}

TEST(Looping, Pipelines_3L_5W) {
  looping_pipelines(3, 5);
}

TEST(Looping, Pipelines_3L_6W) {
  looping_pipelines(3, 6);
}

TEST(Looping, Pipelines_3L_7W) {
  looping_pipelines(3, 7);
}

TEST(Looping, Pipelines_3L_8W) {
  looping_pipelines(3, 8);
}

TEST(Looping, Pipelines_4L_1W) {
  looping_pipelines(4, 1);
}

TEST(Looping, Pipelines_4L_2W) {
  looping_pipelines(4, 2);
}

TEST(Looping, Pipelines_4L_3W) {
  looping_pipelines(4, 3);
}

TEST(Looping, Pipelines_4L_4W) {
  looping_pipelines(4, 4);
}

TEST(Looping, Pipelines_4L_5W) {
  looping_pipelines(4, 5);
}

TEST(Looping, Pipelines_4L_6W) {
  looping_pipelines(4, 6);
}

TEST(Looping, Pipelines_4L_7W) {
  looping_pipelines(4, 7);
}

TEST(Looping, Pipelines_4L_8W) {
  looping_pipelines(4, 8);
}

TEST(Looping, Pipelines_5L_1W) {
  looping_pipelines(5, 1);
}

TEST(Looping, Pipelines_5L_2W) {
  looping_pipelines(5, 2);
}

TEST(Looping, Pipelines_5L_3W) {
  looping_pipelines(5, 3);
}

TEST(Looping, Pipelines_5L_4W) {
  looping_pipelines(5, 4);
}

TEST(Looping, Pipelines_5L_5W) {
  looping_pipelines(5, 5);
}

TEST(Looping, Pipelines_5L_6W) {
  looping_pipelines(5, 6);
}

TEST(Looping, Pipelines_5L_7W) {
  looping_pipelines(5, 7);
}

TEST(Looping, Pipelines_5L_8W) {
  looping_pipelines(5, 8);
}

TEST(Looping, Pipelines_6L_1W) {
  looping_pipelines(6, 1);
}

TEST(Looping, Pipelines_6L_2W) {
  looping_pipelines(6, 2);
}

TEST(Looping, Pipelines_6L_3W) {
  looping_pipelines(6, 3);
}

TEST(Looping, Pipelines_6L_4W) {
  looping_pipelines(6, 4);
}

TEST(Looping, Pipelines_6L_5W) {
  looping_pipelines(6, 5);
}

TEST(Looping, Pipelines_6L_6W) {
  looping_pipelines(6, 6);
}

TEST(Looping, Pipelines_6L_7W) {
  looping_pipelines(6, 7);
}

TEST(Looping, Pipelines_6L_8W) {
  looping_pipelines(6, 8);
}

TEST(Looping, Pipelines_7L_1W) {
  looping_pipelines(7, 1);
}

TEST(Looping, Pipelines_7L_2W) {
  looping_pipelines(7, 2);
}

TEST(Looping, Pipelines_7L_3W) {
  looping_pipelines(7, 3);
}

TEST(Looping, Pipelines_7L_4W) {
  looping_pipelines(7, 4);
}

TEST(Looping, Pipelines_7L_5W) {
  looping_pipelines(7, 5);
}

TEST(Looping, Pipelines_7L_6W) {
  looping_pipelines(7, 6);
}

TEST(Looping, Pipelines_7L_7W) {
  looping_pipelines(7, 7);
}

TEST(Looping, Pipelines_7L_8W) {
  looping_pipelines(7, 8);
}

TEST(Looping, Pipelines_8L_1W) {
  looping_pipelines(8, 1);
}

TEST(Looping, Pipelines_8L_2W) {
  looping_pipelines(8, 2);
}

TEST(Looping, Pipelines_8L_3W) {
  looping_pipelines(8, 3);
}

TEST(Looping, Pipelines_8L_4W) {
  looping_pipelines(8, 4);
}

TEST(Looping, Pipelines_8L_5W) {
  looping_pipelines(8, 5);
}

TEST(Looping, Pipelines_8L_6W) {
  looping_pipelines(8, 6);
}

TEST(Looping, Pipelines_8L_7W) {
  looping_pipelines(8, 7);
}

TEST(Looping, Pipelines_8L_8W) {
  looping_pipelines(8, 8);
}

// ----------------------------------------------------------------------------
//
// ifelse pipeline has three pipes, L lines, w workers
//
// SPS
// ----------------------------------------------------------------------------

int ifelse_pipe_ans(int a) {
  // pipe 1
  if(a / 2 != 0) {
    a += 8;
  }
  // pipe 2
  if(a > 4897) {
    a -= 1834;
  }
  else {
    a += 3;
  }
  // pipe 3
  if((a + 9) / 4 < 50) {
    a += 1;
  }
  else {
    a += 17;
  }

  return a;
}

void ifelse_pipeline(size_t L, unsigned w) {
  srand(time(NULL));

  turbo::Executor executor(w);
  size_t maxN = 200;

  std::vector<int> source(maxN);
  for(auto&& s: source) {
    s = rand() % 9962;
  }
  std::vector<std::array<int, 4>> buffer(L);

  for(size_t N = 1; N < maxN; ++N) {
    turbo::Workflow taskflow;

    std::vector<int> collection;
    collection.reserve(N);

    turbo::Pipeline pl(L,
      // pipe 1
      turbo::Pipe(turbo::PipeType::SERIAL, [&, N](auto& pf){
        if(pf.token() == N) {
          pf.stop();
          return;
        }

        if(source[pf.token()] / 2 == 0) {
          buffer[pf.line()][pf.pipe()] = source[pf.token()];
        }
        else {
          buffer[pf.line()][pf.pipe()] = source[pf.token()] + 8;
        }

      }),

      // pipe 2
      turbo::Pipe(turbo::PipeType::PARALLEL, [&](auto& pf){

        if(buffer[pf.line()][pf.pipe() - 1] > 4897) {
          buffer[pf.line()][pf.pipe()] =  buffer[pf.line()][pf.pipe() - 1] - 1834;
        }
        else {
          buffer[pf.line()][pf.pipe()] = buffer[pf.line()][pf.pipe() - 1] + 3;
        }

      }),

      // pipe 3
      turbo::Pipe(turbo::PipeType::SERIAL, [&](auto& pf){

        if((buffer[pf.line()][pf.pipe() - 1] + 9) / 4 < 50) {
          buffer[pf.line()][pf.pipe()] = buffer[pf.line()][pf.pipe() - 1] + 1;
        }
        else {
          buffer[pf.line()][pf.pipe()] = buffer[pf.line()][pf.pipe() - 1] + 17;
        }

        collection.push_back(buffer[pf.line()][pf.pipe()]);

      })
    );
    auto pl_t = taskflow.composed_of(pl).name("pipeline");

    auto check_t = taskflow.emplace([&](){
      for(size_t n = 0; n < N; ++n) {
        EXPECT_TRUE(collection[n] == ifelse_pipe_ans(source[n]));
      }
    }).name("check");

    pl_t.precede(check_t);

    executor.run(taskflow).wait();

  }
}

TEST(Ifelse, Pipelines_1L_1W) {
  ifelse_pipeline(1, 1);
}

TEST(Ifelse, Pipelines_1L_2W) {
  ifelse_pipeline(1, 2);
}

TEST(Ifelse, Pipelines_1L_3W) {
  ifelse_pipeline(1, 3);
}

TEST(Ifelse, Pipelines_1L_4W) {
  ifelse_pipeline(1, 4);
}

TEST(Ifelse, Pipelines_3L_1W) {
  ifelse_pipeline(3, 1);
}

TEST(Ifelse, Pipelines_3L_2W) {
  ifelse_pipeline(3, 2);
}

TEST(Ifelse, Pipelines_3L_3W) {
  ifelse_pipeline(3, 3);
}

TEST(Ifelse, Pipelines_3L_4W) {
  ifelse_pipeline(3, 4);
}

TEST(Ifelse, Pipelines_5L_1W) {
  ifelse_pipeline(5, 1);
}

TEST(Ifelse, Pipelines_5L_2W) {
  ifelse_pipeline(5, 2);
}

TEST(Ifelse, Pipelines_5L_3W) {
  ifelse_pipeline(5, 3);
}

TEST(Ifelse, Pipelines_5L_4W) {
  ifelse_pipeline(5, 4);
}

TEST(Ifelse, Pipelines_7L_1W) {
  ifelse_pipeline(7, 1);
}

TEST(Ifelse, Pipelines_7L_2W) {
  ifelse_pipeline(7, 2);
}

TEST(Ifelse, Pipelines_7L_3W) {
  ifelse_pipeline(7, 3);
}

TEST(Ifelse, Pipelines_7L_4W) {
  ifelse_pipeline(7, 4);
}

// ----------------------------------------------------------------------------
// pipeline in pipeline
// pipeline has 4 pipes, L lines, W workers
// each subpipeline has 3 pipes, subL lines
//
// pipeline = SPPS
// each subpipeline = SPS
//
// ----------------------------------------------------------------------------

void pipeline_in_pipeline(size_t L, unsigned w, unsigned subL) {


  turbo::Executor executor(w);

  const size_t maxN = 5;
  const size_t maxsubN = 4;

  std::vector<std::vector<int>> source(maxN);
  for(auto&& each: source) {
    each.resize(maxsubN);
    std::iota(each.begin(), each.end(), 0);
  }

  std::vector<std::array<int, 4>> buffer(L);

  // each pipe contains one subpipeline
  // each subpipeline has three pipes, subL lines
  //
  // subbuffers[0][1][2][2] means
  // first line, second pipe, third subline, third subpipe
  std::vector<std::vector<std::vector<std::array<int, 3>>>> subbuffers(L);

  for(auto&& pipes: subbuffers) {
    pipes.resize(4);
    for(auto&& pipe: pipes) {
        pipe.resize(subL);
    }
  }

  for (size_t N = 1; N < maxN; ++N) {
    for(size_t subN = 1; subN < maxsubN; ++subN) {

      size_t j1 = 0, j4 = 0;
      std::atomic<size_t> j2 = 0;
      std::atomic<size_t> j3 = 0;

      // begin of pipeline ---------------------------
      turbo::Pipeline pl(L,

        // begin of pipe 1 -----------------------------
        turbo::Pipe{turbo::PipeType::SERIAL, [&, w, N, subN, subL](auto& pf) mutable {
          if(j1 == N) {
            pf.stop();
            return;
          }

          size_t subj1 = 0, subj3 = 0;
          std::atomic<size_t> subj2 = 0;
          std::vector<int> subcollection;
          subcollection.reserve(subN);

          // subpipeline
          turbo::Pipeline subpl(subL,

            // subpipe 1
            turbo::Pipe{turbo::PipeType::SERIAL, [&, subN](auto& subpf) mutable {
              if(subj1 == subN) {
                subpf.stop();
                return;
              }

              EXPECT_TRUE(subpf.token() % subL == subpf.line());

              subbuffers[pf.line()][pf.pipe()][subpf.line()][subpf.pipe()]
                = source[pf.token()][subj1] + 1;

              ++subj1;
            }},

            // subpipe 2
            turbo::Pipe{turbo::PipeType::PARALLEL, [&, subN](auto& subpf) mutable {
              EXPECT_TRUE(subj2++ < subN);
              EXPECT_TRUE(subpf.token() % subL == subpf.line());
              EXPECT_TRUE(source[pf.token()][subpf.token()] + 1 == subbuffers[pf.line()][pf.pipe()][subpf.line()][subpf.pipe() - 1]);
              subbuffers[pf.line()][pf.pipe()][subpf.line()][subpf.pipe()]
                = source[pf.token()][subpf.token()] + 1;
            }},


            // subpipe 3
            turbo::Pipe{turbo::PipeType::SERIAL, [&, subN](auto& subpf) mutable {
              EXPECT_TRUE(subj3 < subN);
              EXPECT_TRUE(subpf.token() % subL == subpf.line());
              EXPECT_TRUE(source[pf.token()][subj3] + 1 == subbuffers[pf.line()][pf.pipe()][subpf.line()][subpf.pipe() - 1]);
              subbuffers[pf.line()][pf.pipe()][subpf.line()][subpf.pipe()]
                = source[pf.token()][subj3] + 3;
              subcollection.push_back(subbuffers[pf.line()][pf.pipe()][subpf.line()][subpf.pipe()]);
              ++subj3;
            }}
          );

          turbo::Executor executor(w);
          turbo::Workflow taskflow;

          // test task
          auto test_t = taskflow.emplace([&, subN](){
            EXPECT_TRUE(subj1 == subN);
            EXPECT_TRUE(subj2 == subN);
            EXPECT_TRUE(subj3 == subN);
            //EXPECT_TRUE(subpl.num_tokens() == subN);
            EXPECT_TRUE(subcollection.size() == subN);
          }).name("test");

          // subpipeline
          auto subpl_t = taskflow.composed_of(subpl).name("module_of_subpipeline");

          subpl_t.precede(test_t);
          executor.run(taskflow).wait();

          buffer[pf.line()][pf.pipe()] = std::accumulate(
            subcollection.begin(),
            subcollection.end(),
            0
          );

          j1++;
        }},
        // end of pipe 1 -----------------------------

         //begin of pipe 2 ---------------------------
        turbo::Pipe{turbo::PipeType::PARALLEL, [&, w, N, subN, subL](auto& pf) mutable {

          EXPECT_TRUE(j2++ < N);
          int res = std::accumulate(
            source[pf.token()].begin(),
            source[pf.token()].begin() + subN,
            0
          );
          EXPECT_TRUE(buffer[pf.line()][pf.pipe() - 1] == res + 3 * subN);

          size_t subj1 = 0, subj3 = 0;
          std::atomic<size_t> subj2 = 0;
          std::vector<int> subcollection;
          subcollection.reserve(subN);

          // subpipeline
          turbo::Pipeline subpl(subL,

            // subpipe 1
            turbo::Pipe{turbo::PipeType::SERIAL, [&, subN](auto& subpf) mutable {
              if(subj1 == subN) {
                subpf.stop();
                return;
              }

              EXPECT_TRUE(subpf.token() % subL == subpf.line());

              subbuffers[pf.line()][pf.pipe()][subpf.line()][subpf.pipe()]
                = source[pf.token()][subj1] + 1;

              ++subj1;
            }},

            // subpipe 2
            turbo::Pipe{turbo::PipeType::PARALLEL, [&, subN](auto& subpf) mutable {
              EXPECT_TRUE(subj2++ < subN);
              EXPECT_TRUE(subpf.token() % subL == subpf.line());
              EXPECT_TRUE(source[j2][subpf.token()] + 1 == subbuffers[pf.line()][pf.pipe()][subpf.line()][subpf.pipe() - 1]);
              subbuffers[pf.line()][pf.pipe()][subpf.line()][subpf.pipe()]
                = source[pf.token()][subpf.token()] + 1;
            }},


            // subpipe 3
            turbo::Pipe{turbo::PipeType::SERIAL, [&, subN](auto& subpf) mutable {
              EXPECT_TRUE(subj3 < subN);
              EXPECT_TRUE(subpf.token() % subL == subpf.line());
              EXPECT_TRUE(source[pf.token()][subj3] + 1 == subbuffers[pf.line()][pf.pipe()][subpf.line()][subpf.pipe() - 1]);
              subbuffers[pf.line()][pf.pipe()][subpf.line()][subpf.pipe()]
                = source[pf.token()][subj3] + 13;
              subcollection.push_back(subbuffers[pf.line()][pf.pipe()][subpf.line()][subpf.pipe()]);
              ++subj3;
            }}
          );

          turbo::Executor executor(w);
          turbo::Workflow taskflow;

          // test task
          auto test_t = taskflow.emplace([&, subN](){
            EXPECT_TRUE(subj1 == subN);
            EXPECT_TRUE(subj2 == subN);
            EXPECT_TRUE(subj3 == subN);
            //EXPECT_TRUE(subpl.num_tokens() == subN);
            EXPECT_TRUE(subcollection.size() == subN);
          }).name("test");

          // subpipeline
          auto subpl_t = taskflow.composed_of(subpl).name("module_of_subpipeline");

          subpl_t.precede(test_t);
          executor.run(taskflow).wait();

          buffer[pf.line()][pf.pipe()] = std::accumulate(
            subcollection.begin(),
            subcollection.end(),
            0
          );

        }},
        // end of pipe 2 -----------------------------

        // begin of pipe 3 ---------------------------
        turbo::Pipe{turbo::PipeType::SERIAL, [&, w, N, subN, subL](auto& pf) mutable {

          EXPECT_TRUE(j3++ < N);
          int res = std::accumulate(
            source[pf.token()].begin(),
            source[pf.token()].begin() + subN,
            0
          );

          EXPECT_TRUE(buffer[pf.line()][pf.pipe() - 1] == res + 13 * subN);

          size_t subj1 = 0, subj3 = 0;
          std::atomic<size_t> subj2 = 0;
          std::vector<int> subcollection;
          subcollection.reserve(subN);

          // subpipeline
          turbo::Pipeline subpl(subL,

            // subpipe 1
            turbo::Pipe{turbo::PipeType::SERIAL, [&, subN](auto& subpf) mutable {
              if(subj1 == subN) {
                subpf.stop();
                return;
              }

              EXPECT_TRUE(subpf.token() % subL == subpf.line());

              subbuffers[pf.line()][pf.pipe()][subpf.line()][subpf.pipe()]
                = source[pf.token()][subj1] + 1;

              ++subj1;
            }},

            // subpipe 2
            turbo::Pipe{turbo::PipeType::PARALLEL, [&, subN](auto& subpf) mutable {
              EXPECT_TRUE(subj2++ < subN);
              EXPECT_TRUE(subpf.token() % subL == subpf.line());
              EXPECT_TRUE(source[pf.token()][subpf.token()] + 1 == subbuffers[pf.line()][pf.pipe()][subpf.line()][subpf.pipe() - 1]);
              subbuffers[pf.line()][pf.pipe()][subpf.line()][subpf.pipe()]
                = source[pf.token()][subpf.token()] + 1;
            }},


            // subpipe 3
            turbo::Pipe{turbo::PipeType::SERIAL, [&, subN](auto& subpf) mutable {
              EXPECT_TRUE(subj3 < subN);
              EXPECT_TRUE(subpf.token() % subL == subpf.line());
              EXPECT_TRUE(source[pf.token()][subj3] + 1 == subbuffers[pf.line()][pf.pipe()][subpf.line()][subpf.pipe() - 1]);
              subbuffers[pf.line()][pf.pipe()][subpf.line()][subpf.pipe()]
                = source[pf.token()][subj3] + 7;
              subcollection.push_back(subbuffers[pf.line()][pf.pipe()][subpf.line()][subpf.pipe()]);
              ++subj3;
            }}
          );

          turbo::Executor executor(w);
          turbo::Workflow taskflow;

          // test task
          auto test_t = taskflow.emplace([&, subN](){
            EXPECT_TRUE(subj1 == subN);
            EXPECT_TRUE(subj2 == subN);
            EXPECT_TRUE(subj3 == subN);
            //EXPECT_TRUE(subpl.num_tokens() == subN);
            EXPECT_TRUE(subcollection.size() == subN);
          }).name("test");

          // subpipeline
          auto subpl_t = taskflow.composed_of(subpl).name("module_of_subpipeline");

          subpl_t.precede(test_t);
          executor.run(taskflow).wait();

          buffer[pf.line()][pf.pipe()] = std::accumulate(
            subcollection.begin(),
            subcollection.end(),
            0
          );

        }},
        // end of pipe 3 -----------------------------

        // begin of pipe 4 ---------------------------
        turbo::Pipe{turbo::PipeType::SERIAL, [&, subN](auto& pf) mutable {

          int res = std::accumulate(
            source[j4].begin(),
            source[j4].begin() + subN,
            0
          );
          EXPECT_TRUE(buffer[pf.line()][pf.pipe() - 1] == res + 7 * subN);
          j4++;
        }}
        // end of pipe 4 -----------------------------
      );

      turbo::Workflow taskflow;
      taskflow.composed_of(pl).name("module_of_pipeline");
      executor.run(taskflow).wait();
    }
  }
}

TEST(PipelineinPipeline, Pipelines_1L_1W_1subL) {
  pipeline_in_pipeline(1, 1, 1);
}

TEST(PipelineinPipeline, Pipelines_1L_1W_3subL) {
  pipeline_in_pipeline(1, 1, 3);
}

TEST(PipelineinPipeline, Pipelines_1L_1W_4subL) {
  pipeline_in_pipeline(1, 1, 4);
}

TEST(PipelineinPipeline, Pipelines_1L_2W_1subL) {
  pipeline_in_pipeline(1, 2, 1);
}

TEST(PipelineinPipeline, Pipelines_1L_2W_3subL) {
  pipeline_in_pipeline(1, 2, 3);
}

TEST(PipelineinPipeline, Pipelines_1L_2W_4subL) {
  pipeline_in_pipeline(1, 2, 4);
}

TEST(PipelineinPipeline, Pipelines_3L_1W_1subL) {
  pipeline_in_pipeline(3, 1, 1);
}

TEST(PipelineinPipeline, Pipelines_3L_1W_3subL) {
  pipeline_in_pipeline(3, 1, 3);
}

TEST(PipelineinPipeline, Pipelines_3L_1W_4subL) {
  pipeline_in_pipeline(3, 1, 4);
}

TEST(PipelineinPipeline, Pipelines_3L_2W_1subL) {
  pipeline_in_pipeline(3, 2, 1);
}

TEST(PipelineinPipeline, Pipelines_3L_2W_3subL) {
  pipeline_in_pipeline(3, 2, 3);
}

TEST(PipelineinPipeline, Pipelines_3L_2W_4subL) {
  pipeline_in_pipeline(3, 2, 4);
}

TEST(PipelineinPipeline, Pipelines_5L_1W_1subL) {
  pipeline_in_pipeline(5, 1, 1);
}

TEST(PipelineinPipeline, Pipelines_5L_1W_3subL) {
  pipeline_in_pipeline(5, 1, 3);
}

TEST(PipelineinPipeline, Pipelines_5L_1W_4subL) {
  pipeline_in_pipeline(5, 1, 4);
}

TEST(PipelineinPipeline, Pipelines_5L_2W_1subL) {
  pipeline_in_pipeline(5, 2, 1);
}

TEST(PipelineinPipeline, Pipelines_5L_2W_3subL) {
  pipeline_in_pipeline(5, 2, 3);
}

TEST(PipelineinPipeline, Pipelines_5L_2W_4subL) {
  pipeline_in_pipeline(5, 2, 4);
}
