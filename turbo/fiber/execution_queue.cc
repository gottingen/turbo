// Licensed to the Apache Software Foundation (ASF) under one
// or more contributor license agreements.  See the NOTICE file
// distributed with this work for additional information
// regarding copyright ownership.  The ASF licenses this file
// to you under the Apache License, Version 2.0 (the
// "License"); you may not use this file except in compliance
// with the License.  You may obtain a copy of the License at
//
//   http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing,
// software distributed under the License is distributed on an
// "AS IS" BASIS, WITHOUT WARRANTIES OR CONDITIONS OF ANY
// KIND, either express or implied.  See the License for the
// specific language governing permissions and limitations
// under the License.

// fiber - A M:N threading library to make applications more concurrent.

// Date: 2016/04/16 18:43:24

#include "turbo/fiber/execution_queue.h"
#include "turbo/memory/object_pool.h"           // turbo::get_object
#include "turbo/memory/resource_pool.h"         // turbo::get_resource

namespace turbo {


    static_assert(sizeof(ExecutionQueue<int>) == sizeof(ExecutionQueueBase),
                  "sizeof_ExecutionQueue_must_be_the_same_with_ExecutionQueueBase");
    static_assert(sizeof(TaskIterator<int>) == sizeof(TaskIteratorBase),
                  "sizeof_TaskIterator_must_be_the_same_with_TaskIteratorBase");
    namespace /*anonymous*/ {
        typedef turbo::ResourceId<ExecutionQueueBase> slot_id_t;

        [[nodiscard]] inline slot_id_t slot_of_id(uint64_t id) {
            slot_id_t slot = {(id & 0xFFFFFFFFul)};
            return slot;
        }

        inline uint64_t make_id(uint32_t version, slot_id_t slot) {
            return (((uint64_t) version) << 32) | slot.value;
        }
    }  // namespace anonymous

    void ExecutionQueueBase::start_execute(TaskNode *node) {
        node->next = TaskNode::UNCONNECTED;
        node->status = UNEXECUTED;
        node->iterated = false;
        if (node->high_priority) {
            // Add _high_priority_tasks before pushing this task into queue to
            // make sure that _execute_tasks sees the newest number when this
            // task is in the queue. Althouth there might be some useless for
            // loops in _execute_tasks if this thread is scheduled out at this
            // point, we think it's just fine.
            _high_priority_tasks.fetch_add(1, std::memory_order_relaxed);
        }
        TaskNode *const prev_head = _head.exchange(node, std::memory_order_release);
        if (prev_head != nullptr) {
            node->next = prev_head;
            return;
        }
        // Get the right to execute the task, start a fiber to avoid deadlock
        // or stack overflow
        node->next = nullptr;
        node->q = this;
        if (node->in_place) {
            int niterated = 0;
            TURBO_UNUSED(_execute(node, node->high_priority, &niterated));
            TaskNode *tmp = node;
            // return if no more
            if (node->high_priority) {
                _high_priority_tasks.fetch_sub(niterated, std::memory_order_relaxed);
            }
            if (!_more_tasks(tmp, &tmp, !node->iterated)) {
                return_task_node(node);
                return;
            }
        }

        if (nullptr == _options.executor) {
            fiber_id_t tid;
            // We start the execution thread in background instead of foreground as
            // we can't determine whether the code after execute() is urgent (like
            // unlock a std::mutex) in which case implicit context switch may
            // cause undefined behavior (e.g. deadlock)
            if (!fiber_start_background(&tid, &_options.fiber_attr,_execute_tasks, node).ok()) {
                TDLOG_CRITICAL("Fail to start fiber");
                _execute_tasks(node);
            }
        } else {
            if (_options.executor->submit(_execute_tasks, node) != 0) {
                TDLOG_CRITICAL("Fail to submit task");
                _execute_tasks(node);
            }
        }
    }

