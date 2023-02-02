#pragma once

#include <future>
#include "turbo/container/inlined_vector.h"

namespace turbo {

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

}  // end of namespace turbo. ----------------------------------------------------
