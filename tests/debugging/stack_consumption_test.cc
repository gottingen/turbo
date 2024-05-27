//
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

#include <turbo/debugging/internal/stack_consumption.h>

#ifdef TURBO_INTERNAL_HAVE_DEBUGGING_STACK_CONSUMPTION

#include <string.h>

#include <gtest/gtest.h>
#include <turbo/log/log.h>

namespace turbo {
TURBO_NAMESPACE_BEGIN
namespace debugging_internal {
namespace {

static void SimpleSignalHandler(int signo) {
  char buf[100];
  memset(buf, 'a', sizeof(buf));

  // Never true, but prevents compiler from optimizing buf out.
  if (signo == 0) {
    LOG(INFO) << static_cast<void*>(buf);
  }
}

TEST(SignalHandlerStackConsumptionTest, MeasuresStackConsumption) {
  // Our handler should consume reasonable number of bytes.
  EXPECT_GE(GetSignalHandlerStackConsumption(SimpleSignalHandler), 100);
}

}  // namespace
}  // namespace debugging_internal
TURBO_NAMESPACE_END
}  // namespace turbo

#endif  // TURBO_INTERNAL_HAVE_DEBUGGING_STACK_CONSUMPTION
