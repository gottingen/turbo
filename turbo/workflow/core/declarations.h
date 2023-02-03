// Copyright 2023 The Turbo Authors.
//
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

#ifndef TURBO_WORKFLOW_CORE_DECLARATIONS_H_
#define TURBO_WORKFLOW_CORE_DECLARATIONS_H_

namespace turbo {
TURBO_NAMESPACE_BEGIN
    // ----------------------------------------------------------------------------
    // workflow
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

    class Workflow;

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
    class cudaNode;

    class cudaGraph;

    class cudaTask;

    class cudaFlow;

    class cudaFlowCapturer;

    class cudaFlowCapturerBase;

    class cudaCapturingBase;

    class cudaLinearCapturing;

    class cudaSequentialCapturing;

    class cudaRoundRobinCapturing;

    // ----------------------------------------------------------------------------
    // syclFlow
    // ----------------------------------------------------------------------------
    class syclNode;

    class syclGraph;

    class syclTask;

    class syclFlow;

TURBO_NAMESPACE_END
}  // namespace turbo
#endif  // TURBO_WORKFLOW_CORE_DECLARATIONS_H_
