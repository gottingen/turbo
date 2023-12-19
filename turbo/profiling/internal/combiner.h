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


#ifndef TURBO_PROFILING_INTERNAL_COMBINER_H_
#define TURBO_PROFILING_INTERNAL_COMBINER_H_

#include "turbo/container/intrusive_list.h"
#include "turbo/concurrent/internal/thread_local_storage.h"
#include <atomic>

namespace turbo::profiling_internal {

    template<typename Op, typename T1, typename T2>
    inline void call_op_returning_void(
            const Op &op, T1 &v1, const T2 &v2) {
        return op(v1, v2);
    }

    template<typename Combiner>
    class GlobalValue {
    public:
        typedef typename Combiner::result_type result_type;
        typedef typename Combiner::Agent agent_type;

        GlobalValue(agent_type *a, Combiner *c) : _a(a), _c(c) {}

        ~GlobalValue() {}

        // Call this method to unlock tls element and lock the combiner.
        // Unlocking tls element avoids potential deadlock with
        // AgentCombiner::reset(), which also means that tls element may be
        // changed during calling of this method. BE AWARE OF THIS!
        // After this method is called (and before unlock), tls element and
        // global_result will not be changed provided this method is called
        // from the thread owning the agent.
        result_type *lock() {
            _a->element._lock.unlock();
            _c->_lock.lock();
            return &_c->_global_result;
        }

        // Call this method to unlock the combiner and lock tls element again.
        void unlock() {
            _c->_lock.unlock();
            _a->element._lock.lock();
        }

    private:
        agent_type *_a;
        Combiner *_c;
    };

    template<typename T, typename Enabler = void>
    class ElementContainer {
        template<typename> friend
        class GlobalValue;

    public:
        void load(T *out) const {
            std::unique_lock guard(_lock);
            *out = _value;
        }

        void store(const T &new_value) {
            std::unique_lock guard(_lock);
            _value = new_value;
        }

        void exchange(T *prev, const T &new_value) {
            std::unique_lock guard(_lock);
            *prev = _value;
            _value = new_value;
        }

        template<typename Op, typename T1>
        void modify(const Op &op, const T1 &value2) {
            std::unique_lock guard(_lock);
            call_op_returning_void(op, _value, value2);
        }

        // [Unique]
        template<typename Op, typename GlobalValue>
        void merge_global(const Op &op, GlobalValue &global_value) {
            _lock.lock();
            op(global_value, _value);
            _lock.unlock();
        }

    private:
        T _value;
        std::mutex _lock;
    };

    template<typename T>
    class ElementContainer<T, typename std::enable_if<is_atomical<T>::value>::type> {
    public:
        // We don't need any memory fencing here, every op is relaxed.

        inline void load(T *out) const {
            *out = _value.load(std::memory_order_relaxed);
        }

        inline void store(T new_value) {
            _value.store(new_value, std::memory_order_relaxed);
        }

        inline void exchange(T *prev, T new_value) {
            *prev = _value.exchange(new_value, std::memory_order_relaxed);
        }

        // [Unique]
        inline bool compare_exchange_weak(T &expected, T new_value) {
            return _value.compare_exchange_weak(expected, new_value,
                                                std::memory_order_relaxed);
        }

        template<typename Op, typename T1>
        void modify(const Op &op, const T1 &value2) {
            T old_value = _value.load(std::memory_order_relaxed);
            T new_value = old_value;
            call_op_returning_void(op, new_value, value2);
            // There's a contention with the reset operation of combiner,
            // if the tls value has been modified during _op, the
            // compare_exchange_weak operation will fail and recalculation is
            // to be processed according to the new version of value
            while (!_value.compare_exchange_weak(
                    old_value, new_value, std::memory_order_relaxed)) {
                new_value = old_value;
                call_op_returning_void(op, new_value, value2);
            }
        }

    private:
        std::atomic<T> _value;
    };

    template<typename T>
    struct add_cr_non_integral {
        typedef typename std::conditional<std::is_integral<T>::value, T,
                typename std::add_lvalue_reference_t<typename std::add_const_t<T>>>::type type;
    };

    template<typename ResultTp, typename ElementTp, typename Combine, typename Setter>
    class AgentCombiner {
    public:
        typedef ResultTp result_type;
        typedef ElementTp element_type;
        typedef AgentCombiner<ResultTp, ElementTp, Combine, Setter> self_type;

        friend class GlobalValue<self_type>;

        struct Agent : public turbo::intrusive_list_node {
            Agent() : combiner(nullptr) {}

