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

// ----------------------------------------------------------------------------
// Constructors and Assignments
// ----------------------------------------------------------------------------

TEST(ScalablePipeline, Basics) {

  size_t N = 10;

  std::vector< turbo::Pipe<std::function<void(turbo::Pipeflow&)>> > pipes;

  for(size_t i=0; i<N; i++) {
    pipes.emplace_back(turbo::PipeType::SERIAL, [&](turbo::Pipeflow&) {});
  }

  using iterator_type = decltype(pipes)::iterator;

  turbo::ScalablePipeline<iterator_type> rhs;

  EXPECT_TRUE(rhs.num_lines()  == 0);
  EXPECT_TRUE(rhs.num_pipes()  == 0);
  EXPECT_TRUE(rhs.num_tokens() == 0);

  rhs.reset(1, pipes.begin(), pipes.end());

  EXPECT_TRUE(rhs.num_lines()  == 1);
  EXPECT_TRUE(rhs.num_pipes()  == N);
  EXPECT_TRUE(rhs.num_tokens() == 0);

  turbo::ScalablePipeline<iterator_type> lhs(std::move(rhs));

  EXPECT_TRUE(rhs.num_lines()  == 0);
  EXPECT_TRUE(rhs.num_pipes()  == 0);
  EXPECT_TRUE(rhs.num_tokens() == 0);
  EXPECT_TRUE(lhs.num_lines()  == 1);
  EXPECT_TRUE(lhs.num_pipes()  == N);
  EXPECT_TRUE(lhs.num_tokens() == 0);

  rhs = std::move(lhs);

  EXPECT_TRUE(lhs.num_lines()  == 0);
  EXPECT_TRUE(lhs.num_pipes()  == 0);
  EXPECT_TRUE(lhs.num_tokens() == 0);
  EXPECT_TRUE(rhs.num_lines()  == 1);
  EXPECT_TRUE(rhs.num_pipes()  == N);
  EXPECT_TRUE(rhs.num_tokens() == 0);
}

// ----------------------------------------------------------------------------
// Scalable Pipeline
// ----------------------------------------------------------------------------

void scalable_pipeline(size_t num_lines, size_t num_pipes) {

  turbo::Executor executor;
  turbo::Workflow taskflow;

  size_t N = 0;

  std::vector< turbo::Pipe<std::function<void(turbo::Pipeflow&)>> > pipes;
  std::vector< int > data(num_lines, -1);

  for(size_t i=0; i<num_pipes; i++) {
    pipes.emplace_back(turbo::PipeType::SERIAL, [&](turbo::Pipeflow& pf) mutable {

      switch(pf.pipe()) {
        case 0:
          if(pf.token() == 1111) {
            pf.stop();
            return;
          }
          data[pf.line()] = num_pipes * pf.token();
        break;

        default: {
          ++data[pf.line()];
        }
        break;
      }
      //printf("data[%zu]=%d\n", pf.line(), data[pf.line()]);
      EXPECT_TRUE(data[pf.line()] == (pf.token() * num_pipes + pf.pipe()));
      if(pf.pipe() == num_pipes - 1) {
        N++;
      }
    });
  }

  turbo::ScalablePipeline spl(num_lines, pipes.begin(), pipes.end());
  taskflow.composed_of(spl);
  executor.run(taskflow).wait();

  EXPECT_TRUE(N == 1111);
}

TEST(Scalable, Pipeline) {
  for(size_t L=1; L<=10; L++) {
    for(size_t P=1; P<=10; P++) {
      scalable_pipeline(L, P);
    }
  }
}

// ----------------------------------------------------------------------------
// Scalable Pipeline using Reset
// ----------------------------------------------------------------------------

void scalable_pipeline_reset(size_t num_lines, size_t num_pipes) {

  turbo::Executor executor;
  turbo::Workflow taskflow;

  size_t N = 0;

  std::vector< turbo::Pipe<std::function<void(turbo::Pipeflow&)>> > pipes;
  std::vector< int > data(num_lines, -1);

  turbo::ScalablePipeline<typename decltype(pipes)::iterator> spl(num_lines);

  auto init = taskflow.emplace([&](){
    for(size_t i=0; i<num_pipes; i++) {
      pipes.emplace_back(turbo::PipeType::SERIAL, [&](turbo::Pipeflow& pf) mutable {

        switch(pf.pipe()) {
          case 0:
            if(pf.token() == 1111) {
              pf.stop();
              return;
            }
            data[pf.line()] = num_pipes * pf.token();
          break;

          default: {
            ++data[pf.line()];
          }
          break;
        }
        //printf("data[%zu]=%d\n", pf.line(), data[pf.line()]);
        EXPECT_TRUE(data[pf.line()] == (pf.token() * num_pipes + pf.pipe()));

        if(pf.pipe() == num_pipes - 1) {
          N++;
        }
      });
    }
    spl.reset(pipes.begin(), pipes.end());
  });

  auto pipeline = taskflow.composed_of(spl);
  pipeline.succeed(init);
  executor.run(taskflow).wait();

  EXPECT_TRUE(N == 1111);
}

TEST(ScalablePipeline, Reset) {
  for(size_t L=1; L<=10; L++) {
    for(size_t P=1; P<=10; P++) {
      scalable_pipeline_reset(L, P);
    }
  }
}

// ----------------------------------------------------------------------------
// Scalable Pipeline using Iterative Reset
// ----------------------------------------------------------------------------

