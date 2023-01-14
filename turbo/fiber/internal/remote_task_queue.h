

#ifndef TURBO_FIBER_INTERNAL_REMOTE_TASK_QUEUE_H_
#define TURBO_FIBER_INTERNAL_REMOTE_TASK_QUEUE_H_

#include "turbo/container/bounded_queue.h"
#include "turbo/base/profile.h"

namespace turbo::fiber_internal {

    class fiber_worker;

    // A queue for storing fibers created by non-workers. Since non-workers
    // randomly choose a fiber_worker to push which distributes the contentions,
    // this queue is simply implemented as a queue protected with a lock.
    // The function names should be self-explanatory.
    class RemoteTaskQueue {
    public:
        RemoteTaskQueue() {}

        int init(size_t cap) {
            const size_t memsize = sizeof(fiber_id_t) * cap;
            void *q_mem = malloc(memsize);
            if (q_mem == nullptr) {
                return -1;
            }
            turbo::container::bounded_queue<fiber_id_t> q(q_mem, memsize, turbo::container::OWNS_STORAGE);
            _tasks.swap(q);
            return 0;
        }

        bool pop(fiber_id_t *task) {
            if (_tasks.empty()) {
                return false;
            }
            _mutex.lock();
            const bool result = _tasks.pop(task);
            _mutex.unlock();
            return result;
        }

        bool push(fiber_id_t task) {
            _mutex.lock();
            const bool res = push_locked(task);
            _mutex.unlock();
            return res;
        }

        bool push_locked(fiber_id_t task) {
            return _tasks.push(task);
        }

        size_t capacity() const { return _tasks.capacity(); }

    private:

        friend class fiber_worker;
        TURBO_DISALLOW_COPY_AND_ASSIGN(RemoteTaskQueue);

        turbo::container::bounded_queue<fiber_id_t> _tasks;
        std::mutex _mutex;
    };

}  // namespace turbo::fiber_internal

#endif  // TURBO_FIBER_INTERNAL_REMOTE_TASK_QUEUE_H_
