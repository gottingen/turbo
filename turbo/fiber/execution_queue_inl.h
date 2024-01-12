

#ifndef  TURBO_FIBER_INTERNAL_EXECUTION_QUEUE_INL_H_
#define  TURBO_FIBER_INTERNAL_EXECUTION_QUEUE_INL_H_

#include <atomic>
#include <memory>
#include <mutex>
#include "turbo/platform/port.h"
#include "turbo/log/logging.h"
#include "turbo/times/time.h"
#include "turbo/fiber/internal/waitable_event.h"
#include "turbo/status/status.h"

namespace turbo {

    template<typename T>
    struct ExecutionQueueId {
        uint64_t value;
    };

    enum TaskStatus {
        UNEXECUTED = 0,
        EXECUTING = 1,
        EXECUTED = 2
    };

    struct TaskNode;

    class ExecutionQueueBase;

    typedef void (*clear_task_mem)(TaskNode *);

    struct TURBO_CACHE_LINE_ALIGNED TaskNode {
        TaskNode()
                : version(0), status(UNEXECUTED), stop_task(false), iterated(false), high_priority(false),
                  in_place(false), next(UNCONNECTED), q(nullptr) {}

        ~TaskNode() {}

        int cancel(int64_t expected_version) {
            std::unique_lock l(mutex);
            if (version != expected_version) {
                return -1;
            }
            if (status == UNEXECUTED) {
                status = EXECUTED;
                return 0;
            }
            return status == EXECUTED ? -1 : 1;
        }

        void set_executed() {
            std::unique_lock l(mutex);
            status = EXECUTED;
        }

        bool peek_to_execute() {
            std::unique_lock l(mutex);
            if (status == UNEXECUTED) {
                status = EXECUTING;
                return true;
            }
            return false;
        }

        std::mutex mutex;  // to guard version and status
        int64_t version;
        uint8_t status;
        bool stop_task;
        bool iterated;
        bool high_priority;
        bool in_place;
        TaskNode *next;
        ExecutionQueueBase *q;
        union {
            char static_task_mem[56];  // Make sizeof TaskNode exactly 128 bytes
            char *dynamic_task_mem;
        };

        void clear_before_return(clear_task_mem clear_func) {
            if (!stop_task) {
                clear_func(this);
                TDLOG_CHECK(iterated);
            }
            q = nullptr;
            std::unique_lock<std::mutex> lck(mutex);
            ++version;
            const int saved_status = status;
            status = UNEXECUTED;
            lck.unlock();
            TDLOG_CHECK_NE(saved_status, UNEXECUTED);
            TDLOG_WARN_IF(saved_status == EXECUTED, "Return a executed node, did you "
                                                  "return before iterator reached the end?");
        }

        static TaskNode *const UNCONNECTED;
    };

// Specialize TaskNodeAllocator for types with different sizes
    template<size_t size, bool small_object>
    struct TaskAllocatorBase {
    };

    template<size_t size>
    struct TaskAllocatorBase<size, true> {
        inline static void *allocate(TaskNode *node) { return node->static_task_mem; }

        inline static void *get_allocated_mem(TaskNode *node) { return node->static_task_mem; }

        inline static void deallocate(TaskNode *) {}
    };

    template<size_t size>
    struct TaskAllocatorBase<size, false> {
        inline static void *allocate(TaskNode *node) {
            node->dynamic_task_mem = (char *) malloc(size);
            return node->dynamic_task_mem;
        }

        inline static void *get_allocated_mem(TaskNode *node) { return node->dynamic_task_mem; }

        inline static void deallocate(TaskNode *node) {
            free(node->dynamic_task_mem);
        }
    };

    template<typename T>
    struct TaskAllocator : public TaskAllocatorBase<
            sizeof(T), sizeof(T) <= sizeof(TaskNode().static_task_mem)> {
    };

    class TaskIteratorBase;

    class TURBO_CACHE_LINE_ALIGNED ExecutionQueueBase {
        TURBO_NON_COPYABLE(ExecutionQueueBase);

        struct Forbidden {
        };

        friend class TaskIteratorBase;

        struct Dereferencer {
            void operator()(ExecutionQueueBase *queue) {
                if (queue != nullptr) {
                    queue->dereference();
                }
            }
        };

