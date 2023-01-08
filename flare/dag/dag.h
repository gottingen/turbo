

/****************************************************************
 * Copyright (c) 2022, liyinbin
 * All rights reserved.
 * Author by liyinbin (jeff.li) lijippy@163.com
 *****************************************************************/

#ifndef FLARE_DAG_DAG_H_
#define FLARE_DAG_DAG_H_

#include "flare/container/containers.h"
#include "flare/memory/allocator.h"
#include "flare/thread/latch.h"
#include "flare/fiber/fiber_async.h"

namespace flare {
    namespace dag_detail {
        using dag_counter = std::atomic<uint32_t>;

        template<typename T>
        struct dag_context {
            T data;
            allocator::unique_ptr<dag_counter> counters;

            template<typename F>
            inline void invoke(F &&f) {
                f(data);
            }
        };

        template<>
        struct dag_context<void> {
            allocator::unique_ptr<dag_counter> counters;

            template<typename F>
            inline void invoke(F &&f) {
                f();
            }
        };

        template<typename T>
        struct dag_task {
            using type = std::function<void(T)>;
        };
        template<>
        struct dag_task<void> {
            using type = std::function<void()>;
        };
    }  // namespace dag_detail

    ///////////////////////////////////////////////////////////////////////////////
    // Forward declarations
    ///////////////////////////////////////////////////////////////////////////////
    template<typename T>
    class dag_graph;

    template<typename T>
    class dag_builder;

    template<typename T>
    class dag_node_builder;

    ///////////////////////////////////////////////////////////////////////////////
    // dag_base<T>
    ///////////////////////////////////////////////////////////////////////////////

    // dag_base is derived by dag_graph<T> and dag_graph<void>. It has no public API.
    template<typename T>
    class dag_base {
    protected:
        friend dag_builder<T>;
        friend dag_node_builder<T>;

        using run_context = dag_detail::dag_context<T>;
        using counter_type = dag_detail::dag_counter;
        using node_index = size_t;
        using task_type = typename dag_detail::dag_task<T>::type;
        static const constexpr size_t NumReservedNodes = 32;
        static const constexpr size_t NumReservedNumOuts = 4;
        static const constexpr size_t InvalidCounterIndex = ~static_cast<size_t>(0);
        static const constexpr node_index RootIndex = 0;
        static const constexpr node_index InvalidNodeIndex =
                ~static_cast<node_index>(0);

        // dag_graph work node.
        struct dag_node {
            inline dag_node() = default;

            inline dag_node(task_type &&work);

            // The work to perform for this node in the graph.
            task_type work;

            // counterIndex if valid, is the index of the counter in the run_context for
            // this node. The counter is decremented for each completed dependency task
            // (ins), and once it reaches 0, this node will be invoked.
            size_t counterIndex = InvalidCounterIndex;

            // Indices for all downstream nodes.
            containers::vector<node_index, NumReservedNumOuts> outs;
        };

        // init_counters() allocates and initializes the ctx->coutners from
        // initialCounters.
        inline void init_counters(run_context *ctx,
                                  allocator *allocator);

        // notify() is called each time a dependency task (ins) has completed for the
        // node with the given index.
        // If all dependency tasks have completed (or this is the root node) then
        // notify() returns true and the caller should then call invoke().
        inline bool notify(run_context *, node_index);

        // invoke() calls the work function for the node with the given index, then
        // calls notify() and possibly invoke() for all the dependee nodes.
        inline void invoke(run_context *, node_index, latch *);

        // nodes is the full list of the nodes in the graph.
        // nodes[0] is always the root node, which has no dependencies (ins).
        containers::vector<dag_node, NumReservedNodes> nodes;

        // initialCounters is a list of initial counter values to be copied to
        // run_context::counters on dag_graph<>::run().
        // initialCounters is indexed by dag_node::counterIndex, and only contains counts
        // for nodes that have at least 2 dependencies (ins) - because of this the
        // number of entries in initialCounters may be fewer than nodes.
        containers::vector<uint32_t, NumReservedNodes> initialCounters;
    };

    template<typename T>
    dag_base<T>::dag_node::dag_node(task_type &&work) : work(std::move(work)) {}

    template<typename T>
    void dag_base<T>::init_counters(run_context *ctx, allocator *allocator) {
        auto numCounters = initialCounters.size();
        ctx->counters = allocator->make_unique_n<counter_type>(numCounters);
        for (size_t i = 0; i < numCounters; i++) {
            ctx->counters.get()[i] = {initialCounters[i]};
        }
    }

    template<typename T>
    bool dag_base<T>::notify(run_context *ctx, node_index nodeIdx) {
        dag_node *node = &nodes[nodeIdx];

        // If we have multiple dependencies, decrement the counter and check whether
        // we've reached 0.
        if (node->counterIndex == InvalidCounterIndex) {
            return true;
        }
        auto counters = ctx->counters.get();
        auto counter = --counters[node->counterIndex];
        return counter == 0;
    }

