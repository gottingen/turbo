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
#ifndef TURBO_TASKFLOW_CORE_DECLARATIONS_H_
#define TURBO_TASKFLOW_CORE_DECLARATIONS_H_

namespace turbo {

    // ----------------------------------------------------------------------------
    // taskflow
    // ----------------------------------------------------------------------------
    class AsyncTopology;

    class Node;

    class Graph;

    class FlowBuilder;

    class Semaphore;

    class Subflow;

    class Runtime;

    class Task;

    class TaskView;

    class Taskflow;

    class Topology;

    class TopologyBase;

    class Executor;

    class Worker;

    class WorkerView;

    class ObserverInterface;

    class ChromeTracingObserver;

    class TFProfObserver;

    class TFProfManager;

    template<typename T>
    class Future;

    template<typename...Fs>
    class Pipeline;

    // ----------------------------------------------------------------------------
    // cudaFlow
    // ----------------------------------------------------------------------------
    class cudaFlowNode;

    class cudaFlowGraph;

    class cudaTask;

    class cudaFlow;

    class cudaFlowCapturer;

    class cudaFlowOptimizerBase;

    class cudaFlowLinearOptimizer;

    class cudaFlowSequentialOptimizer;

    class cudaFlowRoundRobinOptimizer;

    // ----------------------------------------------------------------------------
    // syclFlow
    // ----------------------------------------------------------------------------
    class syclNode;

    class syclGraph;

    class syclTask;

    class syclFlow;


}  // namespace turbo
#endif  // TURBO_TASKFLOW_CORE_DECLARATIONS_H_
