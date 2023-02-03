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

#include "turbo/workflow/algorithm/sort.h"
#include "turbo/workflow/workflow.h"

// --------------------------------------------------------
// Testcase: BubbleSort
// --------------------------------------------------------
TEST(sort, BubbleSort) {

  for (unsigned w = 1; w <= 9; w += 2) {

    turbo::Executor executor(w);

    for (int end = 10; end <= 1000; end += 200) {

      turbo::Workflow taskflow("BubbleSort");

      std::vector<int> data(end);

      for (auto &d : data)
        d = ::rand() % 100;

      auto gold = data;
      std::sort(gold.begin(), gold.end());

      std::atomic<bool> swapped;

      // init task
      auto init = taskflow.emplace([&swapped]() { swapped = false; });
      auto cond = taskflow.emplace([&swapped]() {
        if (swapped) {
          swapped = false;
          return 0;
        }
        return 1;
      });
      auto stop = taskflow.emplace([]() {});

      auto even_phase = taskflow.emplace([&](turbo::Subflow &sf) {
        for (size_t i = 0; i < data.size(); i += 2) {
          sf.emplace([&, i]() {
            if (i + 1 < data.size() && data[i] > data[i + 1]) {
              std::swap(data[i], data[i + 1]);
              swapped = true;
            }
          });
        }
      });

      auto odd_phase = taskflow.emplace([&](turbo::Subflow &sf) {
        for (size_t i = 1; i < data.size(); i += 2) {
          sf.emplace([&, i]() {
            if (i + 1 < data.size() && data[i] > data[i + 1]) {
              std::swap(data[i], data[i + 1]);
              swapped = true;
            }
          });
        }
      });

      init.precede(even_phase).name("init");
      even_phase.precede(odd_phase).name("even-swap");
      odd_phase.precede(cond).name("odd-swap");
      cond.precede(even_phase, stop).name("cond");

      executor.run(taskflow).wait();

      EXPECT_TRUE(gold == data);
    }
  }
}

// --------------------------------------------------------
// Testcase: SelectionSort
// --------------------------------------------------------
TEST(sort, SelectionSort) {

  std::function<void(turbo::Subflow & sf, std::vector<int> &, int, int, int &)>
      spawn;

  spawn = [&](turbo::Subflow &sf, std::vector<int> &data, int beg, int end,
              int &min) mutable {
    if (!(beg < end)) {
      min = -1;
      return;
    }

    if (end - beg == 1) {
      min = beg;
      return;
    }

    int m = (beg + end + 1) / 2;

    auto minl = new int(-1);
    auto minr = new int(-1);

    auto SL = sf.emplace([&spawn, &data, beg, m,
                          l = minl](turbo::Subflow &sf) mutable {
                  spawn(sf, data, beg, m, *l);
                })
                  .name(std::string("[") + std::to_string(beg) + ':' +
                        std::to_string(m) + ')');

    auto SR = sf.emplace([&spawn, &data, m, end,
                          r = minr](turbo::Subflow &sf) mutable {
                  spawn(sf, data, m, end, *r);
                })
                  .name(std::string("[") + std::to_string(m) + ':' +
                        std::to_string(end) + ')');

    auto SM = sf.emplace([&data, &min, minl, minr]() {
                  if (*minl == -1) {
                    min = *minr;
                  } else if (*minr == -1) {
                    min = *minl;
                    return;
                  } else {
                    min = data[*minl] < data[*minr] ? *minl : *minr;
                  }
                  delete minl;
                  delete minr;
                })
                  .name(std::string("merge [") + std::to_string(beg) + ':' +
                        std::to_string(end) + ')');

    SM.succeed(SL, SR);
  };

  for (unsigned w = 1; w <= 9; w += 2) {

    turbo::Executor executor(w);

    for (int end = 16; end <= 512; end <<= 1) {
      turbo::Workflow taskflow("SelectionSort");

      std::vector<int> data(end);

      for (auto &d : data)
        d = ::rand() % 100;

      auto gold = data;
      std::sort(gold.begin(), gold.end());

      int beg = 0;
      int min = -1;

      auto start = taskflow.emplace([]() {});

      auto argmin =
          taskflow
              .emplace(
                  [&spawn, &data, &beg, end, &min](turbo::Subflow &sf) mutable {
                    spawn(sf, data, beg, end, min);
                  })
              .name(std::string("[0") + ":" + std::to_string(end) + ")");

      auto putmin = taskflow.emplace([&]() {
        std::swap(data[beg], data[min]);
        // std::cout << "select " << data[beg] << '\n';
        beg++;
        if (beg < end) {
          min = -1;
          return 0;
        } else
          return 1;
      });

      start.precede(argmin);
      argmin.precede(putmin);
      putmin.precede(argmin);

      executor.run(taskflow).wait();

      EXPECT_TRUE(gold == data);
      // std::exit(1);
    }
  }
}