void scalable_pipeline_iterative_reset(size_t num_lines, size_t num_pipes) {

  turbo::Executor executor;
  turbo::Workflow taskflow;

  size_t N = 0;

  std::vector< turbo::Pipe<std::function<void(turbo::Pipeflow&)>> > pipes;
  std::vector< int > data(num_lines, -1);

  turbo::ScalablePipeline<typename decltype(pipes)::iterator> spl(num_lines);

  auto init = taskflow.emplace([&](){
    for(size_t i=0; i<num_pipes; i++) {
      pipes.emplace_back(turbo::PipeType::SERIAL, [&](turbo::Pipeflow& pf) mutable {

        switch(pf.pipe()) {
          case 0:
            if(pf.token() == 1111) {
              pf.stop();
              return;
            }
            data[pf.line()] = num_pipes * pf.token();
          break;

          default: {
            ++data[pf.line()];
          }
          break;
        }
        //printf("data[%zu]=%d\n", pf.line(), data[pf.line()]);
        EXPECT_TRUE(data[pf.line()] == (pf.token() * num_pipes + pf.pipe()));

        if(pf.pipe() == num_pipes - 1) {
          N++;
        }
      });
    }
    spl.reset(pipes.begin(), pipes.end());
  });

  auto cond = taskflow.emplace([&, i=0]()mutable{
    EXPECT_TRUE(N == 1111*(i+1));
    spl.reset();
    return (i++ < 3) ? 0 : -1;
  });

  auto pipeline = taskflow.composed_of(spl);
  pipeline.succeed(init)
          .precede(cond);
  cond.precede(pipeline);
  executor.run(taskflow).wait();
}

TEST(ScalablePipeline, IterativeReset) {
  for(size_t L=1; L<=10; L++) {
    for(size_t P=1; P<=10; P++) {
      scalable_pipeline_iterative_reset(L, P);
    }
  }
}

// ----------------------------------------------------------------------------
// Scalable Pipeline Reset
//
// reset(num_lines, pipes.begin(), pipes.end())
// ----------------------------------------------------------------------------

void scalable_pipeline_lines_reset(size_t num_lines, size_t num_pipes) {

  turbo::Executor executor;

  size_t N = 0;

  std::vector<turbo::Pipe<>> pipes;
  turbo::ScalablePipeline<typename decltype(pipes)::iterator> spl;

  for(size_t l = 1; l <= num_lines; ++l) {
    turbo::Workflow taskflow;
    std::vector<int> data(l, -1);

    auto init = taskflow.emplace([&](){
      for(size_t i=0; i<num_pipes; i++) {
        pipes.emplace_back(turbo::PipeType::SERIAL, [&](turbo::Pipeflow& pf) mutable {

          switch(pf.pipe()) {
            case 0:
              if(pf.token() == 1111) {
                pf.stop();
                return;
              }
              data[pf.line()] = num_pipes * pf.token();
            break;

            default: {
              ++data[pf.line()];
            }
            break;
          }
          //printf("data[%zu]=%d\n", pf.line(), data[pf.line()]);
          EXPECT_TRUE(data[pf.line()] == (pf.token() * num_pipes + pf.pipe()));

          if(pf.pipe() == num_pipes - 1) {
            N++;
          }
        });
      }
      spl.reset(l, pipes.begin(), pipes.end());
    });

    auto check = taskflow.emplace([&]()mutable{
      EXPECT_TRUE(N == 1111 * l);
      pipes.clear();
    });

    auto pipeline = taskflow.composed_of(spl);
    pipeline.succeed(init)
            .precede(check);
    executor.run(taskflow).wait();
  }

}

TEST(ScalablePipeline, LinesReset) {
  for(size_t P=1; P<=10; P++) {
    scalable_pipeline_lines_reset(10, P);
  }
}

// ----------------------------------------------------------------------------
//
// ifelse ScalablePipeline has three pipes, L lines, w workers
//
// SPS
// ----------------------------------------------------------------------------

