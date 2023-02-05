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

#ifndef TURBO_WORKFLOW_CORE_WORKFLOW_H_
#define TURBO_WORKFLOW_CORE_WORKFLOW_H_

#include "turbo/workflow/core/flow_builder.h"

namespace turbo {
TURBO_NAMESPACE_BEGIN
    /**
    @class Workflow

    @brief class to create a workflow object

    A %workflow manages a task dependency graph where each task represents a
    callable object (e.g., @std_lambda, @std_function) and an edge represents a
    dependency between two tasks. A task is one of the following types:

      1. static task         : the callable constructible from
                               @c std::function<void()>
      2. dynamic task        : the callable constructible from
                               @c std::function<void(turbo::Subflow&)>
      3. condition task      : the callable constructible from
                               @c std::function<int()>
      4. multi-condition task: the callable constructible from
                               @c %std::function<turbo::InlinedVector<int>()>
      5. module task         : the task constructed from turbo::Workflow::composed_of
      6. runtime task        : the callable constructible from
                               @c std::function<void(turbo::Runtime&)>

    Each task is a basic computation unit and is run by one worker thread
    from an executor.
    The following example creates a simple workflow graph of four static tasks,
    @c A, @c B, @c C, and @c D, where
    @c A runs before @c B and @c C and
    @c D runs after  @c B and @c C.

    @code{.cpp}
    turbo::Executor executor;
    turbo::Workflow workflow("simple");

    turbo::Task A = workflow.emplace([](){ std::cout << "TaskA\n"; });
    turbo::Task B = workflow.emplace([](){ std::cout << "TaskB\n"; });
    turbo::Task C = workflow.emplace([](){ std::cout << "TaskC\n"; });
    turbo::Task D = workflow.emplace([](){ std::cout << "TaskD\n"; });

    A.precede(B, C);  // A runs before B and C
    D.succeed(B, C);  // D runs after  B and C

    executor.run(workflow).wait();
    @endcode

    The workflow object itself is NOT thread-safe. You should not
    modifying the graph while it is running,
    such as adding new tasks, adding new dependencies, and moving
    the workflow to another.
    To minimize the overhead of task creation,
    our runtime leverages a global object pool to recycle
    tasks in a thread-safe manner.

    Please refer to @ref Cookbook to learn more about each task type
    and how to submit a workflow to an executor.
    */
    class Workflow : public FlowBuilder {

        friend class Topology;

        friend class Executor;

        friend class FlowBuilder;

        struct Dumper {
            size_t id;
            std::stack<std::pair<const Node *, const Graph *>> stack;
            std::unordered_map<const Graph *, size_t> visited;
        };

    public:

        /**
        @brief constructs a workflow with the given name

        @code{.cpp}
        turbo::Workflow workflow("My Workflow");
        std::cout << workflow.name();         // "My Workflow"
        @endcode
        */
        Workflow(const std::string &name);

        /**
        @brief constructs a workflow
        */
        Workflow();

        /**
        @brief constructs a workflow from a moved workflow

        Constructing a workflow @c workflow1 from a moved workflow @c workflow2 will
        migrate the graph of @c workflow2 to @c workflow1.
        After the move, @c workflow2 will become empty.

        @code{.cpp}
        turbo::Workflow workflow1(std::move(workflow2));
        assert(workflow2.empty());
        @endcode

        Notice that @c workflow2 should not be running in an executor
        during the move operation, or the behavior is undefined.
        */
        Workflow(Workflow &&rhs);

        /**
        @brief move assignment operator

        Moving a workflow @c workflow2 to another workflow @c workflow1 will destroy
        the existing graph of @c workflow1 and assign it the graph of @c workflow2.
        After the move, @c workflow2 will become empty.

        @code{.cpp}
        workflow1 = std::move(workflow2);
        assert(workflow2.empty());
        @endcode

        Notice that both @c workflow1 and @c workflow2 should not be running
        in an executor during the move operation, or the behavior is undefined.
        */
        Workflow &operator=(Workflow &&rhs);

        /**
        @brief default destructor

        When the destructor is called, all tasks and their associated data
        (e.g., captured data) will be destroyed.
        It is your responsibility to ensure all submitted execution of this
        workflow have completed before destroying it.
        For instance, the following code results in undefined behavior
        since the executor may still be running the workflow while
        it is destroyed after the block.

        @code{.cpp}
        {
          turbo::Workflow workflow;
          executor.run(workflow);
        }
        @endcode

        To fix the problem, we must wait for the execution to complete
        before destroying the workflow.

        @code{.cpp}
        {
          turbo::Workflow workflow;
          executor.run(workflow).wait();
        }
        @endcode
        */
        ~Workflow() = default;