    template<typename T>
    void dag_base<T>::invoke(run_context *ctx, node_index nodeIdx, latch *wg) {
        dag_node *node = &nodes[nodeIdx];

        // Run this node's work.
        if (node->work) {
            ctx->invoke(node->work);
        }

        // Then call notify() on all dependees (outs), and invoke() those that
        // returned true.
        // We buffer the node to invoke (toInvoke) so we can schedule() all but the
        // last node to invoke(), and directly call the last invoke() on this thread.
        // This is done to avoid the overheads of scheduling when a direct call would
        // suffice.
        node_index toInvoke = InvalidNodeIndex;
        for (node_index idx : node->outs) {
            if (notify(ctx, idx)) {
                if (toInvoke != InvalidNodeIndex) {
                    wg->count_up(1);
                    // Schedule while promoting the WaitGroup capture from a pointer
                    // reference to a value. This ensures that the WaitGroup isn't dropped
                    // while in use.
                    attribute attr = kAttrNormal;
                    fiber_async(attr,
                                [=](latch wg) {
                                    invoke(ctx, toInvoke, &wg);
                                    wg.count_down();
                                },
                                *wg);
                }
                toInvoke = idx;
            }
        }
        if (toInvoke != InvalidNodeIndex) {
            invoke(ctx, toInvoke, wg);
        }
    }

    ///////////////////////////////////////////////////////////////////////////////
    // dag_node_builder<T>
    ///////////////////////////////////////////////////////////////////////////////

    // dag_node_builder is the builder interface for a dag_graph node.
    template<typename T>
    class dag_node_builder {
        using node_index = typename dag_base<T>::node_index;

    public:
        // then() builds and returns a new dag_graph node that will be invoked after this
        // node has completed.
        //
        // F is a function that will be called when the new dag_graph node is invoked, with
        // the signature:
        //   void(T)   when T is not void
        // or
        //   void()    when T is void
        template<typename F>
        inline dag_node_builder then(F &&);

    private:
        friend dag_builder<T>;

        inline dag_node_builder(dag_builder<T> *, node_index);

        dag_builder<T> *builder;
        node_index index;
    };

    template<typename T>
    dag_node_builder<T>::dag_node_builder(dag_builder<T> *builder, node_index index)
            : builder(builder), index(index) {}

    template<typename T>
    template<typename F>
    dag_node_builder<T> dag_node_builder<T>::then(F &&work) {
        auto node = builder->node(std::move(work));
        builder->add_dependency(*this, node);
        return node;
    }

    ///////////////////////////////////////////////////////////////////////////////
    // dag_builder<T>
    ///////////////////////////////////////////////////////////////////////////////
    template<typename T>
    class dag_builder {
    public:
        // dag_builder constructor
        inline dag_builder(allocator *allocator = allocator::Default);

        // root() returns the root dag_graph node.
        inline dag_node_builder<T> root();

        // node() builds and returns a new dag_graph node with no initial dependencies.
        // The returned node must be attached to the graph in order to invoke F or any
        // of the dependees of this returned node.
        //
        // F is a function that will be called when the new dag_graph node is invoked, with
        // the signature:
        //   void(T)   when T is not void
        // or
        //   void()    when T is void
        template<typename F>
        inline dag_node_builder<T> node(F &&work);

        // node() builds and returns a new dag_graph node that depends on all the tasks in
        // after to be completed before invoking F.
        //
        // F is a function that will be called when the new dag_graph node is invoked, with
        // the signature:
        //   void(T)   when T is not void
        // or
        //   void()    when T is void
        template<typename F>
        inline dag_node_builder<T> node(
                F &&work,
                std::initializer_list<dag_node_builder<T>> after);

        // add_dependency() adds parent as dependency on child. All dependencies of
        // child must have completed before child is invoked.
        inline void add_dependency(dag_node_builder<T> parent,
                                   dag_node_builder<T> child);

        // build() constructs and returns the dag_graph. No other methods of this class may
        // be called after calling build().
        inline allocator::unique_ptr<dag_graph<T>> build();

    private:
        static const constexpr size_t NumReservedNumIns = 4;
        using dag_node = typename dag_graph<T>::dag_node;

        // The dag_graph being built.
        allocator::unique_ptr<dag_graph<T>> dag;

        // Number of dependencies (ins) for each node in dag->nodes.
        containers::vector<uint32_t, NumReservedNumIns> numIns;
    };

    template<typename T>
    dag_builder<T>::dag_builder(allocator *allocator /* = allocator::Default */)
            : dag(allocator->make_unique<dag_graph<T>>()), numIns(allocator) {
        // Add root
        dag->nodes.emplace_back(dag_node{});
        numIns.emplace_back(0);
    }