int ifelse_spipe_ans(int a) {
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

void ifelse_spipeline(size_t L, unsigned w) {
  srand(time(NULL));

  turbo::Executor executor(w);
  size_t maxN = 200;

  std::vector<int> source(maxN);
  for(auto&& s: source) {
    s = rand() % 9962;
  }
  std::vector<std::array<int, 4>> buffer(L);

  std::vector<turbo::Pipe<>> pipes;
  turbo::ScalablePipeline<typename decltype(pipes)::iterator> pl;

  for(size_t N = 1; N < maxN; ++N) {
    turbo::Workflow taskflow;

    std::vector<int> collection;
    collection.reserve(N);

    // pipe 1
    pipes.emplace_back(turbo::PipeType::SERIAL, [&, N](auto& pf){
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

    });

      // pipe 2
    pipes.emplace_back(turbo::PipeType::PARALLEL, [&](auto& pf){

        if(buffer[pf.line()][pf.pipe() - 1] > 4897) {
          buffer[pf.line()][pf.pipe()] =  buffer[pf.line()][pf.pipe() - 1] - 1834;
        }
        else {
          buffer[pf.line()][pf.pipe()] = buffer[pf.line()][pf.pipe() - 1] + 3;
        }

    });

    // pipe 3
    pipes.emplace_back(turbo::PipeType::SERIAL, [&](auto& pf){

        if((buffer[pf.line()][pf.pipe() - 1] + 9) / 4 < 50) {
          buffer[pf.line()][pf.pipe()] = buffer[pf.line()][pf.pipe() - 1] + 1;
        }
        else {
          buffer[pf.line()][pf.pipe()] = buffer[pf.line()][pf.pipe() - 1] + 17;
        }

        collection.push_back(buffer[pf.line()][pf.pipe()]);

    });

    pl.reset(L, pipes.begin(), pipes.end());

    auto pl_t = taskflow.composed_of(pl).name("pipeline");

    auto check_t = taskflow.emplace([&](){
      for(size_t n = 0; n < N; ++n) {
        EXPECT_TRUE(collection[n] == ifelse_spipe_ans(source[n]));
      }
    }).name("check");

    pl_t.precede(check_t);

    executor.run(taskflow).wait();

    pipes.clear();
  }
}

TEST(ScalablePipeline, Ifelse_1L_1W) {
  ifelse_spipeline(1, 1);
}

TEST(ScalablePipeline, Ifelse_1L_2W) {
  ifelse_spipeline(1, 2);
}

TEST(ScalablePipeline, Ifelse_1L_3W) {
  ifelse_spipeline(1, 3);
}

TEST(ScalablePipeline, Ifelse_1L_4W) {
  ifelse_spipeline(1, 4);
}

TEST(ScalablePipeline, Ifelse_3L_1W) {
  ifelse_spipeline(3, 1);
}

TEST(ScalablePipeline, Ifelse_3L_2W) {
  ifelse_spipeline(3, 2);
}

TEST(ScalablePipeline, Ifelse_3L_3W) {
  ifelse_spipeline(3, 3);
}

TEST(ScalablePipeline, Ifelse_3L_4W) {
  ifelse_spipeline(3, 4);
}

TEST(ScalablePipeline, Ifelse_5L_1W) {
  ifelse_spipeline(5, 1);
}

TEST(ScalablePipeline, Ifelse_5L_2W) {
  ifelse_spipeline(5, 2);
}

TEST(ScalablePipeline, Ifelse_5L_3W) {
  ifelse_spipeline(5, 3);
}

TEST(ScalablePipeline, Ifelse_5L_4W) {
  ifelse_spipeline(5, 4);
}

TEST(ScalablePipeline, Ifelse_7L_1W) {
  ifelse_spipeline(7, 1);
}

TEST(ScalablePipeline, Ifelse_7L_2W) {
  ifelse_spipeline(7, 2);
}

TEST(ScalablePipeline, Ifelse_7L_3W) {
  ifelse_spipeline(7, 3);
}

TEST(ScalablePipeline, Ifelse_7L_4W) {
  ifelse_spipeline(7, 4);
}


// ----------------------------------------------------------------------------
// ScalablePipeline in ScalablePipeline
// pipeline has 4 pipes, L lines, W workers
// each subpipeline has 3 pipes, subL lines
//
// pipeline = SPPS
// each subpipeline = SPS
//
// ----------------------------------------------------------------------------

void spipeline_in_spipeline(size_t L, unsigned w, unsigned subL) {

  turbo::Executor executor(w);

  const size_t maxN = 7;
  const size_t maxsubN = 7;

  std::vector<std::vector<int>> source(maxN);
  for(auto&& each: source) {
    each.resize(maxsubN);
    std::iota(each.begin(), each.end(), 0);
  }

  std::vector<std::array<int, 4>> buffer(L);

  std::vector<turbo::Pipe<>> pipes;
  turbo::ScalablePipeline<typename decltype(pipes)::iterator> pl;

  // each pipe contains one subpipeline
  // each subpipeline has three pipes, subL lines
  //
  // subbuffers[0][1][2][2] means
  // first line, second pipe, third subline, third subpipe
  std::vector<std::vector<std::vector<std::array<int, 3>>>> subbuffers(L);

  for(auto&& buffer: subbuffers) {
    buffer.resize(4);
    for(auto&& each: buffer) {
        each.resize(subL);
    }
  }

  for (size_t N = 1; N < maxN; ++N) {
    for(size_t subN = 1; subN < maxsubN; ++subN) {

      size_t j1 = 0, j4 = 0;
      std::atomic<size_t> j2 = 0;
      std::atomic<size_t> j3 = 0;

      // begin of pipeline ---------------------------

      // begin of pipe 1 -----------------------------
      pipes.emplace_back(turbo::PipeType::SERIAL, [&, w, N, subN, subL](auto& pf) mutable {
        if(j1 == N) {
          pf.stop();
          return;
        }

        size_t subj1 = 0, subj3 = 0;
        std::atomic<size_t> subj2 = 0;
        std::vector<int> subcollection;
        subcollection.reserve(subN);
        std::vector<turbo::Pipe<>> subpipes;
        turbo::ScalablePipeline<typename decltype(subpipes)::iterator> subpl;

        // subpipe 1
        subpipes.emplace_back(turbo::PipeType::SERIAL, [&, subN](auto& subpf) mutable {
            if(subj1 == subN) {
              subpf.stop();
              return;
            }

            EXPECT_TRUE(subpf.token() % subL == subpf.line());

            subbuffers[pf.line()][pf.pipe()][subpf.line()][subpf.pipe()]
              = source[pf.token()][subj1] + 1;

            ++subj1;
        });

        // subpipe 2
        subpipes.emplace_back(turbo::PipeType::PARALLEL, [&, subN](auto& subpf) mutable {
            EXPECT_TRUE(subj2++ < subN);
            EXPECT_TRUE(subpf.token() % subL == subpf.line());
            EXPECT_TRUE(
              source[pf.token()][subpf.token()] + 1 ==
              subbuffers[pf.line()][pf.pipe()][subpf.line()][subpf.pipe() - 1]
            );
            subbuffers[pf.line()][pf.pipe()][subpf.line()][subpf.pipe()]
            = source[pf.token()][subpf.token()] + 1;
        });

        // subpipe 3
        subpipes.emplace_back(turbo::PipeType::SERIAL, [&, subN](auto& subpf) mutable {
          EXPECT_TRUE(subj3 < subN);
          EXPECT_TRUE(subpf.token() % subL == subpf.line());
          EXPECT_TRUE(
            source[pf.token()][subj3] + 1 ==
            subbuffers[pf.line()][pf.pipe()][subpf.line()][subpf.pipe() - 1]
          );
          subbuffers[pf.line()][pf.pipe()][subpf.line()][subpf.pipe()]
            = source[pf.token()][subj3] + 3;
          subcollection.push_back(subbuffers[pf.line()][pf.pipe()][subpf.line()][subpf.pipe()]);
          ++subj3;
        });

        turbo::Executor executor(w);
        turbo::Workflow taskflow;

        // test task
        auto test_t = taskflow.emplace([&, subN](){
          EXPECT_TRUE(subj1 == subN);
          EXPECT_TRUE(subj2 == subN);
          EXPECT_TRUE(subj3 == subN);
          EXPECT_TRUE(subpl.num_tokens() == subN);
          EXPECT_TRUE(subcollection.size() == subN);
        }).name("test");

        // subpipeline
        subpl.reset(subL, subpipes.begin(), subpipes.end());
        auto subpl_t = taskflow.composed_of(subpl).name("module_of_subpipeline");

        subpl_t.precede(test_t);
        executor.run(taskflow).wait();

        buffer[pf.line()][pf.pipe()] = std::accumulate(
          subcollection.begin(),
          subcollection.end(),
          0
        );

        j1++;
      });
      // end of pipe 1 -----------------------------

      //begin of pipe 2 ---------------------------
      pipes.emplace_back(turbo::PipeType::PARALLEL, [&, w, subN, subL](auto& pf) mutable {

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
        std::vector<turbo::Pipe<>> subpipes;
        turbo::ScalablePipeline<typename decltype(subpipes)::iterator> subpl;

        // subpipe 1
        subpipes.emplace_back(turbo::PipeType::SERIAL, [&, subN](auto& subpf) mutable {
          if(subj1 == subN) {
            subpf.stop();
            return;
          }

          EXPECT_TRUE(subpf.token() % subL == subpf.line());

          subbuffers[pf.line()][pf.pipe()][subpf.line()][subpf.pipe()] =
          source[pf.token()][subj1] + 1;

          ++subj1;
        });

        // subpipe 2
        subpipes.emplace_back(turbo::PipeType::PARALLEL, [&, subN](auto& subpf) mutable {
          EXPECT_TRUE(subj2++ < subN);
          EXPECT_TRUE(subpf.token() % subL == subpf.line());
          EXPECT_TRUE(
            source[pf.token()][subpf.token()] + 1 ==
            subbuffers[pf.line()][pf.pipe()][subpf.line()][subpf.pipe() - 1]
          );
          subbuffers[pf.line()][pf.pipe()][subpf.line()][subpf.pipe()]
          = source[pf.token()][subpf.token()] + 1;
        });

        // subpipe 3
        subpipes.emplace_back(turbo::PipeType::SERIAL, [&, subN](auto& subpf) mutable {
          EXPECT_TRUE(subj3 < subN);
          EXPECT_TRUE(subpf.token() % subL == subpf.line());
          EXPECT_TRUE(
            source[pf.token()][subj3] + 1 ==
            subbuffers[pf.line()][pf.pipe()][subpf.line()][subpf.pipe() - 1]
          );
          subbuffers[pf.line()][pf.pipe()][subpf.line()][subpf.pipe()]
          = source[pf.token()][subj3] + 13;
          subcollection.push_back(subbuffers[pf.line()][pf.pipe()][subpf.line()][subpf.pipe()]);
          ++subj3;
        });

        turbo::Executor executor(w);
        turbo::Workflow taskflow;

        // test task
        auto test_t = taskflow.emplace([&, subN](){
          EXPECT_TRUE(subj1 == subN);
          EXPECT_TRUE(subj2 == subN);
          EXPECT_TRUE(subj3 == subN);
          EXPECT_TRUE(subpl.num_tokens() == subN);
          EXPECT_TRUE(subcollection.size() == subN);
        }).name("test");

        // subpipeline
        subpl.reset(subL, subpipes.begin(), subpipes.end());
        auto subpl_t = taskflow.composed_of(subpl).name("module_of_subpipeline");

        subpl_t.precede(test_t);
        executor.run(taskflow).wait();

        buffer[pf.line()][pf.pipe()] = std::accumulate(
          subcollection.begin(),
          subcollection.end(),
          0
        );

      });
      // end of pipe 2 -----------------------------

      // begin of pipe 3 ---------------------------
      pipes.emplace_back(turbo::PipeType::SERIAL, [&, w, N, subN, subL](auto& pf) mutable {

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
        std::vector<turbo::Pipe<>> subpipes;
        turbo::ScalablePipeline<typename decltype(subpipes)::iterator> subpl;

        // subpipe 1
        subpipes.emplace_back(turbo::PipeType::SERIAL, [&, subN](auto& subpf) mutable {
          if(subj1 == subN) {
            subpf.stop();
            return;
          }

          EXPECT_TRUE(subpf.token() % subL == subpf.line());

          subbuffers[pf.line()][pf.pipe()][subpf.line()][subpf.pipe()]
            = source[pf.token()][subj1] + 1;

          ++subj1;
        });

        // subpipe 2
        subpipes.emplace_back(turbo::PipeType::PARALLEL, [&, subN](auto& subpf) mutable {
          EXPECT_TRUE(subj2++ < subN);
          EXPECT_TRUE(subpf.token() % subL == subpf.line());
          EXPECT_TRUE(
            source[pf.token()][subpf.token()] + 1 ==
            subbuffers[pf.line()][pf.pipe()][subpf.line()][subpf.pipe() - 1]
          );
          subbuffers[pf.line()][pf.pipe()][subpf.line()][subpf.pipe()] =
          source[pf.token()][subpf.token()] + 1;
        });

        // subpipe 3
        subpipes.emplace_back(turbo::PipeType::SERIAL, [&, subN](auto& subpf) mutable {
          EXPECT_TRUE(subj3 < subN);
          EXPECT_TRUE(subpf.token() % subL == subpf.line());
          EXPECT_TRUE(
            source[pf.token()][subj3] + 1 ==
            subbuffers[pf.line()][pf.pipe()][subpf.line()][subpf.pipe() - 1]
          );
          subbuffers[pf.line()][pf.pipe()][subpf.line()][subpf.pipe()] =
          source[pf.token()][subj3] + 7;
          subcollection.push_back(subbuffers[pf.line()][pf.pipe()][subpf.line()][subpf.pipe()]);
          ++subj3;
        });

        turbo::Executor executor(w);
        turbo::Workflow taskflow;

        // test task
        auto test_t = taskflow.emplace([&, subN](){
          EXPECT_TRUE(subj1 == subN);
          EXPECT_TRUE(subj2 == subN);
          EXPECT_TRUE(subj3 == subN);
          EXPECT_TRUE(subpl.num_tokens() == subN);
          EXPECT_TRUE(subcollection.size() == subN);
        }).name("test");

        // subpipeline
        subpl.reset(subL, subpipes.begin(), subpipes.end());
        auto subpl_t = taskflow.composed_of(subpl).name("module_of_subpipeline");

        subpl_t.precede(test_t);
        executor.run(taskflow).wait();

        buffer[pf.line()][pf.pipe()] = std::accumulate(
          subcollection.begin(),
          subcollection.end(),
          0
        );

      });
      // end of pipe 3 -----------------------------

      // begin of pipe 4 ---------------------------
      pipes.emplace_back(turbo::PipeType::SERIAL, [&, subN](auto& pf) mutable {
        int res = std::accumulate(
          source[j4].begin(),
          source[j4].begin() + subN,
          0
        );
        EXPECT_TRUE(buffer[pf.line()][pf.pipe() - 1] == res + 7 * subN);
        j4++;
      });
      // end of pipe 4 -----------------------------

      pl.reset(L, pipes.begin(), pipes.end());

      turbo::Workflow taskflow;
      taskflow.composed_of(pl).name("module_of_pipeline");
      executor.run(taskflow).wait();

      pipes.clear();
    }
  }
}

TEST(ScalablePipeline, PipelineinPipeline_1L_1W_1subL) {
  spipeline_in_spipeline(1, 1, 1);
}

TEST(ScalablePipeline, PipelineinPipeline_1L_1W_3subL) {
  spipeline_in_spipeline(1, 1, 3);
}

TEST(ScalablePipeline, PipelineinPipeline_1L_1W_4subL) {
  spipeline_in_spipeline(1, 1, 4);
}

TEST(ScalablePipeline, PipelineinPipeline_1L_2W_1subL) {
  spipeline_in_spipeline(1, 2, 1);
}

TEST(ScalablePipeline, PipelineinPipeline_1L_2W_3subL) {
  spipeline_in_spipeline(1, 2, 3);
}

TEST(ScalablePipeline, PipelineinPipeline_1L_2W_4subL) {
  spipeline_in_spipeline(1, 2, 4);
}

TEST(ScalablePipeline, PipelineinPipeline_3L_1W_1subL) {
  spipeline_in_spipeline(3, 1, 1);
}

TEST(ScalablePipeline, PipelineinPipeline_3L_1W_3subL) {
  spipeline_in_spipeline(3, 1, 3);
}

TEST(ScalablePipeline, PipelineinPipeline_3L_1W_4subL) {
  spipeline_in_spipeline(3, 1, 4);
}

TEST(ScalablePipeline, PipelineinPipeline_3L_2W_1subL) {
  spipeline_in_spipeline(3, 2, 1);
}

TEST(ScalablePipeline, PipelineinPipeline_3L_2W_3subL) {
  spipeline_in_spipeline(3, 2, 3);
}

TEST(ScalablePipeline, PipelineinPipeline_3L_2W_4subL) {
  spipeline_in_spipeline(3, 2, 4);
}

TEST(ScalablePipeline, PipelineinPipeline_5L_1W_1subL) {
  spipeline_in_spipeline(5, 1, 1);
}

TEST(ScalablePipeline, PipelineinPipeline_5L_1W_3subL) {
  spipeline_in_spipeline(5, 1, 3);
}

TEST(ScalablePipeline, PipelineinPipeline_5L_1W_4subL) {
  spipeline_in_spipeline(5, 1, 4);
}

TEST(ScalablePipeline, PipelineinPipeline_5L_2W_1subL) {
  spipeline_in_spipeline(5, 2, 1);
}

TEST(ScalablePipeline, PipelineinPipeline_5L_2W_3subL) {
  spipeline_in_spipeline(5, 2, 3);
}

TEST(ScalablePipeline, PipelineinPipeline_5L_2W_4subL) {
  spipeline_in_spipeline(5, 2, 4);
}

// ----------------------------------------------------------------------------
/* SNIG task graph
// o: normal task
// c: condition task
// p: pipeline
//
// four devices example:
//               o
//            / | | \
//          c  c  c  c -----
//          |  |  |  |     |
//   -----> p  p  p  p     |
//   |     | |   |  |      |
//   ----- c c   c  c      |
//         | |  |  |       |
//         o o  o  o       |
//         \ \  | /        |
//           \||/          |
//            o <-----------
//
// each pipeline has five pipes, L lines, W workers
// each pipeline = SPSPS
*/
// ----------------------------------------------------------------------------

void snig_spipeline(size_t L, unsigned w) {

  size_t NUM_SOURCE = 70000;
  size_t BATCH_SIZE = 100;

  std::array<size_t, 7> NUM_DEVICES = {1, 2, 4, 6, 9, 13, 17};
  std::atomic<size_t> finished{0};
  std::vector<int> source(NUM_SOURCE);
  std::iota(source.begin(), source.end(), 0);

  for(auto&& NUM_DEVICE: NUM_DEVICES) {
    std::vector<std::vector<std::array<int, 5>>> buffers(NUM_DEVICE);
    for(auto&& buffer: buffers) {
      buffer.resize(L);
    }

    turbo::Workflow taskflow;
    turbo::Executor executor(w);

    auto start_t = taskflow.emplace([](){}).name("start");
    auto end_t = taskflow.emplace([](){}).name("end");

    std::vector<turbo::Task> dev_ends(NUM_DEVICE);
    for(auto&& dev_end: dev_ends) {
      dev_end = taskflow.emplace([](){}).name("dev_end");
    }

    std::vector<turbo::Task> first_fetches(NUM_DEVICE);
    std::vector<turbo::Task> fetches(NUM_DEVICE);
    std::vector<std::vector<turbo::Pipe<>>> pipes(NUM_DEVICE);

    // for type
    using pipeline_it = std::vector<turbo::Pipe<>>::iterator;

    std::vector<turbo::Task> module_of_pipelines(NUM_DEVICE);

    std::vector<turbo::ScalablePipeline<pipeline_it>> pipelines(NUM_DEVICE);

    std::vector<size_t> dev_begins(NUM_DEVICE);

    std::vector<size_t> j1s(NUM_DEVICE, 0);
    std::vector<size_t> j3s(NUM_DEVICE, 0);
    std::vector<size_t> j5s(NUM_DEVICE, 0);
    std::vector<std::unique_ptr<std::atomic<size_t>>> j2s(NUM_DEVICE);
    std::vector<std::unique_ptr<std::atomic<size_t>>> j4s(NUM_DEVICE);

    for(size_t dev = 0; dev < NUM_DEVICE; ++dev) {
      j2s[dev] = std::make_unique<std::atomic<size_t>>(0);
      j4s[dev] = std::make_unique<std::atomic<size_t>>(0);
    }

    std::vector<std::vector<int>> collections(NUM_DEVICE);
    for(auto&& collection: collections) {
      collection.reserve(BATCH_SIZE);
    }

    for(size_t dev = 0; dev < NUM_DEVICE; ++dev) {
      first_fetches[dev] = taskflow.emplace([&, dev, BATCH_SIZE](){
        size_t num = finished.fetch_add(BATCH_SIZE);
        dev_begins[dev] = num;
        return num >= NUM_SOURCE;
      }).name("first_fetch");

      // pipe 1
      pipes[dev].emplace_back(
        turbo::PipeType::SERIAL,
        [&, dev, BATCH_SIZE](auto& pf) mutable {
          if(j1s[dev] == BATCH_SIZE) {
            pf.stop();
            return;
          }

          EXPECT_TRUE(pf.token() % L == pf.line());

          buffers[dev][pf.line()][pf.pipe()] = source[dev_begins[dev] + j1s[dev]] + 1;

          ++j1s[dev];
        }
      );

      // pipe 2
      pipes[dev].emplace_back(
        turbo::PipeType::PARALLEL, [&, dev, BATCH_SIZE](auto& pf) mutable {
          EXPECT_TRUE((*j2s[dev])++ < BATCH_SIZE);
          EXPECT_TRUE(pf.token() % L == pf.line());
          EXPECT_TRUE(source[dev_begins[dev] + pf.token()] + 1 == buffers[dev][pf.line()][pf.pipe() - 1]);

          buffers[dev][pf.line()][pf.pipe()] = source[dev_begins[dev] + pf.token()] + 3;

        }
      );

      // pipe 3
      pipes[dev].emplace_back(
        turbo::PipeType::SERIAL, [&, dev, BATCH_SIZE](auto& pf) mutable {
          EXPECT_TRUE(j3s[dev] < BATCH_SIZE);
          EXPECT_TRUE(pf.token() % L == pf.line());
          EXPECT_TRUE(source[dev_begins[dev] + j3s[dev]] + 3 == buffers[dev][pf.line()][pf.pipe() - 1]);

          buffers[dev][pf.line()][pf.pipe()] = source[dev_begins[dev] + j3s[dev]] + 8;

          ++j3s[dev];
        }
      );

      // pipe 4
      pipes[dev].emplace_back(
        turbo::PipeType::PARALLEL, [&, dev, BATCH_SIZE](auto& pf) mutable {
          EXPECT_TRUE((*j4s[dev])++ < BATCH_SIZE);
          EXPECT_TRUE(pf.token() % L == pf.line());
          EXPECT_TRUE(source[dev_begins[dev] + pf.token()] + 8 == buffers[dev][pf.line()][pf.pipe() - 1]);

          buffers[dev][pf.line()][pf.pipe()] = source[dev_begins[dev] + pf.token()] + 9;
        }
      );

      // pipe 5
      pipes[dev].emplace_back(
        turbo::PipeType::SERIAL, [&, dev, BATCH_SIZE](auto& pf) mutable {
          EXPECT_TRUE(j5s[dev] < BATCH_SIZE);
          EXPECT_TRUE(pf.token() % L == pf.line());
          EXPECT_TRUE(source[dev_begins[dev] + j5s[dev]] + 9 == buffers[dev][pf.line()][pf.pipe() - 1]);

          collections[dev].push_back(buffers[dev][pf.line()][pf.pipe() - 1] + 2);

          ++j5s[dev];
        }
      );


      fetches[dev] = taskflow.emplace([&, dev, NUM_SOURCE, BATCH_SIZE](){
        for(size_t b = 0; b < BATCH_SIZE; ++b) {
          EXPECT_TRUE(source[dev_begins[dev] + b] + 9 + 2 == collections[dev][b]);
        }
        collections[dev].clear();
        collections[dev].reserve(BATCH_SIZE);

        size_t num = finished.fetch_add(BATCH_SIZE);
        dev_begins[dev] = num;
        j1s[dev] = 0;
        *j2s[dev] = 0;
        j3s[dev] = 0;
        *j4s[dev] = 0;
        j5s[dev] = 0;
        pipelines[dev].reset();
        return num >= NUM_SOURCE;
      }).name("fetch");
    }

    for(size_t dev = 0; dev < NUM_DEVICE; ++dev) {
      pipelines[dev].reset(L, pipes[dev].begin(), pipes[dev].end());
      module_of_pipelines[dev] = taskflow.composed_of(pipelines[dev]).name("pipeline");
    }

    // dependencies
    for(size_t dev = 0; dev < NUM_DEVICE; ++dev) {
      start_t.precede(first_fetches[dev]);
      first_fetches[dev].precede(
        module_of_pipelines[dev],
        dev_ends[dev]
      );
      module_of_pipelines[dev].precede(fetches[dev]);
      fetches[dev].precede(module_of_pipelines[dev], dev_ends[dev]);
      dev_ends[dev].precede(end_t);
    }


    executor.run(taskflow).wait();
  }
}

TEST(ScalablePipeline, SNIG_1L_1W) {
  snig_spipeline(1, 1);
}

TEST(ScalablePipeline, SNIG_1L_2W) {
  snig_spipeline(1, 2);
}

TEST(ScalablePipeline, SNIG_1L_3W) {
  snig_spipeline(1, 3);
}

TEST(ScalablePipeline, SNIG_3L_1W) {
  snig_spipeline(3, 1);
}

TEST(ScalablePipeline, SNIG_3L_2W) {
  snig_spipeline(3, 2);
}

TEST(ScalablePipeline, SNIG_3L_3W) {
  snig_spipeline(3, 3);
}

TEST(ScalablePipeline, SNIG_5L_1W) {
  snig_spipeline(5, 1);
}

TEST(ScalablePipeline, SNIG_5L_2W) {
  snig_spipeline(5, 2);
}

TEST(ScalablePipeline, SNIG_5L_3W) {
  snig_spipeline(5, 3);
}

TEST(ScalablePipeline, SNIG_7l_1W) {
  snig_spipeline(7, 1);
}

TEST(ScalablePipeline, SNIG_7l_2W) {
  snig_spipeline(7, 2);
}

TEST(ScalablePipeline, SNIG_7l_3W) {
  snig_spipeline(7, 3);
}

// ----------------------------------------------------------------------
//  Subflow pipeline
// -----------------------------------------------------------------------

void spawn(
  turbo::Subflow& sf,
  size_t L,
  size_t NUM_PIPES,
  size_t NUM_RECURS,
  size_t maxN,
  size_t r,
  std::vector<std::vector<int>>& buffer,
  std::vector<std::vector<int>>& source,
  std::vector<std::vector<turbo::Pipe<>>>& pipes,
  std::vector<turbo::ScalablePipeline<typename std::vector<turbo::Pipe<>>::iterator>>& spls,
  size_t& counter
) {

  // construct pipes
  for(size_t p = 0; p < NUM_PIPES; ++p) {
    pipes[r].emplace_back(turbo::PipeType::SERIAL, [&, maxN, r](turbo::Pipeflow& pf) mutable {

      switch(pf.pipe()) {
        case 0:
          if(pf.token() == maxN) {
            pf.stop();
            ++counter;
            return;
          }
          buffer[r][pf.line()] = source[r][pf.token()];
        break;

        default:
          ++buffer[r][pf.line()];
      }

      EXPECT_TRUE(buffer[r][pf.line()] == source[r][pf.token()] + pf.pipe());
    });
  }

  spls[r].reset(L, pipes[r].begin(), pipes[r].end());
  auto spl_t = sf.composed_of(spls[r]).name("module_of_pipeline");

  if(r + 1 < NUM_RECURS) {
    auto spawn_t = sf.emplace([&, L, NUM_PIPES, NUM_RECURS, maxN, r](turbo::Subflow& sf) mutable {
      spawn(sf, L, NUM_PIPES, NUM_RECURS, maxN, r + 1, buffer, source, pipes, spls, counter);
    });
    spawn_t.precede(spl_t);
  }

}

void subflow_spipeline(unsigned NUM_RECURS, unsigned w, size_t L) {

  turbo::Executor executor(w);
  turbo::Workflow taskflow;
  std::vector<turbo::ScalablePipeline<typename std::vector<turbo::Pipe<>>::iterator>> spls(NUM_RECURS);
  std::vector<std::vector<turbo::Pipe<>>> pipes(NUM_RECURS);

  size_t maxN = 1123;
  size_t NUM_PIPES = 5;
  size_t counter = 0;

  std::vector<std::vector<int>> source(NUM_RECURS);
  for(auto&& each: source) {
    each.resize(maxN);
    std::iota(each.begin(), each.end(), 0);
  }

  std::vector<std::vector<int>> buffer(NUM_RECURS);
  for(auto&& each:buffer) {
    each.resize(L);
  }

  auto subflows = taskflow.emplace([&, L, NUM_PIPES, NUM_RECURS, maxN](turbo::Subflow& sf){
    spawn(sf, L, NUM_PIPES, NUM_RECURS, maxN, 0, buffer, source, pipes, spls, counter);
  });

  auto check = taskflow.emplace([&, NUM_RECURS](){
    EXPECT_TRUE(counter == NUM_RECURS);
  }).name("check");

  subflows.precede(check);

  executor.run(taskflow).wait();

}

TEST(ScalablePipeline, Subflow_1R_1W_1L) {
  subflow_spipeline(1, 1, 1);
}

TEST(ScalablePipeline, Subflow_1R_1W_3L) {
  subflow_spipeline(1, 1, 3);
}

TEST(ScalablePipeline, Subflow_1R_1W_4L) {
  subflow_spipeline(1, 1, 4);
}

TEST(ScalablePipeline, Subflow_1R_2W_1L) {
  subflow_spipeline(1, 2, 1);
}

TEST(ScalablePipeline, Subflow_1R_2W_3L) {
  subflow_spipeline(1, 2, 3);
}

TEST(ScalablePipeline, Subflow_1R_2W_4L) {
  subflow_spipeline(1, 2, 4);
}

TEST(ScalablePipeline, Subflow_3R_1W_1L) {
  subflow_spipeline(3, 1, 1);
}

TEST(ScalablePipeline, Subflow_3R_1W_3L) {
  subflow_spipeline(3, 1, 3);
}

TEST(ScalablePipeline, Subflow_3R_1W_4L) {
  subflow_spipeline(3, 1, 4);
}

TEST(ScalablePipeline, Subflow_3R_2W_1L) {
  subflow_spipeline(3, 2, 1);
}

TEST(ScalablePipeline, Subflow_3R_2W_3L) {
  subflow_spipeline(3, 2, 3);
}

TEST(ScalablePipeline, Subflow_3R_2W_4L) {
  subflow_spipeline(3, 2, 4);
}

TEST(ScalablePipeline, Subflow_5R_1W_1L) {
  subflow_spipeline(5, 1, 1);
}

TEST(ScalablePipeline, Subflow_5R_1W_3L) {
  subflow_spipeline(5, 1, 3);
}

TEST(ScalablePipeline, Subflow_5R_1W_4L) {
  subflow_spipeline(5, 1, 4);
}

TEST(ScalablePipeline, Subflow_5R_2W_1L) {
  subflow_spipeline(5, 2, 1);
}

TEST(ScalablePipeline, Subflow_5R_2W_3L) {
  subflow_spipeline(5, 2, 3);
}

TEST(ScalablePipeline, Subflow_5R_2W_4L) {
  subflow_spipeline(5, 2, 4);
}

TEST(ScalablePipeline, Subflow_7R_1W_1L) {
  subflow_spipeline(7, 1, 1);
}

TEST(ScalablePipeline, Subflow_7R_1W_3L) {
  subflow_spipeline(7, 1, 3);
}

TEST(ScalablePipeline, Subflow_7R_1W_4L) {
  subflow_spipeline(7, 1, 4);
}

TEST(ScalablePipeline, Subflow_7R_2W_1L) {
  subflow_spipeline(7, 2, 1);
}

TEST(ScalablePipeline, Subflow_7R_2W_3L) {
  subflow_spipeline(7, 2, 3);
}

TEST(ScalablePipeline, Subflow_7R_2W_4L) {
  subflow_spipeline(7, 2, 4);
}


