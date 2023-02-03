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
#include "workflow.h"

class CustomWorkerBehavior : public turbo::WorkerInterface {

  public:

  CustomWorkerBehavior(std::atomic<size_t>& counter, std::vector<size_t>& ids) : 
    _counter {counter},
    _ids     {ids} {
  }
  
  void scheduler_prologue(turbo::Worker& wv) override {
    _counter++;

    std::scoped_lock lock(_mutex);
    _ids.push_back(wv.id());
  }

  void scheduler_epilogue(turbo::Worker&, std::exception_ptr) override {
    _counter++;
  }

  std::atomic<size_t>& _counter;
  std::vector<size_t>& _ids;

  std::mutex _mutex;

};

TEST(Worker, Interface) {

  const size_t N = 10;

  for(size_t n=1; n<=N; n++) {
    std::atomic<size_t> counter{0};
    std::vector<size_t> ids;

    {
      turbo::Executor executor(n, std::make_shared<CustomWorkerBehavior>(counter, ids));
    }

    EXPECT_TRUE(counter == n*2);
    EXPECT_TRUE(ids.size() == n);

    std::sort(ids.begin(), ids.end(), std::less<int>{});

    for(size_t i=0; i<n; i++) {
      EXPECT_TRUE(ids[i] == i);
    }
  }

}