    template<typename T>
    dag_node_builder<T> dag_builder<T>::root() {
        return dag_node_builder<T>{this, dag_base<T>::RootIndex};
    }

    template<typename T>
    template<typename F>
    dag_node_builder<T> dag_builder<T>::node(F &&work) {
        return node(std::forward<F>(work), {});
    }

    template<typename T>
    template<typename F>
    dag_node_builder<T> dag_builder<T>::node(
            F &&work,
            std::initializer_list<dag_node_builder<T>> after) {
        FLARE_CHECK(numIns.size() == dag->nodes.size()) << "node_builder vectors out of sync";
        auto index = dag->nodes.size();
        numIns.emplace_back(0);
        dag->nodes.emplace_back(dag_node{std::move(work)});
        auto node = dag_node_builder<T>{this, index};
        for (auto in : after) {
            add_dependency(in, node);
        }
        return node;
    }

    template<typename T>
    void dag_builder<T>::add_dependency(dag_node_builder<T> parent,
                                        dag_node_builder<T> child) {
        numIns[child.index]++;
        dag->nodes[parent.index].outs.push_back(child.index);
    }

    template<typename T>
    allocator::unique_ptr<dag_graph<T>> dag_builder<T>::build() {
        auto numNodes = dag->nodes.size();
        FLARE_CHECK(numIns.size() == dag->nodes.size()) << "node_builder vectors out of sync";
        for (size_t i = 0; i < numNodes; i++) {
            if (numIns[i] > 1) {
                auto &node = dag->nodes[i];
                node.counterIndex = dag->initialCounters.size();
                dag->initialCounters.push_back(numIns[i]);
            }
        }
        return std::move(dag);
    }

    ///////////////////////////////////////////////////////////////////////////////
    // dag_graph<T>
    ///////////////////////////////////////////////////////////////////////////////
    template<typename T = void>
    class dag_graph : public dag_base<T> {
    public:
        using builder_type = dag_builder<T>;
        using node_builder_type = dag_node_builder<T>;

        // run() invokes the function of each node in the graph of the dag_graph, passing
        // data to each, starting with the root node. All dependencies need to have
        // completed their function before dependees will be invoked.
        inline void run(T &data,
                        allocator *allocator = allocator::Default);

        inline latch async(T &data,
                           allocator *allocator = allocator::Default);

    };

    template<typename T>
    void dag_graph<T>::run(T &arg, allocator *allocator /* = allocator::Default */) {
        typename dag_base<T>::run_context ctx{arg};
        this->init_counters(&ctx, allocator);
        latch wg;
        this->invoke(&ctx, this->RootIndex, &wg);
        wg.wait();
    }

    template<typename T>
    inline latch dag_graph<T>::async(T &data,
                                     allocator *alloc /*= allocator::Default */) {
        latch wg;
        wg.count_up(1);
        attribute attr = kAttrNormal;
        fiber_async(attr,
                    [&data, alloc, this](latch wg) {
                        typename dag_base<T>::run_context ctx{data};
                        this->init_counters(&ctx, alloc);
                        latch l;
                        this->invoke(&ctx, this->RootIndex, &l);
                        l.wait();
                        wg.count_down();
                    }, wg);
        return wg;
    }

    ///////////////////////////////////////////////////////////////////////////////
    // dag_graph<void>
    ///////////////////////////////////////////////////////////////////////////////
    template<>
    class dag_graph<void> : public dag_base<void> {
    public:
        using builder_type = dag_builder<void>;
        using node_builder_type = dag_node_builder<void>;

        // run() invokes the function of each node in the graph of the dag_graph, starting
        // with the root node. All dependencies need to have completed their function
        // before dependees will be invoked.
        inline void run(allocator *allocator = allocator::Default);

        inline latch async(allocator *allocator = allocator::Default);
    };

    void dag_graph<void>::run(allocator *allocator /* = allocator::Default */) {
        typename dag_base<void>::run_context ctx{};
        this->init_counters(&ctx, allocator);
        latch wg;
        this->invoke(&ctx, this->RootIndex, &wg);
        wg.wait();
    }

    inline latch dag_graph<void>::async(allocator *alloc /*= allocator::Default */) {
        latch wg;
        wg.count_up(1);
        attribute attr = kAttrNormal;
        fiber_async(attr,
                    [alloc, this](latch wg) {
                        typename dag_base<void>::run_context ctx{};
                        this->init_counters(&ctx, alloc);
                        latch l;
                        this->invoke(&ctx, this->RootIndex, &l);
                        l.wait();
                        wg.count_down();
                    }, wg);
        return wg;
    }

}  // namespace flare

#endif  // FLARE_DAG_DAG_H_
