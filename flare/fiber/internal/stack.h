
#ifndef FLARE_FIBER_INTERNAL_STACK_H_
#define FLARE_FIBER_INTERNAL_STACK_H_

#include <assert.h>
#include <gflags/gflags.h>          // DECLARE_int32
#include "flare/fiber/internal/types.h"
#include "flare/fiber/internal/context.h"        // fiber_context_type
#include "flare/memory/object_pool.h"

namespace flare::fiber_internal {

    struct fiber_stack_storage {
        int stacksize;
        int guardsize;
        // Assume stack grows upwards.
        // http://www.boost.org/doc/libs/1_55_0/libs/context/doc/html/context/stack.html
        void *bottom;
        unsigned valgrind_stack_id;

        // Clears all members.
        void zeroize() {
            stacksize = 0;
            guardsize = 0;
            bottom = nullptr;
            valgrind_stack_id = 0;
        }
    };

    // Allocate a piece of stack.
    int allocate_stack_storage(fiber_stack_storage *s, int stacksize, int guardsize);

    // Deallocate a piece of stack. Parameters MUST be returned or set by the
    // corresponding allocate_stack_storage() otherwise behavior is undefined.
    void deallocate_stack_storage(fiber_stack_storage *s);

    enum fiber_stack_type {
        STACK_TYPE_MAIN = 0,
        STACK_TYPE_PTHREAD = FIBER_STACKTYPE_PTHREAD,
        STACK_TYPE_SMALL = FIBER_STACKTYPE_SMALL,
        STACK_TYPE_NORMAL = FIBER_STACKTYPE_NORMAL,
        STACK_TYPE_LARGE = FIBER_STACKTYPE_LARGE
    };

    struct fiber_contextual_stack {
        fiber_context_type context;
        fiber_stack_type stacktype;
        fiber_stack_storage storage;
    };

    // Get a stack in the `type' and run `entry' at the first time that the
    // stack is jumped.
    fiber_contextual_stack *get_stack(fiber_stack_type type, void (*entry)(intptr_t));

    // Recycle a stack. nullptr does nothing.
    void return_stack(fiber_contextual_stack *);

    // Jump from stack `from' to stack `to'. `from' must be the stack of callsite
    // (to save contexts before jumping)
    void jump_stack(fiber_contextual_stack *from, fiber_contextual_stack *to);

}  // namespace flare::fiber_internal

#include "flare/fiber/internal/stack_inl.h"

#endif  // FLARE_FIBER_INTERNAL_STACK_H_
