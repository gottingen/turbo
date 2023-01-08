
#ifndef FLARE_FIBER_FIBER_LATCH_H_
#define FLARE_FIBER_FIBER_LATCH_H_

#include "flare/fiber/internal/fiber.h"

namespace flare {

    // A synchronization primitive to wait for multiple signallers.
    class fiber_latch {
    public:
        fiber_latch(int initial_count = 1);

        ~fiber_latch();

        // Increase current counter by |v|
        void add_count(int v = 1);

        // Reset the counter to |v|
        void reset(int v = 1);

        // Decrease the counter by |sig|
        void signal(int sig = 1);

        // Block current thread until the counter reaches 0.
        // Returns 0 on success, error code otherwise.
        // This method never returns EINTR.
        int wait();

        // Block the current thread until the counter reaches 0 or duetime has expired
        // Returns 0 on success, error code otherwise. ETIMEDOUT is for timeout.
        // This method never returns EINTR.
        int timed_wait(const timespec *duetime);

    private:
        int *_event;
        bool _wait_was_invoked;
    };

}  // namespace flare

#endif  // FLARE_FIBER_FIBER_LATCH_H_