// --------------------------------------------------------
// Testcase: MergeSort
// --------------------------------------------------------
TEST(sort, MergeSort) {

  std::function<void(turbo::Subflow & sf, std::vector<int> &, int, int)> spawn;

  spawn = [&](turbo::Subflow &sf, std::vector<int> &data, int beg,
              int end) mutable {
    if (!(beg < end) || end - beg == 1) {
      return;
    }

    if (end - beg <= 5) {
      std::sort(data.begin() + beg, data.begin() + end);
      return;
    }

    int m = (beg + end + 1) / 2;

    auto SL = sf.emplace([&spawn, &data, beg, m](turbo::Subflow &sf) {
                  spawn(sf, data, beg, m);
                })
                  .name(std::string("[") + std::to_string(beg) + ':' +
                        std::to_string(m) + ')');

    auto SR = sf.emplace([&spawn, &data, m, end](turbo::Subflow &sf) {
                  spawn(sf, data, m, end);
                })
                  .name(std::string("[") + std::to_string(m) + ':' +
                        std::to_string(end) + ')');

    auto SM = sf.emplace([&data, beg, end, m]() {
                  std::vector<int> tmpl, tmpr;
                  for (int i = beg; i < m; ++i)
                    tmpl.push_back(data[i]);
                  for (int i = m; i < end; ++i)
                    tmpr.push_back(data[i]);

                  // merge to data
                  size_t i = 0, j = 0, k = beg;
                  while (i < tmpl.size() && j < tmpr.size()) {
                    data[k++] = (tmpl[i] < tmpr[j] ? tmpl[i++] : tmpr[j++]);
                  }

                  // remaining SL
                  for (; i < tmpl.size(); ++i)
                    data[k++] = tmpl[i];

                  // remaining SR
                  for (; j < tmpr.size(); ++j)
                    data[k++] = tmpr[j];
                })
                  .name(std::string("merge [") + std::to_string(beg) + ':' +
                        std::to_string(end) + ')');

    SM.succeed(SL, SR);
  };

  for (unsigned w = 1; w <= 9; w += 2) {

    turbo::Executor executor(w);

    for (int end = 10; end <= 10000; end = end * 10) {
      turbo::Workflow taskflow("MergeSort");

      std::vector<int> data(end);

      for (auto &d : data)
        d = ::rand() % 100;

      auto gold = data;

      taskflow
          .emplace([&spawn, &data, end](turbo::Subflow &sf) {
            spawn(sf, data, 0, end);
          })
          .name(std::string("[0") + ":" + std::to_string(end) + ")");

      executor.run(taskflow).wait();

      std::sort(gold.begin(), gold.end());

      EXPECT_TRUE(gold == data);
    }
  }
}

// --------------------------------------------------------
// Testcase: QuickSort
// --------------------------------------------------------
TEST(sort, QuickSort) {

  using itr_t = std::vector<int>::iterator;

  std::function<void(turbo::Subflow & sf, std::vector<int> &, itr_t, itr_t)>
      spawn;

  spawn = [&](turbo::Subflow &sf, std::vector<int> &data, itr_t beg,
              itr_t end) mutable {
    if (!(beg < end) || std::distance(beg, end) == 1) {
      return;
    }

    if (std::distance(beg, end) <= 5) {
      std::sort(beg, end);
      return;
    }

    auto pvt = beg + std::distance(beg, end) / 2;

    std::iter_swap(pvt, end - 1);

    pvt = std::partition(beg, end - 1,
                         [end](int item) { return item < *(end - 1); });

    std::iter_swap(pvt, end - 1);

    sf.emplace([&spawn, &data, beg, pvt](turbo::Subflow &sf) {
        spawn(sf, data, beg, pvt);
      })
        .name(std::string("[") + std::to_string(beg - data.begin()) + ':' +
              std::to_string(pvt - data.begin()) + ')');

    sf.emplace([&spawn, &data, pvt, end](turbo::Subflow &sf) {
        spawn(sf, data, pvt + 1, end);
      })
        .name(std::string("[") + std::to_string(pvt - data.begin()) + ':' +
              std::to_string(end - data.begin()) + ')');
  };

  for (unsigned w = 1; w <= 9; w += 2) {

    turbo::Executor executor(w);

    for (int end = 16; end <= 16384; end <<= 1) {

      turbo::Workflow taskflow("QuickSort");

      std::vector<int> data(end);

      for (auto &d : data)
        d = ::rand() % 100;

      auto gold = data;

      taskflow
          .emplace([&spawn, &data](turbo::Subflow &sf) {
            spawn(sf, data, data.begin(), data.end());
          })
          .name(std::string("[0") + ":" + std::to_string(end) + ")");

      executor.run(taskflow).wait();

      std::sort(gold.begin(), gold.end());

      EXPECT_TRUE(gold == data);
    }
  }
}