        /**
        @brief dumps the workflow to a DOT format through a std::ostream target

        @code{.cpp}
        workflow.dump(std::cout);  // dump the graph to the standard output

        std::ofstream ofs("output.dot");
        workflow.dump(ofs);        // dump the graph to the file output.dot
        @endcode

        For dynamically spawned tasks, such as module tasks, subflow tasks,
        and GPU tasks, you need to run the workflow first before you can
        dump the entire graph.

        @code{.cpp}
        turbo::Task parent = workflow.emplace([](turbo::Subflow sf){
          sf.emplace([](){ std::cout << "child\n"; });
        });
        workflow.dump(std::cout);      // this dumps only the parent tasks
        executor.run(workflow).wait();
        workflow.dump(std::cout);      // this dumps both parent and child tasks
        @endcode
        */
        void dump(std::ostream &ostream) const;

        /**
        @brief dumps the workflow to a std::string of DOT format

        This method is similar to turbo::Workflow::dump(std::ostream& ostream),
        but returning a string of the graph in DOT format.
        */
        std::string dump() const;

        /**
        @brief queries the number of tasks
        */
        size_t num_tasks() const;

        /**
        @brief queries the emptiness of the workflow

        An empty workflow has no tasks. That is the return of
        turbo::Workflow::num_tasks is zero.
        */
        bool empty() const;

        /**
        @brief assigns a name to the workflow

        @code{.cpp}
        workflow.name("assign another name");
        @endcode
        */
        void name(const std::string &);

        /**
        @brief queries the name of the workflow

        @code{.cpp}
        std::cout << "my name is: " << workflow.name();
        @endcode
        */
        const std::string &name() const;

        /**
        @brief clears the associated task dependency graph

        When you clear a workflow, all tasks and their associated data
        (e.g., captured data in task callables) will be destroyed.
        The behavior of clearing a running workflow is undefined.
        */
        void clear();

        /**
        @brief applies a visitor to each task in the workflow

        A visitor is a callable that takes an argument of type turbo::Task
        and returns nothing. The following example iterates each task in a
        workflow and prints its name:

        @code{.cpp}
        workflow.for_each_task([](turbo::Task task){
          std::cout << task.name() << '\n';
        });
        @endcode
        */
        template<typename V>
        void for_each_task(V &&visitor) const;

        /**
        @brief returns a reference to the underlying graph object

        A graph object (of type turbo::Graph) is the ultimate storage for the
        task dependency graph and should only be used as an opaque
        data structure to interact with the executor (e.g., composition).
        */
        Graph &graph();

    private:

        mutable std::mutex _mutex;

        std::string _name;

        Graph _graph;

        std::queue<std::shared_ptr<Topology>> _topologies;

        std::optional<std::list<Workflow>::iterator> _satellite;

        void _dump(std::ostream &, const Graph *) const;

        void _dump(std::ostream &, const Node *, Dumper &) const;

        void _dump(std::ostream &, const Graph *, Dumper &) const;
    };

// Constructor
    inline Workflow::Workflow(const std::string &name) :
            FlowBuilder{_graph},
            _name{name} {
    }

    // Constructor
    inline Workflow::Workflow() : FlowBuilder{_graph} {
    }

    // Move constructor
    inline Workflow::Workflow(Workflow &&rhs) : FlowBuilder{_graph} {

        std::scoped_lock<std::mutex> lock(rhs._mutex);

        _name = std::move(rhs._name);
        _graph = std::move(rhs._graph);
        _topologies = std::move(rhs._topologies);
        _satellite = rhs._satellite;

        rhs._satellite.reset();
    }

    // Move assignment
    inline Workflow &Workflow::operator=(Workflow &&rhs) {
        if (this != &rhs) {
            std::scoped_lock<std::mutex, std::mutex> lock(_mutex, rhs._mutex);
            _name = std::move(rhs._name);
            _graph = std::move(rhs._graph);
            _topologies = std::move(rhs._topologies);
            _satellite = rhs._satellite;
            rhs._satellite.reset();
        }
        return *this;
    }

