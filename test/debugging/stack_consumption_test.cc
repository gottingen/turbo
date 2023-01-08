
/****************************************************************
 * Copyright (c) 2022, liyinbin
 * All rights reserved.
 * Author by liyinbin (jeff.li) lijippy@163.com
 *****************************************************************/
#include "flare/debugging/internal/stack_consumption.h"

#ifdef FLARE_INTERNAL_HAVE_DEBUGGING_STACK_CONSUMPTION

#include <string.h>

#include "testing/gtest_wrap.h"
#include "flare/log/logging.h"

namespace flare::debugging {

namespace debugging_internal {
namespace {

static void SimpleSignalHandler(int signo) {
  char buf[100];
  memset(buf, 'a', sizeof(buf));

  // Never true, but prevents compiler from optimizing buf out.
  if (signo == 0) {
      FLARE_LOG(INFO)<<static_cast<void*>(buf);
  }
}

TEST(SignalHandlerStackConsumptionTest, MeasuresStackConsumption) {
  // Our handler should consume reasonable number of bytes.
  EXPECT_GE(GetSignalHandlerStackConsumption(SimpleSignalHandler), 100);
}

}  // namespace
}  // namespace debugging_internal

}  // namespace flare::debugging

#endif  // FLARE_INTERNAL_HAVE_DEBUGGING_STACK_CONSUMPTION