    void *ExecutionQueueBase::_execute_tasks(void *arg) {
        TaskNode *head = (TaskNode *) arg;
        ExecutionQueueBase *m = (ExecutionQueueBase *) head->q;
        TaskNode *cur_tail = nullptr;
        bool destroy_queue = false;
        for (;;) {
            if (head->iterated) {
                TDLOG_CHECK(head->next != nullptr);
                TaskNode *saved_head = head;
                head = head->next;
                m->return_task_node(saved_head);
            }
            turbo::Status rc;
            if (m->_high_priority_tasks.load(std::memory_order_relaxed) > 0) {
                int nexecuted = 0;
                // Don't care the return value
                rc = m->_execute(head, true, &nexecuted);
                m->_high_priority_tasks.fetch_sub(
                        nexecuted, std::memory_order_relaxed);
                if (nexecuted == 0) {
                    // Some high_priority tasks are not in queue
                    sched_yield();
                }
            } else {
                rc = m->_execute(head, false, nullptr);
            }
            if (rc.code() == kESTOP) {
                destroy_queue = true;
            }
            // Release TaskNode until uniterated task or last task
            while (head->next != nullptr && head->iterated) {
                TaskNode *saved_head = head;
                head = head->next;
                m->return_task_node(saved_head);
            }
            if (cur_tail == nullptr) {
                for (cur_tail = head; cur_tail->next != nullptr;
                     cur_tail = cur_tail->next) {}
            }
            // break when no more tasks and head has been executed
            if (!m->_more_tasks(cur_tail, &cur_tail, !head->iterated)) {
                TDLOG_CHECK_EQ(cur_tail, head);
                TDLOG_CHECK(head->iterated);
                m->return_task_node(head);
                break;
            }
        }
        if (destroy_queue) {
            TDLOG_CHECK(m->_head.load(std::memory_order_relaxed) == nullptr);
            TDLOG_CHECK(m->_stopped);
            // Add _join_futex by 2 to make it equal to the next version of the
            // ExecutionQueue from the same slot so that join with old id would
            // return immediatly.
            //
            // 1: release fence to make join sees the newst changes when it sees
            //    the newst _join_futex
            m->_join_futex->fetch_add(2, std::memory_order_release/*1*/);
            fiber_internal::waitable_event_wake_all(m->_join_futex);
            turbo::return_resource(slot_of_id(m->_this_id));
        }
        return nullptr;
    }

    void ExecutionQueueBase::return_task_node(TaskNode *node) {
        node->clear_before_return(_clear_func);
        turbo::return_object<TaskNode>(node);
    }

    void ExecutionQueueBase::_on_recycle() {
        // Push a closed tasks
        while (true) {
            TaskNode *node = turbo::get_object<TaskNode>();
            if (TURBO_LIKELY(node != nullptr)) {
                node->stop_task = true;
                node->high_priority = false;
                node->in_place = false;
                start_execute(node);
                break;
            }
            TDLOG_CHECK(false, "Fail to create task_node_t, {}", terror(errno));
            TURBO_UNUSED(turbo::fiber_sleep_for(turbo::Duration::milliseconds(1)));
        }
    }

    turbo::Status ExecutionQueueBase::join(uint64_t id) {
        const slot_id_t slot = slot_of_id(id);
        ExecutionQueueBase *const m = turbo::address_resource(slot);
        if (m == nullptr) {
            // The queue is not created yet, this join is definitely wrong.
            return turbo::make_status(kEINVAL);
        }
        int expected = _version_of_id(id);
        // acquire fence makes this thread see changes before changing _join_futex.
        while (expected == m->_join_futex->load(std::memory_order_acquire)) {
            TLOG_WARN("Waiting for queue {} to be recycled {}", id, expected);
            auto rs = turbo::fiber_internal::waitable_event_wait(m->_join_futex, expected);
            if (!rs.ok() && rs.code()!= EWOULDBLOCK && rs.code() != EINTR) {
                return rs;
            }
        }
        return turbo::ok_status();
    }

    int ExecutionQueueBase::stop() {
        const uint32_t id_ver = _version_of_id(_this_id);
        uint64_t vref = _versioned_ref.load(std::memory_order_relaxed);
        int i = 0;
        for (;;) {
            TLOG_WARN("Stopping queue {} {}", _this_id, i++);
            if (_version_of_vref(vref) != id_ver) {
                return EINVAL;
            }
            // Try to set version=id_ver+1 (to make later address() return nullptr),
            // retry on fail.
            if (_versioned_ref.compare_exchange_strong(
                    vref, _make_vref(id_ver + 1, _ref_of_vref(vref)),
                    std::memory_order_release,
                    std::memory_order_relaxed)) {
                // Set _stopped to make lattern execute() fail immediately
                _stopped.store(true, std::memory_order_release);
                // Deref additionally which is added at creation so that this
                // queue's reference will hit 0(recycle) when no one addresses it.
                _release_additional_reference();
                // NOTE: This queue may be recycled at this point, don't
                // touch anything.
                return 0;
            }
        }
    }

    turbo::Status ExecutionQueueBase::_execute(TaskNode *head, bool high_priority, int *niterated) {
        if (head != nullptr && head->stop_task) {
            TDLOG_CHECK(head->next == nullptr);
            head->iterated = true;
            head->status = EXECUTED;
            TaskIteratorBase iter(nullptr, this, true, false);
            _execute_func(_meta, _type_specific_function, iter);
            if (niterated) {
                *niterated = 1;
            }
            return make_status(kESTOP);
        }
        TaskIteratorBase iter(head, this, false, high_priority);
        if (iter) {
            _execute_func(_meta, _type_specific_function, iter);
        }
        // We must assign |niterated| with num_iterated even if we couldn't peek
        // any task to execute at the begining, in which case all the iterated
        // tasks have been cancelled at this point. And we must return the
        // correct num_iterated() to the caller to update the counter correctly.
        if (niterated) {
            *niterated = iter.num_iterated();
        }
        return turbo::ok_status();
    }

    TaskNode *ExecutionQueueBase::allocate_node() {
        return turbo::get_object<TaskNode>();
    }