    // Procedure:
    inline void Workflow::clear() {
        _graph._clear();
    }

    // Function: num_tasks
    inline size_t Workflow::num_tasks() const {
        return _graph.size();
    }

    // Function: empty
    inline bool Workflow::empty() const {
        return _graph.empty();
    }

    // Function: name
    inline void Workflow::name(const std::string &name) {
        _name = name;
    }

// Function: name
    inline const std::string &Workflow::name() const {
        return _name;
    }

    // Function: graph
    inline Graph &Workflow::graph() {
        return _graph;
    }

    // Function: for_each_task
    template<typename V>
    void Workflow::for_each_task(V &&visitor) const {
        for (size_t i = 0; i < _graph._nodes.size(); ++i) {
            visitor(Task(_graph._nodes[i]));
        }
    }

// Procedure: dump
    inline std::string Workflow::dump() const {
        std::ostringstream oss;
        dump(oss);
        return oss.str();
    }

// Function: dump
    inline void Workflow::dump(std::ostream &os) const {
        os << "digraph Workflow {\n";
        _dump(os, &_graph);
        os << "}\n";
    }

    // Procedure: _dump
    inline void Workflow::_dump(std::ostream &os, const Graph *top) const {

        Dumper dumper;

        dumper.id = 0;
        dumper.stack.push({nullptr, top});
        dumper.visited[top] = dumper.id++;

        while (!dumper.stack.empty()) {

            auto[p, f] = dumper.stack.top();
            dumper.stack.pop();

            os << "subgraph cluster_p" << f << " {\nlabel=\"";

            // n-level module
            if (p) {
                os << 'm' << dumper.visited[f];
            }
                // top-level workflow graph
            else {
                os << "Workflow: ";
                if (_name.empty()) os << 'p' << this;
                else os << _name;
            }

            os << "\";\n";

            _dump(os, f, dumper);
            os << "}\n";
        }
    }

// Procedure: _dump
    inline void Workflow::_dump(
            std::ostream &os, const Node *node, Dumper &dumper
    ) const {

        os << 'p' << node << "[label=\"";
        if (node->_name.empty()) os << 'p' << node;
        else os << node->_name;
        os << "\" ";

        // shape for node
        switch (node->_handle.index()) {

            case Node::CONDITION:
            case Node::MULTI_CONDITION:
                os << "shape=diamond color=black fillcolor=aquamarine style=filled";
                break;

            case Node::RUNTIME:
                os << "shape=component";
                break;

            case Node::CUDAFLOW:
                os << " style=\"filled\""
                   << " color=\"black\" fillcolor=\"purple\""
                   << " fontcolor=\"white\""
                   << " shape=\"folder\"";
                break;

            case Node::SYCLFLOW:
                os << " style=\"filled\""
                   << " color=\"black\" fillcolor=\"red\""
                   << " fontcolor=\"white\""
                   << " shape=\"folder\"";
                break;

            default:
                break;
        }

        os << "];\n";

        for (size_t s = 0; s < node->_successors.size(); ++s) {
            if (node->_is_conditioner()) {
                // case edge is dashed
                os << 'p' << node << " -> p" << node->_successors[s]
                   << " [style=dashed label=\"" << s << "\"];\n";
            } else {
                os << 'p' << node << " -> p" << node->_successors[s] << ";\n";
            }
        }

        // subflow join node
        if (node->_parent && node->_parent->_handle.index() == Node::DYNAMIC &&
            node->_successors.size() == 0
                ) {
            os << 'p' << node << " -> p" << node->_parent << ";\n";
        }

        // node info
        switch (node->_handle.index()) {

            case Node::DYNAMIC: {
                auto &sbg = std::get_if<Node::Dynamic>(&node->_handle)->subgraph;
                if (!sbg.empty()) {
                    os << "subgraph cluster_p" << node << " {\nlabel=\"Subflow: ";
                    if (node->_name.empty()) os << 'p' << node;
                    else os << node->_name;

                    os << "\";\n" << "color=blue\n";
                    _dump(os, &sbg, dumper);
                    os << "}\n";
                }
            }
                break;

            case Node::CUDAFLOW: {
                std::get_if<Node::cudaFlow>(&node->_handle)->graph->dump(
                        os, node, node->_name
                );
            }
                break;

            case Node::SYCLFLOW: {
                std::get_if<Node::syclFlow>(&node->_handle)->graph->dump(
                        os, node, node->_name
                );
            }
                break;

            default:
                break;
        }
    }