            ~Agent() {
                if (combiner) {
                    combiner->commit_and_erase(this);
                    combiner = nullptr;
                }
            }

            void reset(const ElementTp &val, self_type *c) {
                combiner = c;
                element.store(val);
            }

            template<typename Op>
            void merge_global(const Op &op) {
                GlobalValue<self_type> g(this, combiner);
                element.merge_global(op, g);
            }

            self_type *combiner;
            ElementContainer<ElementTp> element;
        };

        typedef turbo::concurrent_internal::ThreadLocalStorage<Agent> AgentGroup;

        explicit AgentCombiner(const ResultTp result_identity = ResultTp(),
                               const ElementTp element_identity = ElementTp(),
                               const Combine &cop = Combine(), const Setter &sop = Setter())
                : _id(AgentGroup::create_new_resource_id()), _cop(cop), _sop(sop), _global_result(result_identity),
                  _result_identity(result_identity), _element_identity(element_identity) {
        }

        ~AgentCombiner() {
            if (_id >= 0) {
                clear_all_agents();
                AgentGroup::release_resource_id(_id);
                _id = -1;
            }
        }

        // [Threadsafe] May be called from anywhere

        ResultTp combine_agents() const {
            ElementTp tls_value;
            std::unique_lock guard(_lock);
            ResultTp ret = _global_result;
            for (auto it = _agents.begin();
                 it != _agents.end(); it++) {
                it->element.load(&tls_value);
                call_op_returning_void(_cop, ret, tls_value);
            }
            return ret;
        }


        typename add_cr_non_integral<ElementTp>::type element_identity() const { return _element_identity; }

        typename add_cr_non_integral<ResultTp>::type result_identity() const { return _result_identity; }

        // [Threadsafe] May be called from anywhere.
        ResultTp reset_all_agents() {
            ElementTp prev;
            std::unique_lock guard(_lock);
            ResultTp tmp = _global_result;
            _global_result = _result_identity;
            for (auto node = _agents.begin();
                 node != _agents.end(); ++node) {
                node->element.exchange(&prev, _element_identity);
                call_op_returning_void(_sop, tmp, prev);
            }
            return tmp;
        }

        // Always called from the thread owning the agent.
        void commit_and_erase(Agent *agent) {
            if (nullptr == agent) {
                return;
            }
            ElementTp local;
            std::unique_lock guard(_lock);
            // TODO: For non-atomic types, we can pass the reference to op directly.
            // But atomic types cannot. The code is a little troublesome to write.
            agent->element.load(&local);
            call_op_returning_void(_sop, _global_result, local);
            _agents.remove(*agent);
        }

        // Always called from the thread owning the agent
        void commit_and_clear(Agent *agent) {
            if (nullptr == agent) {
                return;
            }
            ElementTp prev;
            std::unique_lock guard(_lock);
            agent->element.exchange(&prev, _element_identity);
            call_op_returning_void(_sop, _global_result, prev);
        }

        // We need this function to be as fast as possible.
        inline Agent *get_or_create_tls_agent() {
            Agent *agent = AgentGroup::get_resource(_id);
            if (!agent) {
                // Create the agent
                agent = AgentGroup::get_or_create_resource(_id);
                if (nullptr == agent) {
                    TLOG_CRITICAL("Fail to create agent");
                    return nullptr;
                }
            }
            if (agent->combiner) {
                return agent;
            }
            agent->reset(_element_identity, this);
            {
                std::unique_lock guard(_lock);
                _agents.push_back(*agent);
            }
            return agent;
        }

        void clear_all_agents() {
            std::unique_lock guard(_lock);
            // reseting agents is must because the agent object may be reused.
            // Set element to be default-constructed so that if it's non-pod,
            // internal allocations should be released.
            auto node = _agents.begin();
            while (node != _agents.end()) {
                node->reset(ElementTp(), nullptr);
                node = _agents.erase(node);
            }
        }

        const Combine &combine_op() const { return _cop; }

        const Setter &setter_op() const { return _sop; }

        bool valid() const { return _id >= 0; }

        Combine &combine_op() { return _cop; }

    private:
        typename AgentGroup::resource_id_type _id;
        Combine _cop;
        Setter _sop;
        mutable std::mutex _lock;
        ResultTp _global_result;
        ResultTp _result_identity;
        ElementTp _element_identity;
        intrusive_list<Agent> _agents;
    };
}  // namespace turbo::profiling_internal

#endif  // TURBO_PROFILING_INTERNAL_COMBINER_H_