    public:
        // User cannot create ExecutionQueue fron construct
        ExecutionQueueBase(Forbidden)
                : _head(nullptr), _versioned_ref(0)  // join() depends on even version
                , _high_priority_tasks(0) {
            _join_futex = turbo::fiber_internal::waitable_event_create_checked<std::atomic<int> >();
            _join_futex->store(0, std::memory_order_relaxed);
        }

        ~ExecutionQueueBase() {
            turbo::fiber_internal::waitable_event_destroy(_join_futex);
        }

        bool stopped() const { return _stopped.load(std::memory_order_acquire); }

        int stop();

        static turbo::Status join(uint64_t id);

    protected:
        typedef int (*execute_func_t)(void *, void *, TaskIteratorBase &);

        typedef std::unique_ptr<ExecutionQueueBase, Dereferencer> unique_ptr_t;

        int dereference();

        static int create(uint64_t *id, const ExecutionQueueOptions *options,
                          execute_func_t execute_func,
                          clear_task_mem clear_func,
                          void *meta, void *type_specific_function);

        [[nodiscard]] static unique_ptr_t address(uint64_t id);

        void start_execute(TaskNode *node);

        TaskNode *allocate_node();

        void return_task_node(TaskNode *node);

    private:

        bool _more_tasks(TaskNode *old_head, TaskNode **new_tail,
                         bool has_uniterated);

        void _release_additional_reference() {
            dereference();
        }

        void _on_recycle();

        turbo::Status _execute(TaskNode *head, bool high_priority, int *niterated);

        static void *_execute_tasks(void *arg);

        [[nodiscard]] static inline uint32_t _version_of_id(uint64_t id) {
            return (uint32_t) (id >> 32);
        }

        [[nodiscard]] static inline uint32_t _version_of_vref(int64_t vref) {
            return (uint32_t) (vref >> 32);
        }

        [[nodiscard]] static inline uint32_t _ref_of_vref(int64_t vref) {
            return (int32_t) (vref & 0xFFFFFFFFul);
        }

        static inline int64_t _make_vref(uint32_t version, int32_t ref) {
            // 1: Intended conversion to uint32_t, nref=-1 is 00000000FFFFFFFF
            return (((uint64_t) version) << 32) | (uint32_t/*1*/) ref;
        }

        // Don't change the order of _head, _versioned_ref and _stopped unless you
        // see improvement of performance in test
        std::atomic<TaskNode *> TURBO_CACHE_LINE_ALIGNED _head;
        std::atomic<uint64_t> TURBO_CACHE_LINE_ALIGNED _versioned_ref;
        std::atomic<bool> TURBO_CACHE_LINE_ALIGNED _stopped;
        std::atomic<int64_t> _high_priority_tasks;
        uint64_t _this_id;
        void *_meta;
        void *_type_specific_function;
        execute_func_t _execute_func;
        clear_task_mem _clear_func;
        ExecutionQueueOptions _options;
        std::atomic<int> *_join_futex;
    };

    template<typename T>
    class ExecutionQueue : public ExecutionQueueBase {
        struct Forbidden {
        };

        friend class TaskIterator<T>;

        typedef ExecutionQueueBase Base;

        ExecutionQueue();

    public:
        typedef ExecutionQueue<T> self_type;

        struct Dereferencer {
            void operator()(self_type *queue) {
                if (queue != nullptr) {
                    queue->dereference();
                }
            }
        };

        typedef std::unique_ptr<self_type, Dereferencer> unique_ptr_t;
        typedef turbo::ExecutionQueueId<T> id_t;
        typedef TaskIterator<T> iterator;

        typedef int (*execute_func_t)(void *, iterator &);

        typedef TaskAllocator<T> allocator;
        static_assert(sizeof(execute_func_t) == sizeof(void *),
                      "sizeof function must be equal to sizeof voidptr");

        static void clear_task_mem(TaskNode *node) {
            T *const task = (T *) allocator::get_allocated_mem(node);
            task->~T();
            allocator::deallocate(node);
        }

        static int execute_task(void *meta, void *specific_function,
                                TaskIteratorBase &it) {
            execute_func_t f = (execute_func_t) specific_function;
            return f(meta, static_cast<iterator &>(it));
        }

        inline static int create(id_t *id, const ExecutionQueueOptions *options,
                                 execute_func_t execute_func, void *meta) {
            return Base::create(&id->value, options, execute_task,
                                clear_task_mem, meta, (void *) execute_func);
        }