    TaskNode *const TaskNode::UNCONNECTED = (TaskNode *) -1L;

    ExecutionQueueBase::unique_ptr_t ExecutionQueueBase::address(uint64_t id) {
        unique_ptr_t ret;
        const slot_id_t slot = slot_of_id(id);
        ExecutionQueueBase *const m = turbo::address_resource(slot);
        if (TURBO_LIKELY(m != nullptr)) {
            // acquire fence makes sure this thread sees latest changes before
            // _dereference()
            const uint64_t vref1 = m->_versioned_ref.fetch_add(
                    1, std::memory_order_acquire);
            const uint32_t ver1 = _version_of_vref(vref1);
            if (ver1 == _version_of_id(id)) {
                ret.reset(m);
                return ret;
            }

            const uint64_t vref2 = m->_versioned_ref.fetch_sub(
                    1, std::memory_order_release);
            const int32_t nref = _ref_of_vref(vref2);
            if (nref > 1) {
                return ret;
            } else if (TURBO_LIKELY(nref == 1)) {
                const uint32_t ver2 = _version_of_vref(vref2);
                if ((ver2 & 1)) {
                    if (ver1 == ver2 || ver1 + 1 == ver2) {
                        uint64_t expected_vref = vref2 - 1;
                        if (m->_versioned_ref.compare_exchange_strong(
                                expected_vref, _make_vref(ver2 + 1, 0),
                                std::memory_order_acquire,
                                std::memory_order_relaxed)) {
                            m->_on_recycle();
                            // We don't return m immediatly when the reference count
                            // reaches 0 as there might be in processing tasks. Instead
                            // _on_recycle would push a `stop_task', after which
                            // is excuted m would be finally reset and returned
                        }
                    } else {
                        TDLOG_CHECK(false, "ref-version={} unref-version={}", ver1, ver2);
                    }
                } else {
                    TDLOG_CHECK_EQ(ver1, ver2);
                    // Addressed a free slot.
                }
            } else {
                TDLOG_CHECK(false, "Over dereferenced id={}", id);
            }
        }
        return ret;
    }

    int ExecutionQueueBase::create(uint64_t *id, const ExecutionQueueOptions *options,
                                   execute_func_t execute_func,
                                   clear_task_mem clear_func,
                                   void *meta, void *type_specific_function) {
        if (execute_func == nullptr || clear_func == nullptr) {
            return EINVAL;
        }

        slot_id_t slot;
        ExecutionQueueBase *const m = turbo::get_resource(&slot, Forbidden());
        if (TURBO_LIKELY(m != nullptr)) {
            m->_execute_func = execute_func;
            m->_clear_func = clear_func;
            m->_meta = meta;
            m->_type_specific_function = type_specific_function;
            TDLOG_CHECK(m->_head.load(std::memory_order_relaxed) == nullptr);
            TDLOG_CHECK_EQ(0, m->_high_priority_tasks.load(std::memory_order_relaxed));
            ExecutionQueueOptions opt;
            if (options != nullptr) {
                opt = *options;
            }
            m->_options = opt;
            m->_stopped.store(false, std::memory_order_relaxed);
            m->_this_id = make_id(
                    _version_of_vref(m->_versioned_ref.fetch_add(
                            1, std::memory_order_release)), slot);
            *id = m->_this_id;
            return 0;
        }
        return ENOMEM;
    }

    inline bool TaskIteratorBase::should_break_for_high_priority_tasks() {
        if (!_high_priority &&
            _q->_high_priority_tasks.load(std::memory_order_relaxed) > 0) {
            _should_break = true;
            return true;
        }
        return false;
    }

    void TaskIteratorBase::operator++() {
        if (!(*this)) {
            return;
        }
        if (_cur_node->iterated) {
            _cur_node = _cur_node->next;
        }
        if (should_break_for_high_priority_tasks()) {
            return;
        }  // else the next high_priority_task would be delayed for at most one task

        while (_cur_node && !_cur_node->stop_task) {
            if (_high_priority == _cur_node->high_priority) {
                if (!_cur_node->iterated && _cur_node->peek_to_execute()) {
                    ++_num_iterated;
                    _cur_node->iterated = true;
                    return;
                }
                _num_iterated += !_cur_node->iterated;
                _cur_node->iterated = true;
            }
            _cur_node = _cur_node->next;
        }
        return;
    }

    TaskIteratorBase::~TaskIteratorBase() {
        // Set the iterated tasks as EXECUTED here instead of waiting them to be
        // returned in _start_execute as the high_priority_task might be in the
        // middle of the linked list and is not going to be returned soon
        if (_is_stopped) {
            return;
        }
        while (_head != _cur_node) {
            if (_head->iterated && _head->high_priority == _high_priority) {
                _head->set_executed();
            }
            _head = _head->next;
        }
        if (_should_break && _cur_node != nullptr
            && _cur_node->high_priority == _high_priority && _cur_node->iterated) {
            _cur_node->set_executed();
        }
    }

} // namespace turbo
