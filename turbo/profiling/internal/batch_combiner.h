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


#ifndef TURBO_PROFILING_INTERNAL_BATCH_COMBINER_H_
#define TURBO_PROFILING_INTERNAL_BATCH_COMBINER_H_

#include "turbo/container/intrusive_list.h"
#include "turbo/concurrent/internal/thread_local_storage.h"
#include "turbo/profiling/batch.h"
#include "turbo/profiling/internal/combiner.h"
#include <atomic>

namespace turbo::profiling_internal {

    template<typename T, size_t N, typename Enabler = void>
    class BatchElementContainer {
        template<typename> friend
        class GlobalValue;

    public:
        void load(turbo::Batch<T,N> *out) const {
            std::unique_lock guard(_lock);
            *out = _value;
        }
        void load(T *out, size_t i) const {
            TURBO_ASSERT(i < N);
            std::unique_lock guard(_lock);
            *out = _value[i];
        }

        void store(const turbo::Batch<T,N> &new_value) {
            std::unique_lock guard(_lock);
            _value = new_value;
        }

        void store(const T &new_value, size_t i) {
            TURBO_ASSERT(i < N);
            std::unique_lock guard(_lock);
            _value[i] = new_value;
        }

        void exchange(turbo::Batch<T,N> *prev, const turbo::Batch<T,N> &new_value) {
            std::unique_lock guard(_lock);
            *prev = _value;
            _value = new_value;
        }

        void exchange(T *prev, const T &new_value, size_t i) {
            TURBO_ASSERT(i < N);
            std::unique_lock guard(_lock);
            *prev = _value[i];
            _value[i] = new_value;
        }

        template<typename Op, typename T1>
        void modify(const Op &op, const T1 &value2, size_t i) {
            TURBO_ASSERT(i < N);
            std::unique_lock guard(_lock);
            call_op_returning_void(op, _value[i], value2);
        }

        // [Unique]
        template<typename Op, typename GlobalValue>
        void merge_global(const Op &op, GlobalValue &global_value) {
            _lock.lock();
            op(global_value, _value);
            _lock.unlock();
        }

    private:
        turbo::Batch<T,N> _value;
        std::mutex _lock;
    };

    template<typename T, size_t N>
    class BatchElementContainer<T, N, std::enable_if_t<is_atomical<T>::value>> {
    public:
        // We don't need any memory fencing here, every op is relaxed.

        inline void load(T *out, size_t i) const {
            TURBO_ASSERT(i < N);
            *out = _value[i].load(std::memory_order_relaxed);
        }

        inline void load(turbo::Batch<T, N> *out) const {
            for (size_t i = 0; i < N; ++i) {
                (*out)[i] = _value[i].load(std::memory_order_relaxed);
            }
        }

        inline void store(const turbo::Batch<T, N> &new_value) {
            for (size_t i = 0; i < N; ++i) {
                _value[i].store(new_value[i], std::memory_order_relaxed);
            }
        }

        inline void store(T new_value, size_t i) {
            _value[i].store(new_value, std::memory_order_relaxed);
        }

        inline void exchange(T *prev, T new_value) {
            *prev = _value.exchange(new_value, std::memory_order_relaxed);
        }

        inline void exchange(T *prev, T new_value, size_t i) {
            *prev = _value[i].exchange(new_value, std::memory_order_relaxed);
        }

        inline void exchange(turbo::Batch<T, N> *prev, const turbo::Batch<T, N> &new_value) {
            for (size_t i = 0; i < N; ++i) {
                (*prev)[i] = _value[i].exchange(new_value[i], std::memory_order_relaxed);
            }
        }

        // [Unique]
        inline bool compare_exchange_weak(T &expected, T new_value, size_t i) {
            return _value[i].compare_exchange_weak(expected, new_value,
                                                   std::memory_order_relaxed);
        }

        inline bool compare_exchange_weak(turbo::Batch<T, N> &expected, const turbo::Batch<T, N> &new_value) {
            bool ret = true;
            for (size_t i = 0; i < N; ++i) {
                ret = ret && _value[i].compare_exchange_weak(expected[i], new_value[i],
                                                              std::memory_order_relaxed);
            }
            return ret;
        }

        template<typename Op, typename T1>
        void modify(const Op &op, const turbo::Batch<T1, N> &value2) {
            turbo::Batch<T, N> old_value;
            load(&old_value);
            turbo::Batch<T, N> new_value = old_value;
            for (size_t i = 0; i < N; ++i) {
                call_op_returning_void(op, new_value[i], value2[i]);
            }
            while(!compare_exchange_weak(old_value, new_value)) {
                new_value = old_value;
                for (size_t i = 0; i < N; ++i) {
                    call_op_returning_void(op, new_value[i], value2[i]);
                }
            }
        }