        [[nodiscard]] inline static unique_ptr_t address(id_t id) {
            Base::unique_ptr_t ptr = Base::address(id.value);
            Base *b = ptr.release();
            unique_ptr_t ret((self_type *) b);
            return ret;
        }

        int execute(typename add_cref<T>::type task) {
            return execute(task, nullptr, nullptr);
        }

        int execute(typename add_cref<T>::type task,
                    const TaskOptions *options, TaskHandle *handle) {
            if (stopped()) {
                return EINVAL;
            }
            TaskNode *node = allocate_node();
            if (TURBO_UNLIKELY(node == nullptr)) {
                return ENOMEM;
            }
            void *const mem = allocator::allocate(node);
            if (TURBO_UNLIKELY(!mem)) {
                return_task_node(node);
                return ENOMEM;
            }
            new(mem) T(task);
            node->stop_task = false;
            TaskOptions opt;
            if (options) {
                opt = *options;
            }
            node->high_priority = opt.high_priority;
            node->in_place = opt.in_place_if_possible;
            if (handle) {
                handle->node = node;
                handle->version = node->version;
            }
            start_execute(node);
            return 0;
        }
    };

    inline ExecutionQueueOptions::ExecutionQueueOptions()
            : fiber_attr(FIBER_ATTR_NORMAL), executor(nullptr) {}

    template<typename T>
    inline int execution_queue_start(
            ExecutionQueueId<T> *id,
            const ExecutionQueueOptions *options,
            int (*execute)(void *meta, TaskIterator <T> &),
            void *meta) {
        return ExecutionQueue<T>::create(id, options, execute, meta);
    }

    template<typename T>
    typename ExecutionQueue<T>::unique_ptr_t
    execution_queue_address(ExecutionQueueId<T> id) {
        return ExecutionQueue<T>::address(id);
    }

    template<typename T>
    inline int execution_queue_execute(ExecutionQueueId<T> id,
                                       typename add_cref<T>::type task) {
        return execution_queue_execute(id, task, nullptr);
    }

    template<typename T>
    inline int execution_queue_execute(ExecutionQueueId<T> id,
                                       typename add_cref<T>::type task,
                                       const TaskOptions *options) {
        return execution_queue_execute(id, task, options, nullptr);
    }

    template<typename T>
    inline int execution_queue_execute(ExecutionQueueId<T> id,
                                       typename add_cref<T>::type task,
                                       const TaskOptions *options,
                                       TaskHandle *handle) {
        typename ExecutionQueue<T>::unique_ptr_t
                ptr = ExecutionQueue<T>::address(id);
        if (ptr != nullptr) {
            return ptr->execute(task, options, handle);
        } else {
            return EINVAL;
        }
    }

    template<typename T>
    inline int execution_queue_stop(ExecutionQueueId<T> id) {
        typename ExecutionQueue<T>::unique_ptr_t
                ptr = ExecutionQueue<T>::address(id);
        if (ptr != nullptr) {
            return ptr->stop();
        } else {
            return EINVAL;
        }
    }

    template<typename T>
    inline turbo::Status execution_queue_join(ExecutionQueueId<T> id) {
        return ExecutionQueue<T>::join(id.value);
    }

    inline TaskOptions::TaskOptions()
            : high_priority(false), in_place_if_possible(false) {}

    inline TaskOptions::TaskOptions(bool high_priority, bool in_place_if_possible)
            : high_priority(high_priority), in_place_if_possible(in_place_if_possible) {}

//--------------------- TaskIterator ------------------------

    inline TaskIteratorBase::operator bool() const {
        return !_is_stopped && !_should_break && _cur_node != nullptr
               && !_cur_node->stop_task;
    }

    template<typename T>
    inline typename TaskIterator<T>::reference
    TaskIterator<T>::operator*() const {
        T *const ptr = (T *const) TaskAllocator<T>::get_allocated_mem(cur_node());
        return *ptr;
    }

    template<typename T>
    TaskIterator <T> &TaskIterator<T>::operator++() {
        TaskIteratorBase::operator++();
        return *this;
    }

    template<typename T>
    void TaskIterator<T>::operator++(int) {
        operator++();
    }

