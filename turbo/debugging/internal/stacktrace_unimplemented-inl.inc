#ifndef TURBO_DEBUGGING_INTERNAL_STACKTRACE_UNIMPLEMENTED_INL_H_
#define TURBO_DEBUGGING_INTERNAL_STACKTRACE_UNIMPLEMENTED_INL_H_

template <bool IS_STACK_FRAMES, bool IS_WITH_CONTEXT>
static int UnwindImpl(void** /* result */, int* /* sizes */,
                      int /* max_depth */, int /* skip_count */,
                      const void* /* ucp */, int *min_dropped_frames) {
  if (min_dropped_frames != nullptr) {
    *min_dropped_frames = 0;
  }
  return 0;
}

namespace turbo {
TURBO_NAMESPACE_BEGIN
namespace debugging_internal {
bool StackTraceWorksForTest() {
  return false;
}
}  // namespace debugging_internal
TURBO_NAMESPACE_END
}  // namespace turbo

#endif  // TURBO_DEBUGGING_INTERNAL_STACKTRACE_UNIMPLEMENTED_INL_H_
