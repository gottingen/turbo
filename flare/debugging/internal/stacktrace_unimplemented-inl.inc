
/****************************************************************
 * Copyright (c) 2022, liyinbin
 * All rights reserved.
 * Author by liyinbin (jeff.li) lijippy@163.com
 *****************************************************************/
#ifndef FLARE_DEBUGGING_INTERNAL_STACKTRACE_UNIMPLEMENTED_INL_H_
#define FLARE_DEBUGGING_INTERNAL_STACKTRACE_UNIMPLEMENTED_INL_H_

template<bool IS_STACK_FRAMES, bool IS_WITH_CONTEXT>
static int UnwindImpl(void ** /* result */, int * /* sizes */,
                      int /* max_depth */, int /* skip_count */,
                      const void * /* ucp */, int *min_dropped_frames) {
    if (min_dropped_frames != nullptr) {
        *min_dropped_frames = 0;
    }
    return 0;
}

namespace flare::debugging {

namespace debugging_internal {
bool stack_trace_works_for_test() {
    return false;
}
}  // namespace debugging_internal

}  // namespace flare::debugging

#endif  // FLARE_DEBUGGING_INTERNAL_STACKTRACE_UNIMPLEMENTED_INL_H_
