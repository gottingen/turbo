// Copyright (C) 2024 EA group inc.
// Author: Jeff.li lijippy@163.com
// All rights reserved.
// This program is free software: you can redistribute it and/or modify
// it under the terms of the GNU Affero General Public License as published
// by the Free Software Foundation, either version 3 of the License, or
// (at your option) any later version.
//
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU Affero General Public License for more details.
//
// You should have received a copy of the GNU Affero General Public License
// along with this program.  If not, see <https://www.gnu.org/licenses/>.
//
//
// Portable implementation - just use glibc
//
// Note:  The glibc implementation may cause a call to malloc.
// This can cause a deadlock in HeapProfiler.

#ifndef TURBO_DEBUGGING_INTERNAL_STACKTRACE_EMSCRIPTEN_INL_H_
#define TURBO_DEBUGGING_INTERNAL_STACKTRACE_EMSCRIPTEN_INL_H_

#include <emscripten.h>

#include <atomic>
#include <cstring>

#include <turbo/base/attributes.h>
#include <turbo/debugging/stacktrace.h>

extern "C" {
uintptr_t emscripten_stack_snapshot();
uint32_t emscripten_stack_unwind_buffer(uintptr_t pc, void *buffer,
                                        uint32_t depth);
}

// Sometimes, we can try to get a stack trace from within a stack
// trace, which can cause a self-deadlock.
// Protect against such reentrant call by failing to get a stack trace.
//
// We use __thread here because the code here is extremely low level -- it is
// called while collecting stack traces from within malloc and mmap, and thus
// can not call anything which might call malloc or mmap itself.
static __thread int recursive = 0;

// The stack trace function might be invoked very early in the program's
// execution (e.g. from the very first malloc).
// As such, we suppress usage of backtrace during this early stage of execution.
static std::atomic<bool> disable_stacktraces(true);  // Disabled until healthy.
// Waiting until static initializers run seems to be late enough.
// This file is included into stacktrace.cc so this will only run once.
TURBO_ATTRIBUTE_UNUSED static int stacktraces_enabler = []() {
  // Check if we can even create stacktraces. If not, bail early and leave
  // disable_stacktraces set as-is.
  // clang-format off
  if (!EM_ASM_INT({ return (typeof wasmOffsetConverter !== 'undefined'); })) {
    return 0;
  }
  // clang-format on
  disable_stacktraces.store(false, std::memory_order_relaxed);
  return 0;
}();

template <bool IS_STACK_FRAMES, bool IS_WITH_CONTEXT>
static int UnwindImpl(void **result, int *sizes, int max_depth, int skip_count,
                      const void *ucp, int *min_dropped_frames) {
  if (recursive || disable_stacktraces.load(std::memory_order_relaxed)) {
    return 0;
  }
  ++recursive;

  static_cast<void>(ucp);  // Unused.
  constexpr int kStackLength = 64;
  void *stack[kStackLength];

  int size;
  uintptr_t pc = emscripten_stack_snapshot();
  size = emscripten_stack_unwind_buffer(pc, stack, kStackLength);

  int result_count = size - skip_count;
  if (result_count < 0) result_count = 0;
  if (result_count > max_depth) result_count = max_depth;
  for (int i = 0; i < result_count; i++) result[i] = stack[i + skip_count];

  if (IS_STACK_FRAMES) {
    // No implementation for finding out the stack frame sizes yet.
    memset(sizes, 0, sizeof(*sizes) * result_count);
  }
  if (min_dropped_frames != nullptr) {
    if (size - skip_count - max_depth > 0) {
      *min_dropped_frames = size - skip_count - max_depth;
    } else {
      *min_dropped_frames = 0;
    }
  }

  --recursive;

  return result_count;
}

namespace turbo {
TURBO_NAMESPACE_BEGIN
namespace debugging_internal {
bool StackTraceWorksForTest() { return true; }
}  // namespace debugging_internal
TURBO_NAMESPACE_END
}  // namespace turbo

#endif  // TURBO_DEBUGGING_INTERNAL_STACKTRACE_EMSCRIPTEN_INL_H_
