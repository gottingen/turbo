
/****************************************************************
 * Copyright (c) 2022, liyinbin
 * All rights reserved.
 * Author by liyinbin (jeff.li) lijippy@163.com
 *****************************************************************///
// -----------------------------------------------------------------------------
// File: stacktrace.h
// -----------------------------------------------------------------------------
//
// This file contains routines to extract the current stack trace and associated
// stack frames. These functions are thread-safe and async-signal-safe.
//
// Note that stack trace functionality is platform dependent and requires
// additional support from the compiler/build system in most cases. (That is,
// this functionality generally only works on platforms/builds that have been
// specifically configured to support it.)
//
// Note: stack traces in flare that do not utilize a symbolizer will result in
// frames consisting of function addresses rather than human-readable function
// names. (See symbolize.h for information on symbolizing these values.)

#ifndef FLARE_DEBUGGING_STACKTRACE_H_
#define FLARE_DEBUGGING_STACKTRACE_H_

#include "flare/base/profile.h"

namespace flare::debugging {


    // get_stack_frames()
    //
    // Records program counter values for up to `max_depth` frames, skipping the
    // most recent `skip_count` stack frames, stores their corresponding values
    // and sizes in `results` and `sizes` buffers, and returns the number of frames
    // stored. (Note that the frame generated for the `flare::debugging::get_stack_frames()`
    // routine itself is also skipped.)
    //
    // Example:
    //
    //      main() { foo(); }
    //      foo() { bar(); }
    //      bar() {
    //        void* result[10];
    //        int sizes[10];
    //        int depth = flare::debugging::get_stack_frames(result, sizes, 10, 1);
    //      }
    //
    // The current stack frame would consist of three function calls: `bar()`,
    // `foo()`, and then `main()`; however, since the `get_stack_frames()` call sets
    // `skip_count` to `1`, it will skip the frame for `bar()`, the most recently
    // invoked function call. It will therefore return 2 and fill `result` with
    // program counters within the following functions:
    //
    //      result[0]       foo()
    //      result[1]       main()
    //
    // (Note: in practice, a few more entries after `main()` may be added to account
    // for startup processes.)
    //
    // Corresponding stack frame sizes will also be recorded:
    //
    //    sizes[0]       16
    //    sizes[1]       16
    //
    // (Stack frame sizes of `16` above are just for illustration purposes.)
    //
    // Stack frame sizes of 0 or less indicate that those frame sizes couldn't
    // be identified.
    //
    // This routine may return fewer stack frame entries than are
    // available. Also note that `result` and `sizes` must both be non-null.
    extern int get_stack_frames(void **result, int *sizes, int max_depth,
                                int skip_count);

    // get_stack_frames_with_context()
    //
    // Records program counter values obtained from a signal handler. Records
    // program counter values for up to `max_depth` frames, skipping the most recent
    // `skip_count` stack frames, stores their corresponding values and sizes in
    // `results` and `sizes` buffers, and returns the number of frames stored. (Note
    // that the frame generated for the `flare::debugging::get_stack_frames_with_context()` routine
    // itself is also skipped.)
    //
    // The `uc` parameter, if non-null, should be a pointer to a `ucontext_t` value
    // passed to a signal handler registered via the `sa_sigaction` field of a
    // `sigaction` struct. (See
    // http://man7.org/linux/man-pages/man2/sigaction.2.html.) The `uc` value may
    // help a stack unwinder to provide a better stack trace under certain
    // conditions. `uc` may safely be null.
    //
    // The `min_dropped_frames` output parameter, if non-null, points to the
    // location to note any dropped stack frames, if any, due to buffer limitations
    // or other reasons. (This value will be set to `0` if no frames were dropped.)
    // The number of total stack frames is guaranteed to be >= skip_count +
    // max_depth + *min_dropped_frames.
    extern int get_stack_frames_with_context(void **result, int *sizes, int max_depth,
                                             int skip_count, const void *uc,
                                             int *min_dropped_frames);

    // get_stack_trace()
    //
    // Records program counter values for up to `max_depth` frames, skipping the
    // most recent `skip_count` stack frames, stores their corresponding values
    // in `results`, and returns the number of frames
    // stored. Note that this function is similar to `flare::debugging::get_stack_frames()`
    // except that it returns the stack trace only, and not stack frame sizes.
    //
    // Example:
    //
    //      main() { foo(); }
    //      foo() { bar(); }
    //      bar() {
    //        void* result[10];
    //        int depth = flare::debugging::get_stack_trace(result, 10, 1);
    //      }
    //
    // This produces:
    //
    //      result[0]       foo
    //      result[1]       main
    //           ....       ...
    //
    // `result` must not be null.
    extern int get_stack_trace(void **result, int max_depth, int skip_count);

