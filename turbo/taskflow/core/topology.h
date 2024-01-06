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
#ifndef TURBO_TASKFLOW_CORE_TOPOLOGY_H_
#define TURBO_TASKFLOW_CORE_TOPOLOGY_H_

namespace turbo {

    class TopologyBase {

    };

    // class: Topology
    class Topology {

        friend class Executor;

        friend class Runtime;

        friend class Node;

        template<typename T>
        friend
        class Future;

        constexpr static int CLEAN = 0;
        constexpr static int CANCELLED = 1;
        constexpr static int EXCEPTION = 2;

    public:

        template<typename P, typename C>
        Topology(Taskflow &, P &&, C &&);

    private:

        Taskflow &_taskflow;

        std::promise<void> _promise;

        turbo::SmallVector<Node *> _sources;

        std::function<bool()> _pred;
        std::function<void()> _call;

        std::atomic<size_t> _join_counter{0};
        std::atomic<int> _state{CLEAN};

        std::exception_ptr _exception{nullptr};

        void _carry_out_promise();
    };

    // Constructor
    template<typename P, typename C>
    Topology::Topology(Taskflow &tf, P &&p, C &&c):
            _taskflow(tf),
            _pred{std::forward<P>(p)},
            _call{std::forward<C>(c)} {
    }

    // Procedure
    inline void Topology::_carry_out_promise() {
        _exception ? _promise.set_exception(_exception) : _promise.set_value();
    }

}  // namespace turbo
#endif  // TURBO_TASKFLOW_CORE_TOPOLOGY_H_
