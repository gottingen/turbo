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
// This is inspired by Craig Silverstein's PowerPC stacktrace code.

#ifndef TURBO_DEBUGGING_INTERNAL_STACKTRACE_ARM_INL_H_
#define TURBO_DEBUGGING_INTERNAL_STACKTRACE_ARM_INL_H_

#include <cstdint>

#include <turbo/debugging/stacktrace.h>

// WARNING:
// This only works if all your code is in either ARM or THUMB mode.  With
// interworking, the frame pointer of the caller can either be in r11 (ARM
// mode) or r7 (THUMB mode).  A callee only saves the frame pointer of its
// mode in a fixed location on its stack frame.  If the caller is a different
// mode, there is no easy way to find the frame pointer.  It can either be
// still in the designated register or saved on stack along with other callee
// saved registers.

// Given a pointer to a stack frame, locate and return the calling
// stackframe, or return nullptr if no stackframe can be found. Perform sanity
// checks (the strictness of which is controlled by the boolean parameter
// "STRICT_UNWINDING") to reduce the chance that a bad pointer is returned.
template<bool STRICT_UNWINDING>
static void **NextStackFrame(void **old_sp) {
  void **new_sp = (void**) old_sp[-1];

  // Check that the transition from frame pointer old_sp to frame
  // pointer new_sp isn't clearly bogus
  if (STRICT_UNWINDING) {
    // With the stack growing downwards, older stack frame must be
    // at a greater address that the current one.
    if (new_sp <= old_sp) return nullptr;
    // Assume stack frames larger than 100,000 bytes are bogus.
    if ((uintptr_t)new_sp - (uintptr_t)old_sp > 100000) return nullptr;
  } else {
    // In the non-strict mode, allow discontiguous stack frames.
    // (alternate-signal-stacks for example).
    if (new_sp == old_sp) return nullptr;
    // And allow frames upto about 1MB.
    if ((new_sp > old_sp)
        && ((uintptr_t)new_sp - (uintptr_t)old_sp > 1000000)) return nullptr;
  }
  if ((uintptr_t)new_sp & (sizeof(void *) - 1)) return nullptr;
  return new_sp;
}

// This ensures that turbo::GetStackTrace sets up the Link Register properly.
#ifdef __GNUC__
void StacktraceArmDummyFunction() __attribute__((noinline));
void StacktraceArmDummyFunction() { __asm__ volatile(""); }
#else
# error StacktraceArmDummyFunction() needs to be ported to this platform.
#endif

template <bool IS_STACK_FRAMES, bool IS_WITH_CONTEXT>
static int UnwindImpl(void** result, int* sizes, int max_depth, int skip_count,
                      const void * /* ucp */, int *min_dropped_frames) {
#ifdef __GNUC__
  void **sp = reinterpret_cast<void**>(__builtin_frame_address(0));
#else
# error reading stack point not yet supported on this platform.
#endif

  // On ARM, the return address is stored in the link register (r14).
  // This is not saved on the stack frame of a leaf function.  To
  // simplify code that reads return addresses, we call a dummy
  // function so that the return address of this function is also
  // stored in the stack frame.  This works at least for gcc.
  StacktraceArmDummyFunction();

  int n = 0;
  while (sp && n < max_depth) {
    // The turbo::GetStackFrames routine is called when we are in some
    // informational context (the failure signal handler for example).
    // Use the non-strict unwinding rules to produce a stack trace
    // that is as complete as possible (even if it contains a few bogus
    // entries in some rare cases).
    void **next_sp = NextStackFrame<!IS_STACK_FRAMES>(sp);

    if (skip_count > 0) {
      skip_count--;
    } else {
      result[n] = *sp;

      if (IS_STACK_FRAMES) {
        if (next_sp > sp) {
          sizes[n] = (uintptr_t)next_sp - (uintptr_t)sp;
        } else {
          // A frame-size of 0 is used to indicate unknown frame size.
          sizes[n] = 0;
        }
      }
      n++;
    }
    sp = next_sp;
  }
  if (min_dropped_frames != nullptr) {
    // Implementation detail: we clamp the max of frames we are willing to
    // count, so as not to spend too much time in the loop below.
    const int kMaxUnwind = 200;
    int num_dropped_frames = 0;
    for (int j = 0; sp != nullptr && j < kMaxUnwind; j++) {
      if (skip_count > 0) {
        skip_count--;
      } else {
        num_dropped_frames++;
      }
      sp = NextStackFrame<!IS_STACK_FRAMES>(sp);
    }
    *min_dropped_frames = num_dropped_frames;
  }
  return n;
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

#endif  // TURBO_DEBUGGING_INTERNAL_STACKTRACE_ARM_INL_H_