    // Procedure: _dump
    inline void Workflow::_dump(
            std::ostream &os, const Graph *graph, Dumper &dumper
    ) const {

        for (const auto &n : graph->_nodes) {

            // regular task
            if (n->_handle.index() != Node::MODULE) {
                _dump(os, n, dumper);
            }
                // module task
            else {
                //auto module = &(std::get_if<Node::Module>(&n->_handle)->module);
                auto module = &(std::get_if<Node::Module>(&n->_handle)->graph);

                os << 'p' << n << "[shape=box3d, color=blue, label=\"";
                if (n->_name.empty()) os << 'p' << n;
                else os << n->_name;

                if (dumper.visited.find(module) == dumper.visited.end()) {
                    dumper.visited[module] = dumper.id++;
                    dumper.stack.push({n, module});
                }

                os << " [m" << dumper.visited[module] << "]\"];\n";

                for (const auto s : n->_successors) {
                    os << 'p' << n << "->" << 'p' << s << ";\n";
                }
            }
        }
    }

    // ----------------------------------------------------------------------------
    // class definition: Future
    // ----------------------------------------------------------------------------

    /**
    @class Future

    @brief class to access the result of an execution

    turbo::Future is a derived class from std::future that will eventually hold the
    execution result of a submitted workflow (turbo::Executor::run)
    or an asynchronous task (turbo::Executor::async, turbo::Executor::silent_async).
    In addition to the base methods inherited from std::future,
    you can call turbo::Future::cancel to cancel the execution of the running workflow
    associated with this future object.
    The following example cancels a submission of a workflow that contains
    1000 tasks each running one second.

    @code{.cpp}
    turbo::Executor executor;
    turbo::Workflow workflow;

    for(int i=0; i<1000; i++) {
      workflow.emplace([](){
        std::this_thread::sleep_for(std::chrono::seconds(1));
      });
    }

    // submit the workflow
    turbo::Future fu = executor.run(workflow);

    // request to cancel the submitted execution above
    fu.cancel();

    // wait until the cancellation finishes
    fu.get();
    @endcode
    */
    template<typename T>
    class Future : public std::future<T> {

        friend class Executor;

        friend class Subflow;

        using handle_t = std::variant<
                std::monostate, std::weak_ptr<Topology>, std::weak_ptr<AsyncTopology>
        >;

        // variant index
        constexpr static auto ASYNC = get_index_v<std::weak_ptr<AsyncTopology>, handle_t>;
        constexpr static auto WORKFLOW = get_index_v<std::weak_ptr<Topology>, handle_t>;

    public:

        /**
        @brief default constructor
        */
        Future() = default;

        /**
        @brief disabled copy constructor
        */
        Future(const Future &) = delete;

        /**
        @brief default move constructor
        */
        Future(Future &&) = default;

        /**
        @brief disabled copy assignment
        */
        Future &operator=(const Future &) = delete;

        /**
        @brief default move assignment
        */
        Future &operator=(Future &&) = default;

        /**
        @brief cancels the execution of the running workflow associated with
               this future object

        @return @c true if the execution can be cancelled or
                @c false if the execution has already completed

        When you request a cancellation, the executor will stop scheduling
        any tasks onwards. Tasks that are already running will continue to finish
        (non-preemptive).
        You can call turbo::Future::wait to wait for the cancellation to complete.
        */
        bool cancel();

    private:

        handle_t _handle;

        template<typename P>
        Future(std::future<T> &&, P &&);
    };

    template<typename T>
    template<typename P>
    Future<T>::Future(std::future<T> &&fu, P &&p) :
            std::future<T>{std::move(fu)},
            _handle{std::forward<P>(p)} {
    }

// Function: cancel
    template<typename T>
    bool Future<T>::cancel() {
        return std::visit([](auto &&arg) {
            using P = std::decay_t<decltype(arg)>;
            if constexpr(std::is_same_v<P, std::monostate>) {
                return false;
            } else {
                auto ptr = arg.lock();
                if (ptr) {
                    ptr->_is_cancelled.store(true, std::memory_order_relaxed);
                    return true;
                }
                return false;
            }
        }, _handle);
    }

TURBO_NAMESPACE_END
}  // namespace turbo

#endif  // TURBO_WORKFLOW_CORE_WORKFLOW_H_
