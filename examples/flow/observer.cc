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
// Demonstrates the use of observer to monitor worker activities.

#include "turbo/taskflow/taskflow.h"

struct MyObserver : public turbo::ObserverInterface {

  MyObserver(const std::string& name) {
    std::cout << "constructing observer " << name << '\n';
  }

  // set_up is a constructor-like method that will be called exactly once
  // passing the number of workers
  void set_up(size_t num_workers) override final {
    std::cout << "setting up observer with " << num_workers << " workers\n";
  }

  // on_entry will be called before a worker runs a task
  void on_entry(turbo::WorkerView wv, turbo::TaskView tv) override final {
    std::ostringstream oss;
    oss << "worker " << wv.id() << " ready to run " << tv.name() << '\n';
    std::cout << oss.str();
  }

  // on_exit will be called after a worker completes a task
  void on_exit(turbo::WorkerView wv, turbo::TaskView tv) override final {
    std::ostringstream oss;
    oss << "worker " << wv.id() << " finished running " << tv.name() << '\n';
    std::cout << oss.str();
  }

};

int main(){

  turbo::Executor executor;

  // Create a taskflow of eight tasks
  turbo::Taskflow taskflow;

  taskflow.emplace([] () { std::cout << "1\n"; }).name("A");
  taskflow.emplace([] () { std::cout << "2\n"; }).name("B");
  taskflow.emplace([] () { std::cout << "3\n"; }).name("C");
  taskflow.emplace([] () { std::cout << "4\n"; }).name("D");
  taskflow.emplace([] () { std::cout << "5\n"; }).name("E");
  taskflow.emplace([] () { std::cout << "6\n"; }).name("F");
  taskflow.emplace([] () { std::cout << "7\n"; }).name("G");
  taskflow.emplace([] () { std::cout << "8\n"; }).name("H");

  // create a default observer
  std::shared_ptr<MyObserver> observer = executor.make_observer<MyObserver>("MyObserver");

  // run the taskflow
  executor.run(taskflow).get();

  // remove the observer (optional)
  executor.remove_observer(std::move(observer));

  return 0;
}