    inline TaskHandle::TaskHandle()
            : node(nullptr), version(0) {}

    inline int execution_queue_cancel(const TaskHandle &h) {
        if (h.node == nullptr) {
            return -1;
        }
        return h.node->cancel(h.version);
    }

        // ---------------------ExecutionQueueBase--------------------
    inline bool ExecutionQueueBase::_more_tasks(
            TaskNode *old_head, TaskNode **new_tail,
            bool has_uniterated) {

        TDLOG_CHECK(old_head->next == nullptr);
        // Try to set _head to nullptr to mark that the execute is done.
        TaskNode *new_head = old_head;
        TaskNode *desired = nullptr;
        bool return_when_no_more = false;
        if (has_uniterated) {
            desired = old_head;
            return_when_no_more = true;
        }
        if (_head.compare_exchange_strong(
                new_head, desired, std::memory_order_acquire)) {
            // No one added new tasks.
            return return_when_no_more;
        }
        TDLOG_CHECK_NE(new_head, old_head);
        // Above acquire fence pairs release fence of exchange in Write() to make
        // sure that we see all fields of requests set.

        // Someone added new requests.
        // Reverse the list until old_head.
        TaskNode *tail = nullptr;
        if (new_tail) {
            *new_tail = new_head;
        }
        TaskNode *p = new_head;
        do {
            while (p->next == TaskNode::UNCONNECTED) {
                sched_yield();
            }
            TaskNode *const saved_next = p->next;
            p->next = tail;
            tail = p;
            p = saved_next;
            TDLOG_CHECK(p != nullptr);
        } while (p != old_head);

        // Link old list with new list.
        old_head->next = tail;
        return true;
    }

    inline int ExecutionQueueBase::dereference() {
        const uint64_t vref = _versioned_ref.fetch_sub(
                1, std::memory_order_release);
        const int32_t nref = _ref_of_vref(vref);
        // We need make the fast path as fast as possible, don't put any extra
        // code before this point
        if (nref > 1) {
            return 0;
        }
        const uint64_t id = _this_id;
        if (TURBO_LIKELY(nref == 1)) {
            const uint32_t ver = _version_of_vref(vref);
            const uint32_t id_ver = _version_of_id(id);
            // Besides first successful stop() adds 1 to version, one of
            // those dereferencing nref from 1->0 adds another 1 to version.
            // Notice "one of those": The wait-free address() may make ref of a
            // version-unmatched slot change from 1 to 0 for mutiple times, we
            // have to use version as a guard variable to prevent returning the
            // executor to pool more than once.
            if (TURBO_LIKELY(ver == id_ver || ver == id_ver + 1)) {
                // sees nref:1->0, try to set version=id_ver+2,--nref.
                // No retry: if version changes, the slot is already returned by
                // another one who sees nref:1->0 concurrently; if nref changes,
                // which must be non-zero, the slot will be returned when
                // nref changes from 1->0 again.
                // Example:
                //       stop():  --nref, sees nref:1->0           (1)
                //                try to set version=id_ver+2      (2)
                //    address():  ++nref, unmatched version        (3)
                //                --nref, sees nref:1->0           (4)
                //                try to set version=id_ver+2      (5)
                // 1,2,3,4,5 or 1,3,4,2,5:
                //            stop() succeeds, address() fails at (5).
                // 1,3,2,4,5: stop() fails with (2), the slot will be
                //            returned by (5) of address()
                // 1,3,4,5,2: stop() fails with (2), the slot is already
                //            returned by (5) of address().
                uint64_t expected_vref = vref - 1;
                if (_versioned_ref.compare_exchange_strong(
                        expected_vref, _make_vref(id_ver + 2, 0),
                        std::memory_order_acquire,
                        std::memory_order_relaxed)) {
                    _on_recycle();
                    // We don't return m immediatly when the reference count
                    // reaches 0 as there might be in processing tasks. Instead
                    // _on_recycle would push a `stop_task' after which is excuted
                    // m would be finally returned and reset
                    return 1;
                }
                return 0;
            }
            TDLOG_CRITICAL("Invalid id={}", id);
            return -1;
        }
        TDLOG_CRITICAL("Over dereferenced id={}", id);
        return -1;
    }

}  // namespace turbo

#endif  // TURBO_FIBER_INTERNAL_EXECUTION_QUEUE_INL_H_
