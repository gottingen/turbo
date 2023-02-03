// Copyright 2022 The Turbo Authors.
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//      https://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.

#include "turbo/random/internal/fastmath.h"

#include "gtest/gtest.h"

#if defined(__native_client__) || defined(__EMSCRIPTEN__)
// NACL has a less accurate implementation of std::log2 than most of
// the other platforms. For some values which should have integral results,
// sometimes NACL returns slightly larger values.
//
// The MUSL libc used by emscripten also has a similar bug.
#define TURBO_RANDOM_INACCURATE_LOG2
#endif

namespace {

TEST(FastMathTest, StirlingLogFactorial) {
  using turbo::random_internal::StirlingLogFactorial;

  EXPECT_NEAR(StirlingLogFactorial(1.0), 0, 1e-3);
  EXPECT_NEAR(StirlingLogFactorial(1.50), 0.284683, 1e-3);
  EXPECT_NEAR(StirlingLogFactorial(2.0), 0.69314718056, 1e-4);

  for (int i = 2; i < 50; i++) {
    double d = static_cast<double>(i);
    EXPECT_NEAR(StirlingLogFactorial(d), std::lgamma(d + 1), 3e-5);
  }
}

}  // namespace