        template<typename Op, typename T1>
        void modify(const Op &op, const T1 &value2, size_t i) {
            T old_value = _value[i].load(std::memory_order_relaxed);
            T new_value = old_value;
            call_op_returning_void(op, new_value, value2);
            // There's a contention with the reset operation of combiner,
            // if the tls value has been modified during _op, the
            // compare_exchange_weak operation will fail and recalculation is
            // to be processed according to the new version of value
            while (!_value[i].compare_exchange_weak(
                    old_value, new_value, std::memory_order_relaxed)) {
                new_value = old_value;
                call_op_returning_void(op, new_value, value2);
            }
        }

    private:
        std::array<std::atomic<T>, N> _value;
    };

    template<typename ResultTp, typename ElementTp, size_t N, typename Combine, typename Setter>
    class BatchAgentCombiner {
    public:
        typedef ResultTp result_type;
        typedef ElementTp element_type;
        typedef BatchAgentCombiner<ResultTp, ElementTp, N, Combine, Setter> self_type;

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
                for (size_t i = 0; i < N; ++i) {
                    element.store(val, i);
                }
            }

            template<typename Op>
            void merge_global(const Op &op) {
                GlobalValue<self_type> g(this, combiner);
                element.merge_global(op, g);
            }

            self_type *combiner;
            BatchElementContainer<ElementTp, N> element;
        };

        typedef turbo::concurrent_internal::ThreadLocalStorage<Agent> AgentGroup;

        explicit BatchAgentCombiner(const ResultTp result_identity = ResultTp(),
                               const ElementTp element_identity = ElementTp(),
                               const Combine &cop = Combine(), const Setter &sop = Setter())
                : _id(AgentGroup::create_new_resource_id()), _cop(cop), _sop(sop), _global_result(result_identity),
                  _result_identity(result_identity), _element_identity(element_identity) {
        }

        ~BatchAgentCombiner() {
            if (_id >= 0) {
                clear_all_agents();
                AgentGroup::release_resource_id(_id);
                _id = -1;
            }
        }

            turbo::Batch<ResultTp, N> combine_agents() const {
            turbo::Batch<ElementTp,N> tls_value;
            std::unique_lock guard(_lock);
            auto ret = _global_result;
            for (auto it = _agents.begin();
                 it != _agents.end(); it++) {
                it->element.load(&tls_value);
                for(size_t i = 0; i < N; ++i) {
                    call_op_returning_void(_cop, ret[i], tls_value[i]);
                }
            }
            return ret;
        }

        ResultTp combine_agents(size_t i) const {
            ResultTp tls_value;
            std::unique_lock guard(_lock);
            auto ret = _global_result[i];
            for (auto it = _agents.begin();
                 it != _agents.end(); it++) {
                it->element.load(&tls_value, i);
                call_op_returning_void(_cop, ret, tls_value);
            }
            return ret;
        }

        typename add_cr_non_integral<ElementTp>::type element_identity() const { return _element_identity; }

        typename add_cr_non_integral<ResultTp>::type result_identity() const { return _result_identity; }

        // [Threadsafe] May be called from anywhere.
        turbo::Batch<ResultTp,N> reset_all_agents() {
            turbo::Batch<ElementTp,N> prev;
            std::unique_lock guard(_lock);
            turbo::Batch<ResultTp,N> tmp = _global_result;
            turbo::Batch<ElementTp,N> tmp2 = _element_identity;
            _global_result = _result_identity;
            for (auto node = _agents.begin();
                 node != _agents.end(); ++node) {
                node->element.exchange(&prev, tmp2);
                for(size_t i = 0; i < N; ++i) {
                    call_op_returning_void(_cop, tmp[i], prev[i]);
                }
            }
            return tmp;
        }

        // Always called from the thread owning the agent.
        void commit_and_erase(Agent *agent) {
            if (nullptr == agent) {
                return;
            }
            turbo::Batch<ElementTp, N> local;
            std::unique_lock guard(_lock);
            // TODO: For non-atomic types, we can pass the reference to op directly.
            // But atomic types cannot. The code is a little troublesome to write.
            agent->element.load(&local);
            for (size_t i = 0; i < N; ++i) {
                call_op_returning_void(_cop, _global_result[i], local[i]);
            }
            _agents.remove(*agent);
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
        turbo::Batch<ResultTp,N> _global_result;
        ResultTp _result_identity;
        ElementTp _element_identity;
        intrusive_list<Agent> _agents;
    };
}  // namespace turbo::profiling_internal

#endif  // TURBO_PROFILING_INTERNAL_BATCH_COMBINER_H_