    // get_stack_trace_with_context()
    //
    // Records program counter values obtained from a signal handler. Records
    // program counter values for up to `max_depth` frames, skipping the most recent
    // `skip_count` stack frames, stores their corresponding values in `results`,
    // and returns the number of frames stored. (Note that the frame generated for
    // the `flare::debugging::get_stack_frames_with_context()` routine itself is also skipped.)
    //
    // The `uc` parameter, if non-null, should be a pointer to a `ucontext_t` value
    // passed to a signal handler registered via the `sa_sigaction` field of a
    // `sigaction` struct. (See
    // http://man7.org/linux/man-pages/man2/sigaction.2.html.) The `uc` value may
    // help a stack unwinder to provide a better stack trace under certain
    // conditions. `uc` may safely be null.
    //
    // The `min_dropped_frames` output parameter, if non-null, points to the
    // location to note any dropped stack frames, if any, due to buffer limitations
    // or other reasons. (This value will be set to `0` if no frames were dropped.)
    // The number of total stack frames is guaranteed to be >= skip_count +
    // max_depth + *min_dropped_frames.
    extern int get_stack_trace_with_context(void **result, int max_depth,
                                            int skip_count, const void *uc,
                                            int *min_dropped_frames);

    // set_stack_unwinder()
    //
    // Provides a custom function for unwinding stack frames that will be used in
    // place of the default stack unwinder when invoking the static
    // GetStack{Frames,Trace}{,WithContext}() functions above.
    //
    // The arguments passed to the unwinder function will match the
    // arguments passed to `flare::debugging::get_stack_frames_with_context()` except that sizes
    // will be non-null iff the caller is interested in frame sizes.
    //
    // If unwinder is set to null, we revert to the default stack-tracing behavior.
    //
    // *****************************************************************************
    // WARNING
    // *****************************************************************************
    //
    // flare::debugging::set_stack_unwinder is not suitable for general purpose use.  It is
    // provided for custom runtimes.
    // Some things to watch out for when calling `flare::debugging::set_stack_unwinder()`:
    //
    // (a) The unwinder may be called from within signal handlers and
    // therefore must be async-signal-safe.
    //
    // (b) Even after a custom stack unwinder has been unregistered, other
    // threads may still be in the process of using that unwinder.
    // Therefore do not clean up any state that may be needed by an old
    // unwinder.
    // *****************************************************************************
    extern void set_stack_unwinder(int (*unwinder)(void **pcs, int *sizes,
                                                   int max_depth, int skip_count,
                                                   const void *uc,
                                                   int *min_dropped_frames));

    // default_stack_unwinder()
    //
    // Records program counter values of up to `max_depth` frames, skipping the most
    // recent `skip_count` stack frames, and stores their corresponding values in
    // `pcs`. (Note that the frame generated for this call itself is also skipped.)
    // This function acts as a generic stack-unwinder; prefer usage of the more
    // specific `GetStack{Trace,Frames}{,WithContext}()` functions above.
    //
    // If you have set your own stack unwinder (with the `set_stack_unwinder()`
    // function above, you can still get the default stack unwinder by calling
    // `default_stack_unwinder()`, which will ignore any previously set stack unwinder
    // and use the default one instead.
    //
    // Because this function is generic, only `pcs` is guaranteed to be non-null
    // upon return. It is legal for `sizes`, `uc`, and `min_dropped_frames` to all
    // be null when called.
    //
    // The semantics are the same as the corresponding `GetStack*()` function in the
    // case where `flare::debugging::set_stack_unwinder()` was never called. Equivalents are:
    //
    //                       null sizes         |        non-nullptr sizes
    //             |==========================================================|
    //     null uc | get_stack_trace()            | get_stack_frames()            |
    // non-null uc | get_stack_trace_with_context() | get_stack_frames_with_context() |
    //             |==========================================================|
    extern int default_stack_unwinder(void **pcs, int *sizes, int max_depth,
                                      int skip_count, const void *uc,
                                      int *min_dropped_frames);

    namespace debugging_internal {
        // Returns true for platforms which are expected to have functioning stack trace
        // implementations. Intended to be used for tests which want to exclude
        // verification of logic known to be broken because stack traces are not
        // working.
        extern bool stack_trace_works_for_test();
    }  // namespace debugging_internal

}  // namespace flare::debugging

#endif  // FLARE_DEBUGGING_STACKTRACE_H_
