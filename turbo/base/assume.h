//
// Created by liyinbin on 2023/1/18.
//

#ifndef TURBO_BASE_ASSUME_H_
#define TURBO_BASE_ASSUME_H_

#include "turbo/base/profile.h"
#include "turbo/log/logging.h"

namespace turbo::base {

    /**
     * assume*() functions can be used to fine-tune optimizations or suppress
     * warnings when certain conditions are provably true, but the compiler is not
     * able to prove them.
     *
     * This is different from assertions: an assertion will place an explicit check
     * that the condition is true, and abort the program if the condition is not
     * verified. Calling assume*() with a condition that is not true at runtime
     * is undefined behavior: for example, it may or may not result in a crash,
     * silently corrupt memory, or jump to a random code path.
     *
     * These functions should only be used on conditions that are provable internal
     * logic invariants; they cannot be used safely if the condition depends on
     * external inputs or data. To detect unexpected conditions that *can* happen,
     * an assertion or exception should be used.
     */

    /**
     * assume(cond) informs the compiler that cond can be assumed true. If cond is
     * not true at runtime the behavior is undefined.
     *
     * The typical use case is to allow the compiler exploit data structure
     * invariants that can trigger better optimizations, for example to eliminate
     * unnecessary bounds checks in a called function. It is recommended to check
     * the generated code or run microbenchmarks to assess whether it is actually
     * effective.
     *
     * The semantics are similar to clang's __builtin_assume(), but intentionally
     * implemented as a function to force the evaluation of its argument, contrary
     * to the builtin, which cannot used with expressions that have side-effects.
     */
    TURBO_FORCE_INLINE void assume(bool cond) {
        TURBO_CHECK(cond) << "compiler-hint assumption fails at runtime";
#if defined(__clang__)
        __builtin_assume(cond);
#elif defined(__GNUC__)
        if (!cond) {
    __builtin_unreachable();
  }
#elif defined(_MSC_VER)
  __assume(cond);
#else
  while (!cond)
    ;
#endif
    }

    /**
     * assume_unreachable() informs the compiler that the statement is not reachable
     * at runtime. It is undefined behavior if the statement is actually reached.
     *
     * Common use cases are to suppress a warning when the compiler cannot prove
     * that the end of a non-void function is not reachable, or to optimize the
     * evaluation of switch/case statements when all the possible values are
     * provably enumerated.
     */
    TURBO_FORCE_INLINE void assume_unreachable() {
        TURBO_CHECK(false) << "compiler-hint unreachability reached at runtime";
#if defined(__GNUC__)
        __builtin_unreachable();
#elif defined(_MSC_VER)
        __assume(0);
#else
  while (!0)
    ;
#endif
    }
}  // namespace turbo::base

#endif  // TURBO_BASE_ASSUME_H_
