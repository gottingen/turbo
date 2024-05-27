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

#include <turbo/debugging/stacktrace.h>

#include <gtest/gtest.h>
#include <turbo/base/macros.h>
#include <turbo/base/optimization.h>

namespace {

// This test is currently only known to pass on Linux x86_64/aarch64.
#if defined(__linux__) && (defined(__x86_64__) || defined(__aarch64__))
TURBO_ATTRIBUTE_NOINLINE void Unwind(void* p) {
  TURBO_ATTRIBUTE_UNUSED static void* volatile sink = p;
  constexpr int kSize = 16;
  void* stack[kSize];
  int frames[kSize];
  turbo::GetStackTrace(stack, kSize, 0);
  turbo::GetStackFrames(stack, frames, kSize, 0);
}

TURBO_ATTRIBUTE_NOINLINE void HugeFrame() {
  char buffer[1 << 20];
  Unwind(buffer);
  TURBO_BLOCK_TAIL_CALL_OPTIMIZATION();
}

TEST(StackTrace, HugeFrame) {
  // Ensure that the unwinder is not confused by very large stack frames.
  HugeFrame();
  TURBO_BLOCK_TAIL_CALL_OPTIMIZATION();
}
#endif

}  // namespace
