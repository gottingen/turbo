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

#ifndef TURBO_WORKFLOW_CORE_TOPOLOGY_H_
#define TURBO_WORKFLOW_CORE_TOPOLOGY_H_

#include <future>
#include "turbo/container/inlined_vector.h"

namespace turbo {
TURBO_NAMESPACE_BEGIN
    // class: TopologyBase
    class TopologyBase {

        friend class Executor;

        friend class Node;

        template<typename T>
        friend
        class Future;

    protected:

        std::atomic<bool> _is_cancelled{false};
    };

// ----------------------------------------------------------------------------

// class: AsyncTopology
    class AsyncTopology : public TopologyBase {
    };

// ----------------------------------------------------------------------------

// class: Topology
    class Topology : public TopologyBase {

        friend class Executor;

        friend class Runtime;

    public:

        template<typename P, typename C>
        Topology(Workflow &, P &&, C &&);

    private:

        Workflow &_workflow;

        std::promise<void> _promise;

        turbo::InlinedVector<Node *> _sources;

        std::function<bool()> _pred;
        std::function<void()> _call;

        std::atomic<size_t> _join_counter{0};
    };

    // Constructor
    template<typename P, typename C>
    Topology::Topology(Workflow &tf, P &&p, C &&c):
            _workflow(tf),
            _pred{std::forward<P>(p)},
            _call{std::forward<C>(c)} {
    }
TURBO_NAMESPACE_END
}  // namespace turbo

#endif  // TURBO_WORKFLOW_CORE_TOPOLOGY_H_
